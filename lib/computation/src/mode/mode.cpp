// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "mode/mode.h"
#include "mode/counters.h"

#include <Arduino.h>

#include "utils/factorize.h"
#include "utils/logging.h"

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
  mode::PerformanceCounter::get().to(mode::Mode::IC);
}

void mode::ManualControl::to_op() {
  digitalWriteFast(PIN_MODE_IC, HIGH);
  digitalWriteFast(PIN_MODE_OP, LOW);
  mode::PerformanceCounter::get().to(mode::Mode::OP);
}

void mode::ManualControl::to_halt() {
  digitalWriteFast(PIN_MODE_IC, HIGH);
  digitalWriteFast(PIN_MODE_OP, HIGH);
  mode::PerformanceCounter::get().to(mode::Mode::HALT);
}

void mode::RealManualControl::enable() {
  if (mode::FlexIOControl::is_initialized()) {
    if (mode::FlexIOControl::is_enabled()) {
      mode::FlexIOControl::disable();
    }
  }
}

void mode::RealManualControl::disable() {
  if (mode::FlexIOControl::is_initialized()) {
    if (!mode::FlexIOControl::is_enabled()) {
      mode::FlexIOControl::enable();
    } else {
      // this should not happen.
    }
  }
}

void mode::RealManualControl::to_ic() {
  enable();
  if (mode::FlexIOControl::is_initialized()) {
    mode::FlexIOControl::to_ic();
  } else {
    mode::ManualControl::to_ic();
  }
}

void mode::RealManualControl::to_op() {
  enable();
  if (mode::FlexIOControl::is_initialized()) {
    mode::FlexIOControl::to_op();
  } else {
    mode::ManualControl::to_op();
  }
}

void mode::RealManualControl::to_halt() {
  enable();
  if (mode::FlexIOControl::is_initialized()) {
    mode::FlexIOControl::to_idle();
  } else {
    mode::ManualControl::to_halt();
  }
}

// storage and default values for static class members
bool mode::FlexIOControl::_is_initialized = false;
bool mode::FlexIOControl::_is_enabled = false;

bool mode::FlexIOControl::init(unsigned long long ic_time_ns, unsigned long long op_time_ns,
                               mode::OnOverload on_overload, mode::OnExtHalt on_ext_halt, mode::Sync sync) {
  // Initialize and reset QTMR
  _init_qtmr_op();
  _reset_qtmr_op();

  // Get FlexIO handler for initialization
  auto flexio = FlexIOHandler::flexIOHandler_list[2];

  // Set clock settings
  // For (3, 0, 0) the clock frequency is 480'000'000
  flexio->setClockSettings(CLK_SEL, CLK_PRED, CLK_PODF);
  auto CLK_FREQ = flexio->computeClockRate();
  auto CLK_FREQ_MHz = CLK_FREQ / 1'000'000;
  // Enable fast access?
  // flexio->port().CTRL |= FLEXIO_CTRL_FASTACC;

  //
  // Configure sync matcher
  //

  // Get internal FlexIO pins from external pins
  uint8_t _sck_flex_pin = flexio->mapIOPinToFlexPin(PIN_SYNC_CLK);
  if (_sck_flex_pin == 0xff)
    return false;

  // Configure a timer to track signal of PIN_SYNC_CLK
  flexio->port().TIMCTL[t_sync_clk] = FLEXIO_TIMCTL_PINSEL(_sck_flex_pin) | FLEXIO_TIMCTL_TIMOD(1);
  flexio->port().TIMCFG[t_sync_clk] = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDEC(2);
  flexio->port().TIMCMP[t_sync_clk] = 0x0000'FF'00;

  // Select a shifter for monitoring PIN_SYNC_ID
  auto _flexio_pin_data_in = flexio->mapIOPinToFlexPin(PIN_SYNC_ID);
  if (_flexio_pin_data_in == 0xff)
    return false;
  // Configure the shifter into continuous match mode, monitoring SYNC_CLK_ID
  flexio->port().SHIFTCTL[z_sync_match] = FLEXIO_SHIFTCTL_TIMSEL(t_sync_clk) |
                                          FLEXIO_SHIFTCTL_PINSEL(_flexio_pin_data_in) |
                                          FLEXIO_SHIFTCTL_SMOD(0b101);
  flexio->port().SHIFTCFG[z_sync_match] = 0;
  // Set compare value in SHIFTBUF[31:16] and mask in SHIFTBUF[15:0] (1=mask, 0=no mask)
  flexio->port().SHIFTBUF[z_sync_match] = 0b01010101'00001111'00000000'00000000;

  // Get a timer which is enabled when there is a match
  // Configure timer
  flexio->port().TIMCTL[t_sync_trigger] =
      FLEXIO_TIMCTL_TRGSEL(4 * z_sync_match + 1) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TIMOD(1);
  flexio->port().TIMCFG[t_sync_trigger] = FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[t_sync_trigger] = 0x0000'01'00;

  //
  //  Configure IDLE state
  //

  switch (sync) {
  case Sync::NONE:
    flexio->port().SHIFTCTL[s_idle] = FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_SMOD_STATE;
    flexio->port().SHIFTBUF[s_idle] = FLEXIO_STATE_SHIFTBUF(0b11111111, s_idle);
  case Sync::MASTER:
  case Sync::SLAVE:
    flexio->port().SHIFTCTL[s_idle] =
        FLEXIO_SHIFTCTL_TIMSEL(t_sync_trigger) | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_SMOD_STATE;
    flexio->port().SHIFTBUF[s_idle] = FLEXIO_STATE_SHIFTBUF(0b11111111, s_ic);
  }
  flexio->port().SHIFTCFG[s_idle] = 0;

  //
  //  Configure state check timer
  //

  // Some states are changed not by timer triggers, but by continuously checking of the inputs
  // Configure state change check timer as fast as possible
  flexio->port().TIMCTL[t_state_check] = FLEXIO_TIMCTL_TIMOD(3);
  flexio->port().TIMCFG[t_state_check] = FLEXIO_TIMCFG_TIMDIS(0) | FLEXIO_TIMCFG_TIMENA(0);
  flexio->port().TIMCMP[t_state_check] = 0x0000'0001;

  //
  //  Configure IC state
  //

  // Sanity check ic_time_ns
  if (ic_time_ns < 100 or ic_time_ns >= 9'000'000'000) {
    LOG_ERROR("FlexIOControl: Requested ic_time_ns cannot be represented by 32bit.");
    return false;
  }

  if (ic_time_ns < 0xFFFFull * 1000ull / CLK_FREQ_MHz) {
    // One 16bit timer is enough actually
    flexio->port().TIMCTL[t_ic] = FLEXIO_TIMCTL_TRGSEL_STATE(s_ic) | FLEXIO_TIMCTL_TIMOD(3) |
                                  FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(17);
    flexio->port().TIMCFG[t_ic] =
        FLEXIO_TIMCFG_TIMRST(6) | FLEXIO_TIMCFG_TIMDIS(0b110) | FLEXIO_TIMCFG_TIMENA(6) | FLEXIO_TIMCFG_TIMOUT(1);
    flexio->port().TIMCMP[t_ic] = ic_time_ns * CLK_FREQ_MHz / 1000;

    // Reset second timer used when going towards higher times
    flexio->port().TIMCTL[t_ic_second] = 0;
    flexio->port().TIMCFG[t_ic_second] = 0;
    flexio->port().TIMCMP[t_ic_second] = 0;
  } else {
    // We split counting to two chained timers
    auto factors = utils::factorize(ic_time_ns * CLK_FREQ_MHz / 1000);
    if (factors.first >= 1 << 16 ||
        factors.second >= 1 << 16) // Factror even is too big for two cascaded timers
      return false;

    // Configure state timer
    flexio->port().TIMCTL[t_ic] = FLEXIO_TIMCTL_TRGSEL_STATE(s_ic) | FLEXIO_TIMCTL_TIMOD(3) |
                                  FLEXIO_TIMCTL_PINPOL;
    flexio->port().TIMCFG[t_ic] = FLEXIO_TIMCFG_TIMRST(6) | FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6);
    flexio->port().TIMCMP[t_ic] = factors.first - 1;
    // Configure second timer
    flexio->port().TIMCTL[t_ic_second] = FLEXIO_TIMCTL_TRGSEL(4 * t_ic + 3) | FLEXIO_TIMCTL_TRGSRC |
                                         FLEXIO_TIMCTL_TIMOD(3) | FLEXIO_TIMCTL_PINCFG(3) |
                                         FLEXIO_TIMCTL_PINSEL(17);
    flexio->port().TIMCFG[t_ic_second] = FLEXIO_TIMCFG_TIMDEC(1) | FLEXIO_TIMCFG_TIMRST(0) |
                                         FLEXIO_TIMCFG_TIMDIS(1) | FLEXIO_TIMCFG_TIMENA(1) |
                                         FLEXIO_TIMCFG_TIMOUT(1);
    flexio->port().TIMCMP[t_ic_second] = factors.second;
  }

  // Configure state shifter
  flexio->port().SHIFTCTL[s_ic] =
      FLEXIO_SHIFTCTL_TIMSEL(t_state_check) | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(15) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_ic] = 0;
  flexio->port().SHIFTBUF[s_ic] =
      FLEXIO_STATE_SHIFTBUF(0b11101111, s_ic, s_ic, s_ic, s_ic, s_op, s_op, s_op, s_op);

  //
  // Configure OP state.
  //
  // State changes are triggered continuously as fast as possible,
  // but we only leave OP when the correct input is set.
  //

  // Sanity check op_time_ns, which we will count with two 16bit timers (=32bit) at CLK_FREQ
  // Also, we don't really want to do extremely short OP times (for now?)
  if (op_time_ns < 100 or op_time_ns > 9'000'000'000) {
    LOG_ERROR("FlexIOControl: Requested op_time_ns cannot be represented by 32bit.");
    return false;
  }

  // Configure a timer to set an input pin high, signaling end of op_time
  if (op_time_ns < 0xFFFFull * 1000ull / CLK_FREQ_MHz) {
    // One 16bit timer is enough actually
    flexio->port().TIMCTL[t_op] = FLEXIO_TIMCTL_TRGSEL_STATE(s_op) | FLEXIO_TIMCTL_TIMOD(3) |
                                  FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(12);
    flexio->port().TIMCFG[t_op] =
        FLEXIO_TIMCFG_TIMRST(6) | FLEXIO_TIMCFG_TIMDIS(0b110) | FLEXIO_TIMCFG_TIMENA(6) | FLEXIO_TIMCFG_TIMOUT(1);
    flexio->port().TIMCMP[t_op] = op_time_ns * CLK_FREQ_MHz / 1000;

    // Reset second timer used when going towards higher op_times
    flexio->port().TIMCTL[t_op_second] = 0;
    flexio->port().TIMCFG[t_op_second] = 0;
    flexio->port().TIMCMP[t_op_second] = 0;
  } else {
    // Configure first timer as pre-scaler
    // But we want to pre-scale as much as possible, even though we then lose resolution
    // But for op times in the range of seconds, we don't need microseconds resolution

    auto factors = utils::factorize(op_time_ns * CLK_FREQ_MHz / 1000);
    if (factors.first >= 1 << 16 ||
        factors.second >= 1 << 16) // Factror even is too big for two cascaded timers
      return false;

    flexio->port().TIMCTL[t_op] =
        FLEXIO_TIMCTL_TRGSEL_STATE(s_op) | FLEXIO_TIMCTL_TIMOD(3) | FLEXIO_TIMCTL_PINPOL;
    flexio->port().TIMCFG[t_op] =
        FLEXIO_TIMCFG_TIMRST(6) | FLEXIO_TIMCFG_TIMDIS(0b110) | FLEXIO_TIMCFG_TIMENA(6);
    flexio->port().TIMCMP[t_op] = factors.first - 1;
    // Configure second timer for 32bit total
    flexio->port().TIMCTL[t_op_second] = FLEXIO_TIMCTL_TRGSEL(4 * t_op + 3) | FLEXIO_TIMCTL_TRGSRC |
                                         FLEXIO_TIMCTL_TIMOD(3) | FLEXIO_TIMCTL_PINCFG(3) |
                                         FLEXIO_TIMCTL_PINSEL(12);
    flexio->port().TIMCFG[t_op_second] =
        FLEXIO_TIMCFG_TIMDEC(1) | FLEXIO_TIMCFG_TIMRST(0) | FLEXIO_TIMCFG_TIMDIS(1) | FLEXIO_TIMCFG_TIMENA(1) | FLEXIO_TIMCFG_TIMOUT(1);
    flexio->port().TIMCMP[t_op_second] = factors.second;
  }

  // Configure state shifter
  flexio->port().SHIFTCTL[s_op] = FLEXIO_SHIFTCTL_TIMSEL(t_state_check) | FLEXIO_SHIFTCTL_PINCFG(3) |
                                  FLEXIO_SHIFTCTL_PINSEL(10) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_op] = 0;
  // Next state after OP depends on FlexIO inputs 10 (0 if overload), 11 (0 if exthalt), 12 (1 if op time over)
  // Check with priority op-time-over > overload > ext halt
  // Comments are t/T whether op-time-over, o/O whether overload active, e/E wether ext halt is true
  uint8_t next_if_overload_and_exthalt = s_op, next_if_overload = s_op, next_if_exthalt = s_op;
  switch (on_ext_halt) {
  case OnExtHalt::IGNORE:
    next_if_exthalt = s_op;
    break;
  case OnExtHalt::PAUSE_THEN_RESTART:
    next_if_exthalt = s_exthalt;
    break;
  }
  switch (on_overload) {
  case OnOverload::IGNORE:
    next_if_overload = s_op;
    next_if_overload_and_exthalt = next_if_exthalt;
    break;
  case OnOverload::HALT:
    next_if_overload = s_overload;
    next_if_overload_and_exthalt = s_overload;
    break;
  }
  flexio->port().SHIFTBUF[s_op] = FLEXIO_STATE_SHIFTBUF(0b11011111,                   // Inputs [12-11-10]
                                                        next_if_overload_and_exthalt, // [0-0-0] = [t-E-O]
                                                        next_if_exthalt,              // [0-0-1] = [t-E-o]
                                                        next_if_overload,             // [0-1-0] = [t-e-O]
                                                        s_op,                         // [0-1-1] = [t-e-o]
                                                        s_end,                        // [1-0-0] = [T-E-O]
                                                        s_end,                        // [1-0-1] = [T-E-o]
                                                        s_end,                        // [1-1-0] = [T-e-O]
                                                        s_end                         // [1-1-1] = [T-e-o]
  );

  //
  // Configure END state.
  //

  flexio->port().SHIFTCTL[s_end] = FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_end] = 0;
  flexio->port().SHIFTBUF[s_end] = FLEXIO_STATE_SHIFTBUF(0b11111111, s_end);

  //
  // Configure OVERLOAD state.
  //

  flexio->port().SHIFTCTL[s_overload] = FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_overload] = 0;
  flexio->port().SHIFTBUF[s_overload] = FLEXIO_STATE_SHIFTBUF(0b11111111, s_overload);

  //
  // Configure EXT HALT state.
  //

  // EXT HALT is a paused state which resumes as soon as the signal is no longer active.
  // But "resuming" is only partially correct, since the full op time is restarted.
  // Thus, the OP state after EXT HALT runs for the full OP time, not just the remainder.
  // This can be solved similarly to the ADC by gating a timer with the OP signal,
  // but that requires additional connections on the PCB.
  // For all currently envisioned EXT HALT applications (e.g. control-systems), this is okay.
  flexio->port().SHIFTCTL[s_exthalt] = FLEXIO_SHIFTCTL_TIMSEL(t_state_check) | FLEXIO_SHIFTCTL_PINCFG(3) |
                                       FLEXIO_SHIFTCTL_PINSEL(10) | FLEXIO_SHIFTCTL_SMOD_STATE;
  flexio->port().SHIFTCFG[s_exthalt] = 0;
  // When selecting next state based on inputs [12-11-10], ignore anything but EXT HALT (11).
  flexio->port().SHIFTBUF[s_exthalt] =
      FLEXIO_STATE_SHIFTBUF(0b11111111, s_exthalt, s_exthalt, s_op, s_op, s_exthalt, s_exthalt, s_op, s_op);

  //
  // Configure miscellaneous flexio stuff
  //

  // Put relevant pins into FlexIO mode
  for (auto pin : {PIN_MODE_IC, PIN_MODE_OP, PIN_MODE_OVERLOAD, PIN_MODE_EXTHALT, PIN_SYNC_ID, PIN_SYNC_CLK}) {
    if (flexio->mapIOPinToFlexPin(pin) == 0xff) {
      return false;
    }
    flexio->setIOPinToFlexMode(pin);
  }

  enable();
  _is_initialized = true;
  return true;
}

void mode::FlexIOControl::disable() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  flexio->port().CTRL &= ~FLEXIO_CTRL_FLEXEN;
  _is_enabled = false;
}

void mode::FlexIOControl::enable() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;
  _is_enabled = true;
}

void mode::FlexIOControl::force_start() { to_ic(); }

void mode::FlexIOControl::to_idle() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  flexio->port().SHIFTSTATE = s_idle;
  mode::PerformanceCounter::get().to(mode::Mode::HALT);
}

void mode::FlexIOControl::to_ic() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  flexio->port().SHIFTSTATE = s_ic;
  mode::PerformanceCounter::get().to(mode::Mode::IC);
}

void mode::FlexIOControl::to_op() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  flexio->port().SHIFTSTATE = s_op;
  mode::PerformanceCounter::get().to(mode::Mode::OP);
}

void mode::FlexIOControl::to_exthalt() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  flexio->port().SHIFTSTATE = s_exthalt;
  mode::PerformanceCounter::get().to(mode::Mode::HALT);
}

void mode::FlexIOControl::to_end() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  flexio->port().SHIFTSTATE = s_end;
  mode::PerformanceCounter::get().to(mode::Mode::HALT);
}

void mode::FlexIOControl::reset() {
  disable();
  delayMicroseconds(1);
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
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

bool mode::FlexIOControl::is_idle() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  return flexio->port().SHIFTSTATE == s_idle;
}

bool mode::FlexIOControl::is_op() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  return flexio->port().SHIFTSTATE == s_op;
}

bool mode::FlexIOControl::is_done() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  auto state = flexio->port().SHIFTSTATE;
  return state == s_end or state == s_overload;
}

bool mode::FlexIOControl::is_overloaded() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  return flexio->port().SHIFTSTATE == s_overload;
}

bool mode::FlexIOControl::is_exthalt() {
  auto flexio = FlexIOHandler::flexIOHandler_list[2];
  return flexio->port().SHIFTSTATE == s_exthalt;
}
