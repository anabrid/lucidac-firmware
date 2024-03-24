// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include "carrier.h"
#include "logging.h"
#include "mac_utils.h"

std::string carrier::Carrier::get_system_mac() {
  return ::get_system_mac();
}

carrier::Carrier::Carrier() : clusters({lucidac::LUCIDAC(0)}) {}

bool carrier::Carrier::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  entity_id = get_system_mac();
  LOG(ANABRID_DEBUG_INIT, entity_id.c_str());
  if (entity_id.empty())
    return false;
  for (auto &cluster : clusters) {
    if (!cluster.init())
      return false;
  }
  return true;
}

bool carrier::Carrier::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  // Carrier has no own configuration parameters currently
  // TODO: Have an option to fail on unexpected configuration
  return true;
}

entities::Entity *carrier::Carrier::get_child_entity(const std::string &child_id) {
  // TODO: Use additional arguments of stoul to check if only part of the string was used
  auto cluster_idx = std::stoul(child_id);
  if (cluster_idx < 0 or clusters.size() < cluster_idx)
    return nullptr;
  return &clusters[cluster_idx];
}

std::vector<entities::Entity *> carrier::Carrier::get_child_entities() {
#ifdef ANABRID_DEBUG_ENTITY
  Serial.println(__PRETTY_FUNCTION__);
#endif
  std::vector<entities::Entity *> children;
  for (auto &cluster : clusters) {
    children.push_back(&cluster);
  }
  return children;
}

void carrier::Carrier::write_to_hardware() {
  for (auto &cluster : clusters) {
    cluster.write_to_hardware();
  }
}