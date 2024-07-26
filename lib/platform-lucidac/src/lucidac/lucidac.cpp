// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "lucidac.h"
#include "entity/entity.h"

const SPISettings platform::LUCIDAC_HAL::F_ADC_SWITCHER_PRG_SPI_SETTINGS{
    4'000'000, MSBFIRST, SPI_MODE2 /* chip expects SPI MODE0, but CLK is inverted on the way */};

LUCIDAC::LUCIDAC() : Carrier({Cluster(0)}), hardware(new LUCIDAC_HAL()) {
  // Other constructor stuff
}

LUCIDAC_HAL::LUCIDAC_HAL()
    : f_acl_prg(bus::address_from_tuple(CARRIER_MADDR, ACL_PRG_FADDR), true),
      f_acl_upd(bus::address_from_tuple(CARRIER_MADDR, ACL_UPD_FADDR)),
      f_acl_clr(bus::address_from_tuple(CARRIER_MADDR, ACL_CRL_FADDR)),
      f_adc_switcher_prg(bus::address_from_tuple(CARRIER_MADDR, ADC_PRG_FADDR),
                         F_ADC_SWITCHER_PRG_SPI_SETTINGS),
      f_adc_switcher_sync(bus::address_from_tuple(CARRIER_MADDR, ADC_STROBE_FADDR)),
      f_adc_switcher_sr_reset(bus::address_from_tuple(CARRIER_MADDR, ADC_RESET_SR_FADDR)),
      f_adc_switcher_matrix_reset(bus::address_from_tuple(CARRIER_MADDR, ADC_RESET_8816_FADDR)),
      f_temperature(bus::address_from_tuple(CARRIER_MADDR, TEMPERATURE_FADDR)) {}

bool LUCIDAC_HAL::write_acl(std::array<LUCIDAC_HAL::ACL, 8> acl) {
  uint8_t sr = 0;
  for (size_t idx = 0; idx < acl.size(); idx++) {
    if (acl[idx] == ACL::EXTERNAL_) {
      sr |= 1 << idx;
    }
  }
  if (!f_acl_prg.transfer8(sr))
    return false;
  f_acl_upd.trigger();
  return true;
}

void LUCIDAC_HAL::reset_acl() {
  f_acl_clr.trigger();
  f_acl_upd.trigger();
}

bool LUCIDAC_HAL::write_adc_bus_mux(std::array<int8_t, 8> channels) {
  // Check that all inputs are < 15
  for (auto channel : channels) {
    if (channel > 15)
      return false;
  }
  // Check that all inputs are different
  for (auto &channel : channels) {
    if (channel < 0)
      continue;
    for (auto &other_channel : channels) {
      if (std::addressof(channel) == std::addressof(other_channel))
        continue;
      if (channel == other_channel)
        return false;
    }
  }

  // Reset previous connections
  // It's easier to do a full reset then to remember all previous connections
  reset_adc_bus_mux();

  // Write data to chip
  for (uint8_t output_idx = 0; output_idx < channels.size(); output_idx++) {
    if (channels[output_idx] >= 0) {
      auto cmd = decltype(f_adc_switcher_prg)::chip_cmd_word(channels[output_idx], output_idx);
      if (!f_adc_switcher_prg.transfer8(cmd))
        return false;
      f_adc_switcher_sync.trigger();
    }
  }
  return true;
}

void LUCIDAC_HAL::reset_adc_bus_mux() { f_adc_switcher_matrix_reset.trigger(); }

bool LUCIDAC::init() {
  if (!Carrier::init())
    return false;

  LOG(ANABRID_DEBUG_INIT, "Detecting front panel...");
  if (!front_panel) {
    front_panel = entities::detect<lucidac::FrontPanel>(bus::address_from_tuple(2, 0));
    if (!front_panel) {
      LOG(ANABRID_DEBUG_INIT, "Warning: Front panel is missing or unknown.");
    }
  }

  LOG(ANABRID_DEBUG_INIT, "Initialising detected front panel...");
  if (front_panel && !front_panel->init())
    return false;
  LOG(ANABRID_DEBUG_INIT, "Front panel initialized.");

  return true;
}

std::vector<entities::Entity *> LUCIDAC::get_child_entities() {
  auto entities = this->carrier::Carrier::get_child_entities();
  if (front_panel)
    entities.push_back(front_panel);
  return entities;
}

entities::Entity *LUCIDAC::get_child_entity(const std::string &child_id) {
  if (child_id == "FP")
    return front_panel;
  return this->carrier::Carrier::get_child_entity(child_id);
}

bool LUCIDAC::write_to_hardware() {
  if (!this->carrier::Carrier::write_to_hardware())
    return false;
  if (front_panel)
    return front_panel->write_to_hardware();
  return true;
}
