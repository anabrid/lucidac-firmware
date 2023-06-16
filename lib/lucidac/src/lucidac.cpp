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

#include "lucidac.h"
#include "local_bus.h"
#include "running_avg.h"

#define RETURN_FALSE_IF_FAILED(x)                                                                             \
  if (!(x))                                                                                                   \
    return false;

auto lucidac::LUCIDAC::get_blocks() {
  return std::array<blocks::FunctionBlock *, 5>{m1block, m2block, ublock, cblock, iblock};
}

bool lucidac::LUCIDAC::init() {
  bus::init();
  for (auto block : get_blocks()) {
    if (block && !block->init())
      return false;
  }
  return true;
}

lucidac::LUCIDAC::LUCIDAC()
    : m1block{new blocks::MIntBlock{0, blocks::MBlock::M1_IDX}}, ublock{new blocks::UBlock{0}}, cblock{new blocks::CBlock{0}}, iblock{new blocks::IBlock{0}} {
  // TODO: Check for existence of blocks here instead of initializing them without checking
}

bool lucidac::LUCIDAC::calibrate(daq::BaseDAQ *daq) {
  RETURN_FALSE_IF_FAILED(calibrate_offsets_ublock_initial(daq));
  // Return to a non-connected state, but keep calibrated offsets
  ublock->reset(true);
  ublock->write_to_hardware();
  return true;
}

bool lucidac::LUCIDAC::calibrate_offsets_ublock_initial(daq::BaseDAQ *daq) {
  // Reset
  ublock->reset();
  // Connect REF signal from UBlock to ADC outputs
  ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF);
  for (auto out_to_adc : std::array<uint8_t, 8>{0, 1, 2, 3, 4, 5, 6, 7}) {
    ublock->connect(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, out_to_adc);
  }
  ublock->write_to_hardware();
  // Let the signal settle
  delayMicroseconds(250);

  auto data_avg = daq->sample_avg(10, 10000);
  for (size_t i = 0; i < data_avg.size(); i++) {
    RETURN_FALSE_IF_FAILED(ublock->change_offset(i, data_avg[i] + 1.0f));
  }
  ublock->write_to_hardware();
  delayMicroseconds(100);

  return true;
  // TODO: Finish calibration sequence
}

void lucidac::LUCIDAC::write_to_hardware() {
  for (auto block : get_blocks()) {
    if (block)
      block->write_to_hardware();
  }
}

bool lucidac::LUCIDAC::route(uint8_t u_in, uint8_t u_out, float c_factor, uint8_t i_out) {
  if (!ublock->connect(u_in, u_out))
    return false;
  if (!cblock->set_factor(u_out, c_factor))
    return false;
  if (!iblock->connect(u_out, i_out))
    return false;
  return true;
}