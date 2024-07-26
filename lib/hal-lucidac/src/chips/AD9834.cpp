// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "AD9834.h"

const SPISettings functions::AD9834::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE2};

functions::AD9834::AD9834(bus::addr_t address) : functions::DataFunction(address, DEFAULT_SPI_SETTINGS) {}

bool functions::AD9834::init() {
  control_register |= CONTROL_B28;
  control_register |= CONTROL_PINSW;
  control_register |= CONTROL_SLEEP1;
  control_register |= CONTROL_SLEEP12;
  control_register |= CONTROL_DIV2;

  begin_communication();
  get_raw_spi().transfer16(CONTROL_RESET);
  get_raw_spi().transfer16(control_register);
  end_communication();

  // TODO: Check if pedantic mode can be implemented
  return true;
}

void functions::AD9834::sleep() {
  control_register |= CONTROL_SLEEP1;
  control_register |= CONTROL_SLEEP12;

  _write_control_register();
}

void functions::AD9834::awake() {
  control_register &= ~CONTROL_SLEEP1;
  control_register &= ~CONTROL_SLEEP12;

  _write_control_register();
}

void functions::AD9834::write_wave_form(WaveForm wave_form) {
  switch (wave_form) {
  case functions::AD9834::WaveForm::TRIANGLE:
    control_register &= ~ENABLE_SQUARE_WAVE;
    control_register |= ENABLE_TRIANGLE_WAVE;
    break;
  case functions::AD9834::WaveForm::SINE_AND_SQUARE:
    control_register &= ~ENABLE_TRIANGLE_WAVE;
    control_register |= ENABLE_SQUARE_WAVE;
    break;
  case functions::AD9834::WaveForm::SINE:
    control_register &= ~ENABLE_TRIANGLE_WAVE;
    control_register &= ~ENABLE_SQUARE_WAVE;
    break;
  }
  _write_control_register();
}

void functions::AD9834::write_frequency(float freq) {
  freq_register = static_cast<uint32_t>((freq * static_cast<float>(1 << 28)) / MASTER_CLK);

  begin_communication();
  get_raw_spi().transfer16((freq_register & REG_FREQ_MASK) | REG_FREQ_0);         // Frequency register 0 LSB
  get_raw_spi().transfer16(((freq_register >> 14) & REG_FREQ_MASK) | REG_FREQ_0); // Frequency register 0 MSB
  end_communication();
}

void functions::AD9834::write_phase(float phase) {
  phase_register = static_cast<uint16_t>(fmod(phase, 2.0f * PI) * 4096.0f / (2.0f * PI));

  begin_communication();
  get_raw_spi().transfer16((phase_register & REG_PHASE_MASK) | REG_PHASE_0); // Phase register 0
  end_communication();
}

float functions::AD9834::get_real_frequency() const {
  return static_cast<float>(freq_register * MASTER_CLK / static_cast<float>(1 << 28));
}

float functions::AD9834::get_real_phase() const {
  return static_cast<float>(phase_register) / 4096.0f * 2.0f * PI;
}

void functions::AD9834::_write_control_register() const {
  begin_communication();
  get_raw_spi().transfer16(control_register);
  end_communication();
}
