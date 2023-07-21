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

#include "iblock.h"
#include "functions.h"

void blocks::functions::ICommandRegisterFunction::write_to_hardware() const {
  begin_communication();
  bus::spi.transfer32(data);
  end_communication();
}

blocks::functions::ICommandRegisterFunction::ICommandRegisterFunction(bus::addr_t address)
    : ::functions::_old_DataFunction(
          address, SPISettings(4'000'000, MSBFIRST,
                               SPI_MODE3 /* chip expects SPI MODE0, but CLK is inverted on the way */)),
      data(0) {}

uint8_t blocks::functions::ICommandRegisterFunction::chip_cmd_word(uint8_t chip_input_idx,
                                                                   uint8_t chip_output_idx, bool connect) {
  return (connect ? 0b1'000'0000 : 0b0'000'0000) | ((chip_output_idx & 0x7) << 4) | (chip_input_idx & 0xF);
}

void blocks::IBlock::write_to_hardware() {
  f_imatrix_reset.trigger();
  delayNanoseconds(420);

  // TODO: This can be further improved by not naively iterating over the output indizes.
  // For each output, send the corresponding 32bit commands.
  // For output_idx < 7, the first two MT8816 chips are used, for >8 the later two.
  // This means, we can set two outputs simultaneously.
  // When setting later outputs, we need to *not* overwrite previous outputs on the other chips
  // Thus we remember what we send them and just send them the same thing again
  uint32_t remembered_command = 0;
  for (decltype(outputs.size()) output_idx = 0; output_idx < outputs.size() / 2; output_idx++) {
    uint32_t command = 0;
    const auto oidx_one_two = output_idx;
    const auto oidx_three_four = output_idx + outputs.size() / 2;
    if (!outputs[oidx_one_two] && !outputs[oidx_three_four])
      continue;

    // We can always set one output in range (0,15) and one in (16,31) in for each output
    for (uint8_t input_idx = 0; input_idx < NUM_INPUTS / 2; input_idx++) {

      command = 0;

      const auto iidx_one_three = input_idx;
      const auto iidx_two_four = input_idx + NUM_INPUTS / 2;
      bool actual_data = false;
      // First chip combines oidx_one_two and iidx_one_three
      if (outputs[oidx_one_two] & INPUT_BITMASK(iidx_one_three)) {
        command |= functions::ICommandRegisterFunction::chip_cmd_word(iidx_one_three, oidx_one_two);
        actual_data = true;
      } else {
        command |= (remembered_command & 0xFF);
      }
      // Similar combination for second chip
      if (outputs[oidx_one_two] & INPUT_BITMASK(iidx_two_four)) {
        command |= functions::ICommandRegisterFunction::chip_cmd_word(iidx_two_four, oidx_one_two) << 8;
        actual_data = true;
      } else {
        command |= (remembered_command & 0xFF00);
      }
      // Third chip
      if (outputs[oidx_three_four] & INPUT_BITMASK(iidx_one_three)) {
        command |= functions::ICommandRegisterFunction::chip_cmd_word(iidx_one_three, oidx_three_four) << 16;
        actual_data = true;
      } else {
        command |= (remembered_command & 0xFF0000);
      }
      // Fourth chip
      if (outputs[oidx_three_four] & INPUT_BITMASK(iidx_two_four)) {
        command |= functions::ICommandRegisterFunction::chip_cmd_word(iidx_two_four, oidx_three_four) << 24;
        actual_data = true;
      } else {
        command |= (remembered_command & 0xFF000000);
      }

      if (actual_data) {
        remembered_command = command;
        // Send out data
        f_cmd.data = command;
        f_cmd.write_to_hardware();
        // Apply command
        f_imatrix_sync.trigger();
      }
    }
  }
}

bool blocks::IBlock::init() {
  if (!FunctionBlock::init()) {
    return false;
  };
  // I-Block matrix is not reset on power-cycle, apparently.
  write_to_hardware();
  return true;
}

bus::addr_t blocks::IBlock::get_block_address() { return bus::idx_to_addr(cluster_idx, BLOCK_IDX, 0); }

bool blocks::IBlock::is_connected(uint8_t input, uint8_t output) {
  if (output >= NUM_OUTPUTS or input >= NUM_INPUTS)
    return false;
  return outputs[output] & INPUT_BITMASK(input);
}

bool blocks::IBlock::connect(uint8_t input, uint8_t output, bool exclusive, bool allow_input_splitting) {
  if (output >= NUM_OUTPUTS or input >= NUM_INPUTS)
    return false;

  // Manually fix pin-numbering, see https://lab.analogparadigm.com/lucidac/hardware/i-block/-/issues/2
  // TODO: Remove once hardware is fixed.
  if ((input >= 6 && input <= 11) || (input >= 22 && input <= 27)) {
    input += 2;
  } else if ((input >= 12 && input <= 13) || (input >= 28 && input <= 29)) {
    input -= 6;
  }

  // Usually, we don't want to connect one input to multiple outputs, so check for other connections
  if (!allow_input_splitting) {
    for (size_t other_output_idx = 0; other_output_idx < outputs.size(); other_output_idx++) {
      if (output == other_output_idx)
        continue;
      if (is_connected(input, other_output_idx)) {
        return false;
      }
    }
  }

  if (exclusive)
    outputs[output] = INPUT_BITMASK(input);
  else
    outputs[output] |= INPUT_BITMASK(input);
  return true;
}

void blocks::IBlock::reset_outputs() {
  for (auto &output : outputs) {
    output = 0;
  }
}

void blocks::IBlock::reset(bool keep_calibration) {
  FunctionBlock::reset(keep_calibration);
  reset_outputs();
}

bool blocks::IBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  // TODO: Implement
  return false;
}

bool blocks::IBlock::disconnect(uint8_t input, uint8_t output) {
  // Fail if input was not connected
  if (!is_connected(input, output))
    return false;
  outputs[output] &= ~INPUT_BITMASK(input);
  return true;
}

bool blocks::IBlock::disconnect(uint8_t output) {
  if (output >= NUM_OUTPUTS)
    return false;
  outputs[output] = 0;
  return true;
}
