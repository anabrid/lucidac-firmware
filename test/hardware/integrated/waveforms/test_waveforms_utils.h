// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <vector>

#include <unity.h>

#include "daq/daq.h"
#include "lucidac/lucidac.h"
#include "run/run.h"
#include "run/run_manager.h"

class LocalBufferRunDataHandler : public run::RunDataHandler {
public:
  std::vector<float> &buffer;

  explicit LocalBufferRunDataHandler(std::vector<float> &buffer) : buffer(buffer) {}

  void handle(volatile uint32_t *data, size_t outer_count, size_t inner_count, const run::Run &run) override {
    for (size_t outer_i = 0; outer_i < outer_count; outer_i++)
      for (size_t inner_i = 0; inner_i < inner_count; inner_i++) {
        const uint32_t number = data[outer_i * inner_count + inner_i];
        buffer.push_back(daq::BaseDAQ::raw_to_float(number));
      }
  }

  void stream(volatile uint32_t *, run::Run &) override {}

  void prepare(run::Run &run) override {
    auto n_samples = run.config.op_time * run.daq_config.get_sample_rate() / 1'000'000'000;
    n_samples += 10; // better safe than sorry
    n_samples *= run.daq_config.get_num_channels();
    buffer.reserve(n_samples);
    buffer.clear();
  }
};

class PersistentErrorRunStateChangeHandler : public run::RunStateChangeHandler {
public:
  bool error = false;

  void handle(run::RunStateChange change, const run::Run &run) override {
    if (change.new_ == run::RunState::ERROR)
      error = true;
  }
};

void test_waveform(platform::LUCIDAC &carrier_, run::Run (*configure_waveform)(),
                   bool (*check_waveform)(const run::Run &, const std::vector<float> &),
                   bool print_data = true) {
  TEST_MESSAGE("Initializing hardware...");
  // Default initialization of carrier and blocks
  TEST_ASSERT(carrier_.init());
  // We do need certain blocks
  for (auto &cluster : carrier_.clusters) {
    TEST_ASSERT_NOT_NULL(cluster.ublock);
    TEST_ASSERT_NOT_NULL(cluster.cblock);
    TEST_ASSERT_NOT_NULL(cluster.iblock);
    TEST_ASSERT_NOT_NULL(cluster.shblock);
    // Test cases should check for M-blocks themselves
  }
  TEST_ASSERT_NOT_NULL(carrier_.ctrl_block);
  carrier_.reset(false);
  TEST_ASSERT(carrier_.write_to_hardware());

  // Let the user-defined function set a configuration
  TEST_MESSAGE("Configuring waveform...");
  auto run = configure_waveform();
  TEST_ASSERT(run.daq_config.is_valid());

  // Calibrate
  TEST_MESSAGE("Calibrating system...");
  daq::OneshotDAQ daq_;
  daq_.init(0);
  TEST_ASSERT(carrier_.calibrate_routes(&daq_));

  std::vector<float> buffer;
  LocalBufferRunDataHandler run_data_handler(buffer);
  PersistentErrorRunStateChangeHandler state_change_handler;
  auto &run_manager = run::RunManager::get();

  TEST_MESSAGE("Computing waveform...");
  run_manager.run_next_flexio(run, &state_change_handler, &run_data_handler);
  delayMicroseconds(5);

  // Print data if requested
  if (print_data) {
    for (auto idx = 0u; idx < buffer.size(); idx++) {
      if (idx % run.daq_config.get_num_channels() == 0) {
        Serial.println();
        Serial.print("DATA\t");
        Serial.print(run.daq_config.index_to_time(idx / run.daq_config.get_num_channels()), 6);
        Serial.print("\t");
      }
      Serial.print(buffer[idx], 4);
      Serial.print("\t");
    }
    Serial.println();

    for (auto channel = 0u; channel < run.daq_config.get_num_channels(); channel++) {
      // Print header
      Serial.print("= CH ");
      Serial.print(channel);
      Serial.print(" ");
      for (auto idx = channel; idx < buffer.size(); idx += run.daq_config.get_num_channels()) {
        Serial.print("=");
      }
      Serial.println();
      // Print plot
      for (auto row_value_ = 120; row_value_ >= -120; row_value_ -= 10) {
        float row_value = static_cast<float>(row_value_) / 100.0f;
        if (row_value_ >= 0)
          Serial.print("+");
        Serial.print(row_value, 2);
        Serial.print(" | ");

        for (auto idx = channel; idx < buffer.size(); idx += run.daq_config.get_num_channels()) {
          if (fabs(row_value - buffer[idx]) <= 0.05f)
            Serial.print("*");
          else if (row_value_ == 0)
            Serial.print("-");
          else
            Serial.print(" ");
        }

        Serial.println();
      }
    }
  }

  // Check data for correctness
  TEST_MESSAGE("Checking waveform...");
  TEST_ASSERT_FALSE(state_change_handler.error);
  TEST_ASSERT(check_waveform(run, buffer));
}
