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

namespace utils {

void shift_5_left(uint8_t *buffer, size_t size);

}

namespace functions {

class _old_UMatrixFunction : public bus::_old_DataFunction {
public:
  static constexpr char number_of_inputs = 16;
  static constexpr char number_of_outputs = 32;

private:
  std::array<uint8_t, number_of_outputs> outputs{};

public:
  static constexpr uint8_t BYTESTREAM_SIZE = 20;

  _old_UMatrixFunction(unsigned short address, const SPISettings &spiSettings,
                       const std::array<uint8_t, number_of_outputs> &outputs);
  _old_UMatrixFunction(unsigned short address, const SPISettings &spiSettings);

  void sync_to_hardware() const;
};

template <std::size_t num_of_outputs> class UMatrixFunction : public bus::DataFunction {
public:
  using bus::DataFunction::DataFunction;

  void transfer(std::array<uint8_t, num_of_outputs> outputs) const;
};

class USignalSwitchFunction : public bus::_old_DataFunction {
  // TODO: Add one abstraction layer
public:
  uint16_t data{0};

  using bus::_old_DataFunction::_old_DataFunction;

  void write_to_hardware() const;
};

} // namespace functions

class UBlock : public FunctionBlock {
public:
  static constexpr uint8_t BLOCK_IDX = bus::U_BLOCK_IDX;

  static constexpr uint8_t NUM_OF_INPUTS = 16;
  static constexpr uint8_t NUM_OF_OUTPUTS = 32;

  static constexpr uint8_t UMATRIX_FUNC_IDX = 1;
  static constexpr uint8_t UMATRIX_SYNC_FUNC_IDX = 2;
  static constexpr uint8_t UMATRIX_RESET_FUNC_IDX = 3;
  static constexpr uint8_t SIGNAL_SWITCHER_CLEAR = 8;
  static constexpr uint8_t SIGNAL_SWITCHER = 9;
  static constexpr uint8_t SIGNAL_SWITCHER_SYNC = 10;

  static const SPISettings UMATRIX_FUNC_SPI_SETTINGS;

protected:
  const functions::UMatrixFunction<NUM_OF_OUTPUTS> f_umatrix;
  const bus::TriggerFunction f_umatrix_sync;

  std::array<uint8_t, NUM_OF_OUTPUTS> output_input_map;

public:
  explicit UBlock(uint8_t clusterIdx);

  //! Connect or disconnect an input to an output. Both input and output are zero-based indizes.
  bool connect(uint8_t input, uint8_t output, bool connect = true);
  bool is_connected(uint8_t input, uint8_t output);

  void write_to_hardware() const;
};

} // namespace blocks

// Include template definitions
#include "ublock.tpl.h"