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

namespace blocks {

namespace functions {

class ICommandRegisterFunction : public bus::_old_DataFunction {
public:
  //! Data is [8bit 4_* SR][8bit 3_* SR][8bit 2_* SR][8bit 1_* SR]
  //!      -> [0-15 X 0-07][16-31 X 0-7][0-15 X 8-15][16-31 X 8-15]   [ input X output ] matrix
  //! Each is [DATA Y2 Y1 Y0 X3 X2 X1 X0]
  uint32_t data = 0;

  using bus::_old_DataFunction::_old_DataFunction;
  explicit ICommandRegisterFunction(bus::addr_t address);

  static uint8_t chip_cmd_word(uint8_t chip_input_idx, uint8_t chip_output_idx, bool connect = true);

  void write_to_hardware() const;
};

} // namespace functions

class IBlock : public FunctionBlock {
public:
  static constexpr uint8_t BLOCK_IDX = bus::I_BLOCK_IDX;
  static constexpr uint8_t IMATRIX_COMMAND_SR_FUNC_IDX = 1;
  static constexpr uint8_t IMATRIX_RESET_FUNC_IDX = 2;
  static constexpr uint8_t IMATRIX_COMMAND_SR_RESET_FUNC_IDX = 3;
  static constexpr uint8_t IMATRIX_SYNC_FUNC_IDX = 4;

  static constexpr uint32_t INPUT_BITMASK(uint8_t input_idx) { return static_cast<uint32_t>(1) << input_idx; }

  static constexpr uint8_t NUM_INPUTS = 32;
  static constexpr uint8_t NUM_OUTPUTS = 16;
  std::array<uint32_t, NUM_OUTPUTS> outputs;
  functions::ICommandRegisterFunction f_cmd;
  bus::TriggerFunction f_imatrix_reset;
  bus::TriggerFunction f_imatrix_sync;

  explicit IBlock(const uint8_t clusterIdx)
      : FunctionBlock(clusterIdx), outputs{0}, f_cmd{bus::idx_to_addr(clusterIdx, BLOCK_IDX,
                                                                      IMATRIX_COMMAND_SR_FUNC_IDX)},
        f_imatrix_reset{bus::idx_to_addr(clusterIdx, BLOCK_IDX, IMATRIX_RESET_FUNC_IDX)},
        f_imatrix_sync{bus::idx_to_addr(clusterIdx, BLOCK_IDX, IMATRIX_SYNC_FUNC_IDX)} {}

  void write_to_hardware();
  void init() override;
};

} // namespace blocks