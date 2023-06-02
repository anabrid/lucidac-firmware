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

#include "analog.h"
#include "base_block.h"
#include "local_bus.h"
#include "functions.h"

namespace blocks {

namespace utils {

void shift_5_left(uint8_t *buffer, size_t size);

}

namespace functions {

class _old_UMatrixFunction : public ::functions::_old_DataFunction {
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

template <std::size_t num_of_outputs> class UMatrixFunction : public ::functions::DataFunction {
public:
  using ::functions::DataFunction::DataFunction;

  //! Convert an output array to data packets and transfer to chip.
  //! Timing: ~5microseconds
  void transfer(std::array<uint8_t, num_of_outputs> outputs) const;
};

class USignalSwitchFunction : public ::functions::_old_DataFunction {
  // TODO: Add one abstraction layer
public:
  uint16_t data{0};

  using ::functions::_old_DataFunction::_old_DataFunction;

  void write_to_hardware() const;
};

class UOffsetLoader {
public:
  static const SPISettings OFFSETS_FUNC_SPI_SETTINGS;
  static constexpr uint16_t OUTPUTS_PER_CHIP = 8;
  static constexpr uint16_t MAX_CHIP_OUTPUT_IDX = OUTPUTS_PER_CHIP - 1;
  // TODO: Min/Max float values may depend on hardware implementation.
  static constexpr float MIN_OFFSET = -0.080f / analog::REF_VOLTAGE;
  static constexpr float MAX_OFFSET = 0.080f / analog::REF_VOLTAGE;
  static constexpr uint16_t MIN_OFFSET_RAW = 0;
  static constexpr uint16_t MAX_OFFSET_RAW = 1023;
  static constexpr uint16_t ZERO_OFFSET_RAW = 512;

protected:
  ::functions::DataFunction f_offsets;
  std::array<::functions::TriggerFunction, 4> f_triggers;

public:
  explicit UOffsetLoader(bus::addr_t ublock_address);

  static uint16_t build_cmd_word(uint8_t chip_output_idx, uint16_t offset_raw);
  static uint16_t offset_to_raw(float offset);

  void set_offsets_and_write_to_hardware(std::array<uint16_t, 32> offsets_raw) const;
  void set_offset_and_write_to_hardware(uint8_t offset_idx, uint16_t offset_raw) const;
  void trigger_load(uint8_t offset_idx) const;
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
  static constexpr uint8_t OFFSETS_DATA_FUNC_IDX = 63; // Non-existent address
  static constexpr uint8_t OFFSETS_LOAD_BASE_FUNC_IDX = 4;
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_CLEAR_FUNC_IDX = 8;
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_FUNC_IDX = 9;
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_SYNC_FUNC_IDX = 10;

  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_IN24_ACL0 = 1 << 0;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_IN25_ACL1 = 1 << 1;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_IN26_ACL2 = 1 << 2;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_IN27_ACL3 = 1 << 3;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_IN28_ACL4 = 1 << 4;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_IN29_ACL5 = 1 << 5;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_IN30_ACL6 = 1 << 6;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_IN31_ACL7 = 1 << 7;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_REF_HALF = 1 << 8;
  __attribute__((unused)) static constexpr uint8_t ALT_SIGNAL_REF_HALF_INPUT = 7;
  static constexpr uint16_t MAX_ALT_SIGNAL = ALT_SIGNAL_REF_HALF;

  static const SPISettings UMATRIX_FUNC_SPI_SETTINGS;
  static const SPISettings ALT_SIGNAL_FUNC_SPI_SETTINGS;

protected:
  const functions::UMatrixFunction<NUM_OF_OUTPUTS> f_umatrix;
  const ::functions::TriggerFunction f_umatrix_sync;
  // Reset disables all output, but rest of logic is unchanged according to datasheet.
  // But I don't really know what that means. Data is still shifted out after a reset
  // and the enable-bits in the data are still set.
  // The datasheet calls the RESET pin OUTPUT ENABLE, so it probably is simply that.
  // Meaning it is completely useless.
  const ::functions::TriggerFunction f_umatrix_reset;
  const ::functions::DataFunction f_alt_signal;
  const ::functions::TriggerFunction f_alt_signal_clear;
  const ::functions::TriggerFunction f_alt_signal_sync;
  const functions::UOffsetLoader f_offset_loader;

  std::array<uint8_t, NUM_OF_OUTPUTS> output_input_map;
  uint16_t alt_signals;
  std::array<uint16_t, NUM_OF_OUTPUTS> offsets;

public:
  explicit UBlock(uint8_t clusterIdx);

  bus::addr_t get_block_address() override;

  void reset();

  void reset_connections();

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

  void reset_alt_signals();
  void reset_offsets();

  bool set_offset(uint8_t output, uint16_t offset_raw);
  bool set_offset(uint8_t output, float offset);

  void write_matrix_to_hardware() const;
  void write_alt_signal_to_hardware() const;
  void write_offsets_to_hardware() const;
  void write_to_hardware() const;
};

} // namespace blocks

// Include template definitions
#include "ublock.tpl.h"
