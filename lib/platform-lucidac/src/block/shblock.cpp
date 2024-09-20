// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "shblock.h"

FLASHMEM blocks::SHBlock::SHBlock(const bus::addr_t block_address) : FunctionBlock("SH", block_address) {}

FLASHMEM blocks::SHBlock::SHBlock() : SHBlock(bus::BLOCK_BADDR(0, bus::SH_BLOCK_IDX)) {}

FLASHMEM void blocks::SHBlock::set_state(State state_) { state = state_; }

FLASHMEM blocks::SHBlock::State blocks::SHBlock::get_state() const { return state; }

FLASHMEM void blocks::SHBlock::reset(entities::ResetAction action) {
  if (action.has(entities::ResetAction::CIRCUIT_RESET))
    state = State::INJECT;
}

FLASHMEM utils::status blocks::SHBlock::write_to_hardware() {
  if (state == State::TRACK)
    set_track.trigger();
  else if (state == State::TRACK_AT_IC)
    set_track_at_ic.trigger();
  else if (state == State::INJECT)
    set_inject.trigger();
  else if (state == State::GAIN_ZERO_TO_SEVEN) {
    set_gain.trigger();
    set_gain_channels_zero_to_seven.trigger();
  } else if (state == State::GAIN_EIGHT_TO_FIFTEEN) {
    set_gain.trigger();
    set_gain_channels_eight_to_fifteen.trigger();
  }

  return utils::status::success();
}

FLASHMEM void blocks::SHBlock::compensate_hardware_offsets(uint32_t track_time, uint32_t inject_time) {
  set_track.trigger();
  delayMicroseconds(track_time);
  set_inject.trigger();
  delayMicroseconds(inject_time);
  state = State::INJECT;
}

FLASHMEM utils::status blocks::SHBlock::config_self_from_json(JsonObjectConst cfg) {
  // return utils::status("SHBlock does not accept configuration");

  // FOR TESTING
  if (cfg.containsKey("state")) {
    if (cfg["state"] == "TRACK")
      set_state(blocks::SHBlock::State::TRACK);
    else if (cfg["state"] == "TRACK_AT_IC")
      set_state(blocks::SHBlock::State::TRACK_AT_IC);
    else if (cfg["state"] == "INJECT")
      set_state(blocks::SHBlock::State::INJECT);
    else if (cfg["state"] == "GAIN_ZERO_TO_SEVEN")
      set_state(blocks::SHBlock::State::GAIN_ZERO_TO_SEVEN);
    else if (cfg["state"] == "GAIN_EIGHT_TO_FIFTEEN")
      set_state(blocks::SHBlock::State::GAIN_EIGHT_TO_FIFTEEN);
    else
      return utils::status("Unknown target state for SH Block");
  }
  return utils::status(0);
}

FLASHMEM
blocks::SHBlock *blocks::SHBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                         bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_)
    return nullptr;

  // Currently, there are no different variants
  if (classifier.variant != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  if (classifier.version < entities::Version(0, 1))
    return nullptr;
  if (classifier.version < entities::Version(0, 2))
    return new SHBlock(block_address);
  return nullptr;
}
