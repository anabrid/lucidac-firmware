// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "lucidac.h"

const SPISettings platform::LUCIDAC_HAL::F_ADC_SWITCHER_PRG_SPI_SETTINGS{
    4'000'000, MSBFIRST, SPI_MODE2 /* chip expects SPI MODE0, but CLK is inverted on the way */};

LUCIDAC::LUCIDAC() : Carrier({Cluster(0)}) {
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
    if (acl[idx] == ACL::EXTERNAL) {
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
