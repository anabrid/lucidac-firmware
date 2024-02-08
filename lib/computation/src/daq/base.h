// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

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