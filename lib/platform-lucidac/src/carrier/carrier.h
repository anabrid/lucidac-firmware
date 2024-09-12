// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>
#include <string>

#include "block/ctrlblock.h"
#include "chips/TMP127Q1.h"
#include "cluster.h"
#include "daq/base.h"
#include "entity/entity.h"
#include "utils/error.h"

using namespace platform;

namespace carrier {

class Carrier_HAL {
public:
  virtual bool write_adc_bus_mux(std::array<int8_t, 8> channels) = 0;
  virtual void reset_adc_bus_mux() = 0;
};

/**
 * \brief Top-level hierarchy controlled by a single microcontroller
 *
 * A Carrier (also refered to as module holder, base board or mother board) contains
 * one microcontroller and multiple clusters, where the clusers hold the actual analog
 * computing hardware.
 *
 * \ingroup Singletons
 **/
class Carrier : public entities::Entity {
public:
  static constexpr int8_t ADC_CHANNEL_DISABLED = -1;

protected:
  Carrier_HAL *hardware;

  std::array<int8_t, 8> adc_channels{ADC_CHANNEL_DISABLED, ADC_CHANNEL_DISABLED, ADC_CHANNEL_DISABLED,
                                     ADC_CHANNEL_DISABLED, ADC_CHANNEL_DISABLED, ADC_CHANNEL_DISABLED,
                                     ADC_CHANNEL_DISABLED, ADC_CHANNEL_DISABLED};

public:
  std::vector<Cluster> clusters;
  blocks::CTRLBlock *ctrl_block = nullptr;

  explicit Carrier(std::vector<Cluster> clusters, Carrier_HAL *hardware);

  entities::EntityClass get_entity_class() const final;

  std::array<uint8_t, 8> get_entity_eui() const final;

  virtual bool init();

  virtual bool calibrate_offset();
  virtual bool calibrate_routes_in_cluster(Cluster &cluster, daq::BaseDAQ *daq_);
  virtual bool calibrate_routes(daq::BaseDAQ *daq_);
  virtual bool calibrate_mblock(Cluster &cluster, blocks::MBlock &mblock, daq::BaseDAQ *daq_);
  virtual bool calibrate_m_blocks(daq::BaseDAQ *daq_);

  virtual void reset(entities::ResetAction action);

  std::vector<Entity *> get_child_entities() override;

  Entity *get_child_entity(const std::string &child_id) override;

  utils::status config_self_from_json(JsonObjectConst cfg) override;

  [[nodiscard]] utils::status write_to_hardware() override;

  [[nodiscard]] const std::array<int8_t, 8> &get_adc_channels() const;
  [[nodiscard]] bool set_adc_channels(const std::array<int8_t, 8> &channels);
  [[nodiscard]] bool set_adc_channel(uint8_t idx, int8_t adc_channel);
  void reset_adc_channels();

  ///@addtogroup User-Functions
  ///@{
  utils::status user_set_extended_config(JsonObjectConst msg_in, JsonObject &msg_out);
  ///@}

public:
  // Module addresses
  static constexpr uint8_t CARRIER_MADDR = 5;
};

} // namespace carrier
