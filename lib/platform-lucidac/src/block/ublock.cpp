// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <algorithm>

#include "ublock.h"

#include "utils/logging.h"

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
      f_umatrix_reset(bus::replace_function_idx(block_address, UMATRIX_RESET_FUNC_IDX)), output_input_map{},
      transmission_mode_register(bus::replace_function_idx(block_address, TRANSMISSION_MODE_FUNC_IDX), true),
      transmission_mode_sync(bus::replace_function_idx(block_address, TRANSMISSION_MODE_SYNC_FUNC_IDX)) {
  reset_connections();
}

bool blocks::UBlock::_i_sanity_check(const uint8_t input) { return input < NUM_OF_INPUTS; }

bool blocks::UBlock::_o_sanity_check(const uint8_t output) { return output < NUM_OF_OUTPUTS; }

bool blocks::UBlock::_io_sanity_check(const uint8_t input, const uint8_t output) {
  return _i_sanity_check(input) && _o_sanity_check(output);
}

void blocks::UBlock::_connect(const uint8_t input, const uint8_t output) { output_input_map[output] = input; }

bool blocks::UBlock::connect(const uint8_t input, const uint8_t output, bool force) {
  if (!_io_sanity_check(input, output))
    return false;

  // Check for other connections on the same output, unless we don't care if we overwrite them
  if (!force and _is_output_connected(output))
    return false;

  // Check if the transmission mode makes the specified connection impossible and if the mode can or should be
  // chnged
  if (a_side_mode != ANALOG_INPUT && ((output < 16 && input != 15) || (output >= 16 && input != 14))) {
    if (!force)
      if (is_input_connected(input))
        return false;
    change_a_side_transmission_mode(ANALOG_INPUT);
  }

  if (b_side_mode != ANALOG_INPUT && ((output < 16 && input == 15) || (output >= 16 && input == 14))) {
    if (!force)
      if (is_input_connected(input))
        return false;
    change_b_side_transmission_mode(ANALOG_INPUT);
  }

  _connect(input, output);
  return true;
}

bool blocks::UBlock::connect_alternative(Transmission_Mode signal_mode, const uint8_t output, bool force,
                                         bool use_a_side) {
  if (!_o_sanity_check(output))
    return false;

  // Check for other connections on the same output, unless we don't care if we overwrite them
  if (!force and _is_output_connected(output))
    return false;

  // Check if the transmission mode makes the specified connection impossible and if the mode can or should be
  // chnged
  if (a_side_mode != signal_mode && use_a_side) {
    // TODO: force for a-side could be added, but since there wont be much use for an a-side
    // alternative signal, you have to use force or set the correct signal mode
    if (!force)
      return false;
    change_a_side_transmission_mode(signal_mode);
  }

  if (b_side_mode != signal_mode && !use_a_side) {
    if (!force)
      if (is_input_connected(14) || is_input_connected(15))
        return false;
    change_b_side_transmission_mode(signal_mode);
    //! Note
  }

  // Make a one to one connection if possible. Uses Input zero as fallback
  uint8_t input;
  if (use_a_side) {
    if (output < 15)
      input = output;
    else if (output == 15)
      input = 0;
    else if (output == 30)
      input = 0;
    else
      input = output - 16;
  } else {
    if (output < 16)
      input = 15;
    else
      input = 14;
  }

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

bool blocks::UBlock::is_connected(const uint8_t input, const uint8_t output) const {
  if (!_io_sanity_check(input, output))
    return false;

  return _is_connected(input, output);
}

bool blocks::UBlock::_is_output_connected(const uint8_t output) const { return output_input_map[output] >= 0; }

bool blocks::UBlock::is_output_connected(const uint8_t output) const {
  if (!_o_sanity_check(output))
    return false;

  return _is_output_connected(output);
}

bool blocks::UBlock::_is_input_connected(const uint8_t input) const {
  for (const auto &output : output_input_map)
    if (output == input)
      return true;
  return false;
}

bool blocks::UBlock::is_input_connected(const uint8_t input) const {
  if (!_i_sanity_check(input))
    return false;
  return _is_input_connected(input);
}

constexpr uint8_t UBLOCK_TRANSMISSION_REGULAR_MASK = 0b0000'0111;
constexpr uint8_t UBLOCK_TRANSMISSION_ALTERNATIVE_MASK = 0b0001'1001;
constexpr uint8_t UBLOCK_TRANSMISSION_SIGNLESS_MASK = 0b0000'0110;

uint8_t blocks::UBlock::change_a_side_transmission_mode(const Transmission_Mode mode) {
  a_side_mode = mode;

  transmission_mode_byte &= ~UBLOCK_TRANSMISSION_REGULAR_MASK;
  transmission_mode_byte |= mode;
  return transmission_mode_byte;
}

uint8_t blocks::UBlock::change_b_side_transmission_mode(const Transmission_Mode mode) {
  b_side_mode = mode;

  bool sign = mode & 1;
  uint8_t rest = transmission_mode_byte;

  transmission_mode_byte &= ~UBLOCK_TRANSMISSION_ALTERNATIVE_MASK;
  transmission_mode_byte |= (mode & UBLOCK_TRANSMISSION_SIGNLESS_MASK) << 2 | sign;
  return transmission_mode_byte;
}

uint8_t blocks::UBlock::change_all_transmission_modes(const Transmission_Mode mode) {
  change_a_side_transmission_mode(mode);
  return change_b_side_transmission_mode(mode);
}

uint8_t
blocks::UBlock::change_all_transmission_modes(const std::pair<Transmission_Mode, Transmission_Mode> modes) {
  change_a_side_transmission_mode(modes.first);
  return change_b_side_transmission_mode(modes.second);
}

std::pair<blocks::UBlock::Transmission_Mode, blocks::UBlock::Transmission_Mode>
blocks::UBlock::get_all_transmission_modes() const {
  return std::make_pair(a_side_mode, b_side_mode);
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
  if (!write_matrix_to_hardware() or !write_transmission_mode_to_hardware()) {
    LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
    return false;
  }
  return true;
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
    // TODO: This -1 may have to be removed due to the changes on the output matrix
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
