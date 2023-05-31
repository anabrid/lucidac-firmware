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

#include "functions.h"
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
const SPISettings blocks::functions::UOffsetLoader::OFFSETS_FUNC_SPI_SETTINGS{8'000'000, MSBFIRST, SPI_MODE2};

void blocks::functions::UOffsetLoader::trigger_load(uint8_t offset_idx) const {
  if (offset_idx >= blocks::UBlock::NUM_OF_OUTPUTS)
    return;
  f_triggers[offset_idx / OUTPUTS_PER_CHIP].trigger();
}

void blocks::functions::UOffsetLoader::set_offset_and_write_to_hardware(uint8_t offset_idx,
                                                                        uint16_t offset_raw) const {
  auto cmd = build_cmd_word(offset_idx % OUTPUTS_PER_CHIP, offset_raw);
  /*
   * CARE: Offset correction does not have an address, it listens to SPI all the time
   * It happens that we can address a non-existent function, but if something changes
   * (e.g. default CLK polarity), this may break.
   * Then send data without data function or fix hardware :)
   */
  f_offsets.transfer16(cmd);
  trigger_load(offset_idx);
}

void blocks::functions::UOffsetLoader::set_offsets_and_write_to_hardware(
    std::array<uint16_t, 32> offsets_raw) const {
  for (unsigned int offset_idx = 0; offset_idx < offsets_raw.size(); offset_idx++) {
    set_offset_and_write_to_hardware(offset_idx, offsets_raw[offset_idx]);
  }
}

uint16_t blocks::functions::UOffsetLoader::build_cmd_word(uint8_t chip_output_idx, uint16_t offset_raw) {
  // Offset chips expect 4bit address (output select) and 10bit value.
  // With REVERSE=LOW, the expected incoming bits are [4bit address LSBFIRST][10bit value MSBFIRST].
  // Since we send 16bit, we need to prepend 2bits in front that will be discarded.

  // Filter offset_idx, chip does nothing with address=0
  if (chip_output_idx > MAX_CHIP_OUTPUT_IDX)
    return 0;

  // Limit offset_raw to MAX_OFFSET_RAW
  if (offset_raw > MAX_OFFSET_RAW)
    offset_raw = MAX_OFFSET_RAW;

  // Lookup table to reverse address in final bit positions
  constexpr std::array<uint16_t, 8> addr_lookup{
      0b00'1000'0000000000, 0b00'0100'0000000000, 0b00'1100'0000000000, 0b00'0010'0000000000,
      0b00'1010'0000000000, 0b00'0110'0000000000, 0b00'1110'0000000000, 0b00'0001'0000000000,
  };

  // Combine the two bit patterns
  return offset_raw | addr_lookup[chip_output_idx];
}

blocks::functions::UOffsetLoader::UOffsetLoader(bus::addr_t ublock_address)
    : f_offsets{bus::replace_function_idx(ublock_address, UBlock::OFFSETS_DATA_FUNC_IDX),
                OFFSETS_FUNC_SPI_SETTINGS},
      f_triggers{
          ::functions::TriggerFunction{
              bus::replace_function_idx(ublock_address, UBlock::OFFSETS_LOAD_BASE_FUNC_IDX + 0)},
          ::functions::TriggerFunction{
              bus::replace_function_idx(ublock_address, UBlock::OFFSETS_LOAD_BASE_FUNC_IDX + 1)},
          ::functions::TriggerFunction{
              bus::replace_function_idx(ublock_address, UBlock::OFFSETS_LOAD_BASE_FUNC_IDX + 2)},
          ::functions::TriggerFunction{
              bus::replace_function_idx(ublock_address, UBlock::OFFSETS_LOAD_BASE_FUNC_IDX + 3)},
      } {}

uint16_t blocks::functions::UOffsetLoader::offset_to_raw(float offset) {
  // Truncate offset
  if (offset >= MAX_OFFSET)
    return MAX_OFFSET_RAW;
  if (offset <= MIN_OFFSET)
    return MIN_OFFSET_RAW;

  // Assumes +/- is symmetrical
  // TODO: Consider MIN_OFFSET_RAW if it's ever non-zero.
  auto offset_scaled = (offset - MIN_OFFSET) / (MAX_OFFSET - MIN_OFFSET);
  return static_cast<uint16_t>(offset_scaled * float(MAX_OFFSET_RAW));
}

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
      f_offset_loader(bus::idx_to_addr(clusterIdx, BLOCK_IDX, 0)), output_input_map{0},
      alt_signals{0}, offsets{0} {
  offsets.fill(decltype(f_offset_loader)::ZERO_OFFSET_RAW);
}

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

void blocks::UBlock::write_alt_signal_to_hardware() const {
  f_alt_signal.transfer16(alt_signals);
  f_alt_signal_sync.trigger();
}

void blocks::UBlock::write_offsets_to_hardware() const {
  f_offset_loader.set_offsets_and_write_to_hardware(offsets);
}

void blocks::UBlock::write_matrix_to_hardware() const {
  f_umatrix.transfer(output_input_map);
  f_umatrix_sync.trigger();
}

void blocks::UBlock::write_to_hardware() const {
  write_alt_signal_to_hardware();
  write_offsets_to_hardware();
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

void blocks::UBlock::reset_offsets() {
  for (auto &offset : offsets) {
    offset = decltype(f_offset_loader)::ZERO_OFFSET_RAW;
  }
}

bool blocks::UBlock::set_offset(uint8_t output, uint16_t offset_raw) {
  if (output >= NUM_OF_OUTPUTS)
    return false;
  if ((offset_raw < decltype(f_offset_loader)::MIN_OFFSET_RAW) or
      (offset_raw > decltype(f_offset_loader)::MAX_OFFSET_RAW))
    return false;
  offsets[output] = offset_raw;
  return true;
}

bool blocks::UBlock::set_offset(uint8_t output, float offset) {
  auto offset_raw = decltype(f_offset_loader)::offset_to_raw(offset);
  return set_offset(output, offset_raw);
}

bus::addr_t blocks::UBlock::get_block_address() { return bus::idx_to_addr(cluster_idx, BLOCK_IDX, 0); }
