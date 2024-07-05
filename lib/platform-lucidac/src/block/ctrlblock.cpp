// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "ctrlblock.h"

blocks::CTRLBlockHAL_V_1_0_1::CTRLBlockHAL_V_1_0_1()
    : f_adc_mux(bus::address_from_tuple(1, 2)), f_adc_mux_latch(bus::address_from_tuple(1, 4)) {}

bool blocks::CTRLBlockHAL_V_1_0_1::write_adc_bus_muxers(ADCBus adc_bus) {
  auto ret = f_adc_mux.transfer8(static_cast<uint8_t>(adc_bus));
  f_adc_mux_latch.trigger();
  return ret;
}

float blocks::CTRLBlockHAL_V_1_0_1::read_temperature() { return 0; }

blocks::CTRLBlock::CTRLBlock() : FunctionBlock("CTRL", bus::address_from_tuple(1, 0)) {}
