// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "carrier.h"

entities::EntityClass carrier::Carrier::get_entity_class() const { return entities::EntityClass::CARRIER; }

const SPISettings carrier::Carrier::F_ADC_SWITCHER_PRG_SPI_SETTINGS{
    4'000'000, MSBFIRST, SPI_MODE2 /* chip expects SPI MODE0, but CLK is inverted on the way */};

carrier::Carrier::Carrier(std::vector<Cluster> clusters)
    : Entity(""), clusters(std::move(clusters)),
      f_acl_prg(bus::address_from_tuple(CARRIER_MADDR, ACL_PRG_FADDR), true),
      f_acl_upd(bus::address_from_tuple(CARRIER_MADDR, ACL_UPD_FADDR)),
      f_acl_clr(bus::address_from_tuple(CARRIER_MADDR, ACL_CRL_FADDR)),
      f_adc_switcher_prg(bus::address_from_tuple(CARRIER_MADDR, ADC_PRG_FADDR),
                         F_ADC_SWITCHER_PRG_SPI_SETTINGS),
      f_adc_switcher_sync(bus::address_from_tuple(CARRIER_MADDR, ADC_STROBE_FADDR)),
      f_adc_switcher_sr_reset(bus::address_from_tuple(CARRIER_MADDR, ADC_RESET_SR_FADDR)),
      f_adc_switcher_matrix_reset(bus::address_from_tuple(CARRIER_MADDR, ADC_RESET_8816_FADDR)),
      f_temperature(bus::address_from_tuple(CARRIER_MADDR, TEMPERATURE_FADDR)) {}

bool carrier::Carrier::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  entity_id = get_system_mac();
  LOG(ANABRID_DEBUG_INIT, entity_id.c_str());
  if (entity_id.empty())
    return false;
  for (auto &cluster : clusters) {
    if (!cluster.init())
      return false;
  }
  return true;
}

std::vector<entities::Entity *> carrier::Carrier::get_child_entities() {
  std::vector<entities::Entity *> children;
  for (auto &cluster : clusters) {
    children.push_back(&cluster);
  }
  return children;
}

entities::Entity *carrier::Carrier::get_child_entity(const std::string &child_id) {
  auto cluster_idx = std::stoul(child_id);
  if (cluster_idx < 0 or clusters.size() < cluster_idx)
    return nullptr;
  return &clusters[cluster_idx];
}

bool carrier::Carrier::config_self_from_json(JsonObjectConst cfg) {
  // TODO: Have an option to fail on unexpected configuration
  return true;
}

void carrier::Carrier::write_to_hardware() {
  for (auto &cluster : clusters) {
    cluster.write_to_hardware();
  }
}
