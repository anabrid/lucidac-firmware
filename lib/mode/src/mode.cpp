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

bool mode::FlexIOControl::init(unsigned int ic_time_ns, unsigned long long op_time_ns) {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  // Reset FlexIO
  // reset();

  // We hardcode timer indices, otherwise we would need to use freeTimers all the time
  int _t_idx = -1;

  // Set clock settings
  // For (3, 0, 0) the clock frequency is 480'000'000
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
  flexio->port().SHIFTBUF[s_idle] = FLEXIO_STATE_SHIFTBUF(0b11111111, s_idle);

  //
  //  Configure IC state
  //

  // Sanity check ic_time_ns, which must be countable by a 16bit timer at CLK_FREQ
  if (ic_time_ns < 100 or ic_time_ns >= 0xFFFF * 2000 / CLK_FREQ_MHz)
    return false;
  // We can change FLEXIO_SHIFTCTL_TIMPOL to only have to divide by 1000, but it does not matter much.
  uint32_t num_ic_clocks = ic_time_ns * CLK_FREQ_MHz / 2000;
  // There is a constant delay (1-2 FlexIO cycles) before enabling the timer, which we correct here
  num_ic_clocks -= 1;

  // Configure state timer
  auto t_ic = ++_t_idx;
  if (t_ic >= 8)
    return false;
  flexio->port().TIMCTL[t_ic] = FLEXIO_TIMCTL_TRGSEL_STATE(s_ic) | FLEXIO_TIMCTL_TIMOD(3);
  flexio->port().TIMCFG[t_ic] = FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[t_ic] = num_ic_clocks & 0xFFFF;

  // Configure state shifter
  flexio->port().SHIFTCTL[s_ic] =
      FLEXIO_SHIFTCTL_TIMSEL(t_ic) | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_ic] = 0;
  flexio->port().SHIFTBUF[s_ic] = FLEXIO_STATE_SHIFTBUF(0b10111111, s_op);

  //
  // Configure OP state.
  //
  // State changes are triggered continuously as fast as possible,
  // but we only leave OP when the correct input is set.
  //

  // Sanity check op_time_ns, which we will count with two 16bit timers (=32bit) at CLK_FREQ
  // That means we can count up to 0xFFFF*0xFFFF/CLK_FREQ_GHz ~= 8.947 seconds
  // Also, we don't really want to do extremely short OP times (for now?)
  if (op_time_ns < 100 or op_time_ns >= 0xFFFFull * 0xFFFFull * 1000ull / CLK_FREQ_MHz)
    return false;

  // Configure state change check timer as fast as possible
  auto t_check = ++_t_idx;
  if (t_check >= 8)
    return false;
  flexio->port().TIMCTL[t_check] = FLEXIO_TIMCTL_TRGSEL_STATE(s_op) | FLEXIO_TIMCTL_TIMOD(3) |
                                   FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(12);
  flexio->port().TIMCFG[t_check] = FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[t_check] = 0x0000'0001;

  // Configure a timer to set an input pin high, signaling end of op_time
  if (op_time_ns < 0xFFFFull * 1000ull / CLK_FREQ_MHz) {
    // One 16bit timer is enough actually
    auto t_op = ++_t_idx;
    if (t_op >= 8)
      return false;
    flexio->port().TIMCTL[t_op] = FLEXIO_TIMCTL_TRGSEL_STATE(s_op) | FLEXIO_TIMCTL_TIMOD(3) |
                                  FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(13) | FLEXIO_TIMCTL_PINPOL;
    flexio->port().TIMCFG[t_op] =
        FLEXIO_TIMCFG_TIMRST(6) | FLEXIO_TIMCFG_TIMDIS(0b110) | FLEXIO_TIMCFG_TIMENA(6);
    flexio->port().TIMCMP[t_op] = op_time_ns * CLK_FREQ_MHz / 1000;
  } else {
    // Configure first timer as pre-scaler
    auto t_op = ++_t_idx;
    if (t_op >= 8)
      return false;
    flexio->port().TIMCTL[t_op] = FLEXIO_TIMCTL_TRGSEL_STATE(s_op) | FLEXIO_TIMCTL_TIMOD(3) |
                                  FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(8) | FLEXIO_TIMCTL_PINPOL;
    flexio->port().TIMCFG[t_op] =
        FLEXIO_TIMCFG_TIMRST(6) | FLEXIO_TIMCFG_TIMDIS(0b110) | FLEXIO_TIMCFG_TIMENA(6);
    flexio->port().TIMCMP[t_op] = 0x0000'FFFF;
    // Configure second timer for 32bit total
    auto t_op_second = ++_t_idx;
    if (t_op_second >= 8)
      return false;
    // There are constraints when using "when timer N-1 does something"
    if (t_op_second != t_op + 1 or t_op_second % 4 == 0)
      return false;
    flexio->port().TIMCTL[t_op_second] = FLEXIO_TIMCTL_TRGSEL(4 * t_op + 3) | FLEXIO_TIMCTL_TRGSRC |
                                         FLEXIO_TIMCTL_TIMOD(3) | FLEXIO_TIMCTL_PINCFG(3) |
                                         FLEXIO_TIMCTL_PINSEL(13) | FLEXIO_TIMCTL_PINPOL;
    flexio->port().TIMCFG[t_op_second] =
        FLEXIO_TIMCFG_TIMDEC(1) | FLEXIO_TIMCFG_TIMRST(0) | FLEXIO_TIMCFG_TIMDIS(1) | FLEXIO_TIMCFG_TIMENA(1);
    flexio->port().TIMCMP[t_op_second] = op_time_ns * CLK_FREQ_MHz / 1000ull / 0xFFFF;
  }

  // Configure state shifter
  flexio->port().SHIFTCTL[s_op] = FLEXIO_SHIFTCTL_TIMSEL(t_check) | FLEXIO_SHIFTCTL_PINCFG(3) |
                                  FLEXIO_SHIFTCTL_PINSEL(13) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_op] = 0;
  flexio->port().SHIFTBUF[s_op] =
      FLEXIO_STATE_SHIFTBUF(0b11011111, s_ic, s_ic, s_ic, s_ic, s_ic, s_ic, s_op, s_end);

  //
  // Configure END state.
  //

  flexio->port().SHIFTCTL[s_end] =
      FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(13) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_end] = 0;
  flexio->port().SHIFTBUF[s_end] =
      FLEXIO_STATE_SHIFTBUF(0b11111111, s_end);

  //
  // Configure miscellaneous flexio stuff
  //

  // Put relevant pins into FlexIO mode
  for (auto pin : {PIN_MODE_IC, PIN_MODE_OP, static_cast<uint8_t>(5) /* FlexIO1:8 for testing outputs */,
                   static_cast<uint8_t>(52) /* FlexIO1:12 for testing outputs */,
                   static_cast<uint8_t>(49) /* FlexIO1:13 for testing outputs */,
                   static_cast<uint8_t>(50) /* FlexIO1:14 for testing outputs */,
                   static_cast<uint8_t>(54) /* FlexIO1:15 for testing outputs */}) {
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

void mode::FlexIOControl::delay_till_done() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  while (flexio->port().SHIFTSTATE != s_end) {
  }
}
