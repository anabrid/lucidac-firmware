// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "block/base.h"
#include "chips/SR74HCT595.h"

namespace functions {

class SyncFunction: public DataFunction {
public:
  using DataFunction::transfer16;
  using DataFunction::DataFunction;
};

}

namespace blocks {

class CTRLBlockHALBase : public FunctionBlockHAL {
public:
  enum class ADCBus : uint8_t {
    // Underlying values correspond to cluster indices
    CL0_GAIN = 0, CL1_GAIN = 1, CL2_GAIN = 2, ADC = 3
  };

  virtual bool write_adc_bus_muxers(ADCBus channel) = 0;

  virtual bool write_sync_id(uint8_t id) = 0;
};

class CTRLBlockHAL_V_1_0_2 : public CTRLBlockHALBase {
protected:
  static const SPISettings F_SYNC_SPI_SETTINGS;

  const functions::SR74HCT595 f_adc_mux;
  const functions::TriggerFunction f_adc_mux_latch;

  const functions::SyncFunction f_sync;

public:
  CTRLBlockHAL_V_1_0_2();

  bool write_adc_bus_muxers(ADCBus adc_bus) override;

  bool write_sync_id(uint8_t id) override;

//private:
//  float read_temperature() override;
};

class CTRLBlock : public FunctionBlock {
public:
  using ADCBus = CTRLBlockHALBase::ADCBus;

public:
  static constexpr auto CLASS_ = entities::EntityClass::CTRL_BLOCK;
  static constexpr uint8_t TYPE = 1;

  static CTRLBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

protected:
  ADCBus adc_bus = ADCBus::ADC;
  CTRLBlockHALBase *hardware;

public:
  explicit CTRLBlock(CTRLBlockHALBase *hardware);

  [[nodiscard]] entities::EntityClass get_entity_class() const final;

  bool init() override;

  void reset(entities::ResetAction action) override;

  [[nodiscard]] utils::status write_to_hardware() override;

  ADCBus get_adc_bus() const;
  void set_adc_bus(ADCBus adc_bus_);
  void reset_adc_bus();
  bool set_adc_bus_to_cluster_gain(uint8_t cluster_idx);

protected:
  utils::status config_self_from_json(JsonObjectConst cfg) override;
};

} // namespace blocks