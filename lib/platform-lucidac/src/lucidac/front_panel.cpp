// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "front_panel.h"

platform::LUCIDACFrontPanel::LUCIDACFrontPanel() : entities::Entity("FP") {}

FLASHMEM bool platform::LUCIDACFrontPanel::init() {
  leds.reset();
  return signal_generator.init();
}

FLASHMEM void platform::LUCIDACFrontPanel::reset() {
  leds.reset();
  signal_generator.sleep();
}

FLASHMEM utils::status platform::LUCIDACFrontPanel::write_to_hardware() {
  utils::status ret;
  ret.attach(leds.write_to_hardware(), "FP LED failure");
  ret.attach(signal_generator.write_to_hardware(), "Signal generator failure");
  return ret;
}

FLASHMEM platform::LUCIDACFrontPanel::LEDs::LEDs()
    : led_register(bus::address_from_tuple(2, LED_REGISTER_IDX), true),
      led_register_store(bus::address_from_tuple(2, LED_REGISTER_STORE_IDX)),
      led_register_reset(bus::address_from_tuple(2, LED_REGISTER_RESET_IDX)) {}

FLASHMEM bool platform::LUCIDACFrontPanel::LEDs::set(const uint8_t led, bool on) {
  if (led > 7)
    return false;

  if (on)
    led_states |= (1 << led);
  else
    led_states &= ~(1 << led);

  return true;
}

FLASHMEM void platform::LUCIDACFrontPanel::LEDs::set_all(const uint8_t states) { led_states = states; }

FLASHMEM void platform::LUCIDACFrontPanel::LEDs::reset() {
  led_states = 0;
  led_register_reset.trigger();
}

FLASHMEM uint8_t platform::LUCIDACFrontPanel::LEDs::get_all() const { return led_states; }

FLASHMEM utils::status platform::LUCIDACFrontPanel::LEDs::write_to_hardware() const {
  bool ret = led_register.transfer8(led_states);
  led_register_store.trigger();
  return utils::status(ret);
}

FLASHMEM platform::LUCIDACFrontPanel::SignalGenerator::SignalGenerator()
    : digital_analog_converter(bus::address_from_tuple(2, DAC_IDX)),
      function_generator(bus::address_from_tuple(2, FUNCTION_GENERATOR_IDX)) {}

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::init() {
  return digital_analog_converter.init() && function_generator.init();
}

FLASHMEM void platform::LUCIDACFrontPanel::SignalGenerator::set_frequency(float frequency) { _frequency = frequency; }

FLASHMEM void platform::LUCIDACFrontPanel::SignalGenerator::set_phase(float phase) { _phase = phase; }

FLASHMEM void platform::LUCIDACFrontPanel::SignalGenerator::set_wave_form(functions::AD9834::WaveForm wave_form) {
  _wave_form = wave_form;
}

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::set_amplitude(float amplitude) {
  _amplitude = amplitude;
  return true;
}

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::set_square_voltage_levels(float low, float high) {
  return set_square_voltage_low(low) && set_square_voltage_high(high);
}

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::set_square_voltage_low(float low) {
  if (fabs(low) > 2.0f)
    return false;
  _square_low_voltage = low;
  return true;
}

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::set_square_voltage_high(float high) {
  if (fabs(high) > 2.0f)
    return false;
  _square_high_voltage = high;
  return true;
}

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::set_offset(float offset) {
  if (fabs(offset) > 2.0f)
    return false;
  _offset = offset;
  return true;
}

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_real_frequency() const {
  return function_generator.get_real_frequency();
}

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_real_phase() const {
  return function_generator.get_real_phase();
}

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_frequency() const { return _frequency; }

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_phase() const { return _phase; }

FLASHMEM functions::AD9834::WaveForm platform::LUCIDACFrontPanel::SignalGenerator::get_wave_form() const {
  return _wave_form;
}

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_amplitude() const { return _amplitude; }

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_square_voltage_low() const {
  return _square_low_voltage;
}

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_square_voltage_high() const {
  return _square_high_voltage;
}

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_offset() const { return _offset; }

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::get_sleep() const { return _sleep; }

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_dac_out0() const { return _dac_out0; }

FLASHMEM float platform::LUCIDACFrontPanel::SignalGenerator::get_dac_out1() const { return _dac_out1; }

FLASHMEM void platform::LUCIDACFrontPanel::SignalGenerator::sleep() { _sleep = true; }

FLASHMEM void platform::LUCIDACFrontPanel::SignalGenerator::awake() { _sleep = false; }

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::set_dac_out0(float value) {
  if (fabs(value) > 2.0f)
    return false;
  _dac_out0 = value;
  return true;
}

FLASHMEM bool platform::LUCIDACFrontPanel::SignalGenerator::set_dac_out1(float value) {
  if (fabs(value) > 2.0f)
    return false;
  _dac_out1 = value;
  return true;
}

// TODO: Implement smart write to hardware for signal generator
FLASHMEM utils::status platform::LUCIDACFrontPanel::SignalGenerator::write_to_hardware() {
  if (_sleep)
    function_generator.sleep();
  else
    function_generator.awake();

  function_generator.write_frequency(_frequency);
  function_generator.write_phase(_phase);
  function_generator.write_wave_form(_wave_form);

  if (!digital_analog_converter.set_channel(DAC_AMPLITUDE_CH, _amplitude))
    return utils::status(-1, "FP DAC");
  if (!digital_analog_converter.set_channel(DAC_SQUARE_LOW_CH, _map_dac_levelshift(_square_low_voltage)))
    return utils::status(-2, "FP DSLC");
  if (!digital_analog_converter.set_channel(DAC_SQUARE_HIGH_CH, _map_dac_levelshift(_square_high_voltage)))
    return utils::status(-3, "FP DSHC");
  if (!digital_analog_converter.set_channel(DAC_OFFSET_CH, _map_dac_levelshift(_offset)))
    return utils::status(-4, "FP DOC");
  if (!digital_analog_converter.set_channel(DAC_OUT0_CH, _map_dac_levelshift(_dac_out0)))
    return utils::status(-5, "FP DO0C");
  if (!digital_analog_converter.set_channel(DAC_OUT1_CH, _map_dac_levelshift(_dac_out1)))
    return utils::status(-6, "FP DO1C");

  delay(50);
  return utils::status::success();
}

FLASHMEM 
platform::LUCIDACFrontPanel *
platform::LUCIDACFrontPanel::from_entity_classifier(entities::EntityClassifier classifier,
                                                    __attribute__((__unused__)) bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_)
    return nullptr;

  // Currently, there are no different variants
  if (classifier.variant != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  if (classifier.version < entities::Version(1, 1))
    return nullptr;
  if (classifier.version < entities::Version(1, 2))
    return new LUCIDACFrontPanel;
  return nullptr;
}

FLASHMEM std::array<uint8_t, 8> platform::LUCIDACFrontPanel::get_entity_eui() const {
  return metadata::MetadataEditor(bus::address_from_tuple(2, 0)).read_eui();
}

FLASHMEM utils::status platform::LUCIDACFrontPanel::config_self_from_json(JsonObjectConst cfg) {
  for (auto cfgItr = cfg.begin(); cfgItr != cfg.end(); ++cfgItr) {
    if (cfgItr->key() == "leds") {
      auto res = _config_leds_from_json(cfgItr->value());
      if (!res)
        return res;
    } else if (cfgItr->key() == "signal_generator") {
      auto res = _config_signal_generator_from_json(cfgItr->value());
      if (!res)
        return res;
    } else {
      return utils::status("LUCIDACFrontPanel: Unknown configuration key");
    }
  }

  return utils::status::success();
}

FLASHMEM utils::status platform::LUCIDACFrontPanel::_config_leds_from_json(const JsonVariantConst &cfg) {
  if (!cfg.is<uint8_t>())
    return utils::status("LUCIDACFrontPanel LED configuration must be encoded as single integer");
  leds.set_all(cfg);
  return utils::status::success();
}

FLASHMEM utils::status platform::LUCIDACFrontPanel::_config_signal_generator_from_json(const JsonVariantConst &cfg) {
  if (!cfg.is<JsonObjectConst>())
    return utils::status("LUCIDACFrontPanel signal generator configuration must be object");

  auto cfg_generator = cfg.as<JsonObjectConst>();
  for (auto cfgItr = cfg_generator.begin(); cfgItr != cfg_generator.end(); ++cfgItr) {
    if (cfgItr->key() == "frequency") {
      if (!cfgItr->value().is<float>())
        return utils::status("LUCIDACFrontPanel frequency value must be float");

      signal_generator.set_frequency(cfgItr->value());

    } else if (cfgItr->key() == "phase") {
      if (!cfgItr->value().is<float>())
        return utils::status("LUCIDACFrontPanel phase value must be float");

      signal_generator.set_phase(cfgItr->value());

    } else if (cfgItr->key() == "wave_form") {
      if (!cfgItr->value().is<std::string>())
        return utils::status("LUCIDACFrontPanel wave form must be string");

      std::string wave_form = cfg_generator["wave_form"];
      if (wave_form == "sine")
        signal_generator.set_wave_form(functions::AD9834::WaveForm::SINE);
      else if (wave_form == "sine_and_square")
        signal_generator.set_wave_form(functions::AD9834::WaveForm::SINE_AND_SQUARE);
      else if (wave_form == "triangle")
        signal_generator.set_wave_form(functions::AD9834::WaveForm::TRIANGLE);
      else
        return utils::status("LUCIDACFrontPanel wave form illegal value");

    } else if (cfgItr->key() == "amplitude") {
      if (!cfgItr->value().is<float>())
        return utils::status("LUCIDACFrontPanel amplitude must be float");

      signal_generator.set_amplitude(cfgItr->value());

    } else if (cfgItr->key() == "square_voltage_low") {
      if (!cfgItr->value().is<float>())
        return utils::status("LUCIDACFrontPanel square_voltage_low must be float");

      signal_generator.set_square_voltage_low(cfgItr->value());

    } else if (cfgItr->key() == "square_voltage_high") {
      if (!cfgItr->value().is<float>())
        return utils::status("LUCIDACFrontPanel square_voltage_high must be float");

      signal_generator.set_square_voltage_high(cfgItr->value());

    } else if (cfgItr->key() == "offset") {
      if (!cfgItr->value().is<float>())
        return utils::status("LUCIDACFrontPanel offset must be float");

      signal_generator.set_offset(cfgItr->value());

    } else if (cfgItr->key() == "sleep") {
      if (!cfgItr->value().is<bool>())
        return utils::status("LUCIDACFrontPanel sleep must be bool");

      if (cfgItr->value())
        signal_generator.sleep();
      else
        signal_generator.awake();

    } else if (cfgItr->key() == "dac_outputs") {
      if (!cfgItr->value().is<JsonArrayConst>())
        return utils::status("LUCIDACFrontPanel dac_outputs must be array");

      auto dac_outputs = cfg_generator["dac_outputs"].as<JsonArrayConst>();
      if (dac_outputs.size() != 2)
        return utils::status("LUCIDACFrontPanel dac_outputs must be array of size 2");

      if (!dac_outputs[0].is<float>())
        return utils::status("LUCIDACFrontPanel dac_outputs[0] is not float");
      if (!dac_outputs[1].is<float>())
        return utils::status("LUCIDACFrontPanel dac_outputs[1] is not float");

      if (!signal_generator.set_dac_out0(dac_outputs[0]))
        return utils::status("LUCIDACFrontPanel dac_outputs[0] illegal value %d", dac_outputs[0]);
      if (!signal_generator.set_dac_out1(dac_outputs[1]))
        return utils::status("LUCIDACFrontPanel dac_outputs[1] illegal value %d", dac_outputs[1]);

    } else {
      return utils::status("LUCIDACFrontPanel: Unexpected keyword: %s", cfgItr->key().c_str());
    }
  }
  return utils::status::success();
}

FLASHMEM void platform::LUCIDACFrontPanel::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);

  cfg["leds"] = leds.get_all();

  auto cfg_generator = cfg.createNestedObject("signal_generator");
  cfg_generator["frequency"] = signal_generator.get_frequency();
  cfg_generator["phase"] = signal_generator.get_phase();

  switch (signal_generator.get_wave_form()) {
  case functions::AD9834::WaveForm::SINE:
    cfg_generator["wave_form"] = "sine";
    break;
  case functions::AD9834::WaveForm::SINE_AND_SQUARE:
    cfg_generator["wave_form"] = "sine_and_square";
    break;
  case functions::AD9834::WaveForm::TRIANGLE:
    cfg_generator["wave_form"] = "triangle";
    break;
  }

  cfg_generator["amplitude"] = signal_generator.get_amplitude();
  cfg_generator["square_voltage_low"] = signal_generator.get_square_voltage_low();
  cfg_generator["square_voltage_high"] = signal_generator.get_square_voltage_high();
  cfg_generator["offset"] = signal_generator.get_offset();
  cfg_generator["sleep"] = signal_generator.get_sleep();

  auto cfg_dac = cfg_generator.createNestedArray("dac_outputs");
  cfg_dac.add(signal_generator.get_dac_out0());
  cfg_dac.add(signal_generator.get_dac_out1());
}
