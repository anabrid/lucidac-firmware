// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "bus/bus.h"
#include "bus/functions.h"

namespace functions {

class AD9834 : public DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  static constexpr uint16_t REG_FREQ_0 = 0x4000;
  static constexpr uint16_t REG_FREQ_1 = 0x8000;
  static constexpr uint16_t REG_PHASE_0 = 0xC000;
  static constexpr uint16_t REG_PHASE_1 = 0xE000;

  static constexpr uint16_t REG_FREQ_MASK = 0x3FFF;
  static constexpr uint16_t REG_PHASE_MASK = 0x0FFF;

  static constexpr uint16_t CONTROL_B28 = 0x2000;
  static constexpr uint16_t CONTROL_HLB = 0x1000;
  static constexpr uint16_t CONTROL_FSEL = 0x0800;
  static constexpr uint16_t CONTROL_PSEL = 0x0400;
  static constexpr uint16_t CONTROL_PINSW = 0x0200; // Enables pin control mode
  static constexpr uint16_t CONTROL_RESET = 0x0100;
  static constexpr uint16_t CONTROL_SLEEP1 = 0x0080;
  static constexpr uint16_t CONTROL_SLEEP12 = 0x0040;
  static constexpr uint16_t CONTROL_OPBITEN = 0x0020; // Enables square wave
  static constexpr uint16_t CONTROL_SIGNPIB = 0x0010;
  static constexpr uint16_t CONTROL_DIV2 = 0x0008;
  static constexpr uint16_t CONTROL_MODE = 0x0002; // Changes output mode to triangle

  static constexpr uint16_t ENABLE_SQUARE_WAVE = CONTROL_OPBITEN;
  static constexpr uint16_t ENABLE_TRIANGLE_WAVE = CONTROL_MODE;

  static constexpr float MASTER_CLK = 75000000.0f;

  enum class WaveForm { SINE, SINE_AND_SQUARE, TRIANGLE };

  using DataFunction::DataFunction;
  explicit AD9834(bus::addr_t address);

  //! Initialises the chip and puts it into sleep mode.
  bool init();
  //! Sets the frequency of the sine / triangle output in Hz. Note that the square output will always operate
  //! on half of the specified frequency.
  void write_frequency(float freq);
  //! Sets the phase of the outputs synchronised to the reset pin. Possible values are mapped to [0, 2PI].
  void write_phase(float phase);
  //! Sets the wave form of the function generator output.
  void write_wave_form(WaveForm wave_form);

  //! Returns the actually set frequency, containing rounding errors.
  float get_real_frequency() const;
  //! Returns the actually set phase, containing rounding errors. Possible values are [0, 2PI].
  float get_real_phase() const;

  //! Sets the sine / triangle output to zero. The square output will stay at high or low level.
  void sleep();
  //! Resumes outputs to regular operation, according to the previously specified frequencies.
  void awake();

private:
  uint16_t control_register = 0;
  uint32_t freq_register = 0;
  uint16_t phase_register = 0;

  void _write_control_register() const;
};

} // namespace functions
