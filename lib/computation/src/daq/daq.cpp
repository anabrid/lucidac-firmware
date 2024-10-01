// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <algorithm>
#include <bitset>

#include "daq/daq.h"
#include "utils/logging.h"
#include "utils/running_avg.h"

FLASHMEM
bool daq::OneshotDAQ::init(__attribute__((unused)) unsigned int sample_rate_unused) {
  pinMode(PIN_CNVST, OUTPUT);
  digitalWriteFast(PIN_CNVST, LOW);
  pinMode(PIN_CLK, OUTPUT);
  digitalWriteFast(PIN_CLK, LOW);

  for (auto pin : PINS_MISO) {
    // Pull-up is on hardware
    pinMode(pin, INPUT);
  }
  return true;
}

float daq::BaseDAQ::raw_to_float(const uint16_t raw) {
  return ((static_cast<float>(raw) - RAW_MINUS_ONE_POINT_TWO_FIVE) /
          (RAW_PLUS_ONE_POINT_TWO_FIVE - RAW_MINUS_ONE_POINT_TWO_FIVE)) *
             -2.5f +
         1.25f;
}

FLASHMEM
std::array<float, daq::NUM_CHANNELS> daq::BaseDAQ::sample_avg(size_t samples, unsigned int delay_us) {
  utils::RunningAverage<std::array<float, daq::NUM_CHANNELS>> avg;
  for (size_t i = 0; i < samples; i++) {
    avg.add(sample());
    delayMicroseconds(delay_us);
  }
  return avg.get_average();
}

size_t daq::BaseDAQ::raw_to_normalized(uint16_t raw) {
  // exact conversion is 0.152658243301, but we only care about so much precision
  return std::max(std::min((static_cast<unsigned int>(raw) * 1527u) / 10000u, 2500u), 0u);
}

std::array<uint16_t, daq::NUM_CHANNELS> daq::OneshotDAQ::sample_raw() {
  decltype(sample_raw()) data;
  sample_raw(data.data());
  return data;
}

// NOT FLASHMEM
void daq::OneshotDAQ::sample_raw(uint16_t *data) {
  // Trigger CNVST
  digitalWriteFast(PIN_CNVST, HIGH);
  delayNanoseconds(1500);
  digitalWriteFast(PIN_CNVST, LOW);

  delayNanoseconds(1000);

  for (auto i = 0U; i < NUM_CHANNELS; i++)
    data[i] = 0;
  for (auto clk_i = 0; clk_i < 14; clk_i++) {
    digitalWriteFast(PIN_CLK, HIGH);
    delayNanoseconds(100);
    // Sample data after rising edge, but only first 14 bits
    if (clk_i < 14) {
      for (unsigned int pin_i = 0; pin_i < NUM_CHANNELS; pin_i++) {
        data[pin_i] |= digitalReadFast(PINS_MISO[pin_i]) ? (1 << (13 - clk_i)) : 0;
      }
    }
    delayNanoseconds(100);
    digitalWriteFast(PIN_CLK, LOW);
    delayNanoseconds(350);
  }
}

FLASHMEM
std::array<float, daq::NUM_CHANNELS> daq::OneshotDAQ::sample() {
  auto data_raw = sample_raw();
  std::array<float, daq::NUM_CHANNELS> data{};
  std::transform(std::begin(data_raw), std::end(data_raw), std::begin(data), raw_to_float);
  return data;
}

float daq::OneshotDAQ::sample(uint8_t index) { return sample()[index]; }

uint16_t daq::OneshotDAQ::sample_raw(uint8_t index) { return sample_raw()[index]; }

std::array<uint16_t, daq::NUM_CHANNELS> daq::OneshotDAQ::sample_avg_raw(size_t samples,
                                                                        unsigned int delay_us) {
  utils::RunningAverage<std::array<uint32_t, daq::NUM_CHANNELS>>
      avg; // Use larger variables to avoid overflows
  for (size_t i = 0; i < samples; i++) {
    avg.add(utils::convert<uint32_t>(sample_raw()));
    delayMicroseconds(delay_us);
  }
  return utils::convert<uint16_t>(avg.get_average());
}

#ifdef ARDUINO

namespace daq {

namespace dma {

// We need a memory segment to implement a ring buffer.
// Its size should be a power-of-2-multiple of NUM_CHANNELS.
// The DMA implements ring buffer by restricting the N lower bits of the destination address to change.
// That means the memory segment must start at a memory address with enough lower bits being zero,
// see manual "data queues [with] power-of-2 size bytes, [...] should start at a 0-modulo-size address".
// One can put this into DMAMEM for increased performance, but that did not work for me right away.
// *MUST* be at least 2*NUM_CHANNELS, otherwise some math later does not work.
__attribute__((aligned(BUFFER_SIZE * 4))) std::array<volatile uint32_t, BUFFER_SIZE> buffer = {0};

DMAChannel channel(false);
run::Run *run = nullptr;
run::RunDataHandler *run_data_handler = nullptr;
volatile bool first_data = false;
volatile bool last_data = false;
volatile bool overflow_data = false;

// NOT FLASHMEM
void interrupt() {
  // Serial.println(__PRETTY_FUNCTION__);
  auto is_half = (channel.TCD->CITER != channel.TCD->BITER);
  if (is_half) {
    overflow_data |= first_data;
    first_data = true;
  } else {
    overflow_data |= last_data;
    last_data = true;
  }

  // Clear interrupt
  channel.clearInterrupt();
  // Memory barrier
#ifdef ARDUINO
  asm("DSB");
#endif
}

std::array<volatile uint32_t, BUFFER_SIZE> get_buffer() { return buffer; }

} // namespace dma

ContinuousDAQ::ContinuousDAQ(run::Run &run, const daq::DAQConfig &daq_config,
                             run::RunDataHandler *run_data_handler)
    : run(run), daq_config(daq_config), run_data_handler(run_data_handler) {}

unsigned int ContinuousDAQ::get_number_of_data_vectors_in_buffer() {
  // Note that this does not consider whether these have been streamed out yet
  return dma::channel.TCD->BITER - dma::channel.TCD->CITER;
}

// NOT FLASHMEM
bool ContinuousDAQ::stream(bool partial) {
  // Pointer to the part of the buffer which (may) contain partial data at end of acquisition time
  static auto partial_buffer_part = dma::buffer.data();

  if (!daq_config)
    return true;
  if (dma::overflow_data)
    return false;

  // Default values for streaming
  volatile uint32_t *active_buffer_part;
  size_t outer_count = dma::BUFFER_SIZE / daq_config.get_num_channels() / 2;

  // Change streaming parameters depending on whether the first or second half of buffer is streamed
  if (dma::first_data) {
    active_buffer_part = dma::buffer.data();
    partial_buffer_part = dma::buffer.data() + dma::BUFFER_SIZE / 2;
    dma::first_data = false;
  } else if (dma::last_data) {
    active_buffer_part = dma::buffer.data() + dma::BUFFER_SIZE / 2;
    partial_buffer_part = dma::buffer.data();
    dma::last_data = false;
  } else if (partial) {
    // Stream the remaining partially filled part of the buffer.
    // This should be done exactly once, after the data acquisition stopped.
    active_buffer_part = partial_buffer_part;
    if (partial_buffer_part == dma::buffer.data())
      // If we have streamed out the second part the last time,
      // the partial data is in the first part of the buffer
      // and get_number_of_data_vectors_in_buffer returns only
      // the number of partial data vectors to stream.
      outer_count = get_number_of_data_vectors_in_buffer();
    else
      // If we have streamed out the first part the last time,
      // the partial data is in the second half,
      // and get_number_of_data_vectors_in_buffer does not consider this
      // and returns number of vectors in first_half plus number of partials
      // in second half, and we have to subtract the number in the first half.
      outer_count = get_number_of_data_vectors_in_buffer() - outer_count;
    if (!outer_count) {
      return true;
    }
  } else
    return true;

  run_data_handler->handle(active_buffer_part, outer_count, daq_config.get_num_channels(), run);
  return true;
}

} // namespace daq

FLASHMEM
daq::FlexIODAQ::FlexIODAQ(run::Run &run, DAQConfig &daq_config, run::RunDataHandler *run_data_handler)
    : ContinuousDAQ(run, daq_config, run_data_handler),
      flexio(FlexIOHandler::mapIOPinToFlexIOHandler(PIN_CNVST, _flexio_pin_cnvst)),
      _flexio_pin_clk(flexio->mapIOPinToFlexPin(PIN_CLK)),
      _flexio_pin_gate(flexio->mapIOPinToFlexPin(PIN_GATE)) {
  std::transform(PINS_MISO.begin(), PINS_MISO.end(), _flexio_pins_miso.begin(),
                 [&](auto pin) { return flexio->mapIOPinToFlexPin(pin); });
}

// NOT FLASHMEM
bool daq::FlexIODAQ::init(unsigned int) {
  LOG_ANABRID_DEBUG_DAQ(__PRETTY_FUNCTION__);
  if (!daq_config.is_valid()) {
    LOG_ERROR("Invalid DAQ config.")
    return false;
  }

  // Update global pointers for dma::interrupt
  dma::run_data_handler = run_data_handler;
  dma::run = &run;

  // Check if any of the pins could not be mapped to the same FlexIO module
  if (_flexio_pin_cnvst == 0xff or _flexio_pin_clk == 0xff or _flexio_pin_gate == 0xff)
    return false;
  for (auto _flexio_pin_miso : _flexio_pins_miso)
    if (_flexio_pin_miso == 0xff)
      return false;

  // Maximum FlexIO clock speed is 120MHz, see https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.1025
  // PLL3 is 480MHz, (3,1,1) divides that by 4 to 120MHz
  // But for state, it also works to use (3,0,0)?
  flexio->setClockSettings(3, 0, 0);

  flexio->setIOPinToFlexMode(PIN_GATE);
  uint8_t _gated_timer_idx = 0;
  flexio->port().TIMCTL[_gated_timer_idx] = FLEXIO_TIMCTL_TRGSEL(2 * _flexio_pin_gate) | FLEXIO_TIMCTL_TRGPOL |
                                            FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TIMOD(0b11);
  flexio->port().TIMCFG[_gated_timer_idx] = FLEXIO_TIMCFG_TIMDIS(0b011) | FLEXIO_TIMCFG_TIMENA(0b110);
  flexio->port().TIMCMP[_gated_timer_idx] = 239;

  uint8_t _sample_timer_idx = 1;
  flexio->port().TIMCTL[_sample_timer_idx] =
      FLEXIO_TIMCTL_TRGSEL(4 * _gated_timer_idx + 3) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TIMOD(0b11);
  flexio->port().TIMCFG[_sample_timer_idx] = FLEXIO_TIMCFG_TIMDEC(0b01) | FLEXIO_TIMCFG_TIMENA(0b110);
  flexio->port().TIMCMP[_sample_timer_idx] = (1'000'000 / daq_config.get_sample_rate()) * 2 - 1;

  flexio->setIOPinToFlexMode(PIN_CNVST);
  uint8_t _cnvst_timer_idx = 2;
  flexio->port().TIMCTL[_cnvst_timer_idx] =
      FLEXIO_TIMCTL_PINSEL(_flexio_pin_cnvst) | FLEXIO_TIMCTL_PINCFG(0b11) | FLEXIO_TIMCTL_TRGSRC |
      FLEXIO_TIMCTL_TRGSEL(4 * _sample_timer_idx + 3) | FLEXIO_TIMCTL_TIMOD(0b10);
  flexio->port().TIMCFG[_cnvst_timer_idx] = FLEXIO_TIMCFG_TIMDIS(0b010) | FLEXIO_TIMCFG_TIMENA(0b111);
  flexio->port().TIMCMP[_cnvst_timer_idx] = 0x0000'10'FF;

  // Add a delay timer to control delay between CNVST and first CLK.
  // Basically a second CNVST timer which is slightly longer and used to trigger first CLK.
  uint8_t _delay_timer_idx = 3;
  flexio->port().TIMCTL[_delay_timer_idx] =
      FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TRGSEL(4 * _sample_timer_idx + 3) | FLEXIO_TIMCTL_TIMOD(0b11);
  flexio->port().TIMCFG[_delay_timer_idx] = FLEXIO_TIMCFG_TIMDIS(0b010) | FLEXIO_TIMCFG_TIMENA(0b111);
  flexio->port().TIMCMP[_delay_timer_idx] = 300;

  flexio->setIOPinToFlexMode(PIN_CLK);
  uint8_t _clk_timer_idx = 4;
  flexio->port().TIMCTL[_clk_timer_idx] =
      FLEXIO_TIMCTL_PINSEL(_flexio_pin_clk) | FLEXIO_TIMCTL_PINCFG(0b11) | FLEXIO_TIMCTL_TRGSRC |
      FLEXIO_TIMCTL_TRGSEL(4 * _delay_timer_idx + 3) | FLEXIO_TIMCTL_TRGPOL | FLEXIO_TIMCTL_TIMOD(0b01);
  flexio->port().TIMCFG[_clk_timer_idx] = FLEXIO_TIMCFG_TIMDIS(0b010) | FLEXIO_TIMCFG_TIMENA(0b110);
  // langsam
  // flexio->port().TIMCMP[_clk_timer_idx] = 0x0000'1F'60;
  // vielfaches von bit-bang algorithm:
  flexio->port().TIMCMP[_clk_timer_idx] = 0x0000'1B'07;
  // maximal schnell
  // flexio->port().TIMCMP[_clk_timer_idx] = 0x0000'1F'05;

  /*
   *  Configure shifters for data capture
   *
   *  Note that CLK shifts 16 times and then the compare event (timer stop/disable) triggers a store.
   *  That means the shift registers will store each 16-bit data packet separately,
   *  not collect 32 bits and then store the full 32 bits.
   *  Thus, part of SHIFTBUF will always be zero.
   */

  for (auto _pin_miso : PINS_MISO) {
    flexio->setIOPinToFlexMode(_pin_miso);
  }
  for (auto _pin_miso_idx = 0u; _pin_miso_idx < _flexio_pins_miso.size(); _pin_miso_idx++) {
    // Trigger all shifters from CLK
    flexio->port().SHIFTCTL[_pin_miso_idx] = FLEXIO_SHIFTCTL_TIMSEL(_clk_timer_idx) | FLEXIO_SHIFTCTL_TIMPOL |
                                             FLEXIO_SHIFTCTL_PINSEL(_flexio_pins_miso[_pin_miso_idx]) |
                                             FLEXIO_SHIFTCTL_SMOD(1);

    flexio->port().SHIFTCFG[_pin_miso_idx] = 0;
  }

  /*
   *  Configure DMA
   */

  // Select shifter zero to generate DMA events.
  // Which shifter is selected should not matter, as long as it is used.
  uint8_t shifter_dma_idx = 0;
  // Set shifter to generate DMA events.
  flexio->port().SHIFTSDEN = 1 << shifter_dma_idx;
  // Configure DMA channel
  dma::channel.begin();

  // Configure minor and major loop of DMA process.
  // One DMA request (SHIFTBUF store) triggers one major loop.
  // A major loop consists of several minor loops.
  // For each loop, source address and destination address is adjusted and data is copied.
  // The number of minor loops is implicitly defined by how much data they should copy.
  // The approach here is to use the minor loops to copy all shift registers when one triggers the DMA.
  // That means each major loop copies over NUM_CHANNELS data points.
  // Triggering multiple major loops fills up the ring buffer.
  // Once all major loops are done, the ring buffer is full and an interrupt is triggered to handle the data.
  // For such configurations DMAChannel provides sourceCircular() and destinationCircular() functions,
  // which are not quite able to handle our case (BITER/CITER and ATTR_DST must be different).
  // That's why we do it by hand :)

  // BITER "beginning iteration count" is the number of major loops.
  // Each major loop fills part (see TCD->NBYTES) of the ring buffer.
  dma::channel.TCD->BITER = dma::buffer.size() / daq_config.get_num_channels();
  // CITER "current major loop iteration count" is the current number of major loops left to perform.
  // It can be used to check progress of the process. It's reset to BITER whe we filled the buffer once.
  dma::channel.TCD->CITER = dma::buffer.size() / daq_config.get_num_channels();

  // Configure source address and its adjustments.
  // We want to circularly copy from SHIFTBUFBIS.
  // In principle, we are only interested in the last 16 bits of each SHIFTBUFBIS.
  // Unfortunately, configuring it in such a way was not successful -- maybe in the future.
  // Instead, we copy the full 32 bit of data.
  // SADDR "source address start" is where the DMA process starts.
  // Set it to the beginning of the SHIFTBUFBIS array.
  dma::channel.TCD->SADDR = flexio->port().SHIFTBUFBIS;
  // NBYTES "minor byte transfer count" is the number of bytes transferred in one minor loop.
  // Set it such that the whole SHIFTBUFBIS array is copied, which is 4 bytes * NUM_CHANNELS
  dma::channel.TCD->NBYTES = 4 * daq_config.get_num_channels();
  // SOFF "source address offset" is the offset added onto SADDR for each minor loop.
  // Set it to 4 bytes, equaling the 32 bits each shift register has.
  dma::channel.TCD->SOFF = 4;
  // ATTR_SRC "source address attribute" is an attribute setting the circularity and the transfer size.
  // The format is [5bit MOD][3bit SIZE].
  // The 5bit MOD is the number of lower address bites allowed to change, effectively circling back to SADDR.
  // The 3bit SIZE defines the source data transfer size.
  // Set MOD to 5, allowing the address bits to change until the end of the SHIFTBUFBIS array and cycling.
  // MOD = 5 because 2^5 = 32, meaning address cycles after 32 bytes, which are 8*32 bits.
  // Set SIZE to 0b010 for 32 bit transfer size.
  uint8_t MOD_SRC = __builtin_ctz(daq_config.get_num_channels() * 4);
  dma::channel.TCD->ATTR_SRC = ((MOD_SRC & 0b11111) << 3) | 0b010;
  // SLAST "last source address adjustment" is an adjustment applied to SADDR after all major iterations.
  // We don't want any adjustments, since we already use ATTR_SRC to implement a circular buffer.
  dma::channel.TCD->SLAST = 0;

  // Configure destination address and its adjustments.
  // We want to circularly copy into a memory ring buffer.
  // Since we always copy 32 bit from SADDR, we will have the memory buffer in a 32 bit layout as well,
  // even though we are again only interested in the lower 16 bits.
  // DADDR "destination address start" is the start of the destination ring buffer.
  // Set to address of ring buffer.
  dma::channel.TCD->DADDR = dma::buffer.data();
  // DOOFF "destination address offset" is the offset added to DADDR for each minor loop.
  // Set to 4 bytes, equaling the 32 bits each shift register has.
  dma::channel.TCD->DOFF = 4;
  // ATTR_SRC "destination address attribute" is analogous to ATTR_SRC
  // Set first 5 bit MOD according to size of ring buffer.
  // Since only power-of-two number of channels N<=8 is allowed, N*NUM_CHANNELS will always fit for each N.
  // Set last 3 bits to 0b010 for 32 bit transfer size.
  uint8_t MOD = __builtin_ctz(dma::BUFFER_SIZE * 4);
  // Check if memory buffer address is aligned such that MOD lower bits are zero (maybe this is too pedantic?)
  if (reinterpret_cast<uintptr_t>(dma::buffer.data()) & ~(~static_cast<uintptr_t>(0) << MOD)) {
    LOG_ERROR("DMA buffer memory range is not sufficiently aligned.")
    return false;
  }
  dma::channel.TCD->ATTR_DST = ((MOD & 0b11111) << 3) | 0b010;
  // DLASTSGA "last destination address adjustment or next TCD" is similar to SLAST.
  // We don't want any adjustments, since we already use ATTR_DST to implement a circular buffer.
  dma::channel.TCD->SLAST = 0;

  // Call an interrupt when done
  dma::channel.attachInterrupt(dma::interrupt);
  dma::channel.interruptAtCompletion();
  dma::channel.interruptAtHalf();
  // Trigger from "shifter full" DMA event
  dma::channel.triggerAtHardwareEvent(flexio->shiftersDMAChannel(shifter_dma_idx));
  // Enable dma channel
  dma::channel.enable();

  if (dma::channel.error()) {
    LOG_ERROR("dma::channel.error");
    return false;
  }

  return true;
}

void daq::FlexIODAQ::enable() {
  if (!daq_config)
    return;
  flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;
}

std::array<uint16_t, daq::NUM_CHANNELS> daq::FlexIODAQ::sample_raw() { return {}; }

std::array<float, daq::NUM_CHANNELS> daq::FlexIODAQ::sample() { return {}; }

float daq::FlexIODAQ::sample(uint8_t index) { return 0; }

void daq::FlexIODAQ::reset() {
  LOG_ANABRID_DEBUG_DAQ(__PRETTY_FUNCTION__);
  flexio->port().CTRL &= ~FLEXIO_CTRL_FLEXEN;
  flexio->port().CTRL |= FLEXIO_CTRL_SWRST;
  delayNanoseconds(100);
  flexio->port().CTRL &= ~FLEXIO_CTRL_SWRST;
  delayNanoseconds(100);
  // TODO: REMOVE!
  for (auto &data : dma::buffer)
    data = 0;
  dma::first_data = dma::last_data = dma::overflow_data = false;
}

bool daq::FlexIODAQ::finalize() {
  /*
  Serial.println("dma::buffer memory location");
  Serial.println(reinterpret_cast<uintptr_t>(dma::buffer.begin()));
  Serial.println(reinterpret_cast<uintptr_t>(dma::buffer.end()));
  Serial.println(dma::channel.TCD->ATTR_DST, BIN);
  Serial.println(dma::channel.TCD->CITER);
  for (auto data : dma::buffer) {
    Serial.println(std::bitset<32>(data).to_string().c_str());
  }
  */

  // Clear global pointers for dma::interrupt
  dma::run_data_handler = nullptr;
  dma::run = nullptr;

  // If DAQConfig is not active, we don't care about errors
  if (!daq_config)
    return true;

  // Check some errors
  if (dma::channel.error()) {
    LOG_ERROR("DAQ DMA error.")
    return false;
  }
  uint32_t _shifterr_mask =
      (daq_config.get_num_channels() > 1) ? ((1u << daq_config.get_num_channels()) - 1) : 1u;
  if (flexio->port().SHIFTERR & _shifterr_mask) {
    LOG_ERROR("DAQ SHIFTERR error.");
    return false;
  }
  if (dma::overflow_data) {
    LOG_ERROR("DAQ overflow.");
    return false;
  }

  return true;
}

#endif

FLASHMEM int daq::OneshotDAQ::sample(JsonObjectConst msg_in, JsonObject &msg_out) {
  std::array<float, daq::NUM_CHANNELS> data{};

  auto do_sample_avg = msg_in["sample_avg"];
  if (do_sample_avg) {
    data = sample_avg(do_sample_avg["size_samples"], do_sample_avg["avg_us"]);
  } else {
    data = sample();
  }

  if (msg_in.containsKey("channel")) {
    uint8_t channel = msg_in["channel"];
    if (channel < 0 || channel >= NUM_CHANNELS) {
      msg_out["error"] = "Channel has to be a single number.";
      return 1;
    }

    msg_out["data"] = data[channel];
    return 0;
  } else {
    auto data_json = msg_out.createNestedArray("data");
    copyArray(data.data(), data.size(), data_json);
    return 0;
  }
}

const char *daq::BaseDAQ::raw_to_str(uint16_t raw) {
  PROGMEM static constexpr std::array<char[7], 2501> normalized_to_float_str_arr = {
      " 1.250", " 1.249", " 1.248", " 1.247", " 1.246", " 1.245", " 1.244", " 1.243", " 1.242", " 1.241",
      " 1.240", " 1.239", " 1.238", " 1.237", " 1.236", " 1.235", " 1.234", " 1.233", " 1.232", " 1.231",
      " 1.230", " 1.229", " 1.228", " 1.227", " 1.226", " 1.225", " 1.224", " 1.223", " 1.222", " 1.221",
      " 1.220", " 1.219", " 1.218", " 1.217", " 1.216", " 1.215", " 1.214", " 1.213", " 1.212", " 1.211",
      " 1.210", " 1.209", " 1.208", " 1.207", " 1.206", " 1.205", " 1.204", " 1.203", " 1.202", " 1.201",
      " 1.200", " 1.199", " 1.198", " 1.197", " 1.196", " 1.195", " 1.194", " 1.193", " 1.192", " 1.191",
      " 1.190", " 1.189", " 1.188", " 1.187", " 1.186", " 1.185", " 1.184", " 1.183", " 1.182", " 1.181",
      " 1.180", " 1.179", " 1.178", " 1.177", " 1.176", " 1.175", " 1.174", " 1.173", " 1.172", " 1.171",
      " 1.170", " 1.169", " 1.168", " 1.167", " 1.166", " 1.165", " 1.164", " 1.163", " 1.162", " 1.161",
      " 1.160", " 1.159", " 1.158", " 1.157", " 1.156", " 1.155", " 1.154", " 1.153", " 1.152", " 1.151",
      " 1.150", " 1.149", " 1.148", " 1.147", " 1.146", " 1.145", " 1.144", " 1.143", " 1.142", " 1.141",
      " 1.140", " 1.139", " 1.138", " 1.137", " 1.136", " 1.135", " 1.134", " 1.133", " 1.132", " 1.131",
      " 1.130", " 1.129", " 1.128", " 1.127", " 1.126", " 1.125", " 1.124", " 1.123", " 1.122", " 1.121",
      " 1.120", " 1.119", " 1.118", " 1.117", " 1.116", " 1.115", " 1.114", " 1.113", " 1.112", " 1.111",
      " 1.110", " 1.109", " 1.108", " 1.107", " 1.106", " 1.105", " 1.104", " 1.103", " 1.102", " 1.101",
      " 1.100", " 1.099", " 1.098", " 1.097", " 1.096", " 1.095", " 1.094", " 1.093", " 1.092", " 1.091",
      " 1.090", " 1.089", " 1.088", " 1.087", " 1.086", " 1.085", " 1.084", " 1.083", " 1.082", " 1.081",
      " 1.080", " 1.079", " 1.078", " 1.077", " 1.076", " 1.075", " 1.074", " 1.073", " 1.072", " 1.071",
      " 1.070", " 1.069", " 1.068", " 1.067", " 1.066", " 1.065", " 1.064", " 1.063", " 1.062", " 1.061",
      " 1.060", " 1.059", " 1.058", " 1.057", " 1.056", " 1.055", " 1.054", " 1.053", " 1.052", " 1.051",
      " 1.050", " 1.049", " 1.048", " 1.047", " 1.046", " 1.045", " 1.044", " 1.043", " 1.042", " 1.041",
      " 1.040", " 1.039", " 1.038", " 1.037", " 1.036", " 1.035", " 1.034", " 1.033", " 1.032", " 1.031",
      " 1.030", " 1.029", " 1.028", " 1.027", " 1.026", " 1.025", " 1.024", " 1.023", " 1.022", " 1.021",
      " 1.020", " 1.019", " 1.018", " 1.017", " 1.016", " 1.015", " 1.014", " 1.013", " 1.012", " 1.011",
      " 1.010", " 1.009", " 1.008", " 1.007", " 1.006", " 1.005", " 1.004", " 1.003", " 1.002", " 1.001",
      " 1.000", " 0.999", " 0.998", " 0.997", " 0.996", " 0.995", " 0.994", " 0.993", " 0.992", " 0.991",
      " 0.990", " 0.989", " 0.988", " 0.987", " 0.986", " 0.985", " 0.984", " 0.983", " 0.982", " 0.981",
      " 0.980", " 0.979", " 0.978", " 0.977", " 0.976", " 0.975", " 0.974", " 0.973", " 0.972", " 0.971",
      " 0.970", " 0.969", " 0.968", " 0.967", " 0.966", " 0.965", " 0.964", " 0.963", " 0.962", " 0.961",
      " 0.960", " 0.959", " 0.958", " 0.957", " 0.956", " 0.955", " 0.954", " 0.953", " 0.952", " 0.951",
      " 0.950", " 0.949", " 0.948", " 0.947", " 0.946", " 0.945", " 0.944", " 0.943", " 0.942", " 0.941",
      " 0.940", " 0.939", " 0.938", " 0.937", " 0.936", " 0.935", " 0.934", " 0.933", " 0.932", " 0.931",
      " 0.930", " 0.929", " 0.928", " 0.927", " 0.926", " 0.925", " 0.924", " 0.923", " 0.922", " 0.921",
      " 0.920", " 0.919", " 0.918", " 0.917", " 0.916", " 0.915", " 0.914", " 0.913", " 0.912", " 0.911",
      " 0.910", " 0.909", " 0.908", " 0.907", " 0.906", " 0.905", " 0.904", " 0.903", " 0.902", " 0.901",
      " 0.900", " 0.899", " 0.898", " 0.897", " 0.896", " 0.895", " 0.894", " 0.893", " 0.892", " 0.891",
      " 0.890", " 0.889", " 0.888", " 0.887", " 0.886", " 0.885", " 0.884", " 0.883", " 0.882", " 0.881",
      " 0.880", " 0.879", " 0.878", " 0.877", " 0.876", " 0.875", " 0.874", " 0.873", " 0.872", " 0.871",
      " 0.870", " 0.869", " 0.868", " 0.867", " 0.866", " 0.865", " 0.864", " 0.863", " 0.862", " 0.861",
      " 0.860", " 0.859", " 0.858", " 0.857", " 0.856", " 0.855", " 0.854", " 0.853", " 0.852", " 0.851",
      " 0.850", " 0.849", " 0.848", " 0.847", " 0.846", " 0.845", " 0.844", " 0.843", " 0.842", " 0.841",
      " 0.840", " 0.839", " 0.838", " 0.837", " 0.836", " 0.835", " 0.834", " 0.833", " 0.832", " 0.831",
      " 0.830", " 0.829", " 0.828", " 0.827", " 0.826", " 0.825", " 0.824", " 0.823", " 0.822", " 0.821",
      " 0.820", " 0.819", " 0.818", " 0.817", " 0.816", " 0.815", " 0.814", " 0.813", " 0.812", " 0.811",
      " 0.810", " 0.809", " 0.808", " 0.807", " 0.806", " 0.805", " 0.804", " 0.803", " 0.802", " 0.801",
      " 0.800", " 0.799", " 0.798", " 0.797", " 0.796", " 0.795", " 0.794", " 0.793", " 0.792", " 0.791",
      " 0.790", " 0.789", " 0.788", " 0.787", " 0.786", " 0.785", " 0.784", " 0.783", " 0.782", " 0.781",
      " 0.780", " 0.779", " 0.778", " 0.777", " 0.776", " 0.775", " 0.774", " 0.773", " 0.772", " 0.771",
      " 0.770", " 0.769", " 0.768", " 0.767", " 0.766", " 0.765", " 0.764", " 0.763", " 0.762", " 0.761",
      " 0.760", " 0.759", " 0.758", " 0.757", " 0.756", " 0.755", " 0.754", " 0.753", " 0.752", " 0.751",
      " 0.750", " 0.749", " 0.748", " 0.747", " 0.746", " 0.745", " 0.744", " 0.743", " 0.742", " 0.741",
      " 0.740", " 0.739", " 0.738", " 0.737", " 0.736", " 0.735", " 0.734", " 0.733", " 0.732", " 0.731",
      " 0.730", " 0.729", " 0.728", " 0.727", " 0.726", " 0.725", " 0.724", " 0.723", " 0.722", " 0.721",
      " 0.720", " 0.719", " 0.718", " 0.717", " 0.716", " 0.715", " 0.714", " 0.713", " 0.712", " 0.711",
      " 0.710", " 0.709", " 0.708", " 0.707", " 0.706", " 0.705", " 0.704", " 0.703", " 0.702", " 0.701",
      " 0.700", " 0.699", " 0.698", " 0.697", " 0.696", " 0.695", " 0.694", " 0.693", " 0.692", " 0.691",
      " 0.690", " 0.689", " 0.688", " 0.687", " 0.686", " 0.685", " 0.684", " 0.683", " 0.682", " 0.681",
      " 0.680", " 0.679", " 0.678", " 0.677", " 0.676", " 0.675", " 0.674", " 0.673", " 0.672", " 0.671",
      " 0.670", " 0.669", " 0.668", " 0.667", " 0.666", " 0.665", " 0.664", " 0.663", " 0.662", " 0.661",
      " 0.660", " 0.659", " 0.658", " 0.657", " 0.656", " 0.655", " 0.654", " 0.653", " 0.652", " 0.651",
      " 0.650", " 0.649", " 0.648", " 0.647", " 0.646", " 0.645", " 0.644", " 0.643", " 0.642", " 0.641",
      " 0.640", " 0.639", " 0.638", " 0.637", " 0.636", " 0.635", " 0.634", " 0.633", " 0.632", " 0.631",
      " 0.630", " 0.629", " 0.628", " 0.627", " 0.626", " 0.625", " 0.624", " 0.623", " 0.622", " 0.621",
      " 0.620", " 0.619", " 0.618", " 0.617", " 0.616", " 0.615", " 0.614", " 0.613", " 0.612", " 0.611",
      " 0.610", " 0.609", " 0.608", " 0.607", " 0.606", " 0.605", " 0.604", " 0.603", " 0.602", " 0.601",
      " 0.600", " 0.599", " 0.598", " 0.597", " 0.596", " 0.595", " 0.594", " 0.593", " 0.592", " 0.591",
      " 0.590", " 0.589", " 0.588", " 0.587", " 0.586", " 0.585", " 0.584", " 0.583", " 0.582", " 0.581",
      " 0.580", " 0.579", " 0.578", " 0.577", " 0.576", " 0.575", " 0.574", " 0.573", " 0.572", " 0.571",
      " 0.570", " 0.569", " 0.568", " 0.567", " 0.566", " 0.565", " 0.564", " 0.563", " 0.562", " 0.561",
      " 0.560", " 0.559", " 0.558", " 0.557", " 0.556", " 0.555", " 0.554", " 0.553", " 0.552", " 0.551",
      " 0.550", " 0.549", " 0.548", " 0.547", " 0.546", " 0.545", " 0.544", " 0.543", " 0.542", " 0.541",
      " 0.540", " 0.539", " 0.538", " 0.537", " 0.536", " 0.535", " 0.534", " 0.533", " 0.532", " 0.531",
      " 0.530", " 0.529", " 0.528", " 0.527", " 0.526", " 0.525", " 0.524", " 0.523", " 0.522", " 0.521",
      " 0.520", " 0.519", " 0.518", " 0.517", " 0.516", " 0.515", " 0.514", " 0.513", " 0.512", " 0.511",
      " 0.510", " 0.509", " 0.508", " 0.507", " 0.506", " 0.505", " 0.504", " 0.503", " 0.502", " 0.501",
      " 0.500", " 0.499", " 0.498", " 0.497", " 0.496", " 0.495", " 0.494", " 0.493", " 0.492", " 0.491",
      " 0.490", " 0.489", " 0.488", " 0.487", " 0.486", " 0.485", " 0.484", " 0.483", " 0.482", " 0.481",
      " 0.480", " 0.479", " 0.478", " 0.477", " 0.476", " 0.475", " 0.474", " 0.473", " 0.472", " 0.471",
      " 0.470", " 0.469", " 0.468", " 0.467", " 0.466", " 0.465", " 0.464", " 0.463", " 0.462", " 0.461",
      " 0.460", " 0.459", " 0.458", " 0.457", " 0.456", " 0.455", " 0.454", " 0.453", " 0.452", " 0.451",
      " 0.450", " 0.449", " 0.448", " 0.447", " 0.446", " 0.445", " 0.444", " 0.443", " 0.442", " 0.441",
      " 0.440", " 0.439", " 0.438", " 0.437", " 0.436", " 0.435", " 0.434", " 0.433", " 0.432", " 0.431",
      " 0.430", " 0.429", " 0.428", " 0.427", " 0.426", " 0.425", " 0.424", " 0.423", " 0.422", " 0.421",
      " 0.420", " 0.419", " 0.418", " 0.417", " 0.416", " 0.415", " 0.414", " 0.413", " 0.412", " 0.411",
      " 0.410", " 0.409", " 0.408", " 0.407", " 0.406", " 0.405", " 0.404", " 0.403", " 0.402", " 0.401",
      " 0.400", " 0.399", " 0.398", " 0.397", " 0.396", " 0.395", " 0.394", " 0.393", " 0.392", " 0.391",
      " 0.390", " 0.389", " 0.388", " 0.387", " 0.386", " 0.385", " 0.384", " 0.383", " 0.382", " 0.381",
      " 0.380", " 0.379", " 0.378", " 0.377", " 0.376", " 0.375", " 0.374", " 0.373", " 0.372", " 0.371",
      " 0.370", " 0.369", " 0.368", " 0.367", " 0.366", " 0.365", " 0.364", " 0.363", " 0.362", " 0.361",
      " 0.360", " 0.359", " 0.358", " 0.357", " 0.356", " 0.355", " 0.354", " 0.353", " 0.352", " 0.351",
      " 0.350", " 0.349", " 0.348", " 0.347", " 0.346", " 0.345", " 0.344", " 0.343", " 0.342", " 0.341",
      " 0.340", " 0.339", " 0.338", " 0.337", " 0.336", " 0.335", " 0.334", " 0.333", " 0.332", " 0.331",
      " 0.330", " 0.329", " 0.328", " 0.327", " 0.326", " 0.325", " 0.324", " 0.323", " 0.322", " 0.321",
      " 0.320", " 0.319", " 0.318", " 0.317", " 0.316", " 0.315", " 0.314", " 0.313", " 0.312", " 0.311",
      " 0.310", " 0.309", " 0.308", " 0.307", " 0.306", " 0.305", " 0.304", " 0.303", " 0.302", " 0.301",
      " 0.300", " 0.299", " 0.298", " 0.297", " 0.296", " 0.295", " 0.294", " 0.293", " 0.292", " 0.291",
      " 0.290", " 0.289", " 0.288", " 0.287", " 0.286", " 0.285", " 0.284", " 0.283", " 0.282", " 0.281",
      " 0.280", " 0.279", " 0.278", " 0.277", " 0.276", " 0.275", " 0.274", " 0.273", " 0.272", " 0.271",
      " 0.270", " 0.269", " 0.268", " 0.267", " 0.266", " 0.265", " 0.264", " 0.263", " 0.262", " 0.261",
      " 0.260", " 0.259", " 0.258", " 0.257", " 0.256", " 0.255", " 0.254", " 0.253", " 0.252", " 0.251",
      " 0.250", " 0.249", " 0.248", " 0.247", " 0.246", " 0.245", " 0.244", " 0.243", " 0.242", " 0.241",
      " 0.240", " 0.239", " 0.238", " 0.237", " 0.236", " 0.235", " 0.234", " 0.233", " 0.232", " 0.231",
      " 0.230", " 0.229", " 0.228", " 0.227", " 0.226", " 0.225", " 0.224", " 0.223", " 0.222", " 0.221",
      " 0.220", " 0.219", " 0.218", " 0.217", " 0.216", " 0.215", " 0.214", " 0.213", " 0.212", " 0.211",
      " 0.210", " 0.209", " 0.208", " 0.207", " 0.206", " 0.205", " 0.204", " 0.203", " 0.202", " 0.201",
      " 0.200", " 0.199", " 0.198", " 0.197", " 0.196", " 0.195", " 0.194", " 0.193", " 0.192", " 0.191",
      " 0.190", " 0.189", " 0.188", " 0.187", " 0.186", " 0.185", " 0.184", " 0.183", " 0.182", " 0.181",
      " 0.180", " 0.179", " 0.178", " 0.177", " 0.176", " 0.175", " 0.174", " 0.173", " 0.172", " 0.171",
      " 0.170", " 0.169", " 0.168", " 0.167", " 0.166", " 0.165", " 0.164", " 0.163", " 0.162", " 0.161",
      " 0.160", " 0.159", " 0.158", " 0.157", " 0.156", " 0.155", " 0.154", " 0.153", " 0.152", " 0.151",
      " 0.150", " 0.149", " 0.148", " 0.147", " 0.146", " 0.145", " 0.144", " 0.143", " 0.142", " 0.141",
      " 0.140", " 0.139", " 0.138", " 0.137", " 0.136", " 0.135", " 0.134", " 0.133", " 0.132", " 0.131",
      " 0.130", " 0.129", " 0.128", " 0.127", " 0.126", " 0.125", " 0.124", " 0.123", " 0.122", " 0.121",
      " 0.120", " 0.119", " 0.118", " 0.117", " 0.116", " 0.115", " 0.114", " 0.113", " 0.112", " 0.111",
      " 0.110", " 0.109", " 0.108", " 0.107", " 0.106", " 0.105", " 0.104", " 0.103", " 0.102", " 0.101",
      " 0.100", " 0.099", " 0.098", " 0.097", " 0.096", " 0.095", " 0.094", " 0.093", " 0.092", " 0.091",
      " 0.090", " 0.089", " 0.088", " 0.087", " 0.086", " 0.085", " 0.084", " 0.083", " 0.082", " 0.081",
      " 0.080", " 0.079", " 0.078", " 0.077", " 0.076", " 0.075", " 0.074", " 0.073", " 0.072", " 0.071",
      " 0.070", " 0.069", " 0.068", " 0.067", " 0.066", " 0.065", " 0.064", " 0.063", " 0.062", " 0.061",
      " 0.060", " 0.059", " 0.058", " 0.057", " 0.056", " 0.055", " 0.054", " 0.053", " 0.052", " 0.051",
      " 0.050", " 0.049", " 0.048", " 0.047", " 0.046", " 0.045", " 0.044", " 0.043", " 0.042", " 0.041",
      " 0.040", " 0.039", " 0.038", " 0.037", " 0.036", " 0.035", " 0.034", " 0.033", " 0.032", " 0.031",
      " 0.030", " 0.029", " 0.028", " 0.027", " 0.026", " 0.025", " 0.024", " 0.023", " 0.022", " 0.021",
      " 0.020", " 0.019", " 0.018", " 0.017", " 0.016", " 0.015", " 0.014", " 0.013", " 0.012", " 0.011",
      " 0.010", " 0.009", " 0.008", " 0.007", " 0.006", " 0.005", " 0.004", " 0.003", " 0.002", " 0.001",
      "-0.000", "-0.001", "-0.002", "-0.003", "-0.004", "-0.005", "-0.006", "-0.007", "-0.008", "-0.009",
      "-0.010", "-0.011", "-0.012", "-0.013", "-0.014", "-0.015", "-0.016", "-0.017", "-0.018", "-0.019",
      "-0.020", "-0.021", "-0.022", "-0.023", "-0.024", "-0.025", "-0.026", "-0.027", "-0.028", "-0.029",
      "-0.030", "-0.031", "-0.032", "-0.033", "-0.034", "-0.035", "-0.036", "-0.037", "-0.038", "-0.039",
      "-0.040", "-0.041", "-0.042", "-0.043", "-0.044", "-0.045", "-0.046", "-0.047", "-0.048", "-0.049",
      "-0.050", "-0.051", "-0.052", "-0.053", "-0.054", "-0.055", "-0.056", "-0.057", "-0.058", "-0.059",
      "-0.060", "-0.061", "-0.062", "-0.063", "-0.064", "-0.065", "-0.066", "-0.067", "-0.068", "-0.069",
      "-0.070", "-0.071", "-0.072", "-0.073", "-0.074", "-0.075", "-0.076", "-0.077", "-0.078", "-0.079",
      "-0.080", "-0.081", "-0.082", "-0.083", "-0.084", "-0.085", "-0.086", "-0.087", "-0.088", "-0.089",
      "-0.090", "-0.091", "-0.092", "-0.093", "-0.094", "-0.095", "-0.096", "-0.097", "-0.098", "-0.099",
      "-0.100", "-0.101", "-0.102", "-0.103", "-0.104", "-0.105", "-0.106", "-0.107", "-0.108", "-0.109",
      "-0.110", "-0.111", "-0.112", "-0.113", "-0.114", "-0.115", "-0.116", "-0.117", "-0.118", "-0.119",
      "-0.120", "-0.121", "-0.122", "-0.123", "-0.124", "-0.125", "-0.126", "-0.127", "-0.128", "-0.129",
      "-0.130", "-0.131", "-0.132", "-0.133", "-0.134", "-0.135", "-0.136", "-0.137", "-0.138", "-0.139",
      "-0.140", "-0.141", "-0.142", "-0.143", "-0.144", "-0.145", "-0.146", "-0.147", "-0.148", "-0.149",
      "-0.150", "-0.151", "-0.152", "-0.153", "-0.154", "-0.155", "-0.156", "-0.157", "-0.158", "-0.159",
      "-0.160", "-0.161", "-0.162", "-0.163", "-0.164", "-0.165", "-0.166", "-0.167", "-0.168", "-0.169",
      "-0.170", "-0.171", "-0.172", "-0.173", "-0.174", "-0.175", "-0.176", "-0.177", "-0.178", "-0.179",
      "-0.180", "-0.181", "-0.182", "-0.183", "-0.184", "-0.185", "-0.186", "-0.187", "-0.188", "-0.189",
      "-0.190", "-0.191", "-0.192", "-0.193", "-0.194", "-0.195", "-0.196", "-0.197", "-0.198", "-0.199",
      "-0.200", "-0.201", "-0.202", "-0.203", "-0.204", "-0.205", "-0.206", "-0.207", "-0.208", "-0.209",
      "-0.210", "-0.211", "-0.212", "-0.213", "-0.214", "-0.215", "-0.216", "-0.217", "-0.218", "-0.219",
      "-0.220", "-0.221", "-0.222", "-0.223", "-0.224", "-0.225", "-0.226", "-0.227", "-0.228", "-0.229",
      "-0.230", "-0.231", "-0.232", "-0.233", "-0.234", "-0.235", "-0.236", "-0.237", "-0.238", "-0.239",
      "-0.240", "-0.241", "-0.242", "-0.243", "-0.244", "-0.245", "-0.246", "-0.247", "-0.248", "-0.249",
      "-0.250", "-0.251", "-0.252", "-0.253", "-0.254", "-0.255", "-0.256", "-0.257", "-0.258", "-0.259",
      "-0.260", "-0.261", "-0.262", "-0.263", "-0.264", "-0.265", "-0.266", "-0.267", "-0.268", "-0.269",
      "-0.270", "-0.271", "-0.272", "-0.273", "-0.274", "-0.275", "-0.276", "-0.277", "-0.278", "-0.279",
      "-0.280", "-0.281", "-0.282", "-0.283", "-0.284", "-0.285", "-0.286", "-0.287", "-0.288", "-0.289",
      "-0.290", "-0.291", "-0.292", "-0.293", "-0.294", "-0.295", "-0.296", "-0.297", "-0.298", "-0.299",
      "-0.300", "-0.301", "-0.302", "-0.303", "-0.304", "-0.305", "-0.306", "-0.307", "-0.308", "-0.309",
      "-0.310", "-0.311", "-0.312", "-0.313", "-0.314", "-0.315", "-0.316", "-0.317", "-0.318", "-0.319",
      "-0.320", "-0.321", "-0.322", "-0.323", "-0.324", "-0.325", "-0.326", "-0.327", "-0.328", "-0.329",
      "-0.330", "-0.331", "-0.332", "-0.333", "-0.334", "-0.335", "-0.336", "-0.337", "-0.338", "-0.339",
      "-0.340", "-0.341", "-0.342", "-0.343", "-0.344", "-0.345", "-0.346", "-0.347", "-0.348", "-0.349",
      "-0.350", "-0.351", "-0.352", "-0.353", "-0.354", "-0.355", "-0.356", "-0.357", "-0.358", "-0.359",
      "-0.360", "-0.361", "-0.362", "-0.363", "-0.364", "-0.365", "-0.366", "-0.367", "-0.368", "-0.369",
      "-0.370", "-0.371", "-0.372", "-0.373", "-0.374", "-0.375", "-0.376", "-0.377", "-0.378", "-0.379",
      "-0.380", "-0.381", "-0.382", "-0.383", "-0.384", "-0.385", "-0.386", "-0.387", "-0.388", "-0.389",
      "-0.390", "-0.391", "-0.392", "-0.393", "-0.394", "-0.395", "-0.396", "-0.397", "-0.398", "-0.399",
      "-0.400", "-0.401", "-0.402", "-0.403", "-0.404", "-0.405", "-0.406", "-0.407", "-0.408", "-0.409",
      "-0.410", "-0.411", "-0.412", "-0.413", "-0.414", "-0.415", "-0.416", "-0.417", "-0.418", "-0.419",
      "-0.420", "-0.421", "-0.422", "-0.423", "-0.424", "-0.425", "-0.426", "-0.427", "-0.428", "-0.429",
      "-0.430", "-0.431", "-0.432", "-0.433", "-0.434", "-0.435", "-0.436", "-0.437", "-0.438", "-0.439",
      "-0.440", "-0.441", "-0.442", "-0.443", "-0.444", "-0.445", "-0.446", "-0.447", "-0.448", "-0.449",
      "-0.450", "-0.451", "-0.452", "-0.453", "-0.454", "-0.455", "-0.456", "-0.457", "-0.458", "-0.459",
      "-0.460", "-0.461", "-0.462", "-0.463", "-0.464", "-0.465", "-0.466", "-0.467", "-0.468", "-0.469",
      "-0.470", "-0.471", "-0.472", "-0.473", "-0.474", "-0.475", "-0.476", "-0.477", "-0.478", "-0.479",
      "-0.480", "-0.481", "-0.482", "-0.483", "-0.484", "-0.485", "-0.486", "-0.487", "-0.488", "-0.489",
      "-0.490", "-0.491", "-0.492", "-0.493", "-0.494", "-0.495", "-0.496", "-0.497", "-0.498", "-0.499",
      "-0.500", "-0.501", "-0.502", "-0.503", "-0.504", "-0.505", "-0.506", "-0.507", "-0.508", "-0.509",
      "-0.510", "-0.511", "-0.512", "-0.513", "-0.514", "-0.515", "-0.516", "-0.517", "-0.518", "-0.519",
      "-0.520", "-0.521", "-0.522", "-0.523", "-0.524", "-0.525", "-0.526", "-0.527", "-0.528", "-0.529",
      "-0.530", "-0.531", "-0.532", "-0.533", "-0.534", "-0.535", "-0.536", "-0.537", "-0.538", "-0.539",
      "-0.540", "-0.541", "-0.542", "-0.543", "-0.544", "-0.545", "-0.546", "-0.547", "-0.548", "-0.549",
      "-0.550", "-0.551", "-0.552", "-0.553", "-0.554", "-0.555", "-0.556", "-0.557", "-0.558", "-0.559",
      "-0.560", "-0.561", "-0.562", "-0.563", "-0.564", "-0.565", "-0.566", "-0.567", "-0.568", "-0.569",
      "-0.570", "-0.571", "-0.572", "-0.573", "-0.574", "-0.575", "-0.576", "-0.577", "-0.578", "-0.579",
      "-0.580", "-0.581", "-0.582", "-0.583", "-0.584", "-0.585", "-0.586", "-0.587", "-0.588", "-0.589",
      "-0.590", "-0.591", "-0.592", "-0.593", "-0.594", "-0.595", "-0.596", "-0.597", "-0.598", "-0.599",
      "-0.600", "-0.601", "-0.602", "-0.603", "-0.604", "-0.605", "-0.606", "-0.607", "-0.608", "-0.609",
      "-0.610", "-0.611", "-0.612", "-0.613", "-0.614", "-0.615", "-0.616", "-0.617", "-0.618", "-0.619",
      "-0.620", "-0.621", "-0.622", "-0.623", "-0.624", "-0.625", "-0.626", "-0.627", "-0.628", "-0.629",
      "-0.630", "-0.631", "-0.632", "-0.633", "-0.634", "-0.635", "-0.636", "-0.637", "-0.638", "-0.639",
      "-0.640", "-0.641", "-0.642", "-0.643", "-0.644", "-0.645", "-0.646", "-0.647", "-0.648", "-0.649",
      "-0.650", "-0.651", "-0.652", "-0.653", "-0.654", "-0.655", "-0.656", "-0.657", "-0.658", "-0.659",
      "-0.660", "-0.661", "-0.662", "-0.663", "-0.664", "-0.665", "-0.666", "-0.667", "-0.668", "-0.669",
      "-0.670", "-0.671", "-0.672", "-0.673", "-0.674", "-0.675", "-0.676", "-0.677", "-0.678", "-0.679",
      "-0.680", "-0.681", "-0.682", "-0.683", "-0.684", "-0.685", "-0.686", "-0.687", "-0.688", "-0.689",
      "-0.690", "-0.691", "-0.692", "-0.693", "-0.694", "-0.695", "-0.696", "-0.697", "-0.698", "-0.699",
      "-0.700", "-0.701", "-0.702", "-0.703", "-0.704", "-0.705", "-0.706", "-0.707", "-0.708", "-0.709",
      "-0.710", "-0.711", "-0.712", "-0.713", "-0.714", "-0.715", "-0.716", "-0.717", "-0.718", "-0.719",
      "-0.720", "-0.721", "-0.722", "-0.723", "-0.724", "-0.725", "-0.726", "-0.727", "-0.728", "-0.729",
      "-0.730", "-0.731", "-0.732", "-0.733", "-0.734", "-0.735", "-0.736", "-0.737", "-0.738", "-0.739",
      "-0.740", "-0.741", "-0.742", "-0.743", "-0.744", "-0.745", "-0.746", "-0.747", "-0.748", "-0.749",
      "-0.750", "-0.751", "-0.752", "-0.753", "-0.754", "-0.755", "-0.756", "-0.757", "-0.758", "-0.759",
      "-0.760", "-0.761", "-0.762", "-0.763", "-0.764", "-0.765", "-0.766", "-0.767", "-0.768", "-0.769",
      "-0.770", "-0.771", "-0.772", "-0.773", "-0.774", "-0.775", "-0.776", "-0.777", "-0.778", "-0.779",
      "-0.780", "-0.781", "-0.782", "-0.783", "-0.784", "-0.785", "-0.786", "-0.787", "-0.788", "-0.789",
      "-0.790", "-0.791", "-0.792", "-0.793", "-0.794", "-0.795", "-0.796", "-0.797", "-0.798", "-0.799",
      "-0.800", "-0.801", "-0.802", "-0.803", "-0.804", "-0.805", "-0.806", "-0.807", "-0.808", "-0.809",
      "-0.810", "-0.811", "-0.812", "-0.813", "-0.814", "-0.815", "-0.816", "-0.817", "-0.818", "-0.819",
      "-0.820", "-0.821", "-0.822", "-0.823", "-0.824", "-0.825", "-0.826", "-0.827", "-0.828", "-0.829",
      "-0.830", "-0.831", "-0.832", "-0.833", "-0.834", "-0.835", "-0.836", "-0.837", "-0.838", "-0.839",
      "-0.840", "-0.841", "-0.842", "-0.843", "-0.844", "-0.845", "-0.846", "-0.847", "-0.848", "-0.849",
      "-0.850", "-0.851", "-0.852", "-0.853", "-0.854", "-0.855", "-0.856", "-0.857", "-0.858", "-0.859",
      "-0.860", "-0.861", "-0.862", "-0.863", "-0.864", "-0.865", "-0.866", "-0.867", "-0.868", "-0.869",
      "-0.870", "-0.871", "-0.872", "-0.873", "-0.874", "-0.875", "-0.876", "-0.877", "-0.878", "-0.879",
      "-0.880", "-0.881", "-0.882", "-0.883", "-0.884", "-0.885", "-0.886", "-0.887", "-0.888", "-0.889",
      "-0.890", "-0.891", "-0.892", "-0.893", "-0.894", "-0.895", "-0.896", "-0.897", "-0.898", "-0.899",
      "-0.900", "-0.901", "-0.902", "-0.903", "-0.904", "-0.905", "-0.906", "-0.907", "-0.908", "-0.909",
      "-0.910", "-0.911", "-0.912", "-0.913", "-0.914", "-0.915", "-0.916", "-0.917", "-0.918", "-0.919",
      "-0.920", "-0.921", "-0.922", "-0.923", "-0.924", "-0.925", "-0.926", "-0.927", "-0.928", "-0.929",
      "-0.930", "-0.931", "-0.932", "-0.933", "-0.934", "-0.935", "-0.936", "-0.937", "-0.938", "-0.939",
      "-0.940", "-0.941", "-0.942", "-0.943", "-0.944", "-0.945", "-0.946", "-0.947", "-0.948", "-0.949",
      "-0.950", "-0.951", "-0.952", "-0.953", "-0.954", "-0.955", "-0.956", "-0.957", "-0.958", "-0.959",
      "-0.960", "-0.961", "-0.962", "-0.963", "-0.964", "-0.965", "-0.966", "-0.967", "-0.968", "-0.969",
      "-0.970", "-0.971", "-0.972", "-0.973", "-0.974", "-0.975", "-0.976", "-0.977", "-0.978", "-0.979",
      "-0.980", "-0.981", "-0.982", "-0.983", "-0.984", "-0.985", "-0.986", "-0.987", "-0.988", "-0.989",
      "-0.990", "-0.991", "-0.992", "-0.993", "-0.994", "-0.995", "-0.996", "-0.997", "-0.998", "-0.999",
      "-1.000", "-1.001", "-1.002", "-1.003", "-1.004", "-1.005", "-1.006", "-1.007", "-1.008", "-1.009",
      "-1.010", "-1.011", "-1.012", "-1.013", "-1.014", "-1.015", "-1.016", "-1.017", "-1.018", "-1.019",
      "-1.020", "-1.021", "-1.022", "-1.023", "-1.024", "-1.025", "-1.026", "-1.027", "-1.028", "-1.029",
      "-1.030", "-1.031", "-1.032", "-1.033", "-1.034", "-1.035", "-1.036", "-1.037", "-1.038", "-1.039",
      "-1.040", "-1.041", "-1.042", "-1.043", "-1.044", "-1.045", "-1.046", "-1.047", "-1.048", "-1.049",
      "-1.050", "-1.051", "-1.052", "-1.053", "-1.054", "-1.055", "-1.056", "-1.057", "-1.058", "-1.059",
      "-1.060", "-1.061", "-1.062", "-1.063", "-1.064", "-1.065", "-1.066", "-1.067", "-1.068", "-1.069",
      "-1.070", "-1.071", "-1.072", "-1.073", "-1.074", "-1.075", "-1.076", "-1.077", "-1.078", "-1.079",
      "-1.080", "-1.081", "-1.082", "-1.083", "-1.084", "-1.085", "-1.086", "-1.087", "-1.088", "-1.089",
      "-1.090", "-1.091", "-1.092", "-1.093", "-1.094", "-1.095", "-1.096", "-1.097", "-1.098", "-1.099",
      "-1.100", "-1.101", "-1.102", "-1.103", "-1.104", "-1.105", "-1.106", "-1.107", "-1.108", "-1.109",
      "-1.110", "-1.111", "-1.112", "-1.113", "-1.114", "-1.115", "-1.116", "-1.117", "-1.118", "-1.119",
      "-1.120", "-1.121", "-1.122", "-1.123", "-1.124", "-1.125", "-1.126", "-1.127", "-1.128", "-1.129",
      "-1.130", "-1.131", "-1.132", "-1.133", "-1.134", "-1.135", "-1.136", "-1.137", "-1.138", "-1.139",
      "-1.140", "-1.141", "-1.142", "-1.143", "-1.144", "-1.145", "-1.146", "-1.147", "-1.148", "-1.149",
      "-1.150", "-1.151", "-1.152", "-1.153", "-1.154", "-1.155", "-1.156", "-1.157", "-1.158", "-1.159",
      "-1.160", "-1.161", "-1.162", "-1.163", "-1.164", "-1.165", "-1.166", "-1.167", "-1.168", "-1.169",
      "-1.170", "-1.171", "-1.172", "-1.173", "-1.174", "-1.175", "-1.176", "-1.177", "-1.178", "-1.179",
      "-1.180", "-1.181", "-1.182", "-1.183", "-1.184", "-1.185", "-1.186", "-1.187", "-1.188", "-1.189",
      "-1.190", "-1.191", "-1.192", "-1.193", "-1.194", "-1.195", "-1.196", "-1.197", "-1.198", "-1.199",
      "-1.200", "-1.201", "-1.202", "-1.203", "-1.204", "-1.205", "-1.206", "-1.207", "-1.208", "-1.209",
      "-1.210", "-1.211", "-1.212", "-1.213", "-1.214", "-1.215", "-1.216", "-1.217", "-1.218", "-1.219",
      "-1.220", "-1.221", "-1.222", "-1.223", "-1.224", "-1.225", "-1.226", "-1.227", "-1.228", "-1.229",
      "-1.230", "-1.231", "-1.232", "-1.233", "-1.234", "-1.235", "-1.236", "-1.237", "-1.238", "-1.239",
      "-1.240", "-1.241", "-1.242", "-1.243", "-1.244", "-1.245", "-1.246", "-1.247", "-1.248", "-1.249",
      "-1.250"
  };
  return normalized_to_float_str_arr[raw_to_normalized(raw)];
}
