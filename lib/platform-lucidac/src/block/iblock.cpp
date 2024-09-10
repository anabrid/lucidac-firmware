// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "block/iblock.h"

#include "bus/functions.h"
#include "utils/logging.h"

const SPISettings functions::ICommandRegisterFunction::DEFAULT_SPI_SETTINGS{
    4'000'000, MSBFIRST, SPI_MODE2 /* chip expects SPI MODE0, but CLK is inverted on the way */};

functions::ICommandRegisterFunction::ICommandRegisterFunction(bus::addr_t address)
    : SR74HCT595(address, DEFAULT_SPI_SETTINGS) {}

uint8_t functions::ICommandRegisterFunction::chip_cmd_word(uint8_t chip_input_idx, uint8_t chip_output_idx,
                                                           bool connect) {
  return (connect ? 0b1'000'0000 : 0b0'000'0000) | ((chip_output_idx & 0x7) << 4) | (chip_input_idx & 0xF);
}

bool blocks::IBlock::write_to_hardware() {
  return hardware->write_upscaling(scaling_factors) and hardware->write_outputs(outputs);
}

bool blocks::IBlock::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  if (!FunctionBlock::init()) {
    return false;
  };
  // I-Block matrix is not reset on power-cycle, apparently.
  if (!write_to_hardware())
    return false;
  return true;
}

bool blocks::IBlock::_is_connected(uint8_t input, uint8_t output) const {
  return outputs[output] & INPUT_BITMASK(input);
}

bool blocks::IBlock::_is_output_connected(uint8_t output) const { return outputs[output]; }

bool blocks::IBlock::is_connected(uint8_t input, uint8_t output) const {
  if (output >= NUM_OUTPUTS or input >= NUM_INPUTS)
    return false;
  return _is_connected(input, output);
}

bool blocks::IBlock::is_anything_connected() const {
  for (auto &output : outputs)
    if (_is_output_connected(output))
      return true;
  return false;
}

bool blocks::IBlock::connect(uint8_t input, uint8_t output, bool exclusive, bool allow_input_splitting) {
  if (output >= NUM_OUTPUTS or input >= NUM_INPUTS)
    return false;

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
  reset_upscaling();
}

FLASHMEM utils::status blocks::IBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  for (auto cfgItr = cfg.begin(); cfgItr != cfg.end(); ++cfgItr) {
    if (cfgItr->key() == "outputs") {
      auto res = _config_outputs_from_json(cfgItr->value());
      if(!res) return res;
    } else if (cfgItr->key() == "upscaling") {
      auto res = _config_upscaling_from_json(cfgItr->value());
      if(!res) return res;
    } else {
      return utils::status("IBlock: Unknown configuration key");
    }
  }
  return utils::status::success();
}

FLASHMEM utils::status blocks::IBlock::_config_outputs_from_json(const JsonVariantConst &cfg) {
  // Handle a mapping of output to list of inputs
  // This only overrides outputs that are specifically mentioned
  if (cfg.is<JsonObjectConst>()) {
    for (JsonPairConst keyval : cfg.as<JsonObjectConst>()) {
      // Key defines output
      // TODO: Check conversion from string to number

      std::string output_str = keyval.key().c_str();
      if (!utils::is_number(output_str.begin(), output_str.end()))
        return utils::status("IBlock: Expected number but key is '%s'", output_str.c_str());

      auto output = std::stoul(output_str);
      // Disconnect also sanity checks output index for us
      if (!disconnect(output))
        return utils::status("IBlock: Could not disconnect output '%d', probably out of range", output);
      // Input may be given as list or as a single number
      auto res = _connect_from_json(keyval.value(), output);
      if(!res) return res;
    }
    return utils::status::success();
  }
  // Handle a list of outputs
  // This must overwrite all outputs, so we clear all of them
  else if (cfg.is<JsonArrayConst>()) {
    auto outputs_json = cfg.as<JsonArrayConst>();
    if (outputs_json.size() != NUM_OUTPUTS)
      return utils::status("IBlock: Given array size %d does not meet expected %d", outputs_json.size(), NUM_OUTPUTS);
    reset_outputs();
    uint8_t idx = 0;
    for (JsonVariantConst input_spec : outputs_json) {
      // Input may be given as list or as a single number
      auto res = _connect_from_json(input_spec, idx++);
      if(!res) return res;
    }
    return utils::status::success();
  }
  return utils::status("IBlock: Expected either array or object as configuration");
}

FLASHMEM utils::status blocks::IBlock::_config_upscaling_from_json(const JsonVariantConst &cfg) {
  if (cfg.is<JsonObjectConst>()) {
    for (JsonPairConst keyval : cfg.as<JsonObjectConst>()) {
      if (!keyval.value().is<bool>())
        return utils::status("IBlock upscaling must be boolean");

      auto input = std::stoul(keyval.key().c_str());
      if (input > NUM_INPUTS)
        return utils::status("IBlock upscaling too many values");

      set_upscaling(input, keyval.value());
      return utils::status::success();
    }
  } else if (cfg.is<JsonArrayConst>()) {
    auto array = cfg.as<JsonArrayConst>();
    if (array.size() != NUM_INPUTS)
      return utils::status("IBlock upscaling wrong number of inputs");

    for (uint8_t input = 0; input < NUM_INPUTS; input++) {
      if (!array[input].is<bool>())
        return utils::status("IBlock upscaling expecting boolean");
      set_upscaling(input, array[input]);
    }
    return utils::status::success();
  }
  return utils::status("IBlock upscaling: Either provide list or object");
}

FLASHMEM utils::status blocks::IBlock::_connect_from_json(const JsonVariantConst &input_spec, uint8_t output) {
  if (input_spec.isNull()) {
    // Output is already disconnected from outer code
  } else if (input_spec.is<JsonArrayConst>()) {
    for (auto input : input_spec.as<JsonArrayConst>()) {
      if (!input.is<uint8_t>())
        return utils::status("IBlock: input is not an integer");
      if (!connect(input, output))
        return utils::status("IBlock: Could not connect %d -> %d, probably out of bounds", input, outputs);
    }
  } else if (input_spec.is<uint8_t>()) {
    if (!connect(input_spec, output))
      return utils::status("IBlock: Could not connect %d -> %d.", input_spec, output);
  } else {
    return utils::status("IBlock: Illegal data type in input specification");
  }
  return utils::status::success();
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

bool blocks::IBlock::set_upscaling(uint8_t input, bool upscale) {
  if (input >= NUM_INPUTS)
    return false;
  scaling_factors[input] = upscale;
  return true;
}

void blocks::IBlock::set_upscaling(std::bitset<NUM_INPUTS> scales) { scaling_factors = scales; }

void blocks::IBlock::reset_upscaling() { scaling_factors.reset(); }

bool blocks::IBlock::get_upscaling(uint8_t output) const {
  if (output > 32)
    return false;
  else
    return scaling_factors[output];
}

FLASHMEM void blocks::IBlock::config_self_to_json(JsonObject &cfg) {
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

  auto upscaling_cfg = cfg.createNestedArray("upscaling");
  for (uint8_t input = 0; input < NUM_INPUTS; input++) {
    if (scaling_factors[input])
      upscaling_cfg.add(true);
    else
      upscaling_cfg.add(false);
  }
}

FLASHMEM blocks::IBlock *blocks::IBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                       bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_)
    return nullptr;

  // Currently, there are no different variants
  if (classifier.variant != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  if (classifier.version < entities::Version(1, 2))
    return nullptr;
  if (classifier.version < entities::Version(1, 3))
    return new IBlock(block_address, new IBlockHAL_V_1_2_X(block_address));
  return nullptr;
}

const std::array<uint32_t, blocks::IBlock::NUM_OUTPUTS> &blocks::IBlock::get_outputs() const {
  return outputs;
}

void blocks::IBlock::set_outputs(const std::array<uint32_t, NUM_OUTPUTS> &outputs_) { outputs = outputs_; }

blocks::IBlockHAL_V_1_2_X::IBlockHAL_V_1_2_X(bus::addr_t block_address)
    : f_cmd{bus::replace_function_idx(block_address, 2)},
      f_imatrix_reset{bus::replace_function_idx(block_address, 4)}, f_imatrix_sync{bus::replace_function_idx(
                                                                        block_address, 3)},
      scaling_register{bus::replace_function_idx(block_address, 5), true},
      scaling_register_sync{bus::replace_function_idx(block_address, 6)} {}

bool blocks::IBlockHAL_V_1_2_X::write_outputs(const std::array<uint32_t, 16> &outputs) {
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
        if (!f_cmd.transfer32(command)) {
          LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
          return false;
        }
        // Apply command
        f_imatrix_sync.trigger();
      }
    }
  }
  return true;
}

bool blocks::IBlockHAL_V_1_2_X::write_upscaling(std::bitset<32> upscaling) {
  if (!scaling_register.transfer32(upscaling.to_ulong()))
    return false;
  scaling_register_sync.trigger();
  return true;
}
