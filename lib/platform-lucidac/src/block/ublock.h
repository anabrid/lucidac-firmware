// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "block/base.h"
#include "bus/bus.h"
#include "bus/functions.h"
#include "carrier/analog.h"

namespace utils {

void shift_5_left(uint8_t *buffer, size_t size);

}

namespace functions {

class UMatrixFunction : public functions::DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  using DataFunction::DataFunction;
  explicit UMatrixFunction(bus::addr_t address);

  //! Convert an output array to data packets and transfer to chip.
  //! Timing: ~5microseconds
  template <size_t num_of_outputs>
  void transfer(const std::array<uint8_t, num_of_outputs>& outputs) const;
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
  static constexpr uint16_t ZERO_OFFSET_RAW = 511;

protected:
  DataFunction f_offsets;
  std::array<TriggerFunction, 4> f_triggers;

public:
  UOffsetLoader(bus::addr_t ublock_address, uint8_t offsets_data_func_idx,
                         uint8_t offsets_load_base_func_idx);

  static uint16_t build_cmd_word(uint8_t chip_output_idx, uint16_t offset_raw);
  static uint16_t offset_to_raw(float offset);

  void set_offsets_and_write_to_hardware(std::array<uint16_t, 32> offsets_raw) const;
  void set_offset_and_write_to_hardware(uint8_t offset_idx, uint16_t offset_raw) const;
  void trigger_load(uint8_t offset_idx) const;
};

} // namespace functions

namespace blocks {

/**
 * The Lucidac U-Block (U for Voltage) is represented by this class.
 *
 * This class provides an in-memory representation of the XBAR bit matrix,
 * neat way of manipulating it and flushing it out to the hardware.
 *
 * As a Lucidac can only have a single U-Block, this is kind of a singleton.
 * Typical usage happens via the Lucidac class.
 *
 * @TODO: Should ensure that there is no more then one bit per line,
 *        cf. https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/issues/8
 *
 **/
class UBlock : public FunctionBlock {
public:
  static constexpr uint8_t BLOCK_IDX = bus::U_BLOCK_IDX;

  static constexpr uint8_t NUM_OF_INPUTS = 16;
  static constexpr uint8_t NUM_OF_OUTPUTS = 32;

  static constexpr std::array<uint8_t, NUM_OF_OUTPUTS> OUTPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

  static constexpr std::array<uint8_t, 8> OUTPUT_IDX_RANGE_TO_ADC() { return {0, 1, 2, 3, 4, 5, 6, 7}; };

  static constexpr std::array<uint8_t, 8> IDX_RANGE_TO_ACL_OUT() {
    // return {15, 14, 13, 12, 11, 10, 9, 8};
    return { 8,  9, 10, 11, 12, 13, 14, 15};
  };

  // TODO: Make this constexpr
  static uint8_t OUTPUT_IDX_RANGE_TO_ADC(uint8_t idx) { return UBlock::OUTPUT_IDX_RANGE_TO_ADC()[idx]; };

  // TODO: Make this constexpr
  static uint8_t IDX_RANGE_TO_ACL_OUT(uint8_t idx) { return UBlock::IDX_RANGE_TO_ACL_OUT()[idx]; };

  static constexpr uint8_t UMATRIX_FUNC_IDX = 1;
  static constexpr uint8_t UMATRIX_SYNC_FUNC_IDX = 2;
  static constexpr uint8_t UMATRIX_RESET_FUNC_IDX = 3;
  static constexpr uint8_t OFFSETS_DATA_FUNC_IDX = 63; // Non-existent address
  static constexpr uint8_t OFFSETS_LOAD_BASE_FUNC_IDX = 4;
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_CLEAR_FUNC_IDX = 8;
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_FUNC_IDX = 9;
  static constexpr uint8_t ALT_SIGNAL_SWITCHER_SYNC_FUNC_IDX = 10;

  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_ACL0 = 1 << 0;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_ACL1 = 1 << 1;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_ACL2 = 1 << 2;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_ACL3 = 1 << 3;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_ACL4 = 1 << 4;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_ACL5 = 1 << 5;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_ACL6 = 1 << 6;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_ACL7 = 1 << 7;
  __attribute__((unused)) static constexpr uint16_t ALT_SIGNAL_REF_HALF = 1 << 8;
  __attribute__((unused)) static constexpr uint8_t ALT_SIGNAL_REF_HALF_INPUT = 7;
  static constexpr uint16_t MAX_ALT_SIGNAL = ALT_SIGNAL_REF_HALF;

  static const SPISettings ALT_SIGNAL_FUNC_SPI_SETTINGS;

protected:
  const functions::UMatrixFunction f_umatrix;
  const functions::TriggerFunction f_umatrix_sync;
  // Reset disables all output, but rest of logic is unchanged according to datasheet.
  // But I don't really know what that means. Data is still shifted out after a reset
  // and the enable-bits in the data are still set.
  // The datasheet calls the RESET pin OUTPUT ENABLE, so it probably is simply that.
  // Meaning it is completely useless.
  const functions::TriggerFunction f_umatrix_reset;
  const functions::DataFunction f_alt_signal;
  const functions::TriggerFunction f_alt_signal_clear;
  const functions::TriggerFunction f_alt_signal_sync;
  const functions::UOffsetLoader f_offset_loader;

  std::array<uint8_t, NUM_OF_OUTPUTS> output_input_map;
  uint16_t alt_signals;
  std::array<uint16_t, NUM_OF_OUTPUTS> offsets;

private:
  //! Check whether an input is connected to an output, without sanity checks.
  bool _is_connected(uint8_t input, uint8_t output);

public:
  explicit UBlock(uint8_t clusterIdx);

  bus::addr_t get_block_address() override;

  void reset(bool keep_offsets) override;

  void reset_connections();

  //! Connect an input to an output, if output is unused. Both input and output are zero-based indizes.
  bool connect(uint8_t input, uint8_t output, bool allow_disconnections = false);

  //! Disconnect an input from an output, if they are connected. Both input and output are zero-based indizes.
  bool disconnect(uint8_t input, uint8_t output);

  //! Disconnect all inputs from an output. Fails for invalid arguments.
  bool disconnect(uint8_t output);

  //! Check whether an input is connected to an output.
  bool is_connected(uint8_t input, uint8_t output);

  //! Use one or more of the alternative signal on the respective input signal.
  [[deprecated("Use connect_alt_signal instead.")]] bool use_alt_signals(uint16_t alt_signal);

  //! Enable and connect an alternative signal to an output. Use UBlock::ALT_SIGNAL_* for alt_signal.
  //! This does not allow disconnections, since then you may be left with enabled alt signals blocking other
  //! signals. Use reset for now if you need to disconnect an alternative signal.
  bool connect_alt_signal(uint16_t alt_signal, uint8_t output);

  //! Check whether an alternative signal is used.
  bool is_alt_signal_used(uint16_t alt_signal) const;

  //! Get bit-wise combination of used alternative signals.
  uint16_t get_alt_signals() const;

  void reset_alt_signals();
  void reset_offsets();

  bool set_offset(uint8_t output, uint16_t offset_raw);
  bool set_offset(uint8_t output, float offset);
  bool change_offset(uint8_t output, float delta);

  void write_matrix_to_hardware() const;
  void write_alt_signal_to_hardware() const;
  void write_offsets_to_hardware() const;
  void write_to_hardware() override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;
};

} // namespace blocks

// Include template definitions
#include "ublock.tpl.h"
