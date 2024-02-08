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

#include <array>
#include <cstdint>

#include <ArduinoJson.h>

namespace daq {

constexpr uint8_t NUM_CHANNELS = 8;

constexpr uint8_t PIN_CNVST = 7;
constexpr uint8_t PIN_CLK = 6;
constexpr uint8_t PIN_GATE = 32;
constexpr std::array<uint8_t, NUM_CHANNELS> PINS_MISO = {34, 35, 36, 37, 11, 10, 9, 8};
constexpr unsigned int DEFAULT_SAMPLE_RATE = 400'000;


typedef std::array<float, NUM_CHANNELS> data_vec_t;

class DAQConfig {
  uint8_t num_channels = NUM_CHANNELS;
  unsigned int sample_rate = DEFAULT_SAMPLE_RATE;
  bool sample_op = true;
  bool sample_op_end = true;

public:
  DAQConfig() = default;
  DAQConfig(uint8_t num_channels, unsigned int sample_rate);

  uint8_t get_num_channels() const;
  unsigned int get_sample_rate() const;
  bool should_sample_op() const;
  bool should_sample_op_end() const;

  bool is_valid() const;

  explicit operator bool() const {
      return num_channels and (sample_op or sample_op_end);
  };

  static DAQConfig from_json(JsonObjectConst&& json);
};

}