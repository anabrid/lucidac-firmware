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

const SPISettings functions::UOffsetLoader::OFFSETS_FUNC_SPI_SETTINGS{8'000'000, MSBFIRST, SPI_MODE2};

void functions::UOffsetLoader::trigger_load(uint8_t offset_idx) const {
  if (offset_idx >= blocks::UBlock::NUM_OF_OUTPUTS)
    return;
  f_triggers[offset_idx / OUTPUTS_PER_CHIP].trigger();
}

void functions::UOffsetLoader::set_offset_and_write_to_hardware(uint8_t offset_idx,
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

void functions::UOffsetLoader::set_offsets_and_write_to_hardware(std::array<uint16_t, 32> offsets_raw) const {
  for (unsigned int offset_idx = 0; offset_idx < offsets_raw.size(); offset_idx++) {
    set_offset_and_write_to_hardware(offset_idx, offsets_raw[offset_idx]);
  }
}

uint16_t functions::UOffsetLoader::build_cmd_word(uint8_t chip_output_idx, uint16_t offset_raw) {
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

functions::UOffsetLoader::UOffsetLoader(bus::addr_t ublock_address, uint8_t offsets_data_func_idx,
                                        uint8_t offsets_load_base_func_idx)
    : f_offsets{bus::replace_function_idx(ublock_address, offsets_data_func_idx), OFFSETS_FUNC_SPI_SETTINGS},
      f_triggers{
          TriggerFunction{bus::replace_function_idx(ublock_address, offsets_load_base_func_idx + 0)},
          TriggerFunction{bus::replace_function_idx(ublock_address, offsets_load_base_func_idx + 1)},
          TriggerFunction{bus::replace_function_idx(ublock_address, offsets_load_base_func_idx + 2)},
          TriggerFunction{bus::replace_function_idx(ublock_address, offsets_load_base_func_idx + 3)},
      } {}

uint16_t functions::UOffsetLoader::offset_to_raw(float offset) {
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

const SPISettings blocks::UBlock::ALT_SIGNAL_FUNC_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE1};

blocks::UBlock::UBlock(bus::addr_t block_address)
    : FunctionBlock("U", block_address), f_umatrix(bus::replace_function_idx(block_address, UMATRIX_FUNC_IDX)),
      f_umatrix_sync(bus::replace_function_idx(block_address, UMATRIX_SYNC_FUNC_IDX)),
      f_umatrix_reset(bus::replace_function_idx(block_address, UMATRIX_RESET_FUNC_IDX)),
      f_alt_signal(bus::replace_function_idx(block_address, ALT_SIGNAL_SWITCHER_FUNC_IDX),
                   ALT_SIGNAL_FUNC_SPI_SETTINGS),
      f_alt_signal_clear(bus::replace_function_idx(block_address, ALT_SIGNAL_SWITCHER_CLEAR_FUNC_IDX)),
      f_alt_signal_sync(bus::replace_function_idx(block_address, ALT_SIGNAL_SWITCHER_SYNC_FUNC_IDX)),
      f_offset_loader(bus::replace_function_idx(block_address, 0), OFFSETS_DATA_FUNC_IDX,
                      OFFSETS_LOAD_BASE_FUNC_IDX),
      output_input_map{0}, alt_signals{0}, offsets{0} {
  offsets.fill(decltype(f_offset_loader)::ZERO_OFFSET_RAW);
}

bool blocks::UBlock::connect(uint8_t input, uint8_t output, bool allow_disconnections) {
  // Sanity check
  if (input >= NUM_OF_INPUTS or output >= NUM_OF_OUTPUTS)
    return false;

  // Check for other connections on the same output, unless we don't care if we overwrite them
  if (!allow_disconnections and output_input_map[output]) {
    return false;
  }

  output_input_map[output] = input + 1;
  return true;
}

bool blocks::UBlock::disconnect(uint8_t input, uint8_t output) {
  // Sanity check
  if (input >= NUM_OF_INPUTS or output >= NUM_OF_OUTPUTS)
    return false;

  if (_is_connected(input, output)) {
    output_input_map[output] = 0;
    return true;
  } else {
    return false;
  }
}

bool blocks::UBlock::disconnect(uint8_t output) {
  if (output >= NUM_OF_OUTPUTS)
    return false;
  output_input_map[output] = 0;
  return true;
}

bool blocks::UBlock::_is_connected(uint8_t input, uint8_t output) {
  return output_input_map[output] == input + 1;
}

bool blocks::UBlock::is_connected(uint8_t input, uint8_t output) {
  // Sanity check
  if (input >= NUM_OF_INPUTS or output >= NUM_OF_OUTPUTS)
    return false;

  return _is_connected(input, output);
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

void blocks::UBlock::write_to_hardware() {
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
  std::fill(begin(offsets), end(offsets), decltype(f_offset_loader)::ZERO_OFFSET_RAW);
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

void blocks::UBlock::reset_connections() { std::fill(begin(output_input_map), end(output_input_map), 0); }

void blocks::UBlock::reset_alt_signals() { alt_signals = 0; }

void blocks::UBlock::reset(bool keep_offsets) {
  FunctionBlock::reset(keep_offsets);
  reset_connections();
  reset_alt_signals();
  if (!keep_offsets)
    reset_offsets();
}

bool blocks::UBlock::change_offset(uint8_t output, float delta) {
  auto delta_raw =
      decltype(f_offset_loader)::offset_to_raw(delta) - decltype(f_offset_loader)::ZERO_OFFSET_RAW;
  return set_offset(output, static_cast<uint16_t>(offsets[output] + delta_raw));
}

bool blocks::UBlock::connect_alt_signal(uint16_t alt_signal, uint8_t output) {
  // Sanity check
  if (alt_signal > MAX_ALT_SIGNAL)
    return false;

  // Some alt signals may only be connected to certain outputs
  bool success = false;
  if (alt_signal == ALT_SIGNAL_REF_HALF) {
    success = connect(ALT_SIGNAL_REF_HALF_INPUT, output, false);
  } else {
    if (output < 16)
      return false;

    // Select input depending on alt_signal
    uint8_t input;
    switch (alt_signal) {
    case ALT_SIGNAL_ACL0:
      input = 8;
      break;
    case ALT_SIGNAL_ACL1:
      input = 9;
      break;
    case ALT_SIGNAL_ACL2:
      input = 10;
      break;
    case ALT_SIGNAL_ACL3:
      input = 11;
      break;
    case ALT_SIGNAL_ACL4:
      input = 12;
      break;
    case ALT_SIGNAL_ACL5:
      input = 13;
      break;
    case ALT_SIGNAL_ACL6:
      input = 14;
      break;
    case ALT_SIGNAL_ACL7:
      input = 15;
      break;
    default:
      return false;
    }
    success = connect(input, output, false);
  }

  // Enable alt signal, but only if signal was connected successfully
  if (success)
    alt_signals |= alt_signal;

  return success;
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
  if (cfg.containsKey("alt_signals")) {
    if (cfg["alt_signals"].is<JsonArrayConst>()) {
      for (JsonVariantConst signal : cfg["alt_signals"].as<JsonArrayConst>()) {
        if (!signal.is<unsigned short>())
          return false;
        alt_signals |= 1 << signal.as<unsigned short>();
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
