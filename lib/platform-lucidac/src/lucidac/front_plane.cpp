// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "front_plane.h"

platform::lucidac::FrontPlane::FrontPlane() : entities::Entity("FP") {}

bool platform::lucidac::FrontPlane::init() { return signal_generator.init(); }

platform::lucidac::FrontPlane::LEDs::LEDs()
    : led_register(bus::address_from_tuple(2, LED_REGISTER_IDX), true),
      led_register_store(bus::address_from_tuple(2, LED_REGISTER_STORE_IDX)),
      led_register_reset(bus::address_from_tuple(2, LED_REGISTER_RESET_IDX)) {}

bool platform::lucidac::FrontPlane::LEDs::set(const uint8_t led, bool on) {
  if (led > 7)
    return false;

  if (on)
    led_states |= (1 << led);
  else
    led_states &= ~(1 << led);

  return true;
}

void platform::lucidac::FrontPlane::LEDs::set_all(const uint8_t states) { led_states = states; }

void platform::lucidac::FrontPlane::LEDs::reset() {
  led_states = 0;
  led_register_reset.trigger();
}

uint8_t platform::lucidac::FrontPlane::LEDs::get_all() const { return led_states; }

bool platform::lucidac::FrontPlane::LEDs::write_to_hardware() const {
  bool ret = led_register.transfer8(led_states);
  led_register_store.trigger();
  return ret;
}

platform::lucidac::FrontPlane::SignalGenerator::SignalGenerator()
    : digital_analog_converter(bus::address_from_tuple(2, DAC_IDX)),
      function_generator(bus::address_from_tuple(2, FUNCTION_GENERATOR_IDX)) {}

bool platform::lucidac::FrontPlane::SignalGenerator::init() {
  return digital_analog_converter.init() && function_generator.init();
}

void platform::lucidac::FrontPlane::SignalGenerator::set_frequency(float frequency) {
  _frequency = frequency;
}

void platform::lucidac::FrontPlane::SignalGenerator::set_phase(float phase) { _phase = phase; }

void platform::lucidac::FrontPlane::SignalGenerator::set_wave_form(functions::AD9834::WaveForm wave_form) {
  _wave_form = wave_form;
}

bool platform::lucidac::FrontPlane::SignalGenerator::set_amplitude(float amplitude) {
  _amplitude = amplitude;
  return true;
}

bool platform::lucidac::FrontPlane::SignalGenerator::set_square_voltage_levels(float low, float high) {
  return set_square_voltage_low(low) && set_square_voltage_high(high);
}

bool platform::lucidac::FrontPlane::SignalGenerator::set_square_voltage_low(float low) {
  if (fabs(low) > 2.0f)
    return false;
  _square_low_voltage = low;
  return true;
}

bool platform::lucidac::FrontPlane::SignalGenerator::set_square_voltage_high(float high) {
  if (fabs(high) > 2.0f)
    return false;
  _square_high_voltage = high;
  return true;
}

bool platform::lucidac::FrontPlane::SignalGenerator::set_offset(float offset) {
  if (fabs(offset) > 2.0f)
    return false;
  _offset = offset;
  return true;
}

float platform::lucidac::FrontPlane::SignalGenerator::get_real_frequency() const {
  return function_generator.get_real_frequency();
}

float platform::lucidac::FrontPlane::SignalGenerator::get_real_phase() const {
  return function_generator.get_real_phase();
}

float platform::lucidac::FrontPlane::SignalGenerator::get_frequency() const { return _frequency; }

float platform::lucidac::FrontPlane::SignalGenerator::get_phase() const { return _phase; }

functions::AD9834::WaveForm platform::lucidac::FrontPlane::SignalGenerator::get_wave_form() const {
  return _wave_form;
}

float platform::lucidac::FrontPlane::SignalGenerator::get_amplitude() const { return _amplitude; }

float platform::lucidac::FrontPlane::SignalGenerator::get_square_voltage_low() const {
  return _square_low_voltage;
}

float platform::lucidac::FrontPlane::SignalGenerator::get_square_voltage_high() const {
  return _square_high_voltage;
}

float platform::lucidac::FrontPlane::SignalGenerator::get_offset() const { return _offset; }

bool platform::lucidac::FrontPlane::SignalGenerator::get_sleep() const { return _sleep; }

float platform::lucidac::FrontPlane::SignalGenerator::get_dac_out0() const { return _dac_out0; }

float platform::lucidac::FrontPlane::SignalGenerator::get_dac_out1() const { return _dac_out1; }

void platform::lucidac::FrontPlane::SignalGenerator::sleep() { _sleep = true; }

void platform::lucidac::FrontPlane::SignalGenerator::awake() { _sleep = false; }

bool platform::lucidac::FrontPlane::SignalGenerator::set_dac_out0(float value) {
  if (fabs(value) > 2.0f)
    return false;
  _dac_out0 = value;
  return true;
}

bool platform::lucidac::FrontPlane::SignalGenerator::set_dac_out1(float value) {
  if (fabs(value) > 2.0f)
    return false;
  _dac_out1 = value;
  return true;
}

// TODO: Implement smart write to hardware for signal generator
bool platform::lucidac::FrontPlane::SignalGenerator::write_to_hardware() {
  if (_sleep)
    function_generator.sleep();
  else
    function_generator.awake();

  function_generator.write_frequency(_frequency);
  function_generator.write_phase(_phase);
  function_generator.write_wave_form(_wave_form);

  return digital_analog_converter.set_channel(DAC_AMPLITUDE_CH, _amplitude) &&
         digital_analog_converter.set_channel(DAC_SQUARE_LOW_CH, _map_dac_levelshift(_square_low_voltage)) &&
         digital_analog_converter.set_channel(DAC_SQUARE_HIGH_CH, _map_dac_levelshift(_square_high_voltage)) &&
         digital_analog_converter.set_channel(DAC_OFFSET_CH, _map_dac_levelshift(_offset)) &&
         digital_analog_converter.set_channel(DAC_OUT0_CH, _map_dac_levelshift(_dac_out0)) &&
         digital_analog_converter.set_channel(DAC_OUT1_CH, _map_dac_levelshift(_dac_out1));
}

platform::lucidac::FrontPlane *
platform::lucidac::FrontPlane::from_entity_classifier(entities::EntityClassifier classifier) {
  if (!classifier or classifier.class_enum != CLASS_)
    return nullptr;

  // Currently, there are no different variants or versions
  if (classifier.variant != entities::EntityClassifier::DEFAULT_ or
      classifier.version != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  // Return default implementation
  return new FrontPlane;
}

bool platform::lucidac::FrontPlane::config_self_from_json(JsonObjectConst cfg) {
  if (!cfg["leds"]["signal_generator"]) {
    if (cfg["leds"].is<uint8_t>())
      leds.set_all(cfg["leds"]);
    else
      return false;

    if (cfg["signal_generator"].is<JsonObjectConst>()) {
      auto cfg_generator = cfg["signal_generator"].as<JsonObjectConst>();
      if (!cfg_generator["frequency"]["phase"]["wave_form"]["amplitude"]["square_voltage_low"]
                        ["square_voltage_high"]["offset"]["sleep"]["dac_outputs"]) {
        if (cfg_generator["frequency"].is<float>())
          signal_generator.set_frequency(cfg_generator["frequency"]);
        else
          return false;

        if (cfg_generator["phase"].is<float>())
          signal_generator.set_phase(cfg_generator["phase"]);
        else
          return false;

        if (cfg_generator["wave_form"].is<std::string>()) {
          std::string wave_form = cfg_generator["wave_form"];
          if (wave_form == "sine")
            signal_generator.set_wave_form(functions::AD9834::WaveForm::SINE);
          else if (wave_form == "sine_and_square")
            signal_generator.set_wave_form(functions::AD9834::WaveForm::SINE_AND_SQUARE);
          else if (wave_form == "triangle")
            signal_generator.set_wave_form(functions::AD9834::WaveForm::TRIANGLE);
          else
            return false;
        } else
          return false;

        if (cfg_generator["amplitude"].is<float>())
          signal_generator.set_amplitude(cfg_generator["amplitude"]);
        else
          return false;

        if (cfg_generator["square_voltage_low"].is<float>())
          signal_generator.set_square_voltage_low(cfg_generator["square_voltage_low"]);
        else
          return false;

        if (cfg_generator["square_voltage_high"].is<float>())
          signal_generator.set_square_voltage_high(cfg_generator["square_voltage_high"]);
        else
          return false;

        if (cfg_generator["offset"].is<float>())
          signal_generator.set_offset(cfg_generator["offset"]);
        else
          return false;

        if (cfg_generator["sleep"].is<bool>()) {
          if (cfg_generator["sleep"])
            signal_generator.sleep();
          else
            signal_generator.awake();
        } else
          return false;

        if (cfg_generator["dac_outputs"].is<JsonArrayConst>()) {
          auto dac_outputs = cfg_generator["dac_outputs"].as<JsonArrayConst>();
          if (dac_outputs.size() != 2)
            return false;

          if (dac_outputs[0].is<float>())
            if (!signal_generator.set_dac_out0(dac_outputs[0]))
              return false;

          if (dac_outputs[1].is<float>())
            if (!signal_generator.set_dac_out1(dac_outputs[1]))
              return false;
        }

      } else
        return false;
    } else
      return false;

    return true;
  }
  return false;
}

void platform::lucidac::FrontPlane::config_self_to_json(JsonObject &cfg) {
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
