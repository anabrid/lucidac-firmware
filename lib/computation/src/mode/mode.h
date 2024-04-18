// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <FlexIO_t4.h>
#include <array>
#include <cstdint>

namespace mode {

constexpr uint8_t PIN_MODE_IC = 2;
constexpr uint8_t PIN_MODE_OP = 3;
constexpr uint8_t PIN_QTMR_OP_GATE = 12;

constexpr unsigned int DEFAULT_IC_TIME = 100'000;
constexpr unsigned long long DEFAULT_OP_TIME = 1'000'000;

class ManualControl {
public:
  static void init();
  static void to_ic();
  static void to_op();
  static void to_halt();
};

class FlexIOControl {
private:
  static constexpr uint8_t CLK_SEL = 3, CLK_PRED = 0, CLK_PODF = 0;
  static constexpr uint8_t s_idle = 0, s_ic = 1, s_op = 2, s_pause = 3, s_end = 4;

  static constexpr std::array<uint8_t, 5> get_states() { return {s_idle, s_ic, s_op, s_pause, s_end}; }

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

public:
  static bool init(unsigned int ic_time_ns, unsigned long long op_time_ns);

  static void disable();
  static void enable();
  static void reset();

  // QTMR functions
  static void _init_qtmr_op();
  static void _reset_qtmr_op();
  static unsigned long long get_actual_op_time();

  static void force_start();
  static void to_idle();
  static void to_ic();
  static void to_op();
  static void to_pause();
  static void to_end();

  static bool is_done();

  static void delay_till_done();
};

} // namespace mode