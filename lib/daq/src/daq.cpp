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
  if (!_init_cnvst(sample_rate))
    return false;

  return true;
}

bool daq::FlexIODAQ::_init_cnvst(unsigned int sample_rate) {
  LOG_ANABRID_DEBUG_DAQ(__PRETTY_FUNCTION__);
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
