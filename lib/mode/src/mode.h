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

#pragma once

#include <FlexIO_t4.h>
#include <array>
#include <cstdint>

namespace mode {

constexpr uint8_t PIN_MODE_IC = 4;
constexpr uint8_t PIN_MODE_OP = 3;

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
  static constexpr std::array<uint8_t ,5> get_states(){ return {s_idle, s_ic, s_op, s_pause, s_end};}

  static constexpr auto FLEXIO_SHIFTCTL_SMOD_STATE = FLEXIO_SHIFTCTL_SMOD(6);

  static constexpr auto FLEXIO_TIMCTL_TRGSEL_STATE(uint8_t s_) {
    return FLEXIO_TIMCTL_TRGSEL(4 * s_ + 1) | FLEXIO_TIMCTL_TRGSRC;
  }

public:
  static bool init(unsigned int ic_time_ns, unsigned int op_time_ns);

  static void disable();
  static void enable();
  static void reset();

  static void force_start();
  static void to_idle();
  static void to_ic();
  static void to_op();
  static void to_pause();
  static void to_end();
};

} // namespace mode