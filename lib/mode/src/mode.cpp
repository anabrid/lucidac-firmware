// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

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
  // Initialize and reset QTMR
  _init_qtmr_op();
  _reset_qtmr_op();

  // Get FlexIO handler for initialization
  auto flexio = FlexIOHandler::flexIOHandler_list[0];

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
  // TODO: Change IC timer to a two-stage timer like the OP timer to increase maximum IC time
  if (ic_time_ns < 100 or ic_time_ns >= 270001)
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
  // Just limit to 1 second for now, the actual math is slightly more complicated
  // Also, we don't really want to do extremely short OP times (for now?)
  if (op_time_ns < 100 or op_time_ns > 1'000'000'000)
    return false;

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
    // But we want to pre-scale as much as possible, even though we then lose resolution
    // But for op times in the range of seconds, we don't need microseconds resolution
    auto order_of_magnitude = min(static_cast<unsigned int>(log10(op_time_ns)), 9);
    // Don't use float-type pow
    // Also divider can't be larger than 273, otherwise pre-scaler > 2^16
    unsigned int divider = 1;
    if (order_of_magnitude >= 6)
      divider = 10;
    if (order_of_magnitude >= 8)
      divider = 100;

    auto t_op = ++_t_idx;
    if (t_op >= 8)
      return false;
    flexio->port().TIMCTL[t_op] = FLEXIO_TIMCTL_TRGSEL_STATE(s_op) | FLEXIO_TIMCTL_TIMOD(3) |
                                  FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(8) | FLEXIO_TIMCTL_PINPOL;
    flexio->port().TIMCFG[t_op] =
        FLEXIO_TIMCFG_TIMRST(6) | FLEXIO_TIMCFG_TIMDIS(0b110) | FLEXIO_TIMCFG_TIMENA(6);
    flexio->port().TIMCMP[t_op] = divider*240 - 1;
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
    flexio->port().TIMCMP[t_op_second] = (op_time_ns / (divider*1000ull)) * 2 - 1;
  }

  // Configure state change check timer as fast as possible
  auto t_check = ++_t_idx;
  if (t_check >= 8)
    return false;
  flexio->port().TIMCTL[t_check] = FLEXIO_TIMCTL_TRGSEL_STATE(s_op) | FLEXIO_TIMCTL_TIMOD(3) |
                                   FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(12);
  flexio->port().TIMCFG[t_check] = FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[t_check] = 0x0000'0001;

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
  while (!is_done()) {
  }
}

void mode::FlexIOControl::_reset_qtmr_op() {
  TMR1_CNTR1 = 0;
  TMR1_CNTR2 = 0;
}

void mode::FlexIOControl::_init_qtmr_op() {
  CCM_CCGR6 |= CCM_CCGR6_QTIMER1(CCM_CCGR_ON);

  // Configure timer 1 of first QTMR module to do input-gated counting
  TMR1_CTRL1 = 0; // stop
  TMR1_CNTR1 = 0; // reset counter
  TMR1_SCTRL1 = 0;
  TMR1_LOAD1 = 0;
  TMR1_CSCTRL1 = 0;
  TMR1_LOAD1 = 0;       // start val after compare
  TMR1_COMP11 = 0xffff; // count up to this val, interrupt,  and start again
  TMR1_CMPLD11 = 0xffff;
  // Set CM=0 for now, enable later, select fastest clock with PCS, select gating signal with SCS
  TMR1_CTRL1 = TMR_CTRL_CM(0) | TMR_CTRL_PCS(8) | TMR_CTRL_SCS(1);
  // Invert secondary signal (gating when HIGH, counting when LOW)
  TMR1_SCTRL1 = TMR_SCTRL_IPS;

  // Configure timer 2 of first QTMR module to cascade from timer 1
  TMR1_CTRL2 = 0;
  TMR1_CNTR2 = 0; // reset counter
  TMR1_SCTRL2 = 0;
  TMR1_LOAD2 = 0;
  TMR1_CSCTRL2 = 0;
  TMR1_LOAD2 = 0;       // start val after compare
  TMR1_COMP12 = 0xffff; // count up to this val and start again
  TMR1_CMPLD12 = 0xffff;
  // Set CM=0 for now, enable later, select first timer with PCS
  TMR1_CTRL2 = TMR_CTRL_CM(0) | TMR_CTRL_PCS(4 + 1);

  // Put PIN_QTMR_OP_GATE in QTimer mode
  *(portConfigRegister(PIN_QTMR_OP_GATE)) = 1; // ALT 1
  // Enable timers in reverse order
  TMR1_CTRL2 |= TMR_CTRL_CM(7);
  TMR1_CTRL1 |= TMR_CTRL_CM(3);
}

unsigned long long mode::FlexIOControl::get_actual_op_time() {
  // TODO: This is currently measured, but of course it can be calculated
  return (TMR1_CNTR2 * 0xFFFF + TMR1_CNTR1) * 671 / 100;
}

bool mode::FlexIOControl::is_done() {
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  return flexio->port().SHIFTSTATE == s_end;
}
