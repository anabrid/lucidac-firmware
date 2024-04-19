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
#include "chips/SR74HCT595.h"

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
  template <size_t num_of_outputs> void transfer(const std::array<int8_t, num_of_outputs> &outputs) const;
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

enum UBlock_Transmission_Mode : uint8_t {
  ANALOG_INPUT = 0b00,
  BIG_REF = 0b01,
  SMALL_REF = 0b10,
  GROUND = 0b11
};

class UBlock : public FunctionBlock {
public:
  static constexpr uint8_t BLOCK_IDX = bus::U_BLOCK_IDX;

  static constexpr uint8_t NUM_OF_INPUTS = 16;
  static constexpr uint8_t NUM_OF_OUTPUTS = 32;

  static constexpr std::array<uint8_t, NUM_OF_INPUTS> INPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15};
  };

  static constexpr std::array<uint8_t, NUM_OF_OUTPUTS> OUTPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

  static constexpr uint8_t TRANSMISSION_MODE_FUNC_IDX = 2;
  static constexpr uint8_t TRANSMISSION_MODE_SYNC_FUNC_IDX = 4;

  static constexpr uint8_t UMATRIX_FUNC_IDX = 5;
  static constexpr uint8_t UMATRIX_SYNC_FUNC_IDX = 6;
  static constexpr uint8_t UMATRIX_RESET_FUNC_IDX = 7;

protected:
  const functions::UMatrixFunction f_umatrix;
  const functions::TriggerFunction f_umatrix_sync;
  // Reset disables all output, but rest of logic is unchanged according to datasheet.
  // But I don't really know what that means. Data is still shifted out after a reset
  // and the enable-bits in the data are still set.
  // The datasheet calls the RESET pin OUTPUT ENABLE, so it probably is simply that.
  // Meaning it is completely useless.
  const functions::TriggerFunction f_umatrix_reset;

  std::array<int8_t, NUM_OF_OUTPUTS> output_input_map;

  const functions::SR74HCT595 transmission_mode_register;
  const functions::TriggerFunction transmission_mode_sync;

  uint8_t transmission_mode_byte;

  // Default sanity checks for input and output indizes
  static bool _i_sanity_check(const uint8_t input);
  static bool _o_sanity_check(const uint8_t output);
  static bool _io_sanity_check(const uint8_t input, const uint8_t output);

private:
  //! Check whether an input is connected to an output, without sanity checks.
  bool _is_connected(const uint8_t input, const uint8_t output) const;

  //! Connects output with given input, without sanity checks or disconnection prevention.
  void _connect(const uint8_t input, const uint8_t output);

  //! Disconnects output, without sanity checks.
  void _disconnect(const uint8_t output);

  //! Check whether an output is connected to any input, without sanity checks.
  bool _is_output_connected(const uint8_t output) const;

public:
  explicit UBlock(const uint8_t clusterIdx);

  bus::addr_t get_block_address() override;

  void reset(const bool keep_offsets) override;

  void reset_connections();

  //! Connect an input to an output, if output is unused. Both input and output are zero-based indizes.
  bool connect(const uint8_t input, const uint8_t output, const bool allow_disconnections = false);

  //! Disconnect an input from an output, if they are connected. Both input and output are zero-based indizes.
  bool disconnect(const uint8_t input, const uint8_t output);

  //! Disconnect all inputs from an output. Fails for invalid arguments.
  bool disconnect(const uint8_t output);

  //! Check whether an input is connected to an output.
  bool is_connected(const uint8_t input, const uint8_t output);

  //! Check whether an output is connected to any input.
  bool is_output_connected(const uint8_t output);

  //! Changes the transmission mode of the ublock for the calibration processes.
  void changeTransmissionMode(const UBlock_Transmission_Mode mode, const bool negative,
                              const bool use_alt = false);

  void write_matrix_to_hardware() const;
  void write_transmission_mode_to_hardware() const;
  void write_to_hardware() override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;
};

} // namespace blocks

// Include template definitions
#include "ublock.tpl.h"
