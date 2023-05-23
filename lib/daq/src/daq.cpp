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

#include "daq.h"

bool daq::FlexIODAQ::init(unsigned int sample_rate) {
  // Maximum FlexIO clock speed is 120MHz, see https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.1025
  // PLL3 is 480MHz, divide that by 2*2 = 4
  flexio->setClockSettings(3, 1, 1);
  if (!_init_cnvst(0))
    return false;
  if (!_init_clk())
    return false;

  return true;
}

bool daq::FlexIODAQ::_init_cnvst(unsigned int sample_rate) {
  flexio->setIOPinToFlexMode(PIN_CNVST);

  uint8_t _sample_timer_idx = flexio->requestTimers(1);
  flexio->port().TIMCTL[_sample_timer_idx] = FLEXIO_TIMCTL_TIMOD(0b11);
  flexio->port().TIMCFG[_sample_timer_idx] = 0;
  flexio->port().TIMCMP[_sample_timer_idx] = 1500;

  uint8_t _cnvst_timer_idx = flexio->requestTimers(1);
  flexio->port().TIMCTL[_cnvst_timer_idx] =
      FLEXIO_TIMCTL_PINSEL(_flexio_pin_cnvst) | FLEXIO_TIMCTL_PINCFG(0b11) | FLEXIO_TIMCTL_TRGSRC |
      FLEXIO_TIMCTL_TRGSEL(4 * _sample_timer_idx + 3) | FLEXIO_TIMCTL_TIMOD(0b10);
  flexio->port().TIMCFG[_cnvst_timer_idx] = FLEXIO_TIMCFG_TIMDIS(0b010) | FLEXIO_TIMCFG_TIMENA(0b111);
  flexio->port().TIMCMP[_cnvst_timer_idx] = 0x0000'10'40;

  flexio->setIOPinToFlexMode(PIN_CLK);
  uint8_t _clk_timer_idx = flexio->requestTimers(1);
  flexio->port().TIMCTL[_clk_timer_idx] =
      FLEXIO_TIMCTL_PINSEL(_flexio_pin_clk) | FLEXIO_TIMCTL_PINCFG(0b11) | FLEXIO_TIMCTL_TRGSRC |
      FLEXIO_TIMCTL_TRGSEL(4 * _cnvst_timer_idx + 3) | FLEXIO_TIMCTL_TIMOD(0b01) | FLEXIO_TIMCTL_TRGPOL;
  flexio->port().TIMCFG[_clk_timer_idx] = FLEXIO_TIMCFG_TIMDIS(0b010) | FLEXIO_TIMCFG_TIMENA(0b110);
  flexio->port().TIMCMP[_clk_timer_idx] = 0x0000'1F'01;
  return true;
}

bool daq::FlexIODAQ::_init_clk() { return true; }

daq::FlexIODAQ::FlexIODAQ()
    : flexio(FlexIOHandler::mapIOPinToFlexIOHandler(PIN_CNVST, _flexio_pin_cnvst)),
      _flexio_pin_clk(flexio->mapIOPinToFlexPin(PIN_CLK)) {}

void daq::FlexIODAQ::enable() { flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN; }

void daq::OneshotDAQ::init() {
  pinMode(PIN_CNVST, OUTPUT);
  digitalWriteFast(PIN_CNVST, LOW);
  pinMode(PIN_CLK, OUTPUT);
  digitalWriteFast(PIN_CLK, LOW);

  for (auto pin : PINS_MISO) {
    // Pull-up is on hardware
    pinMode(pin, INPUT);
  }
}

std::array<uint16_t, daq::NUM_CHANNELS> daq::OneshotDAQ::sample_raw() {
  // Trigger CNVST
  digitalWriteFast(PIN_CNVST, HIGH);
  delayNanoseconds(1500);
  digitalWriteFast(PIN_CNVST, LOW);

  delayNanoseconds(500);

  decltype(sample_raw()) data{0};
  for (auto clk_i = 0; clk_i < 16; clk_i++) {
    digitalWriteFast(PIN_CLK, HIGH);
    // Sample data after rising edge
    for (unsigned int pin_i = 0; pin_i < data.size(); pin_i++) {
      data[pin_i] |= digitalReadFast(PINS_MISO[pin_i]) ? (1 << (15 - clk_i)) : 0;
    }
    delayNanoseconds(50);
    digitalWriteFast(PIN_CLK, LOW);
    delayNanoseconds(250);
  }

  return data;
}
