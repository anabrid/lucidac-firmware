// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "lucidac.h"
#include "utils/mac.h"

const SPISettings platform::LUCIDAC_HAL::F_ADC_SWITCHER_PRG_SPI_SETTINGS{
    4'000'000, MSBFIRST, SPI_MODE2 /* chip expects SPI MODE0, but CLK is inverted on the way */};

FLASHMEM LUCIDAC::LUCIDAC(LUCIDAC_HAL *hardware) : Carrier({Cluster(0)}, hardware), hardware(hardware) {}

FLASHMEM LUCIDAC::LUCIDAC() : LUCIDAC(new LUCIDAC_HAL()) {}

FLASHMEM LUCIDAC_HAL::LUCIDAC_HAL()
    : f_acl_prg(bus::address_from_tuple(CARRIER_MADDR, ACL_PRG_FADDR), true),
      f_acl_upd(bus::address_from_tuple(CARRIER_MADDR, ACL_UPD_FADDR)),
      f_acl_clr(bus::address_from_tuple(CARRIER_MADDR, ACL_CRL_FADDR)),
      f_adc_switcher_prg(bus::address_from_tuple(CARRIER_MADDR, ADC_PRG_FADDR),
                         F_ADC_SWITCHER_PRG_SPI_SETTINGS),
      f_adc_switcher_sync(bus::address_from_tuple(CARRIER_MADDR, ADC_STROBE_FADDR)),
      f_adc_switcher_sr_reset(bus::address_from_tuple(CARRIER_MADDR, ADC_RESET_SR_FADDR)),
      f_adc_switcher_matrix_reset(bus::address_from_tuple(CARRIER_MADDR, ADC_RESET_8816_FADDR)),
      f_temperature(bus::address_from_tuple(CARRIER_MADDR, TEMPERATURE_FADDR)) {}

FLASHMEM bool LUCIDAC_HAL::write_acl(std::array<LUCIDAC_HAL::ACL, 8> acl) {
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

FLASHMEM void LUCIDAC_HAL::reset_acl() {
  f_acl_clr.trigger();
  f_acl_upd.trigger();
}

FLASHMEM bool LUCIDAC_HAL::write_adc_bus_mux(std::array<int8_t, 8> channels) {
  // Reset previous connections
  // It's easier to do a full reset then to remember all previous connections
  reset_adc_bus_mux();
  delayNanoseconds(420);

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

FLASHMEM void LUCIDAC_HAL::reset_adc_bus_mux() { f_adc_switcher_matrix_reset.trigger(); }

FLASHMEM bool LUCIDAC::init() {
  if (!Carrier::init())
    return false;

  LOG(ANABRID_DEBUG_INIT, "Detecting front panel...");
  if (!front_panel) {
    front_panel = entities::detect<LUCIDACFrontPanel>(bus::address_from_tuple(2, 0));
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

FLASHMEM void LUCIDAC::reset(bool keep_calibration) {
  Carrier::reset(keep_calibration);
  if (front_panel)
    front_panel->reset();
  reset_acl_select();
  reset_adc_channels();
}

FLASHMEM std::vector<entities::Entity *> LUCIDAC::get_child_entities() {
  auto entities = this->carrier::Carrier::get_child_entities();
  if (front_panel)
    entities.push_back(front_panel);
  return entities;
}

FLASHMEM entities::Entity *LUCIDAC::get_child_entity(const std::string &child_id) {
  if (child_id == "FP")
    return front_panel;
  return this->carrier::Carrier::get_child_entity(child_id);
}

FLASHMEM int LUCIDAC::write_to_hardware() {
  int carrier_return = Carrier::write_to_hardware();
  if (carrier_return != 1)
    return carrier_return;
  if (front_panel and !front_panel->write_to_hardware())
    return -4;
  if (!hardware->write_acl(acl_select))
    return -5;
  return true;
}

FLASHMEM const std::array<LUCIDAC::ACL, 8> &LUCIDAC::get_acl_select() const { return acl_select; }

FLASHMEM void LUCIDAC::set_acl_select(const std::array<ACL, 8> &acl_select_) { acl_select = acl_select_; }

FLASHMEM bool LUCIDAC::set_acl_select(uint8_t idx, LUCIDAC::ACL acl) {
  if (idx >= acl_select.size())
    return false;
  acl_select[idx] = acl;
  return true;
}

FLASHMEM void LUCIDAC::reset_acl_select() { std::fill(acl_select.begin(), acl_select.end(), ACL::INTERNAL_); }

FLASHMEM bool LUCIDAC::calibrate_routes(daq::BaseDAQ *daq_) {
  auto old_acl_selection = get_acl_select();
  reset_acl_select();
  if (!hardware->write_acl(acl_select))
    return false;

  if (!Carrier::calibrate_routes(daq_))
    return false;

  set_acl_select(old_acl_selection);
  return hardware->write_acl(acl_select);
}

FLASHMEM int LUCIDAC::get_entities(JsonObjectConst msg_in, JsonObject &msg_out) {
  auto carrier_res = this->carrier::Carrier::get_entities(msg_in, msg_out);
  if (carrier_res != 0)
    return carrier_res;
  if (front_panel) {
    auto fp_obj = msg_out["entities"]["/FP"] = front_panel->get_entity_classifier();
    fp_obj["eui"] = utils::toString(front_panel->get_entity_eui());
  }
  return 0;
}

FLASHMEM utils::status platform::LUCIDAC::config_self_from_json(JsonObjectConst cfg) {
  auto res = this->carrier::Carrier::config_self_from_json(cfg);
  if (!res)
    return res;

  for (auto cfgItr = cfg.begin(); cfgItr != cfg.end(); ++cfgItr) {
    if (cfgItr->key() == "adc_channels") {
      auto res = _config_adc_from_json(cfgItr->value());
      if (!res)
        return utils::status(530, "Could not configure ADCs from configuration.");
    } else if (cfgItr->key() == "acl_select") {
      auto res = _config_acl_from_json(cfgItr->value());
      if (!res)
        return utils::status(531, "Could not configure ACLs from configuration.");
    } else if (strlen(cfgItr->key().c_str()) >= 1 && cfgItr->key().c_str()[0] == '/') {
      // An sub-entity is refered to. This is already handled by
      // config_from_json() towards the children so it is ignored here.
    } else {
      return utils::status(532, "LUCIDAC: Unknown configuration key");
    }
  }

  return utils::status::success();
}

FLASHMEM bool platform::LUCIDAC::_config_adc_from_json(const JsonVariantConst &cfg) {
  if (!cfg.is<JsonArrayConst>())
    return false;

  auto cfg_adc_channels = cfg.as<JsonArrayConst>();
  for (size_t i = 0; i < cfg_adc_channels.size() && i < adc_channels.size(); i++) {
    adc_channels[i] = cfg_adc_channels[i].isNull() ? ADC_CHANNEL_DISABLED : cfg_adc_channels[i];
  }
  return hardware->write_adc_bus_mux(adc_channels);
}

FLASHMEM bool platform::LUCIDAC::_config_acl_from_json(const JsonVariantConst &cfg) {
  if (!cfg.is<JsonArrayConst>())
    return false;

  auto cfg_acl_select = cfg.as<JsonArrayConst>();
  for (size_t i = 0; i < cfg_acl_select.size() && i < acl_select.size(); i++) {
    if (cfg_acl_select[i] == "internal") {
      acl_select[i] = platform::LUCIDAC_HAL::ACL::INTERNAL_;
    } else if (cfg_acl_select[i] == "external") {
      acl_select[i] = platform::LUCIDAC_HAL::ACL::EXTERNAL_;
    } else {
      LOG_ALWAYS("platform::LUCIDAC::config_self_from_json: Expected acl_select[i] to be either 'internal' "
                 "or 'external' string")
      return false;
    }
  }
  return hardware->write_acl(acl_select);
}

FLASHMEM void platform::LUCIDAC::config_self_to_json(JsonObject &cfg) {
  // TODO: This code is very quick and dirty. Use JSON custom converters to make nicer.

  auto cfg_adc_channels = cfg.createNestedArray("adc_channels");
  auto cfg_acl_select = cfg.createNestedArray("acl_select");

  for (size_t i = 0; i < adc_channels.size(); i++) {
    if (adc_channels[i] == ADC_CHANNEL_DISABLED)
      cfg_adc_channels.add(nullptr); // json "none"
    else
      cfg_adc_channels.add(adc_channels[i]);
  }

  for (size_t i = 0; i < acl_select.size(); i++)
    cfg_acl_select.add(acl_select[i] == platform::LUCIDAC_HAL::ACL::INTERNAL_ ? "internal" : "external");
}
