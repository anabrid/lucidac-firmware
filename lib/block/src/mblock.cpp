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

#include "mblock.h"

blocks::MBlock::MBlock(uint8_t cluster_idx, uint8_t slot_idx)
    : blocks::FunctionBlock{cluster_idx}, slot_idx{slot_idx} {}

bus::addr_t blocks::MBlock::get_block_address() { return bus::idx_to_addr(cluster_idx, slot_idx, 0); }

blocks::MIntBlock::MIntBlock(uint8_t cluster_idx, uint8_t slot_idx)
    : blocks::MBlock{cluster_idx, slot_idx}, f_ic_dac(bus::idx_to_addr(cluster_idx, slot_idx, IC_FUNC_IDX)),
      f_time_factor(bus::idx_to_addr(cluster_idx, slot_idx, TIME_FACTOR_FUNC_IDX), true),
      f_time_factor_sync(bus::idx_to_addr(cluster_idx, slot_idx, TIME_FACTOR_SYNC_FUNC_IDX)), ic_raw{0} {
  std::fill(std::begin(time_factors), std::end(time_factors), DEFAULT_TIME_FACTOR);
}

bool blocks::MIntBlock::set_ic(uint8_t idx, float value) {
  if (idx >= ic_raw.size())
    return false;
  if (value > 1.0f)
    value = 1.0f;
  if (value < -1.0f)
    value = -1.0f;
  ic_raw[idx] = decltype(f_ic_dac)::float_to_raw(value);
  return true;
}

void blocks::MIntBlock::write_to_hardware() {
  for (decltype(ic_raw.size()) i = 0; i < ic_raw.size(); i++) {
    f_ic_dac.set_channel(i, ic_raw[i]);
  }
  write_time_factors_to_hardware();
}

bool blocks::MIntBlock::init() {
  f_ic_dac.init();
  return true;
}

bool blocks::MIntBlock::set_time_factor(uint8_t int_idx, unsigned int k) {
  if (!(k == 100 or k == 10000))
    return false;
  if (int_idx >= 8)
    return false;
  time_factors[int_idx] = k;
  return true;
}

void blocks::MIntBlock::write_time_factors_to_hardware() {
  uint8_t switches = 0;
  for (size_t index = 0; index < time_factors.size(); index++) {
    if (time_factors[index] != DEFAULT_TIME_FACTOR) {
      switches |= 1 << index;
    }
  }
  f_time_factor.transfer(switches);
  f_time_factor_sync.trigger();
}
