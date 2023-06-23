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
#include "daq.h"

namespace lucidac {

class LUCIDAC {
public:
  blocks::MBlock* m1block = nullptr;
  blocks::MBlock* m2block = nullptr;
  blocks::UBlock* ublock = nullptr;
  blocks::CBlock* cblock = nullptr;
  blocks::IBlock* iblock = nullptr;

  LUCIDAC();

  bool init();
  auto get_blocks();

  bool calibrate(daq::BaseDAQ* daq);

  void write_to_hardware();

  bool route(uint8_t u_in, uint8_t u_out, float c_factor, uint8_t i_out);

  void reset(bool keep_calibration);

protected:
  bool calibrate_offsets_ublock_initial(daq::BaseDAQ* daq);
};

}