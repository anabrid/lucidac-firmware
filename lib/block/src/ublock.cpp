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

#include <algorithm>

#include "ublock.h"

void blocks::utils::shift_5_left(uint8_t *buffer, size_t size) {
  for (size_t idx = 0; idx < size - 1; idx++) {
    auto next = buffer[idx + 1];
    // Shift by 5, leaving zeros at 0-4
    buffer[idx] <<= 5;
    buffer[idx] |= (next >> 3) & 0x1F;
  }
  buffer[size - 1] <<= 5;
}

const SPISettings blocks::UBlock::UMATRIX_FUNC_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE2};
const SPISettings blocks::UBlock::ALT_SIGNAL_FUNC_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE1};

blocks::functions::_old_UMatrixFunction::_old_UMatrixFunction(const unsigned short address,
                                                              const SPISettings &spiSettings)
    : _old_DataFunction(address, spiSettings), outputs{0} {}

blocks::functions::_old_UMatrixFunction::_old_UMatrixFunction(
    const unsigned short address, const SPISettings &spiSettings,
    const std::array<uint8_t, number_of_outputs> &outputs)
    : _old_DataFunction(address, spiSettings), outputs(outputs) {}

void blocks::functions::_old_UMatrixFunction::sync_to_hardware() const {
  uint8_t buffer[BYTESTREAM_SIZE] = {}; // initialized with zeros

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

  for (auto idx = number_of_outputs; idx > 0; idx--) {
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
  bus::spi.transfer(buffer, sizeof(buffer));
  end_communication();

  // TODO: Trigger sync address, but that happens outside of this function.
}

void blocks::functions::USignalSwitchFunction::write_to_hardware() const {
  begin_communication();
  bus::spi.transfer16(data);
  end_communication();
}

blocks::UBlock::UBlock(const uint8_t clusterIdx)
    : FunctionBlock(clusterIdx),
      f_umatrix(bus::idx_to_addr(clusterIdx, BLOCK_IDX, UMATRIX_FUNC_IDX), UMATRIX_FUNC_SPI_SETTINGS),
      f_umatrix_sync(bus::idx_to_addr(clusterIdx, BLOCK_IDX, UMATRIX_SYNC_FUNC_IDX)),
      f_umatrix_reset(bus::idx_to_addr(clusterIdx, BLOCK_IDX, UMATRIX_RESET_FUNC_IDX)),
      f_alt_signal(bus::idx_to_addr(clusterIdx, BLOCK_IDX, ALT_SIGNAL_SWITCHER_FUNC_IDX),
                   ALT_SIGNAL_FUNC_SPI_SETTINGS),
      f_alt_signal_clear(bus::idx_to_addr(clusterIdx, BLOCK_IDX, ALT_SIGNAL_SWITCHER_CLEAR_FUNC_IDX)),
      f_alt_signal_sync(bus::idx_to_addr(clusterIdx, BLOCK_IDX, ALT_SIGNAL_SWITCHER_SYNC_FUNC_IDX)),
      output_input_map{0}, alt_signals{0} {}

bool blocks::UBlock::connect(uint8_t input, uint8_t output, bool connect) {
  // Sanity check
  if (input >= NUM_OF_INPUTS or output >= NUM_OF_OUTPUTS)
    return false;

  // Disconnect is possible if input/output are actually connected
  if (!connect) {
    if (output_input_map[output] == input + 1) {
      output_input_map[output] = 0;
      return true;
    } else {
      return false;
    }
  }

  // Before connecting, check if input is already used by another output
  auto output_using_input = std::find_if(std::begin(output_input_map), std::end(output_input_map),
                                         [&input](const uint8_t input_) { return input_ == input + 1; });
  if (output_using_input != std::end(output_input_map))
    return false;

  // Connect
  output_input_map[output] = input + 1;

  return true;
}

bool blocks::UBlock::is_connected(uint8_t input, uint8_t output) {
  // Sanity check
  if (input >= NUM_OF_INPUTS or output >= NUM_OF_OUTPUTS)
    return false;

  return output_input_map[output] == input;
}

void blocks::UBlock::write_matrix_to_hardware() const {
  f_umatrix.transfer(output_input_map);
  f_umatrix_sync.trigger();
}

void blocks::UBlock::write_alt_signal_to_hardware() const {
  f_alt_signal.transfer16(alt_signals);
  f_alt_signal_sync.trigger();
}

void blocks::UBlock::write_to_hardware() const {
  write_alt_signal_to_hardware();
  write_matrix_to_hardware();
}

bool blocks::UBlock::use_alt_signals(uint16_t alt_signal) {
  if (alt_signal > MAX_ALT_SIGNAL)
    return false;
  alt_signals |= alt_signal;
  return true;
}

bool blocks::UBlock::is_alt_signal_used(uint16_t alt_signal) const { return alt_signals & alt_signal; }

uint16_t blocks::UBlock::get_alt_signals() const { return alt_signals; }
