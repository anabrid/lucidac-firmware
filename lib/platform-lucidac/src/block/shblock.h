// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "base.h"
#include "bus/functions.h"

namespace functions {}

namespace platform {
class Cluster;
}

namespace blocks {

class SHBlock : public blocks::FunctionBlock {
public:
  // Entity hardware identifier information.
  static constexpr auto CLASS_ = entities::EntityClass::SH_BLOCK;

  entities::EntityClass get_entity_class() const final { return CLASS_; };

  static SHBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

  enum class State {
    TRACK, GAIN, INJECT
  };

  enum class GainChannels {
    ZERO_TO_SEVEN, EIGHT_TO_FIFTEEN
  };

protected:
  State state = State::INJECT;

  // Default state after reset is inject with a potentially random inject current
  const functions::TriggerFunction set_track{bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 2)};
  const functions::TriggerFunction set_track_at_ic{bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 3)};
  const functions::TriggerFunction set_gain{bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 4)};
  const functions::TriggerFunction set_gain_channels_zero_to_seven{
      bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 5)};
  const functions::TriggerFunction set_gain_channels_eight_to_fifteen{
      bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 6)};
  const functions::TriggerFunction set_inject{bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 7)};

public:
  static constexpr uint8_t BLOCK_IDX = bus::SH_BLOCK_IDX;

  SHBlock();
  explicit SHBlock(bus::addr_t block_address);

  [[nodiscard]] bool write_to_hardware() override;

  void compensate_hardware_offsets();

  void to_gain(GainChannels channels);

protected:
  bool config_self_from_json(JsonObjectConst cfg) override;

  friend class ::platform::Cluster;
};

} // namespace blocks
