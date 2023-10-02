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

#pragma once

#include <DMAChannel.h>
#include <FlexIO_t4.h>
#include <array>

/// @brief Routines for Data Aquisition (DAQ) / Analog2Digital converters (ADCs)
namespace daq {

constexpr uint8_t NUM_CHANNELS = 8;
constexpr uint8_t PIN_CNVST = 7;
constexpr uint8_t PIN_CLK = 6;
constexpr uint8_t PIN_GATE = 32;
constexpr std::array<uint8_t, NUM_CHANNELS> PINS_MISO = {34, 35, 36, 37, 11, 10, 9, 8};

constexpr unsigned int DEFAULT_SAMPLE_RATE = 100'000;

typedef std::array<float, NUM_CHANNELS> data_vec_t;

class BaseDAQ {
protected:
  static constexpr uint16_t RAW_MINUS_ONE = 2461;
  static constexpr uint16_t RAW_PLUS_ONE = 5733;

public:
  virtual bool init(unsigned int sample_rate) = 0;

  static float raw_to_float(uint16_t raw);

  virtual std::array<uint16_t, NUM_CHANNELS> sample_raw() = 0;
  virtual std::array<float, NUM_CHANNELS> sample() = 0;
  virtual float sample(uint8_t index) = 0;
  std::array<float, NUM_CHANNELS> sample_avg(size_t samples, unsigned int delay_us);
};

class FlexIODAQ : public BaseDAQ {
// TODO: Currently public for hacking
public:
  volatile uint32_t dma_buffer[8] __attribute__((used, aligned(32))) = {1, 2, 3, 4, 5, 6, 7, 8};

  FlexIOHandler *flexio;

  uint8_t _flexio_pin_cnvst;
  uint8_t _flexio_pin_clk;
  uint8_t _flexio_pin_gate;
  std::array<uint8_t, NUM_CHANNELS> _flexio_pins_miso;

public:
  FlexIODAQ();

  bool init(unsigned int sample_rate) override;
  void enable();
  void reset();

  std::array<uint16_t, NUM_CHANNELS> sample_raw() override;
  std::array<float, NUM_CHANNELS> sample() override;
  float sample(uint8_t index) override;
};

/**
 * Do single captures with the ADC.
 * This class is meant to be used as quasi Singleton standalone.
 **/
class OneshotDAQ : public BaseDAQ {
public:
  bool init(__attribute__((unused)) unsigned int sample_rate_unused) override;
  std::array<uint16_t, NUM_CHANNELS> sample_raw() override;
  std::array<float, NUM_CHANNELS> sample() override;

  /// Extracts a single number of a full word capture.
  float sample(uint8_t index) override;
};

} // namespace daq
