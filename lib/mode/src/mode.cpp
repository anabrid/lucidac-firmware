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

#include "mode.h"

#include <Arduino.h>
#include <limits>

void mode::ManualControl::init() {
  digitalWriteFast(PIN_MODE_IC, HIGH);
  digitalWriteFast(PIN_MODE_OP, HIGH);
  pinMode(PIN_MODE_IC, OUTPUT);
  pinMode(PIN_MODE_OP, OUTPUT);
  digitalWriteFast(PIN_MODE_IC, HIGH);
  digitalWriteFast(PIN_MODE_OP, HIGH);
}

void mode::ManualControl::to_ic() {
  digitalWriteFast(PIN_MODE_OP, HIGH);
  digitalWriteFast(PIN_MODE_IC, LOW);
}

void mode::ManualControl::to_op() {
  digitalWriteFast(PIN_MODE_IC, HIGH);
  digitalWriteFast(PIN_MODE_OP, LOW);
}

void mode::ManualControl::to_halt() {
  digitalWriteFast(PIN_MODE_IC, HIGH);
  digitalWriteFast(PIN_MODE_OP, HIGH);
}

bool mode::FlexIOControl::init(unsigned int ic_time_ns, unsigned int op_time_ns) {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  // Reset FlexIO
  // reset();

  // Claim all shifters, even though this will probably not prevent conflicts in different translation units
  for (auto s_ : get_states()) {
    if (!flexio->claimShifter(s_))
      return false;
  }

  // Set clock settings
  flexio->setClockSettings(CLK_SEL, CLK_PRED, CLK_PODF);
  auto CLK_FREQ = flexio->computeClockRate();
  auto CLK_FREQ_MHz = CLK_FREQ / 1'000'000;
  // Enable fast access?
  // flexio->port().CTRL |= FLEXIO_CTRL_FASTACC;

  //
  //  Configure IDLE state
  //

  flexio->port().SHIFTCTL[s_idle] = FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_idle] = 0;
  flexio->port().SHIFTBUF[s_idle] = 0b11111111'000'000'000'000'000'000'000'000;

  //
  //  Configure IC state
  //

  // Sanity check ic_time_ns, which must be countable by a 16bit timer at CLK_FREQ
  // We can change FLEXIO_SHIFTCTL_TIMPOL to only have to divide by 1000, but it does not matter much.
  uint32_t num_ic_clocks = ic_time_ns * CLK_FREQ_MHz / 2000;
  // There is a constant delay (1-2 FlexIO cycles) before enabling the timer, which we correct here
  num_ic_clocks -= 1;
  if (ic_time_ns < 100 or num_ic_clocks >= 1 << 16)
    return false;

  // Configure state timer
  auto t_ic = flexio->requestTimers(1);
  if (t_ic == 0xff)
    return false;
  flexio->port().TIMCTL[t_ic] = FLEXIO_TIMCTL_TRGSEL_STATE(s_ic) | FLEXIO_TIMCTL_TIMOD(3) | FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(8);
  flexio->port().TIMCFG[t_ic] = FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[t_ic] = num_ic_clocks & 0xFFFF;

  // Configure state shifter
  flexio->port().SHIFTCTL[s_ic] =
      FLEXIO_SHIFTCTL_TIMSEL(t_ic) | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_ic] = 0;
  flexio->port().SHIFTBUF[s_ic] = 0b10111111'010'010'010'010'010'010'010'010;

  //
  // Configure OP state
  //

  // Sanity check op_time_ns, which we will count with two 16bit timers (=32bit) at CLK_FREQ
  // TODO: Correctly check for "overflows"
  uint32_t num_op_clocks = op_time_ns * CLK_FREQ_MHz / 2000;
  if (op_time_ns < 100 or num_op_clocks >= std::numeric_limits<uint32_t>::max())
    return false;

  // Configure state timer
  auto t_op = flexio->requestTimers(1);
  if (t_op == 0xff)
    return false;
  flexio->port().TIMCTL[t_op] = FLEXIO_TIMCTL_TRGSEL_STATE(s_op) | FLEXIO_TIMCTL_TIMOD(3);
  flexio->port().TIMCFG[t_op] = FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[t_op] = num_op_clocks & 0xFFFF;

  // Configure state shifter
  flexio->port().SHIFTCTL[s_op] =
      FLEXIO_SHIFTCTL_TIMSEL(t_op) | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_op] = 0;
  flexio->port().SHIFTBUF[s_op] = 0b11011111'000'000'000'000'000'000'000'000;

  // Put relevant pins into FlexIO mode
  for (auto pin : {PIN_MODE_IC, PIN_MODE_OP, static_cast<uint8_t>(5) /* FlexIO1:8 for testing outputs */}) {
    if (flexio->mapIOPinToFlexPin(pin) == 0xff)
      return false;
    flexio->setIOPinToFlexMode(pin);
  }

  enable();
  return true;
}

void mode::FlexIOControl::disable() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  flexio->port().CTRL &= ~FLEXIO_CTRL_FLEXEN;
}

void mode::FlexIOControl::enable() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;
}

void mode::FlexIOControl::force_start() { to_ic(); }

void mode::FlexIOControl::to_idle() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  flexio->port().SHIFTSTATE = s_idle;
}

void mode::FlexIOControl::to_ic() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  flexio->port().SHIFTSTATE = s_ic;
}

void mode::FlexIOControl::to_op() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  flexio->port().SHIFTSTATE = s_op;
}

void mode::FlexIOControl::to_pause() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  flexio->port().SHIFTSTATE = s_pause;
}

void mode::FlexIOControl::to_end() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  flexio->port().SHIFTSTATE = s_end;
}

void mode::FlexIOControl::reset() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  flexio->port().CTRL |= FLEXIO_CTRL_SWRST;
  delayMicroseconds(1);
  flexio->port().CTRL &= ~FLEXIO_CTRL_SWRST;
  delayMicroseconds(1);
}
