// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "lucidac.h"

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

void LUCIDAC::reset(bool keep_calibration) {
  Carrier::reset(keep_calibration);
  reset_acl_select();
  reset_adc_channels();
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
  if (front_panel and !front_panel->write_to_hardware())
    return false;
  return hardware->write_acl(acl_select) and hardware->write_adc_bus_mux(adc_channels);
}

const std::array<LUCIDAC::ACL, 8> &LUCIDAC::get_acl_select() const { return acl_select; }

void LUCIDAC::set_acl_select(const std::array<ACL, 8> &acl_select_) { acl_select = acl_select_; }

bool LUCIDAC::set_acl_select(uint8_t idx, LUCIDAC::ACL acl) {
  if (idx >= acl_select.size())
    return false;
  acl_select[idx] = acl;
  return true;
}

void LUCIDAC::reset_acl_select() { std::fill(acl_select.begin(), acl_select.end(), ACL::INTERNAL_); }

const std::array<int8_t, 8> &LUCIDAC::get_adc_channels() const { return adc_channels; }

bool LUCIDAC::set_adc_channels(const std::array<int8_t, 8> &channels) {
  // Check that all inputs are < 15
  for (auto channel : channels) {
    if (channel > 15)
      return false;
  }
  // Check that all inputs are different
  // This is actually not necessary, in principle one could split one signal to multiple ADC.
  // But for now we prohibit this, as it's more likely an error than an actual use-case.
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
  adc_channels = channels;
  return true;
}

bool LUCIDAC::set_adc_channel(uint8_t idx, int8_t adc_channel) {
  if (idx >= adc_channels.size())
    return false;
  if (adc_channel < 0)
    adc_channel = ADC_CHANNEL_DISABLED;

  // Check if channel is already routed to an ADC
  // This is not really necessary, but probably a good idea (see set_adc_channels)
  if (adc_channel != ADC_CHANNEL_DISABLED)
    for (auto other_idx = 0u; other_idx < adc_channels.size(); other_idx++)
      if (idx != other_idx and adc_channels[other_idx] == adc_channel)
        return false;

  adc_channels[idx] = adc_channel;
  return true;
}

void LUCIDAC::reset_adc_channels() {
  std::fill(adc_channels.begin(), adc_channels.end(), ADC_CHANNEL_DISABLED);
}

int LUCIDAC::get_entities(JsonObjectConst msg_in, JsonObject &msg_out) {
  auto carrier_res = this->carrier::Carrier::get_entities(msg_in, msg_out);
  if(carrier_res != 0) return carrier_res;
  if(front_panel)
    msg_out["entities"]["/FP"] = front_panel->get_entity_classifier();
  return 0;
}

bool platform::LUCIDAC::config_self_from_json(JsonObjectConst cfg) {
  bool carrier_res = this->carrier::Carrier::config_self_from_json(cfg);
  if(!carrier_res) return false;

  // TODO: This code is very quick and dirty. Use JSON custom converters to make nicer.

  if(cfg.containsKey("adc_channels")) {
    auto cfg_adc_channels = cfg["adc_channels"].as<JsonArrayConst>();
    /*if(cfg_adc_channels.size() != adc_channels.size()) {
      LOG_ALWAYS("platform::LUCIDAC::config_self_from_json: Error, given adc_channels has wrong size");
      return false;
    }*/
    for(int i=0; i<cfg_adc_channels.size() && i<adc_channels.size(); i++) {
      Serial.printf("platform::LUCIDAC::config_self_from_json adc_channels[%d] = %d\n", i, cfg_adc_channels[i]);
      adc_channels[i] = cfg_adc_channels[i];
    }
    bool written = hardware->write_adc_bus_mux(adc_channels);
    if(!written) return written;
  }

  if(cfg.containsKey("acl_select")) {
    auto cfg_acl_select = cfg["acl_select"].as<JsonArrayConst>();
    for(int i=0; i<cfg_acl_select.size() && i<acl_select.size(); i++) {
      Serial.printf("platform::LUCIDAC::config_self_from_json acl_select[%d] = %s\n", i, cfg_acl_select[i].as<const char*>());
      if(cfg_acl_select[i] == "internal") {
        acl_select[i] = platform::LUCIDAC_HAL::ACL::INTERNAL_;
      } else if(cfg_acl_select[i] == "external") {
        acl_select[i] = platform::LUCIDAC_HAL::ACL::EXTERNAL_;
      } else {
        LOG_ALWAYS("platform::LUCIDAC::config_self_from_json: Expected acl_select[i] to be either 'internal' or 'external' string")
        return false;
      }
    }
    bool written = hardware->write_acl(acl_select);
    if(!written) return written;
  }

  return true;
}

void platform::LUCIDAC::config_self_to_json(JsonObject &cfg) {
  // TODO: This code is very quick and dirty. Use JSON custom converters to make nicer.

  auto cfg_acl_channels = cfg.createNestedArray("acl_channels");
  auto cfg_acl_select   = cfg.createNestedArray("acl_select");

  for(int i=0; i<adc_channels.size(); i++)
    cfg_acl_channels.add(adc_channels[i]);

  for(int i=0; i<acl_select.size(); i++)
    cfg_acl_select.add(acl_select[i] == platform::LUCIDAC_HAL::ACL::INTERNAL_ ? "internal" : "external" );
}
