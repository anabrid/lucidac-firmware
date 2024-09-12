// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "ctrlblock.h"

const SPISettings blocks::CTRLBlockHAL_V_1_0_2::F_SYNC_SPI_SETTINGS{1'000'000, MSBFIRST, SPI_MODE0};

FLASHMEM blocks::CTRLBlockHAL_V_1_0_2::CTRLBlockHAL_V_1_0_2()
    : f_adc_mux(bus::address_from_tuple(1, 2), true), f_adc_mux_latch(bus::address_from_tuple(1, 3)),
      f_sync(bus::address_from_tuple(1, 4), F_SYNC_SPI_SETTINGS) {}

FLASHMEM bool blocks::CTRLBlockHAL_V_1_0_2::write_adc_bus_muxers(ADCBus adc_bus) {
  if (!f_adc_mux.transfer8(static_cast<uint8_t>(adc_bus)))
    return false;
  f_adc_mux_latch.trigger();
  return true;
}

// float blocks::CTRLBlockHAL_V_1_0_2::read_temperature() { return 0; }

FLASHMEM bool blocks::CTRLBlockHAL_V_1_0_2::write_sync_id(uint8_t id) {
  // The very naive encoding proposed in
  // https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/issues/2
  // uses only 6 bits of the 16 bits to ensure a "unique stream".
  // IDs is thus limited to 0-63 currently
  if (id > 0b00'111111)
    return false;
  f_sync.transfer16((id << 1) | 0b1000'0001);
  return true;
}

FLASHMEM blocks::CTRLBlock::CTRLBlock(CTRLBlockHALBase *hardware)
    : FunctionBlock("CTRL", bus::address_from_tuple(1, 0)), hardware(hardware) {}

FLASHMEM blocks::CTRLBlock *blocks::CTRLBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                             bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_ or classifier.type != TYPE)
    return nullptr;

  // Currently, there are no variants.
  // TODO: After adapting classifier to (major, minor, patch) version,
  //       fail on v1.0.1, which has missing hardware components and should not be used.
  return new CTRLBlock(new CTRLBlockHAL_V_1_0_2());
}

FLASHMEM entities::EntityClass blocks::CTRLBlock::get_entity_class() const { return CLASS_; }

FLASHMEM utils::status blocks::CTRLBlock::config_self_from_json(JsonObjectConst cfg) { return utils::status::success(); }

FLASHMEM utils::status blocks::CTRLBlock::write_to_hardware() { return utils::status(hardware->write_adc_bus_muxers(adc_bus)); }

FLASHMEM bool blocks::CTRLBlock::init() {
  // Hardware defaults are not very good, e.g. adc bus is by default on CL0_GAIN.
  // That's why we write more sane defaults on init.
  return utils::status(FunctionBlock::init() and write_to_hardware());
}

blocks::CTRLBlock::ADCBus blocks::CTRLBlock::get_adc_bus() const { return adc_bus; }

FLASHMEM void blocks::CTRLBlock::set_adc_bus(blocks::CTRLBlock::ADCBus adc_bus_) { adc_bus = adc_bus_; }

FLASHMEM void blocks::CTRLBlock::reset_adc_bus() { adc_bus = ADCBus::ADC; }

FLASHMEM bool blocks::CTRLBlock::set_adc_bus_to_cluster_gain(uint8_t cluster_idx) {
  if (cluster_idx >= 3)
    return false;
  set_adc_bus(static_cast<ADCBus>(cluster_idx));
  return true;
}

FLASHMEM void blocks::CTRLBlock::reset(entities::ResetAction action) {
  FunctionBlock::reset(action);
  reset_adc_bus();
}
