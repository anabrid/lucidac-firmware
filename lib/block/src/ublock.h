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
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_CLEAR_FUNC_IDX = 8;
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_FUNC_IDX = 9;
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_SYNC_FUNC_IDX = 10;

  static constexpr uint16_t ALT_SIGNAL_IN24_ACL0 = 1 << 0;
  static constexpr uint16_t ALT_SIGNAL_IN25_ACL1 = 1 << 1;
  static constexpr uint16_t ALT_SIGNAL_IN26_ACL2 = 1 << 2;
  static constexpr uint16_t ALT_SIGNAL_IN27_ACL3 = 1 << 3;
  static constexpr uint16_t ALT_SIGNAL_IN28_ACL4 = 1 << 4;
  static constexpr uint16_t ALT_SIGNAL_IN29_ACL5 = 1 << 5;
  static constexpr uint16_t ALT_SIGNAL_IN30_ACL6 = 1 << 6;
  static constexpr uint16_t ALT_SIGNAL_IN31_ACL7 = 1 << 7;
  static constexpr uint16_t ALT_SIGNAL_REF_HALF = 1 << 8;
  static constexpr uint16_t MAX_ALT_SIGNAL = ALT_SIGNAL_REF_HALF;

  static const SPISettings UMATRIX_FUNC_SPI_SETTINGS;
  static const SPISettings ALT_SIGNAL_FUNC_SPI_SETTINGS;

protected:
  const functions::UMatrixFunction<NUM_OF_OUTPUTS> f_umatrix;
  const bus::TriggerFunction f_umatrix_sync;
  const bus::DataFunction f_alt_signal;
  const bus::TriggerFunction f_alt_signal_clear;
  const bus::TriggerFunction f_alt_signal_sync;

  std::array<uint8_t, NUM_OF_OUTPUTS> output_input_map;
  uint16_t alt_signals;

public:
  explicit UBlock(uint8_t clusterIdx);

  //! Connect or disconnect an input to an output. Both input and output are zero-based indizes.
  bool connect(uint8_t input, uint8_t output, bool connect = true);

  //! Check whether an input is connected to an output.
  bool is_connected(uint8_t input, uint8_t output);

  //! Use one or more of the alternative signal on the respective input signal.
  bool use_alt_signals(uint16_t alt_signal);

  //! Check whether an alternative signal is used.
  bool is_alt_signal_used(uint16_t alt_signal) const;

  //! Get bit-wise combination of used alternative signals.
  uint16_t get_alt_signals() const;

  void write_matrix_to_hardware() const;
  void write_alt_signal_to_hardware() const;
  void write_to_hardware() const;
};

} // namespace blocks

// Include template definitions
#include "ublock.tpl.h"