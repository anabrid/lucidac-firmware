// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "block/blocks.h"
#include "daq/daq.h"
#include "entity/entity.h"

namespace platform {

/**
 * The Lucidac class represents all (most) hardware of the Lucidac.
 * It serves as the primary entry point for on-microcontroller programming.
 **/
class Cluster : public entities::Entity {
private:
  uint8_t cluster_idx;

public:
  blocks::MBlock *m1block = nullptr;
  blocks::MBlock *m2block = nullptr;
  blocks::UBlock *ublock = nullptr;
  blocks::CBlock *cblock = nullptr;
  blocks::IBlock *iblock = nullptr;

  explicit Cluster(uint8_t cluster_idx = 0);

  // TODO: Delete copy and assignment operators
  // Cluster(Cluster const &) = delete;
  // Cluster &operator=(Cluster const &) = delete;

  entities::EntityClass get_entity_class() const final { return entities::EntityClass::CLUSTER; }

  bool init();
  std::array<blocks::FunctionBlock *, 5> get_blocks() const;

  bool calibrate(daq::BaseDAQ *daq);

  void write_to_hardware();

  /**
   * Register a route throught the cluster.
   *
   * Note that this does not immediately configure hardware but just prepares the
   * in-memory representations of the individual blocks. Use write_to_hardware() to
   * flush all blocks.
   *
   * Note that previously existing routes (also from previous IC/OP cycles) most
   * certainly still exist both in-memory representation and in the hardware. Use
   * reset() to flush both of them.
   *
   * Note that this currently yields undefined behaviour if a route on the
   * same u_out already exists.
   *
   * @arg u_in Row index [0..15] for U-Block input (corresponding to some M-Block output)
   * @arg u_out Column index [0..31] to use for routing through U/C/I block
   * @arg c_factor Coefficient value [-20,+20] for coefficient at position idx=u_out
   * @arg i_out Row index [0..15] for I-Block output (corresponding to some M-Block input)
   * @returns false in case of illegal input data, else true
   **/
  bool route(uint8_t u_in, uint8_t u_out, float c_factor, uint8_t i_out);

  /**
   * Register a route using one of the alternate signals. See `route` for additional information.
   * Some alt signals may not be arbitrarily routed.
   */
  bool route_alt_signal(uint16_t alt_signal, uint8_t u_out, float c_factor, uint8_t i_out);

  void reset(bool keep_calibration);

  std::vector<Entity *> get_child_entities() override;

  Entity *get_child_entity(const std::string &child_id) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  bool calibrate_offsets_ublock_initial(daq::BaseDAQ *daq);
};

} // namespace lucidac
