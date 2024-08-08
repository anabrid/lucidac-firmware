// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "bus/bus.h"
#include "entity/entity.h"

#include "chips/AD9834.h"
#include "chips/DAC60508.h"
#include "chips/SR74HCT595.h"

namespace platform {

/**
 * The Lucidac Front Panel is represented by this class.
 *
 * This class provides a interface for all the functions available on the Lucidac Front Panel.
 *
 * A Lucidac can only have a single front panel.
 * Typical usage happens via the Lucidac class.
 *
 **/
class LUCIDACFrontPanel : public entities::Entity {
public:
  // Entity hardware identifier information.
  static constexpr auto CLASS_ = entities::EntityClass::FRONT_PANEL;

  static LUCIDACFrontPanel *from_entity_classifier(entities::EntityClassifier classifier,
                                            __attribute__((__unused__)) bus::addr_t block_address);

  entities::EntityClass get_entity_class() const final { return CLASS_; }

  bool config_self_from_json(JsonObjectConst cfg) override;

  Entity *get_child_entity(const std::string &child_id) override { return nullptr; }

  std::vector<Entity *> get_child_entities() override { return {}; };

  void config_self_to_json(JsonObject &cfg) override;

public:
  static constexpr uint8_t FUNCTION_GENERATOR_IDX = 2;
  static constexpr uint8_t DAC_IDX = 3;
  static constexpr uint8_t LED_REGISTER_IDX = 4;
  static constexpr uint8_t LED_REGISTER_STORE_IDX = 5;
  static constexpr uint8_t LED_REGISTER_RESET_IDX = 6;

  static constexpr uint8_t DAC_AMPLITUDE_CH = 0;
  static constexpr uint8_t DAC_SQUARE_HIGH_CH = 1;
  static constexpr uint8_t DAC_SQUARE_LOW_CH = 2;
  static constexpr uint8_t DAC_OFFSET_CH = 3;
  static constexpr uint8_t DAC_OUT0_CH = 4;
  static constexpr uint8_t DAC_OUT1_CH = 5;

  LUCIDACFrontPanel();

  //! Initializes the front panel and puts the frequency generator to sleep.
  bool init();

  //! Writes the hardware state of the LEDs and the signal generator
  bool write_to_hardware();

  class LEDs {
  public:
    LEDs();

    //! Sets specified front led to the specified value.
    bool set(const uint8_t led, bool on);
    //! Sets all eight front leds at once.
    void set_all(const uint8_t states);
    //! Disables all eight front leds.
    void reset();

    //! Returns the current state of the front leds.
    uint8_t get_all() const;

    bool write_to_hardware() const;

  private:
    const functions::SR74HCT595 led_register;
    const functions::TriggerFunction led_register_store;
    const functions::TriggerFunction led_register_reset;

    uint8_t led_states = 0;
  } leds;

  class SignalGenerator {
  public:
    SignalGenerator();
    //! Initializes the signal generator and puts it to sleep.
    bool init();

    //! Sets the frequency of the sine / triangle output in Hz. Note that the square output will always operate
    //! on half of the specified frequency. Returns the actually set frequency.
    void set_frequency(float frequency);
    //! Sets the phase of the frequency outputs synchronised to the reset pin. Possible values are [0, 2 PI].
    //! Returns the actually set phase.
    void set_phase(float phase);
    //! Sets the wave form of the function generator output.
    void set_wave_form(functions::AD9834::WaveForm wave_form);
    //! Sets the amplitude of the sine or triangle wave. Possible values are: [?, ?]. The amplitude of the
    //! square output is not affected by this function.
    bool set_amplitude(float amplitude);
    //! Sets the lower and upper voltage of the square output. Possible values are: [-2V, 2V]. The amplitude of
    //! the sine / triangle output is not affected by this function.
    bool set_square_voltage_levels(float low, float high);
    //! Sets the lower voltage of the square output. Possible values are: [-2V, 2V]. The amplitude of
    //! the sine / triangle output is not affected by this function.
    bool set_square_voltage_low(float low);
    //! Sets the upper voltage of the square output. Possible values are: [-2V, 2V]. The amplitude of
    //! the sine / triangle output is not affected by this function.
    bool set_square_voltage_high(float high);
    //! Sets the constant voltage offset of the sine or triangle outpu. The levels of the square output are not
    //! affected by this function.
    bool set_offset(float offset);

    //! Returns the actually set frequency of the function generator, containing rounding errors.
    float get_real_frequency() const;
    //! Returns the actually set phase of the function generator, containing rounding errors. Possible values
    //! are mapped to [0, 2PI].
    float get_real_phase() const;

    float get_frequency() const;
    float get_phase() const;
    functions::AD9834::WaveForm get_wave_form() const;
    float get_amplitude() const;
    float get_square_voltage_low() const;
    float get_square_voltage_high() const;
    float get_offset() const;
    bool get_sleep() const;

    float get_dac_out0() const;
    float get_dac_out1() const;

    //! Sets the sine / triangle output of the function generator to zero. The square output will stay at high
    //! or low level.
    void sleep();
    //! Resumes outputs of the function generator to regular operation, according to the previously specified
    //! frequencies.
    void awake();

    //! Writes the DACout0 constant voltage output. Possible values are: [-2V, 2V].
    bool set_dac_out0(float value);
    //! Writes the DACout1 constant voltage output. Possible values are: [-2V, 2V].
    bool set_dac_out1(float value);

    bool write_to_hardware();

  private:
    const functions::DAC60508 digital_analog_converter;
    functions::AD9834 function_generator;

    //! Remapping for the built in levelshift on some dac outputs.
    static constexpr float _map_dac_levelshift(float x) {
      return (x + 2.0f) * (0.0f - 2.5f) / (2.0f + 2.0f) + 2.5f;
    }

    float _frequency;
    float _phase;
    functions::AD9834::WaveForm _wave_form;
    float _amplitude;
    float _square_low_voltage;
    float _square_high_voltage;
    float _offset;
    float _dac_out0;
    float _dac_out1;

    bool _sleep = true;

  } signal_generator;
};
} // namespace platform
