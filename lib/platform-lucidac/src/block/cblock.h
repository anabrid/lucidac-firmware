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

#include "block/base.h"
#include "bus/bus.h"
#include "chips/AD5452.h"
#include "chips/SR74HCT595.h"

namespace blocks {

/**
 * The Lucidac Coefficient Block (C-Block) is represented by this class.
 * 
 * This class provides a neat interface for setting digital potentiometers
 * without having to worry for "raw" DPT values or upscaling factors
 * (allowing the DPTs to take values between [-20,+20] instead of only
 * [-1,+1]).
 * 
 * As a Lucidac can only have a single C-Block, this is kind of a singleton.
 * Typical usage happens via the Lucidac class.
 * 
 **/
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
  std::array<const functions::AD5452, NUM_COEFF> f_coeffs;
  const functions::SR74HCT595 f_upscaling;
  const functions::TriggerFunction f_upscaling_sync;
  const functions::TriggerFunction f_upscaling_clear;

  std::array<uint16_t, NUM_COEFF> factors_{{0}};
  uint32_t upscaling_{0};

  void set_upscaling(uint8_t idx, bool enable);
  bool is_upscaled(uint8_t idx);

  void write_factors_to_hardware();
  void write_upscaling_to_hardware();

public:
  explicit CBlock(uint8_t clusterIdx);

  bus::addr_t get_block_address() override;

  /**
   * Set a particular digital potentiometer.
   * 
   * idx means the coefficient ID and is between [0,31].
   * factor is the actual analog value between [-20,20].
   * 
   * Note that calling this function only stores the value
   * in the in-memory representation of the hybrid controller.
   * The in-memory representation is flushed to hardware by
   * calling write_to_hardware().
   * 
   * @returns false in case of invalid input, true else.
   **/
  bool set_factor(uint8_t idx, float factor);
  float get_factor(uint8_t idx);

  void write_to_hardware() override;
  void reset(bool keep_calibration) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;
};

} // namespace blocks