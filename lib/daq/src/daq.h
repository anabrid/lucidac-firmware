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

#include <FlexIO_t4.h>
#include <array>

namespace daq {

constexpr uint8_t NUM_CHANNELS = 8;
constexpr uint8_t PIN_CNVST = 7;
constexpr uint8_t PIN_CLK = 6;
constexpr std::array<uint8_t, NUM_CHANNELS> PINS_MISO = {34, 35, 36, 37, 11, 10, 9, 8};

class BaseDAQ {
protected:
  static constexpr uint16_t RAW_MINUS_ONE = 2461;
  static constexpr uint16_t RAW_PLUS_ONE = 5733;
public:
  virtual bool init(unsigned int sample_rate) = 0;

  static float raw_to_float(uint16_t raw);

  virtual std::array<uint16_t, NUM_CHANNELS> sample_raw() = 0;
  virtual std::array<float, NUM_CHANNELS> sample() = 0;
};

class FlexIODAQ : public BaseDAQ {
private:
  FlexIOHandler *flexio;

  uint8_t _flexio_pin_cnvst;
  uint8_t _flexio_pin_clk;
  std::array<uint8_t, NUM_CHANNELS> _flexio_pins_miso;

public:
  FlexIODAQ();

  bool init(unsigned int sample_rate) override;
  bool _init_cnvst(unsigned int sample_rate);
  void enable();
  bool _init_clk();
};

class OneshotDAQ : public BaseDAQ {
public:
  bool init(__attribute__((unused)) unsigned int sample_rate_unused) override;
  std::array<uint16_t, NUM_CHANNELS> sample_raw() override;
  std::array<float, NUM_CHANNELS> sample() override;
};

}
