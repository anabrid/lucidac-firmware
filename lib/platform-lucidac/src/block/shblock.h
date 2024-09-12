// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "base.h"
#include "bus/functions.h"

namespace blocks {

class SHBlock : public blocks::FunctionBlock {
public:
  // Entity hardware identifier information.
  static constexpr auto CLASS_ = entities::EntityClass::SH_BLOCK;
  static constexpr uint8_t BLOCK_IDX = bus::SH_BLOCK_IDX;

  enum class State { TRACK, TRACK_AT_IC, INJECT, GAIN_ZERO_TO_SEVEN, GAIN_EIGHT_TO_FIFTEEN };

  SHBlock();
  explicit SHBlock(bus::addr_t block_address);

  //! Sets the state the SHBlock currently is in. Call write_to_hardware() to actually apply changes to
  //! hardware
  void set_state(State state_);
  State get_state() const;

  //! Resets all internal states. Block is left in track mode afterwards. Requires write_to_hardware()
  void reset(entities::ResetAction action) override;

  //! Applies current class state to actually hardware
  [[nodiscard]] utils::status write_to_hardware() override;

  // Automatically does an track and inject sequence. This directly writes to hardware. Delays for track time
  // and inject time can be set optionally in microseconds. Block will be left in inject mode afterwards
  void compensate_hardware_offsets(uint32_t track_time = 10000, uint32_t inject_time = 5000);

  entities::EntityClass get_entity_class() const final { return CLASS_; };

  static SHBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

protected:
  State state = State::TRACK;

  // Default state after reset is inject with a potentially random inject current
  const functions::TriggerFunction set_track{bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 2)};
  const functions::TriggerFunction set_track_at_ic{bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 3)};
  const functions::TriggerFunction set_gain{bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 4)};
  const functions::TriggerFunction set_gain_channels_zero_to_seven{
      bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 5)};
  const functions::TriggerFunction set_gain_channels_eight_to_fifteen{
      bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 6)};
  const functions::TriggerFunction set_inject{bus::address_from_tuple(bus::SH_BLOCK_BADDR(0), 7)};

  utils::status config_self_from_json(JsonObjectConst cfg) override;
};

} // namespace blocks
