// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <unity.h>

#include "carrier/cluster.h"
#include "daq/base.h"

constexpr float ANABRID_TESTS_TARGET_PRECISION = 0.05f;
constexpr bool ANABRID_TESTS_USE_ACL_OUT = true;

std::array<float, 16> measure_sh_gain(blocks::CTRLBlock *ctrl_block, platform::Cluster &cluster,
                                      daq::BaseDAQ *DAQ, unsigned int averages = 4,
                                      unsigned int averages_delay = 10) {
  // CARE: The SH-Block adds its offset corrections on gain measurements (as is expected),
  //       but starts with a random offset, so without calibration may give random signals.

  // Save and change ADC bus slection
  auto old_adcbus = ctrl_block->get_adc_bus();
  ctrl_block->set_adc_bus_to_cluster_gain(cluster.get_cluster_idx());
  if (!ctrl_block->write_to_hardware())
    return {999};

  // Measure all channels by using SH gain outputs and switching channels
  cluster.shblock->set_state(blocks::SHBlock::State::GAIN_ZERO_TO_SEVEN);
  (void)cluster.shblock->write_to_hardware();
  delay(100);
  auto data_zero_to_seven = DAQ->sample_avg(averages, averages_delay);
  cluster.shblock->set_state(blocks::SHBlock::State::GAIN_EIGHT_TO_FIFTEEN);
  (void)cluster.shblock->write_to_hardware();
  delay(100);
  auto data_eight_to_fifteen = DAQ->sample_avg(averages, averages_delay);

  std::array<float, 16> data{};
  std::copy(data_eight_to_fifteen.begin(), data_eight_to_fifteen.end(),
            std::copy(data_zero_to_seven.begin(), data_zero_to_seven.end(), data.begin()));

  // Restore ADC bus selection
  ctrl_block->set_adc_bus(old_adcbus);
  if (!ctrl_block->write_to_hardware())
    return {999};

  return data;
}
