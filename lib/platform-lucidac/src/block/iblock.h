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


namespace functions {

/**
 * DataFunction to transfer 32bit of data to the I-Block matrix command registry.
 *
 * Data is [8bit 4_* SR][8bit 3_* SR][8bit 2_* SR][8bit 1_* SR]
 *      -> [0-15 X 0-07][16-31 X 0-7][0-15 X 8-15][16-31 X 8-15]   [ input X output ] matrix
 * Each is [DATA Y2 Y1 Y0 X3 X2 X1 X0]
 * Data bit comes first, most significant bit comes first (in SPI)
 *
 * See chip_cmd_word and IBlock::write_to_hardware for more information and the actual calculation of the bitstream.
 */
class ICommandRegisterFunction : public DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  using DataFunction::DataFunction;
  explicit ICommandRegisterFunction(bus::addr_t address);

  static uint8_t chip_cmd_word(uint8_t chip_input_idx, uint8_t chip_output_idx, bool connect = true);
};

} // namespace functions


namespace blocks {

/**
 * The Lucidac I-Block (I for Current; the Implicit Summing Block) is
 * represented by this class.
 *
 * This class provides an in-memory representation of the bit matrix,
 * neat way of manipulating it and flushing it out to the hardware.
 *
 * As a Lucidac can only have a single I-Block, this is kind of a singleton.
 * Typical usage happens via the Lucidac class.
 *
 **/
class IBlock : public FunctionBlock {
public:
  static constexpr uint32_t INPUT_BITMASK(uint8_t input_idx) { return static_cast<uint32_t>(1) << input_idx; }

  static constexpr uint8_t NUM_INPUTS = 32;
  static constexpr uint8_t NUM_OUTPUTS = 16;

  static constexpr std::array<uint8_t, NUM_INPUTS> INPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

  static constexpr std::array<uint8_t, NUM_OUTPUTS> OUTPUT_IDX_RANGE() {
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  };

protected:
  void reset_outputs();

  bool _connect_from_json(const JsonVariantConst &input_spec, uint8_t output);

  void config_self_to_json(JsonObject &cfg) override;

  bool _is_connected(uint8_t input, uint8_t output);

public:
  std::array<uint32_t, NUM_OUTPUTS> outputs;
  const functions::ICommandRegisterFunction f_cmd;
  const functions::TriggerFunction f_imatrix_reset;
  const functions::TriggerFunction f_imatrix_sync;

  explicit IBlock(const uint8_t clusterIdx)
      : FunctionBlock("I", clusterIdx), outputs{0}, f_cmd{bus::address_from_tuple(bus::I_BLOCK_BADDR(clusterIdx), 2)},
        f_imatrix_reset{bus::address_from_tuple(bus::I_BLOCK_BADDR(clusterIdx), 4)},
        f_imatrix_sync{bus::address_from_tuple(bus::I_BLOCK_BADDR(clusterIdx), 3)} {}

  bus::addr_t get_block_address() override;

  void write_to_hardware() override;

  bool init() override;

  void reset(bool keep_calibration) override;

  /**
   * Connects an input line [0..31] to an output line [0..15]
   * by setting an appropriate bit/switch in the respective position
   * in the matrix.
   *
   * Note that this function only manipulates the in-memory representation
   * and does not immediately write outs to hardware.
   *
   * Note that calls to connect() only add bits to the existing configuration.
   * Use reset() to reset the matrix.
   *
   * The flag `exlusive` disconnects all other inputs from the chosen output.
   *
   * @returns false in case of invalid input, true else.
   *
   **/
  bool connect(uint8_t input, uint8_t output, bool exclusive = false, bool allow_input_splitting = false);

  //! Whether an input is connected to an output.
  bool is_connected(uint8_t input, uint8_t output);

  //! Disconnect one input from an output. Fails for invalid arguments or if no input is connected.
  bool disconnect(uint8_t input, uint8_t output);

  //! Disconnect all inputs from an output. Fails for invalid arguments.
  bool disconnect(uint8_t output);

  bool config_self_from_json(JsonObjectConst cfg) override;
};

} // namespace blocks