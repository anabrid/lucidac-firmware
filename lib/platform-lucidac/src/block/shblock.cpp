// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "shblock.h"

bool blocks::SHBlock::config_self_from_json(JsonObjectConst cfg) { return false; }

bus::addr_t blocks::SHBlock::get_block_address() { return bus::SH_BLOCK_BADDR(cluster_idx); }

bool blocks::SHBlock::write_to_hardware() { return true; }

blocks::SHBlock::SHBlock(const uint8_t clusterIdx) : FunctionBlock("SH", clusterIdx) {}
