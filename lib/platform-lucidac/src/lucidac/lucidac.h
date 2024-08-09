// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "block/blocks.h"
#include "carrier/carrier.h"
#include "front_panel.h"

namespace platform {

class LUCIDAC_HAL : public carrier::Carrier_HAL {
public:
  // Module addresses
  static constexpr uint8_t CARRIER_MADDR = 5;
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

public:
  LUCIDAC_HAL();

  enum class ACL { INTERNAL_ = 0, EXTERNAL_ = 1 };

  //! Write bits to ACL shift register, from I-block input 24 (first element) to 31 (last element)
  bool write_acl(std::array<ACL, 8> acl);

  void reset_acl();

  //! Write channel selection to ADC bus muxer.
  //! Each element in channels selects the input index for the n-th ADC bus output (-1 disables).
  bool write_adc_bus_mux(std::array<int8_t, 8> channels) override;

  void reset_adc_bus_mux() override;
};

class LUCIDAC : public carrier::Carrier {
public:
  using ACL = LUCIDAC_HAL::ACL;

protected:
  LUCIDAC_HAL *hardware{};

  std::array<ACL, 8> acl_select{ACL::INTERNAL_, ACL::INTERNAL_, ACL::INTERNAL_, ACL::INTERNAL_,
                                ACL::INTERNAL_, ACL::INTERNAL_, ACL::INTERNAL_, ACL::INTERNAL_};

public:
  LUCIDACFrontPanel *front_panel = nullptr;

  explicit LUCIDAC(LUCIDAC_HAL* hardware);
  LUCIDAC();

  bool init() override;

  void reset(bool keep_calibration) override;

  std::vector<Entity *> get_child_entities() override;

  Entity *get_child_entity(const std::string &child_id) override;

  [[nodiscard]] bool write_to_hardware() override;

  [[nodiscard]] const std::array<ACL, 8> &get_acl_select() const;
  void set_acl_select(const std::array<ACL, 8> &acl_select_);
  [[nodiscard]] bool set_acl_select(uint8_t idx, ACL acl);
  void reset_acl_select();

};

} // namespace platform
