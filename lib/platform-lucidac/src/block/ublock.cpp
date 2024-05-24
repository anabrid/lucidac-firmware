// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <algorithm>

#include "bus/functions.h"
#include "ublock.h"

void utils::shift_5_left(uint8_t *buffer, size_t size) {
  for (size_t idx = 0; idx < size - 1; idx++) {
    auto next = buffer[idx + 1];
    // Shift by 5, leaving zeros at 0-4
    buffer[idx] <<= 5;
    buffer[idx] |= (next >> 3) & 0x1F;
  }
  buffer[size - 1] <<= 5;
}

const SPISettings functions::UMatrixFunction::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE2};

functions::UMatrixFunction::UMatrixFunction(bus::addr_t address)
    : functions::DataFunction(address, DEFAULT_SPI_SETTINGS) {}

blocks::UBlock::UBlock(bus::addr_t block_address)
    : FunctionBlock("U", block_address), f_umatrix(bus::replace_function_idx(block_address, UMATRIX_FUNC_IDX)),
      f_umatrix_sync(bus::replace_function_idx(block_address, UMATRIX_SYNC_FUNC_IDX)),
      f_umatrix_reset(bus::replace_function_idx(block_address, UMATRIX_RESET_FUNC_IDX)),
      output_input_map{},
      transmission_mode_register(bus::replace_function_idx(block_address, TRANSMISSION_MODE_FUNC_IDX), true),
      transmission_mode_sync(bus::replace_function_idx(block_address, TRANSMISSION_MODE_SYNC_FUNC_IDX)),
      transmission_mode_byte(0) {
  reset_connections();
}

bool blocks::UBlock::_i_sanity_check(const uint8_t input) { return input < NUM_OF_INPUTS; }

bool blocks::UBlock::_o_sanity_check(const uint8_t output) { return output < NUM_OF_OUTPUTS; }

bool blocks::UBlock::_io_sanity_check(const uint8_t input, const uint8_t output) {
  return _i_sanity_check(input) && _o_sanity_check(output);
}

void blocks::UBlock::_connect(const uint8_t input, const uint8_t output) { output_input_map[output] = input; }

bool blocks::UBlock::connect(const uint8_t input, const uint8_t output, bool allow_disconnections) {
  if (!_io_sanity_check(input, output))
    return false;

  // Check for other connections on the same output, unless we don't care if we overwrite them
  if (!allow_disconnections and _is_output_connected(output))
    return false;

  _connect(input, output);
  return true;
}

void blocks::UBlock::_disconnect(const uint8_t output) { output_input_map[output] = -1; }

bool blocks::UBlock::disconnect(const uint8_t input, const uint8_t output) {
  if (!_io_sanity_check(input, output))
    return false;

  if (_is_connected(input, output)) {
    _disconnect(output);
    return true;
  } else {
    return false;
  }
}

bool blocks::UBlock::disconnect(const uint8_t output) {
  if (!_o_sanity_check(output))
    return false;
  _disconnect(output);
  return true;
}

bool blocks::UBlock::_is_connected(const uint8_t input, const uint8_t output) const {
  return output_input_map[output] == input;
}

bool blocks::UBlock::is_connected(const uint8_t input, const uint8_t output) {
  if (!_io_sanity_check(input, output))
    return false;

  return _is_connected(input, output);
}

bool blocks::UBlock::_is_output_connected(const uint8_t output) const { return output_input_map[output] >= 0; }

bool blocks::UBlock::is_output_connected(const uint8_t output) {
  if (!_o_sanity_check(output))
    return false;

  return _is_output_connected(output);
}

constexpr uint8_t UBLOCK_TRANSMISSION_REGULAR_MASK = 0b0000'0111;
constexpr uint8_t UBLOCK_TRANSMISSION_ALTERNATIVE_MASK = 0b0001'1001;
constexpr uint8_t UBLOCK_TRANSMISSION_SIGNLESS_MASK = 0b0000'0110;

uint8_t blocks::UBlock::change_transmission_mode(const Transmission_Mode mode,
                                                 const Transmission_Target target) {
  bool sign = mode & 1;
  uint8_t rest = transmission_mode_byte;

  if (target == Transmission_Target::REGULAR) {
    rest &= ~UBLOCK_TRANSMISSION_REGULAR_MASK;
    transmission_mode_byte = rest | mode;
  } else if (target == Transmission_Target::ALTERNATIVE) {
    rest &= ~UBLOCK_TRANSMISSION_ALTERNATIVE_MASK;
    transmission_mode_byte = rest | (mode & UBLOCK_TRANSMISSION_SIGNLESS_MASK) << 2 | sign;
  } else if (target == Transmission_Target::REGULAR_AND_ALTERNATIVE) {
    rest &= ~(UBLOCK_TRANSMISSION_REGULAR_MASK | UBLOCK_TRANSMISSION_ALTERNATIVE_MASK);
    transmission_mode_byte = rest | mode | (mode & UBLOCK_TRANSMISSION_SIGNLESS_MASK) << 2 | sign;
  }

  return transmission_mode_byte;
}

bool blocks::UBlock::write_matrix_to_hardware() const {
  if (!f_umatrix.transfer(output_input_map))
    return false;
  f_umatrix_sync.trigger();
  return true;
}

bool blocks::UBlock::write_transmission_mode_to_hardware() const {
  if (!transmission_mode_register.transfer8(transmission_mode_byte))
    return false;
  transmission_mode_sync.trigger();
  return true;
}

bool blocks::UBlock::write_to_hardware() {
  return write_matrix_to_hardware() && write_transmission_mode_to_hardware();
}

void blocks::UBlock::reset_connections() { std::fill(begin(output_input_map), end(output_input_map), -1); }

void blocks::UBlock::reset(const bool keep_offsets) {
  FunctionBlock::reset(keep_offsets);
  reset_connections();
}

bool blocks::UBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  if (cfg.containsKey("outputs")) {
    // Handle an array of outputs
    // In this case, all outputs are reset
    if (cfg["outputs"].is<JsonArrayConst>()) {
      auto outputs = cfg["outputs"].as<JsonArrayConst>();
      if (outputs.size() != NUM_OF_OUTPUTS)
        return false;
      reset_connections();
      uint8_t idx = 0;
      for (JsonVariantConst input : outputs) {
        if (input.isNull()) {
          idx++;
          continue;
        } else if (!input.is<uint8_t>()) {
          return false;
        }
        if (!connect(input.as<uint8_t>(), idx++))
          return false;
      }
    }
    // Handle a mapping of outputs
    // In this case, only explicitly passed outputs are changed
    // To allow reconnections (swapping), we first clear all outputs and set them afterwards.
    if (cfg["outputs"].is<JsonObjectConst>()) {
      for (JsonPairConst keyval : cfg["outputs"].as<JsonObjectConst>()) {
        // Sanity check that any input passed is a number
        if (!(keyval.value().is<uint8_t>() or keyval.value().isNull()))
          return false;
        // TODO: Check conversion from string to number
        auto output = std::stoul(keyval.key().c_str());
        disconnect(output);
      }
      for (JsonPairConst keyval : cfg["outputs"].as<JsonObjectConst>()) {
        // TODO: Check conversion from string to number
        auto output = std::stoul(keyval.key().c_str());
        // Type is checked above
        if (keyval.value().isNull())
          continue;
        auto input = keyval.value().as<uint8_t>();
        if (!connect(input, output))
          return false;
      }
    }
  }

  // The combination of checks above must not ignore any valid config dictionary
  return true;
}

void blocks::UBlock::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);
  // Save outputs into cfg
  auto outputs_cfg = cfg.createNestedArray("outputs");
  for (const auto &output : output_input_map) {
    if (output)
      outputs_cfg.add(output - 1);
    else
      outputs_cfg.add(nullptr);
  }
}

blocks::UBlock *blocks::UBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                       bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_)
    return nullptr;

  // Currently, there are no different variants or versions
  if (classifier.variant != entities::EntityClassifier::DEFAULT_ or
      classifier.version != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  // Return default implementation
  return new UBlock(block_address);
}
