// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "shblock.h"

blocks::SHBlock::SHBlock(const bus::addr_t block_address) : FunctionBlock("SH", block_address) {}

blocks::SHBlock::SHBlock() : SHBlock(bus::BLOCK_BADDR(0, bus::SH_BLOCK_IDX)) {}

bool blocks::SHBlock::config_self_from_json(JsonObjectConst cfg) { return false; }

bool blocks::SHBlock::write_to_hardware() { return true; }

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

void blocks::SHBlock::compensate_hardware_offsets() {
  set_track.trigger();
  delay(100);
  set_inject.trigger();
  delay(100);
}

void blocks::SHBlock::to_gain(blocks::SHBlock::GainChannels channels) {
  state = State::GAIN;
  set_gain.trigger();
  switch(channels) {
  case GainChannels::ZERO_TO_SEVEN:
    set_gain_channels_zero_to_seven.trigger();
    break;
  case GainChannels::EIGHT_TO_FIFTEEN:
    set_gain_channels_eight_to_fifteen.trigger();
    break;
  }
}
