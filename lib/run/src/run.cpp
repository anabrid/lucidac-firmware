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

#include "run.h"

#include <utility>

run::RunConfig run::RunConfig::from_json(JsonObjectConst &json) {
  return {
      .ic_time = json["ic_time"], .op_time = json["op_time"], .halt_on_overload = json["halt_on_overload"]};
}

run::Run::Run(std::string id, const run::RunConfig &config)
    : id(std::move(id)), config(config), daq_config{} {}

run::Run::Run(std::string id, const run::RunConfig &config, const daq::DAQConfig &daq_config)
    : id(std::move(id)), config(config), daq_config(daq_config) {}

run::Run run::Run::from_json(JsonObjectConst &json) {
  auto json_run_config = json["config"].as<JsonObjectConst>();
  auto run_config = RunConfig::from_json(json_run_config);
  auto daq_config = daq::DAQConfig::from_json(json["daq_config"].as<JsonObjectConst>());
  auto id = json["id"].as<std::string>();
  if (id.size() != 32 + 4) {
    id = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
  }
  return {id, run_config, daq_config};
}

run::RunStateChange run::Run::to(run::RunState new_state, unsigned int t) {
  auto old = state;
  state = new_state;
  return {t, old, state};
}
