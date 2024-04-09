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

#include "bus/functions.h"
#include "logging.h"

const SPISettings functions::ICommandRegisterFunction::DEFAULT_SPI_SETTINGS{
    4'000'000, MSBFIRST, SPI_MODE3 /* chip expects SPI MODE0, but CLK is inverted on the way */};

functions::ICommandRegisterFunction::ICommandRegisterFunction(bus::addr_t address)
    : DataFunction(address, DEFAULT_SPI_SETTINGS) {}

uint8_t functions::ICommandRegisterFunction::chip_cmd_word(uint8_t chip_input_idx, uint8_t chip_output_idx,
                                                           bool connect) {
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


    //  We are iterating over hardware indexes and look up the appropriate place in logically
    //  sequentially indexed table.
    const auto oidx_one_two = output_idx;
    const auto oidx_three_four = output_idx + outputs.size() / 2;
    if (!outputs[oidx_one_two] && !outputs[oidx_three_four])
      continue;

    // We can always set one output in range (0,15) and one in (16,31) in for each output
    for (uint8_t input_idx = 0; input_idx < NUM_INPUTS / 2; input_idx++) {

      command = 0;

      // On REV0 hardware, some inputs are wrongly connected/labeled
      // (see https://lab.analogparadigm.com/lucidac/hardware/i-block/-/issues/2)
      // and need to be fixed according to _logical_to_hardware_input.
      // We can do so in the inner loop, because the correction is only ever switching inputs on the same chip.
      // _logical_to_hardware_input is written to handle inputs from 0...NUM_INPUTS,
      // but in fact we only need to correct 0...NUM_INPUTS/2.
      // TODO: Remove once hardware is fixed.
      const auto iidx_one_three = _logical_to_hardware_input(input_idx);
      const auto iidx_two_four = _logical_to_hardware_input(input_idx + NUM_INPUTS / 2);
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
        f_cmd.transfer32(command);
        // Apply command
        f_imatrix_sync.trigger();
      }
    }
  }
}

bool blocks::IBlock::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  if (!FunctionBlock::init()) {
    return false;
  };
  // I-Block matrix is not reset on power-cycle, apparently.
  write_to_hardware();
  return true;
}

bus::addr_t blocks::IBlock::get_block_address() { return bus::idx_to_addr(cluster_idx, BLOCK_IDX, 0); }

bool blocks::IBlock::_is_connected(uint8_t input, uint8_t output) {
  return outputs[output] & INPUT_BITMASK(input);
}

bool blocks::IBlock::is_connected(uint8_t input, uint8_t output) {
  if (output >= NUM_OUTPUTS or input >= NUM_INPUTS)
    return false;
  return _is_connected(input, output);
}

uint8_t blocks::IBlock::_hardware_to_logical_input(uint8_t hardware_input) {
  // Manually fix pin-numbering, see https://lab.analogparadigm.com/lucidac/hardware/i-block/-/issues/2
  // TODO: Remove once hardware is fixed.
  if ((hardware_input >= 8 && hardware_input <= 13) || (hardware_input >= 24 && hardware_input <= 29)) {
    return hardware_input - 2;
  } else if ((hardware_input >= 6 && hardware_input <= 7) || (hardware_input >= 22 && hardware_input <= 23)) {
    return hardware_input + 6;
  }
  return hardware_input;
}

uint8_t blocks::IBlock::_logical_to_hardware_input(uint8_t logical_input) {
  // Manually fix pin-numbering, see https://lab.analogparadigm.com/lucidac/hardware/i-block/-/issues/2
  // TODO: Remove once hardware is fixed.
  if ((logical_input >= 6 && logical_input <= 11) || (logical_input >= 22 && logical_input <= 27)) {
    return logical_input + 2;
  } else if ((logical_input >= 12 && logical_input <= 13) || (logical_input >= 28 && logical_input <= 29)) {
    return logical_input - 6;
  }
  return logical_input;
}

bool blocks::IBlock::connect(uint8_t input, uint8_t output, bool exclusive, bool allow_input_splitting) {
  if (output >= NUM_OUTPUTS or input >= NUM_INPUTS)
    return false;

  // Usually, we don't want to connect one input to multiple outputs, so check for other connections
  if (!allow_input_splitting) {
    for (size_t other_output_idx = 0; other_output_idx < outputs.size(); other_output_idx++) {
      if (output == other_output_idx)
        continue;
      if (is_connected(input, other_output_idx)) { // operates on logical indexes
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
  if (cfg.containsKey("outputs")) {
    // Handle a mapping of output to list of inputs
    // This only overrides outputs that are specifically mentioned
    if (cfg["outputs"].is<JsonObjectConst>()) {
      for (JsonPairConst keyval : cfg["outputs"].as<JsonObjectConst>()) {
        // Key defines output
        // TODO: Check conversion from string to number
        auto output = std::stoul(keyval.key().c_str());
        // Disconnect also sanity checks output index for us
        if (!disconnect(output))
          return false;
        // Input may be given as list or as a single number
        if (!_connect_from_json(keyval.value(), output))
          return false;
      }
    }
    // Handle a list of outputs
    // This must overwrite all outputs, so we clear all of them
    if (cfg["outputs"].is<JsonArrayConst>()) {
      auto outputs_json = cfg["outputs"].as<JsonArrayConst>();
      if (outputs_json.size() != NUM_OUTPUTS)
        return false;
      reset_outputs();
      uint8_t idx = 0;
      for (JsonVariantConst input_spec : outputs_json) {
        // Input may be given as list or as a single number
        if (!_connect_from_json(input_spec, idx++))
          return false;
      }
    }
  }
  return true;
}

bool blocks::IBlock::_connect_from_json(const JsonVariantConst &input_spec, uint8_t output) {
  if (input_spec.isNull()) {
    // Output is already disconnected from outer code
  } else if (input_spec.is<JsonArrayConst>()) {
    for (auto input : input_spec.as<JsonArrayConst>()) {
      if (!input.is<uint8_t>())
        return false;
      if (!connect(input, output))
        return false;
    }
  } else if (input_spec.is<uint8_t>()) {
    if (!connect(input_spec, output))
      return false;
  } else {
    return false;
  }
  return true;
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

void blocks::IBlock::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);
  // Save outputs into cfg
  auto outputs_cfg = cfg.createNestedArray("outputs");
  for (uint8_t output = 0; output < NUM_OUTPUTS; output++) {
    if (outputs[output]) {
      auto inputs_cfg = outputs_cfg.createNestedArray();
      for (uint8_t input = 0; input < NUM_INPUTS; input++)
        if (_is_connected(input, output))
          inputs_cfg.add(input);
    } else {
      outputs_cfg.add(nullptr);
    }
  }
}
