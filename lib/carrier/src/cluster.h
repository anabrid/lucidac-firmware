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

#pragma once

#include "blocks.h"
#include "entity.h"
#include "daq.h"

namespace lucidac {

/**
 * The Lucidac class represents all (most) hardware of the Lucidac.
 * It serves as the primary entry point for on-microcontroller programming.
 **/
class LUCIDAC : public entities::Entity {
public:
  blocks::MBlock *m1block = nullptr;
  blocks::MBlock *m2block = nullptr;
  blocks::UBlock *ublock = nullptr;
  blocks::CBlock *cblock = nullptr;
  blocks::IBlock *iblock = nullptr;

  explicit LUCIDAC(uint8_t cluster_idx = 0);

  bool init();
  auto get_blocks();

  bool calibrate(daq::BaseDAQ *daq);

  void write_to_hardware();

  /**
   * Register a route throught the lucidac.
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

  Entity *get_child_entity(const std::string &child_id) override;
  bool config_from_json(JsonObjectConst cfg) override;

protected:
  bool calibrate_offsets_ublock_initial(daq::BaseDAQ *daq);
};

} // namespace lucidac