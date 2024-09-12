// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <algorithm>

#include "ublock.h"

#include "utils/logging.h"

FLASHMEM void utils::shift_5_left(uint8_t *buffer, size_t size) {
  for (size_t idx = 0; idx < size - 1; idx++) {
    auto next = buffer[idx + 1];
    // Shift by 5, leaving zeros at 0-4
    buffer[idx] <<= 5;
    buffer[idx] |= (next >> 3) & 0x1F;
  }
  buffer[size - 1] <<= 5;
}

const SPISettings functions::UMatrixFunction::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE2};

FLASHMEM functions::UMatrixFunction::UMatrixFunction(bus::addr_t address)
    : functions::DataFunction(address, DEFAULT_SPI_SETTINGS) {}

FLASHMEM blocks::UBlock::UBlock(bus::addr_t block_address, UBlockHAL *hardware)
    : FunctionBlock("U", block_address), hardware(hardware), output_input_map{} {
  reset_connections();
}

FLASHMEM bool blocks::UBlock::_i_sanity_check(const uint8_t input) { return input < NUM_OF_INPUTS; }

FLASHMEM bool blocks::UBlock::_o_sanity_check(const uint8_t output) { return output < NUM_OF_OUTPUTS; }

FLASHMEM bool blocks::UBlock::_io_sanity_check(const uint8_t input, const uint8_t output) {
  return _i_sanity_check(input) && _o_sanity_check(output);
}

FLASHMEM void blocks::UBlock::_connect(const uint8_t input, const uint8_t output) { output_input_map[output] = input; }

FLASHMEM bool blocks::UBlock::connect(const uint8_t input, const uint8_t output, bool force) {
  if (!_io_sanity_check(input, output))
    return false;

  // Check for other connections on the same output, unless we don't care if we overwrite them
  if (!force and _is_output_connected(output))
    return false;

  // Check if the transmission mode makes the specified connection impossible and if the mode can or should be
  // chnged
  if (a_side_mode != Transmission_Mode::ANALOG_INPUT &&
      ((output < 16 && input != 15) || (output >= 16 && input != 14))) {
    if (!force)
      if (is_input_connected(input))
        return false;
    change_a_side_transmission_mode(Transmission_Mode::ANALOG_INPUT);
  }

  if (b_side_mode != Transmission_Mode::ANALOG_INPUT &&
      ((output < 16 && input == 15) || (output >= 16 && input == 14))) {
    if (!force)
      if (is_input_connected(input))
        return false;
    change_b_side_transmission_mode(Transmission_Mode::ANALOG_INPUT);
  }

  _connect(input, output);
  return true;
}

FLASHMEM bool blocks::UBlock::connect_alternative(Transmission_Mode signal_mode, const uint8_t output, bool force,
                                         bool use_a_side) {
  if (!_o_sanity_check(output))
    return false;

  // Do not consider Transmission_Mode::ANALOG_INPUT a valid mode
  if (signal_mode == Transmission_Mode::ANALOG_INPUT)
    return false;

  // When we want to connect a reference signal, we expect the reference magnitude to be one
  if ((signal_mode == Transmission_Mode::POS_REF or signal_mode == Transmission_Mode::NEG_REF) and
      ref_magnitude != Reference_Magnitude::ONE and !force)
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

FLASHMEM void blocks::UBlock::_disconnect(const uint8_t output) { output_input_map[output] = -1; }

FLASHMEM bool blocks::UBlock::disconnect(const uint8_t input, const uint8_t output) {
  if (!_io_sanity_check(input, output))
    return false;

  if (_is_connected(input, output)) {
    _disconnect(output);
    return true;
  } else {
    return false;
  }
}

FLASHMEM bool blocks::UBlock::disconnect(const uint8_t output) {
  if (!_o_sanity_check(output))
    return false;
  _disconnect(output);
  return true;
}

FLASHMEM bool blocks::UBlock::_is_connected(const uint8_t input, const uint8_t output) const {
  return output_input_map[output] == input;
}

FLASHMEM bool blocks::UBlock::is_connected(const uint8_t input, const uint8_t output) const {
  if (!_io_sanity_check(input, output))
    return false;

  return _is_connected(input, output);
}

FLASHMEM bool blocks::UBlock::_is_output_connected(const uint8_t output) const { return output_input_map[output] >= 0; }

FLASHMEM bool blocks::UBlock::is_output_connected(const uint8_t output) const {
  if (!_o_sanity_check(output))
    return false;

  return _is_output_connected(output);
}

FLASHMEM bool blocks::UBlock::_is_input_connected(const uint8_t input) const {
  for (const auto &output : output_input_map)
    if (output == input)
      return true;
  return false;
}

FLASHMEM bool blocks::UBlock::is_input_connected(const uint8_t input) const {
  if (!_i_sanity_check(input))
    return false;
  return _is_input_connected(input);
}

FLASHMEM bool blocks::UBlock::is_anything_connected() const {
  for (auto output : OUTPUT_IDX_RANGE())
    if (_is_output_connected(output))
      return true;
  return false;
}

FLASHMEM void blocks::UBlock::change_a_side_transmission_mode(const Transmission_Mode mode) { a_side_mode = mode; }

FLASHMEM void blocks::UBlock::change_b_side_transmission_mode(const Transmission_Mode mode) { b_side_mode = mode; }

FLASHMEM void blocks::UBlock::change_all_transmission_modes(const Transmission_Mode mode) {
  change_a_side_transmission_mode(mode);
  change_b_side_transmission_mode(mode);
}

FLASHMEM void blocks::UBlock::change_all_transmission_modes(
    const std::pair<Transmission_Mode, Transmission_Mode> modes) {
  change_a_side_transmission_mode(modes.first);
  change_b_side_transmission_mode(modes.second);
}

FLASHMEM std::pair<blocks::UBlock::Transmission_Mode, blocks::UBlock::Transmission_Mode>
blocks::UBlock::get_all_transmission_modes() const {
  return std::make_pair(a_side_mode, b_side_mode);
}

FLASHMEM blocks::UBlock::Reference_Magnitude blocks::UBlock::get_reference_magnitude() { return ref_magnitude; }

FLASHMEM void blocks::UBlock::change_reference_magnitude(blocks::UBlock::Reference_Magnitude ref) {
  ref_magnitude = ref;
}

FLASHMEM utils::status blocks::UBlock::write_to_hardware() {
  if (!hardware->write_outputs(output_input_map) or
      !hardware->write_transmission_modes_and_ref({a_side_mode, b_side_mode}, ref_magnitude)) {
    LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
    return utils::status::failure();
  }
  return utils::status::success();
}

FLASHMEM void blocks::UBlock::reset_connections() { std::fill(begin(output_input_map), end(output_input_map), -1); }

FLASHMEM void blocks::UBlock::reset(entities::ResetAction action) {
  FunctionBlock::reset(action);
  change_all_transmission_modes(blocks::UBlock::Transmission_Mode::ANALOG_INPUT);
  reset_connections();
  reset_reference_magnitude();
}

FLASHMEM
utils::status blocks::UBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  for (auto cfgItr = cfg.begin(); cfgItr != cfg.end(); ++cfgItr) {
    if (cfgItr->key() == "outputs") {
      auto ret = _config_outputs_from_json(cfgItr->value());
      if (!ret)
        return ret;
    } else if (cfgItr->key() == "constant") {
      auto ret = _config_constants_from_json(cfgItr->value());
      if (!ret)
        return ret;
    } else {
      return utils::status("UBlock: Unknown configuration key");
    }
  }
  return utils::status::success();
}

FLASHMEM
utils::status blocks::UBlock::_config_outputs_from_json(const JsonVariantConst &cfg) {
  // Handle an array of outputs
  // In this case, all outputs are reset
  if (cfg.is<JsonArrayConst>()) {
    auto outputs = cfg.as<JsonArrayConst>();
    if (outputs.size() != NUM_OF_OUTPUTS)
      return utils::status("Nblock: Wrong number of configuration elements. Given %d, expected %d",
                           outputs.size(), NUM_OF_OUTPUTS);
    reset_connections();
    uint8_t idx = 0;
    for (JsonVariantConst input : outputs) {
      if (input.isNull()) {
        idx++;
        continue;
      } else if (!input.is<uint8_t>()) {
        return utils::status("UBlock: Input is not an integer");
      }
      if (!connect(input.as<uint8_t>(), idx++))
        return utils::status("UBlock: Cannot connect input at idx");
    }
    return utils::status::success();
  }
  // Handle a mapping of outputs
  // In this case, only explicitly passed outputs are changed
  // To allow reconnections (swapping), we first clear all outputs and set them afterwards.
  else if (cfg.is<JsonObjectConst>()) {
    for (JsonPairConst keyval : cfg.as<JsonObjectConst>()) {
      // Sanity check that any input passed is a number
      if (!(keyval.value().is<uint8_t>() or keyval.value().isNull()))
        return utils::status("UBlock: Given input is not a number '%s'", keyval.value().as<const char *>());
      // TODO: Check conversion from string to number
      auto output = std::stoul(keyval.key().c_str());
      !disconnect(output);
    }
    for (JsonPairConst keyval : cfg.as<JsonObjectConst>()) {
      // TODO: Check conversion from string to number
      auto output = std::stoul(keyval.key().c_str());
      // Type is checked above
      if (keyval.value().isNull())
        continue;
      auto input = keyval.value().as<uint8_t>();
      if (!connect(input, output))
        return utils::status("UBlock: Could not connect %d -> %d", input, output);
    }
    return utils::status::success();
  }
  return utils::status("UBlock either requires array or object configuration");
}

FLASHMEM
utils::status blocks::UBlock::_config_constants_from_json(const JsonVariantConst &cfg) {
  float constant = cfg.as<float>();
  if (cfg.is<bool>())
    constant = cfg.as<bool>() ? 1 : 0;
  if (cfg.isNull())
    constant = 0;
  if (constant == 0) {
    // Turn OFF constant usage. We don't use Transmission_Mode::GROUND because
    // that's really pointless, we then also can just not connect the relevant UBlock switch.
    change_b_side_transmission_mode(blocks::UBlock::Transmission_Mode::ANALOG_INPUT);
    reset_reference_magnitude();
    utils::status::success();
  }
  if (constant > 0)
    change_b_side_transmission_mode(blocks::UBlock::Transmission_Mode::POS_REF);
  if (constant < 0)
    change_b_side_transmission_mode(blocks::UBlock::Transmission_Mode::NEG_REF);
  if (abs(constant) == 0.1f)
    change_reference_magnitude(blocks::UBlock::Reference_Magnitude::ONE_TENTH);
  else if (abs(constant) == 1.0f)
    change_reference_magnitude(blocks::UBlock::Reference_Magnitude::ONE);
  else
    return utils::status("UBlock::config_self_from_json: Pass +-0.1 or +-1.");
  return utils::status::success();
}

FLASHMEM void blocks::UBlock::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);
  // Save outputs into cfg
  auto outputs_cfg = cfg.createNestedArray("outputs");
  for (const auto &output : output_input_map) {
    if (output >= 0)
      outputs_cfg.add(output);
    else
      outputs_cfg.add(nullptr);
  }
  // Constant setting to JSON
  if (b_side_mode == blocks::UBlock::Transmission_Mode::ANALOG_INPUT) {
    cfg["constant"] = false;
  } else {
    float val = -123;
    // we support here much more then at the setter side, because the firmware
    // side has so much more states that simply have no representation at client side.
    switch (b_side_mode) {
    case blocks::UBlock::Transmission_Mode::POS_REF:
      val = +1.0;
      break;
    case blocks::UBlock::Transmission_Mode::NEG_REF:
      val = -1.0;
      break;
    case blocks::UBlock::Transmission_Mode::GROUND:
      val = 0;
      break;
    default:
      break;
    }
    if (ref_magnitude == blocks::UBlock::Reference_Magnitude::ONE_TENTH) {
      val *= 0.1;
    }
    cfg["constant"] = val;
  }
}

FLASHMEM
blocks::UBlock *blocks::UBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                       bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_ or classifier.type != TYPE)
    return nullptr;

  if (classifier.version < entities::Version(1, 2))
    return nullptr;
  if (classifier.version < entities::Version(1, 3))
    return new UBlock(block_address, new UBlockHAL_V_1_2_X(block_address));
  return nullptr;
}

FLASHMEM void blocks::UBlock::reset_reference_magnitude() { ref_magnitude = Reference_Magnitude::ONE; }

// blocks::UBlock_SwappedSR::UBlock_SwappedSR(bus::addr_t block_address)
//     : FunctionBlock("U", block_address), output_input_map{} {}

FLASHMEM bool blocks::UBlockHAL_Dummy::write_outputs(std::array<int8_t, 32> outputs) { return true; }

FLASHMEM bool blocks::UBlockHAL_Dummy::write_transmission_modes_and_ref(
    std::pair<Transmission_Mode, Transmission_Mode> modes, blocks::UBlockHAL::Reference_Magnitude ref) {
  return true;
}

FLASHMEM void blocks::UBlockHAL_Dummy::reset_transmission_modes_and_ref() {}

FLASHMEM blocks::UBlockHAL_Common::UBlockHAL_Common(bus::addr_t block_address, const uint8_t f_umatrix_cs,
                                           const uint8_t f_umatrix_sync_cs,
                                           const uint8_t f_transmission_mode_register_cs,
                                           const uint8_t f_transmission_mode_sync_cs,
                                           const uint8_t f_transmission_mode_reset_cs)
    : f_umatrix(bus::replace_function_idx(block_address, f_umatrix_cs)),
      f_umatrix_sync(bus::replace_function_idx(block_address, f_umatrix_sync_cs)),
      f_transmission_mode_register(bus::replace_function_idx(block_address, f_transmission_mode_register_cs),
                                   true),
      f_transmission_mode_sync(bus::replace_function_idx(block_address, f_transmission_mode_sync_cs)),
      f_transmission_mode_reset(bus::replace_function_idx(block_address, f_transmission_mode_reset_cs)) {}

FLASHMEM bool blocks::UBlockHAL_Common::write_outputs(std::array<int8_t, 32> outputs) {
  if (!f_umatrix.transfer(outputs))
    return false;
  f_umatrix_sync.trigger();
  return true;
}

FLASHMEM bool blocks::UBlockHAL_Common::write_transmission_modes_and_ref(
    std::pair<Transmission_Mode, Transmission_Mode> modes, Reference_Magnitude ref) {
  // Reference magnitude is defined by lowest bit
  // Group "A" mode is defined by the next two bits
  // Group "B" mode is defined by the next two bits
  // Remaining bits are not used
  uint8_t data = 0b000'00'00'0;
  data |= static_cast<uint8_t>(ref);
  data |= (static_cast<uint8_t>(modes.first) << 1);
  data |= (static_cast<uint8_t>(modes.second) << 3);

  if (!f_transmission_mode_register.transfer8(data))
    return false;
  f_transmission_mode_sync.trigger();
  return true;
}

FLASHMEM void blocks::UBlockHAL_Common::reset_transmission_modes_and_ref() { f_transmission_mode_reset.trigger(); }
