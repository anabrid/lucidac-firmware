// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

// hint: This function could be avoided to be a template if the signature
//       was just transfer(const uint8_t* const outputs, uint8_t num_of_outputs).
//       The template provides little advantage here.

template <size_t num_of_outputs>
void functions::UMatrixFunction::transfer(const std::array<uint8_t, num_of_outputs> &outputs) const {
  constexpr uint8_t NUM_BYTES = num_of_outputs * 5 / 8;
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

  begin_communication();
  // Unfortunately, the chip responsible for output 0-7 is the second chip in the SPI-chain.
  // That means that we need to swap the first 10 bytes with the second 10 bytes.
  // TODO: Instead, rewrite this function to already consider this.
  bus::spi.transfer(buffer + sizeof(buffer) / 2, nullptr, sizeof(buffer) / 2);
  bus::spi.transfer(buffer, nullptr, sizeof(buffer) / 2);
  end_communication();
  // You must trigger the SYNC of the chip with the sync trigger function.
}