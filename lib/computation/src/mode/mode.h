// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <FlexIO_t4.h>
#include <array>
#include <cstdint>

#include "utils/helpers.h"

namespace mode {

constexpr uint8_t PIN_MODE_IC = 40;       // FlexIO 3:4
constexpr uint8_t PIN_MODE_OP = 41;       // FlexIO 3:5
constexpr uint8_t PIN_MODE_OVERLOAD = 20; // FlexIO 3:10
constexpr uint8_t PIN_MODE_EXTHALT = 21;  // FlexIO 3:11
constexpr uint8_t PIN_SYNC_CLK = 22;      // FlexIO 3:8
constexpr uint8_t PIN_SYNC_ID = 23;       // FlexIO 3:9
constexpr uint8_t PIN_QTMR_OP_GATE = 12;  // QTimer11

constexpr unsigned int DEFAULT_IC_TIME = 100'000;
constexpr unsigned long long DEFAULT_OP_TIME = 1'000'000;

enum class OnOverload {
  // In the future, we might want to pause, go to debug, ...?
  IGNORE,
  HALT
};

enum class OnExtHalt {
  // In the future, we might want to do different things
  IGNORE,
  PAUSE_THEN_RESTART
};

enum class Sync { NONE, MASTER, SLAVE };

enum class Mode { IC, OP, HALT };

/**
 * Note that the ManualControl does not work any more once FlexIOControl::init()
 * was called, even if FlexIOControl::disable() is used. This is because the relevant GPIOs
 * are in FlexMode and therefore no more directly addressable. In this case, use the FlexIOControl
 * counterparts for manual control, for instance FlexIOControl::to_ic(), or use the
 * RealManualControl as wrapper between both.
 **/
class ManualControl {
public:
  static void init();
  static void to_ic();
  static void to_op();
  static void to_halt();
};

/**
 * This class can do a manual control even after FlexIOControl has started. It does so
 * by checking whether FlexIO has been used so far and disabling/enabling FlexIO per need.
 **/
class RealManualControl {
public:
  static bool is_enabled;
  static void disable();
  static void enable();

  static void to_ic();
  static void to_op();
  static void to_halt();
};

class FlexIOControl {
private:
  static constexpr uint8_t CLK_SEL = 3, CLK_PRED = 0, CLK_PODF = 0;
  // Hand-tuned assignments of shifters/states
  static constexpr uint8_t s_idle = 0, s_ic = 1, s_op = 2, s_exthalt = 3, s_end = 4, s_overload = 5,
                           z_sync_match = 7;
  // Hand-tuned assignments of timers
  static constexpr uint8_t t_sync_clk = 0, t_sync_trigger = 1, t_ic = 2, t_ic_second = 3, t_op = 4, t_op_second = 5, t_state_check = 6;
  // Check constraints just to be safe
  static_assert(t_sync_clk <= 7 and t_sync_trigger <= 7 and t_ic <= 7 and t_op <= 7 and t_op_second <= 7 and
                    t_state_check <= 7,
                "Timer index out of range.");
  static_assert(all_unique(std::make_tuple(t_sync_clk, t_sync_trigger, t_ic, t_ic_second, t_op, t_op_second,
                                           t_state_check)),
                "All values must be unique.");
  static_assert(t_op_second == t_op + 1 and t_op_second % 4,
                "Chained timers must have consecutive indices, but not 3->4.");
  static_assert(t_ic_second == t_ic + 1 and t_ic_second % 4,
                "Chained timers must have consecutive indices, but not 3->4.");

  static constexpr std::array<uint8_t, 6> get_states() {
    return {s_idle, s_ic, s_op, s_exthalt, s_end, s_overload};
  }

  static constexpr auto FLEXIO_SHIFTCTL_SMOD_STATE = FLEXIO_SHIFTCTL_SMOD(6);

  static constexpr auto FLEXIO_TIMCTL_TRGSEL_STATE(uint8_t s_) {
    return FLEXIO_TIMCTL_TRGSEL(4 * s_ + 1) | FLEXIO_TIMCTL_TRGSRC;
  }

  static constexpr uint32_t FLEXIO_STATE_SHIFTBUF(const uint8_t outputs, const uint8_t next_0,
                                                  const uint8_t next_1, const uint8_t next_2,
                                                  const uint8_t next_3, const uint8_t next_4,
                                                  const uint8_t next_5, const uint8_t next_6,
                                                  const uint8_t next_7) {
    // flexio->port().SHIFTBUF[n] is
    // [8 bit outputs][3 bit next state][3 bit][3 bit][3 bit][3 bit][3 bit][3 bit][3 bit]
    // e.g. 0b11011111'010'010'000'010'010'010'000'010;
    // With [3 bit next] according to
    // FXIO_D[PINSEL+2] FXIO_D[PINSEL+1] FXIO_D[PINSEL] Next State Value
    // 0 0 0 SHIFTBUFi[2:0]
    // 0 0 1 SHIFTBUFi[5:3]
    // 0 1 0 SHIFTBUFi[8:6]
    // 0 1 1 SHIFTBUFi[11:9]
    // ... ... ... ...
    // 1 1 1 SHIFTBUFi[23:21]
    return (outputs << 24) | ((next_7 & 0b111) << 21) | ((next_6 & 0b111) << 18) | ((next_5 & 0b111) << 15) |
           ((next_4 & 0b111) << 12) | ((next_3 & 0b111) << 9) | ((next_2 & 0b111) << 6) |
           ((next_1 & 0b111) << 3) | ((next_0 & 0b111) << 0);
  }

  static constexpr uint32_t FLEXIO_STATE_SHIFTBUF(const uint8_t outputs, const uint8_t next) {
    return FLEXIO_STATE_SHIFTBUF(outputs, next, next, next, next, next, next, next, next);
  }

  // global static bool default to false
  static bool _is_initialized, _is_enabled;

public:
  static bool init(unsigned long long ic_time_ns, unsigned long long op_time_ns,
                   mode::OnOverload on_overload = mode::OnOverload::HALT,
                   mode::OnExtHalt on_ext_halt = mode::OnExtHalt::IGNORE,
                   mode::Sync sync = mode::Sync::NONE);
  static bool is_initialized() { return _is_initialized; }

  static void disable();
  static void enable();
  static bool is_enabled() { return _is_enabled; }
  static void reset();

  // QTMR functions
  static void _init_qtmr_op();
  static void _reset_qtmr_op();
  static unsigned long long get_actual_op_time();

  static void force_start();
  static void to_idle();
  static void to_ic();
  static void to_op();
  static void to_exthalt();
  static void to_end();

  static bool is_idle();
  static bool is_op();
  static bool is_done();
  static bool is_overloaded();
  static bool is_exthalt();

  static void delay_till_done();
};

} // namespace mode
