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

#include <cstdint>
#include <array>

// hint: This function could be avoided to be a template if the signature
//       was just transfer(const uint8_t* const outputs, uint8_t num_of_outputs).
//       The template provides little advantage here.

template <size_t num_of_outputs>
void functions::UMatrixFunction::transfer(const std::array<uint8_t, num_of_outputs>& outputs) const {
  constexpr uint8_t NUM_BYTES = num_of_outputs*5/8;
  uint8_t buffer[NUM_BYTES] = {}; // initialized with zeros

  /*
   *  Explanation
   *  -----------
   *
   *  The two XBar chips accept a 80bit binary stream each, resulting in 160bit = 20byte
   * total. Each output on a chip uses 5bit: [1bit enable][4bit input select]. The
   * 20byte stream has the last output in front. To set e.g. the last output to input
   * number 6 (5 because 0-based: 5=B0101), use in binary buffer =
   * 10101'00000'00000'.....'...
   *             |-8bit--||-8bit---||---
   *
   *  this->outputs defines which input an output should use.
   *    outputs[i] = zero means the output is disabled (don't set enable bit)
   *    outputs[i] = j means use input j (1-based) => input j-1 (0-based in bit stream)
   *
   *  Since the output configurations are 5bit wide, we need to do some bit-shifting to
   * put each at the correct position in the buffer. 1) Place the correct 5bit sequence
   * _at the end_ of buffer 2) Shift _the whole buffer_ by 5bit 3) Repeat until buffer
   * is full
   */

  for (auto idx = num_of_outputs; idx > 0; idx--) {
    // Start from the back
    auto selected_input = outputs[idx - 1];

    // If an output is enabled, write correct 5bit sequence to _end_ of buffer
    if (selected_input > 0) {
      // Enable at bit 5
      buffer[sizeof(buffer) - 1] |= B00010000;
      // Input number, max 4bits (thus & 0x0F)
      buffer[sizeof(buffer) - 1] |= ((selected_input - 1) & 0x0F);
    }

    // 5-bit shift the whole buffer, but not in last loop
    if (idx > 1)
      utils::shift_5_left(buffer, sizeof(buffer));
  }

  DataFunction::transfer(buffer, nullptr, sizeof(buffer));
  // You must trigger the SYNC of the chip with the sync trigger function.
}