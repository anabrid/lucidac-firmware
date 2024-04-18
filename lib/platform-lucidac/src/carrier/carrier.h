// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>

#include "cluster.h"
#include "chips/TMP127Q1.h"
#include "entity/entity.h"

using namespace lucidac;

namespace carrier {

class Carrier : public entities::Entity {
public:
  static std::string get_system_mac();

public:
  std::array<LUCIDAC, 1> clusters;

  Carrier();

  bool init();

  std::vector<Entity *> get_child_entities() override;

  Entity *get_child_entity(const std::string &child_id) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

  void write_to_hardware();

  // REV1 specific things
  // TODO: These are partly LUCIDAC specific things, which should be rebased on `56-refactor-...` branch.
public:
  // Module addresses
  static constexpr uint8_t CARRIER_MADDR = 5;
  static constexpr uint8_t CTRL_MADDR = 2;
  static constexpr uint8_t BACKPLANE_MADDR = 2;
  static constexpr uint8_t FRONTPLANE_MADDR = 2;
  // Function addresses
  static constexpr uint8_t METADATA_FADDR = bus::METADATA_FUNC_IDX;
  static constexpr uint8_t TEMPERATURE_FADDR = 1;
  static constexpr uint8_t ADC_PRG_FADDR = 2;
  static constexpr uint8_t ADC_RESET_8816_FADDR = 3;
  static constexpr uint8_t ADC_RESET_SR_FADDR = 4;
  static constexpr uint8_t ADC_STROBE_FADDR = 5;
  static constexpr uint8_t ACL_PRG_FADDR = 6;
  static constexpr uint8_t ACL_UPD_FADDR = 7;
  static constexpr uint8_t ACL_CRL_FADDR = 8;
protected:
  // Functions to configure ACL signal muxer
  const functions::SR74HCT595 f_acl_prg;
  const functions::TriggerFunction f_acl_upd;
  const functions::TriggerFunction f_acl_clr;
  // Functions to configure ADC signal switching matrix
  // TODO: Replace by separate function or abstraction that does not allow connecting two inputs to one output
  // SPI settings are unfortunately different from IBlock
  static const SPISettings F_ADC_SWITCHER_PRG_SPI_SETTINGS;
  const functions::ICommandRegisterFunction f_adc_switcher_prg;
  const functions::TriggerFunction f_adc_switcher_sync;
  const functions::TriggerFunction f_adc_switcher_sr_reset;
  const functions::TriggerFunction f_adc_switcher_matrix_reset;
  // Temperature function
  const functions::TMP127Q1 f_temperature;
};

} // namespace carrier