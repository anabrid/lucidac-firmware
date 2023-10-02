// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <algorithm>

#include "daq.h"
#include "logging.h"
#include "running_avg.h"

DMAChannel dma(false);

void _dma_interrupt() {
  digitalToggleFast(13);

  // Clear interrupt
  dma.clearInterrupt();
  // Memory barrier
  asm("DSB");
}

bool daq::FlexIODAQ::init(unsigned int sample_rate) {
  LOG_ANABRID_DEBUG_DAQ(__PRETTY_FUNCTION__);
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

  if (sample_rate > 1'000'000 or sample_rate < 32)
    return false;

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
  flexio->port().TIMCMP[_sample_timer_idx] = (1'000'000 / sample_rate) * 2 - 1;

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
  flexio->port().TIMCMP[_clk_timer_idx] = 0x0000'1F'07;
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
  for (auto _pin_miso_idx = 0; _pin_miso_idx < _flexio_pins_miso.size(); _pin_miso_idx++) {
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
  dma.begin();

  // Configure minor and major loop count of DMA process.
  // One DMA request (SHIFTBUF store) triggers one major loop.
  // A major loop consists of several minor loops, which are executed one after the other.
  // For each loop, source address and destination address is adjusted and data is copied.
  // The general approach is to use the minor loops to copy all shift registers when one triggers the DMA.
  // That means each major loop copies over NUM_CHANNELS data points.
  // Triggering multiple major loops fills up the ring buffer.
  // Once all major loops are done, the ring buffer is full and an interrupt is triggered to handle the data.

  // BITER "beginning iteration count" is the number of minor loops in one major loop.
  // This must equal the number of shift registers that should be read out.
  dma.TCD->BITER = NUM_CHANNELS;
  // CITER "current major loop iteration count" is the number of major loops.
  // This must be adapted to fit the ring buffer size.
  dma.TCD->CITER = dma_buffer.size()/NUM_CHANNELS;

  // Configure source address and its adjustments.
  // We want to circularly copy from SHIFTBUFBIS.
  // In principle, we are only interested in the last 16 bits of each SHIFTBUFBIS.
  // Unfortunately, configuring it in such a way was not successful -- maybe in the future.
  // Instead, we copy the full 32 bit of data.
  // SADDR "source address start" is where the DMA process starts.
  // Set it to the beginning of the SHIFTBUFBIS array.
  dma.TCD->SADDR = flexio->port().SHIFTBUFBIS;
  // NBYTES "minor byte transfer count" is the number of bytes transferred in one minor loop.
  // Set it such that the whole SHIFTBUFBIS array is copied, which is 4 bytes * NUM_CHANNELS
  dma.TCD->NBYTES = 4 * NUM_CHANNELS;
  // SOFF "source address offset" is the offset added onto SADDR for each minor loop.
  // Set it to 4 bytes, equaling the 32 bits each shift register has.
  dma.TCD->SOFF = 4;             // 32bit offset (=1 shift buffer)
  // ATTR_SRC "source address attribute" is an attribute setting the circularity and the transfer size.
  // The format is [5bit MOD][3bit SIZE].
  // The 5bit MOD is the number of lower address bites allowed to change, effectively circling back to SADDR.
  // The 3bit SIZE defines the source data transfer size.
  // Set MOD to 5, allowing the address bits to change until the end of the SHIFTBUFBIS array and cycling.
  // Set SIZE to 0b010 for 32 bit transfer size.
  dma.TCD->ATTR_SRC = B00101010; // [5bit MOD, 00011=3lower bits may change][3bit SIZE, 010=32bit]

  // Configure destination address and its adjustments.
  // We want to circularly copy into a memory ring buffer.
  // Since we always copy 32 bit from SADDR, we will have the memory buffer in a 32 bit layout as well,
  // even though we are again only interested in the lower 16 bits.
  // DADDR "destination address start" is the start of the destination ring buffer.
  // Set to address of ring buffer.
  dma.TCD->DADDR = dma_buffer.data();
  // DOOFF "destination address offset" is the offset added to DADDR for each minor loop.
  // Set to 4 bytes, equaling the 32 bits each shift register has.
  dma.TCD->DOFF = 4;
  // ATTR_SRC "destination address attribute" is analogous to ATTR_SRC
  // Set first 5 bit MOD according to size of ring buffer.
  // Set last 3 bits to 0b010 for 32 bit transfer size.
  uint8_t MOD = __builtin_ctz(BUFFER_SIZE*4);
  // Check if memory buffer address is aligned such that MOD lower bits are zero (maybe this is too pedantic?)
  // TODO: This is not necessary, since we cycle back to DADDR not to address with MOD zero bits -- simplify.
  if (reinterpret_cast<uintptr_t>(dma_buffer.data()) & ~(~static_cast<uintptr_t>(0) << MOD)) {
    LOG_ERROR("DMA buffer memory range is not sufficiently aligned.")
    return false;
  }
  dma.TCD->ATTR_DST = ((MOD & 0b11111) << 3) | 0b010;

  // Call an interrupt when done
  dma.attachInterrupt(_dma_interrupt);
  dma.interruptAtCompletion();
  // Trigger from "shifter full" DMA event
  dma.triggerAtHardwareEvent(flexio->shiftersDMAChannel(shifter_dma_idx));
  // Enable dma channel
  dma.enable();

  if (dma.error())
    return false;

  return true;
}

daq::FlexIODAQ::FlexIODAQ()
    : flexio(FlexIOHandler::mapIOPinToFlexIOHandler(PIN_CNVST, _flexio_pin_cnvst)),
      _flexio_pin_clk(flexio->mapIOPinToFlexPin(PIN_CLK)),
      _flexio_pin_gate(flexio->mapIOPinToFlexPin(PIN_GATE)) {
  std::transform(PINS_MISO.begin(), PINS_MISO.end(), _flexio_pins_miso.begin(),
                 [&](auto pin) { return flexio->mapIOPinToFlexPin(pin); });
}

void daq::FlexIODAQ::enable() { flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN; }

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
}

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
  return ((static_cast<float>(raw) - RAW_MINUS_ONE) / (RAW_PLUS_ONE - RAW_MINUS_ONE)) * 2.0f - 1.0f;
}

std::array<float, daq::NUM_CHANNELS> daq::BaseDAQ::sample_avg(size_t samples, unsigned int delay_us) {
  utils::RunningAverageVec<NUM_CHANNELS> avg;
  for (size_t i = 0; i < samples; i++) {
    avg.add(sample());
    delayMicroseconds(delay_us);
  }
  return avg.get_average();
}

std::array<uint16_t, daq::NUM_CHANNELS> daq::OneshotDAQ::sample_raw() {
  // Trigger CNVST
  digitalWriteFast(PIN_CNVST, HIGH);
  delayNanoseconds(1500);
  digitalWriteFast(PIN_CNVST, LOW);

  delayNanoseconds(1000);

  decltype(sample_raw()) data{0};
  for (auto clk_i = 0; clk_i < 14; clk_i++) {
    digitalWriteFast(PIN_CLK, HIGH);
    delayNanoseconds(100);
    // Sample data after rising edge, but only first 14 bits
    if (clk_i < 14) {
      for (unsigned int pin_i = 0; pin_i < data.size(); pin_i++) {
        data[pin_i] |= digitalReadFast(PINS_MISO[pin_i]) ? (1 << (13 - clk_i)) : 0;
      }
    }
    delayNanoseconds(100);
    digitalWriteFast(PIN_CLK, LOW);
    delayNanoseconds(350);
  }

  return data;
}

std::array<float, daq::NUM_CHANNELS> daq::OneshotDAQ::sample() {
  auto data_raw = sample_raw();
  std::array<float, daq::NUM_CHANNELS> data{};
  std::transform(std::begin(data_raw), std::end(data_raw), std::begin(data), raw_to_float);
  return data;
}

float daq::OneshotDAQ::sample(uint8_t index) { return sample()[index]; }
