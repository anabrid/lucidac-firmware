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

#include <array>
#include <cstdint>

#include "base_block.h"
#include "local_bus.h"
#include "AD5452.h"
#include "SR74HCT595.h"

namespace blocks {

class CBlock : public FunctionBlock {
public:
  static constexpr uint8_t BLOCK_IDX = bus::C_BLOCK_IDX;

  static constexpr uint8_t COEFF_BASE_FUNC_IDX = 1;
  static constexpr uint8_t SCALE_SWITCHER = 33;
  static constexpr uint8_t SCALE_SWITCHER_SYNC = 34;
  static constexpr uint8_t SCALE_SWITCHER_CLEAR = 35;

  static constexpr uint8_t NUM_COEFF = 32;
  static constexpr float MAX_REAL_FACTOR = 2.0f;
  static constexpr float MIN_REAL_FACTOR = -2.0f;
  // TODO: Upscaling is not *exactly* 10
  static constexpr float UPSCALING = 10.055f;
  static constexpr float MAX_FACTOR = MAX_REAL_FACTOR*UPSCALING;
  static constexpr float MIN_FACTOR = MIN_REAL_FACTOR*UPSCALING;

protected:
  std::array<::functions::AD5452, NUM_COEFF> f_coeffs;
  ::functions::SR74HCT595 f_upscaling;
  ::functions::TriggerFunction f_upscaling_sync;
  ::functions::TriggerFunction f_upscaling_clear;

  std::array<uint16_t, NUM_COEFF> factors_{0};
  uint32_t upscaling_{0};

  void set_upscaling(uint8_t idx, bool enable);

  void write_factors_to_hardware();
  void write_upscaling_to_hardware();

public:
  explicit CBlock(uint8_t clusterIdx);

  bus::addr_t get_block_address() override;

  bool set_factor(uint8_t idx, float factor);

  void write_to_hardware() override;
};

} // namespace blocks