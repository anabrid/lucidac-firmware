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

#include "base.h"

daq::DAQConfig::DAQConfig(uint8_t num_channels, unsigned int sample_rate)
    : num_channels(num_channels), sample_rate(sample_rate) {
  // CARE: num_channels must be power-of-two and the object should be checked with is_valid() afterwards
}

daq::DAQConfig daq::DAQConfig::from_json(JsonObjectConst &&json) {
  DAQConfig daq_config;
  // Return default if no options are given
  if (json.isNull())
    return daq_config;
  // Otherwise, parse DAQConfig from json
  auto num_channels = json["num_channels"];
  if (!num_channels.isNull() and num_channels.is<unsigned int>())
    daq_config.num_channels = num_channels;
  auto sample_rate = json["sample_rate"];
  if (!sample_rate.isNull() and sample_rate.is<unsigned int>())
    daq_config.sample_rate = sample_rate;
  auto sample_op = json["sample_op"];
  if (!sample_op.isNull() and sample_op.is<bool>())
    daq_config.sample_op = sample_op;
  auto sample_op_end = json["sample_op_end"];
  if (!sample_op_end.isNull() and sample_op_end.is<bool>())
    daq_config.sample_op_end = sample_op_end;
  return daq_config;
}

uint8_t daq::DAQConfig::get_num_channels() const { return num_channels; }

unsigned int daq::DAQConfig::get_sample_rate() const { return sample_rate; }

bool daq::DAQConfig::should_sample_op() const { return sample_op; }

bool daq::DAQConfig::should_sample_op_end() const { return sample_op_end; }

bool daq::DAQConfig::is_valid() const {
  // Total effective samples per seconds is limited due to streaming speed
  if (sample_rate * num_channels > 1'000'000 or sample_rate < 32)
    return false;
  // Number of channels must be power-of-two
  if ((num_channels > 0) and ((num_channels & (num_channels - 1)) != 0))
    return false;
  return true;
}
