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

blocks::UBlock::UBlock(const uint8_t clusterIdx)
    : FunctionBlock("U", clusterIdx), f_umatrix(bus::idx_to_addr(clusterIdx, BLOCK_IDX, UMATRIX_FUNC_IDX)),
      f_umatrix_sync(bus::idx_to_addr(clusterIdx, BLOCK_IDX, UMATRIX_SYNC_FUNC_IDX)),
      f_umatrix_reset(bus::idx_to_addr(clusterIdx, BLOCK_IDX, UMATRIX_RESET_FUNC_IDX)), output_input_map{},
      transmission_mode_register(bus::idx_to_addr(cluster_idx, BLOCK_IDX, TRANSMISSION_MODE_FUNC_IDX), true),
      transmission_mode_sync(bus::idx_to_addr(cluster_idx, BLOCK_IDX, TRANSMISSION_MODE_SYNC_FUNC_IDX)),
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

constexpr uint8_t UBLOCK_TRANSISSION_REGULAR_MASK = 0b0000'0111;
constexpr uint8_t UBLOCK_TRANSISSION_ALTERNATIVE_MASK = 0b0001'1001;

void blocks::UBlock::changeTransmissionMode(const UBlock_Transmission_Mode mode, const bool sign,
                                            const bool use_alt) {
  uint8_t rest = transmission_mode_byte;
  if (!use_alt)
    rest &= ~UBLOCK_TRANSISSION_REGULAR_MASK;
  else
    rest &= ~UBLOCK_TRANSISSION_ALTERNATIVE_MASK;

  transmission_mode_byte = rest | (mode << (1 + 2 * use_alt)) | sign;
}

void blocks::UBlock::write_matrix_to_hardware() const {
  f_umatrix.transfer(output_input_map);
  f_umatrix_sync.trigger();
}

void blocks::UBlock::write_transmission_mode_to_hardware() const {
  transmission_mode_register.transfer(transmission_mode_byte);
  transmission_mode_sync.trigger();
}

void blocks::UBlock::write_to_hardware() {
  write_matrix_to_hardware();
  write_transmission_mode_to_hardware();
}

bus::addr_t blocks::UBlock::get_block_address() { return bus::idx_to_addr(cluster_idx, BLOCK_IDX, 0); }

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
