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

blocks::MIntBlock::MIntBlock(uint8_t cluster_idx, uint8_t m_slot)
    : blocks::MBlock{cluster_idx}, slot_idx(m_slot),
      f_ic_dac(bus::idx_to_addr(cluster_idx, m_slot, IC_FUNC_IDX)), ic_raw{0} {}

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
}

void blocks::MIntBlock::init() { f_ic_dac.init(); }
