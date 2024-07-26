// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "base.h"
#include "chips/SR74HCT595.h"

namespace blocks {

class CTRLBlockHALBase : public FunctionBlockHAL {
public:
  enum class ADCBus : uint8_t { CL0_GAIN = 0, CL1_GAIN = 1, CL2_GAIN = 2, ADC = 3 };

  virtual bool write_adc_bus_muxers(ADCBus channel) = 0;
};

class CTRLBlockHAL_V_1_0_1 : public CTRLBlockHALBase {
protected:
  const functions::SR74HCT595 f_adc_mux;
  const functions::TriggerFunction f_adc_mux_latch;

public:
  CTRLBlockHAL_V_1_0_1();

  bool write_adc_bus_muxers(ADCBus adc_bus) override;

private:
  float read_temperature() override;
};

class CTRLBlock : public FunctionBlock {
protected:
  CTRLBlockHALBase *hardware{};

public:
  CTRLBlock();

  using ADCBus = CTRLBlockHALBase::ADCBus;
};

} // namespace blocks