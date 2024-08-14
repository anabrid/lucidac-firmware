// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <bitset>
#include <cstdint>

#include "block/base.h"
#include "bus/functions.h"
#include "chips/SR74HCT595.h"

/// @brief  namespace for internal helpers
namespace functions {

/**
 * DataFunction to transfer 32bit of data to the I-Block matrix command registry.
 *
 * Data is [8bit 4_* SR][8bit 3_* SR][8bit 2_* SR][8bit 1_* SR]
 *      -> [0-15 X 0-07][16-31 X 0-7][0-15 X 8-15][16-31 X 8-15]   [ input X output ] matrix
 * Each is [DATA Y2 Y1 Y0 X3 X2 X1 X0]
 * Data bit comes first, most significant bit comes first (in SPI)
 *
 * See chip_cmd_word and IBlock::write_to_hardware for more information and the actual calculation of the
 * bitstream.
 */
class ICommandRegisterFunction : public SR74HCT595 {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  using SR74HCT595::SR74HCT595;
  explicit ICommandRegisterFunction(bus::addr_t address);

  static uint8_t chip_cmd_word(uint8_t chip_input_idx, uint8_t chip_output_idx, bool connect = true);
};

} // namespace functions

namespace blocks {

class IBlockHAL : public FunctionBlockHAL {
public:
  static constexpr uint8_t NUM_INPUTS = 32;
  static constexpr uint8_t NUM_OUTPUTS = 16;

  static constexpr uint32_t INPUT_BITMASK(uint8_t input_idx) { return static_cast<uint32_t>(1) << input_idx; }

  virtual bool write_outputs(const std::array<uint32_t, 16> &outputs) = 0;
  virtual bool write_upscaling(std::bitset<32> upscaling) = 0;
};

class IBlockHALDummy : public IBlockHAL {
public:
  bool write_outputs(const std::array<uint32_t, 16> &outputs) override { return true; }

  bool write_upscaling(std::bitset<32> upscaling) override { return true; }

  explicit IBlockHALDummy(bus::addr_t) {}
};

class IBlockHAL_V_1_2_0 : public IBlockHAL {
protected:
  const functions::ICommandRegisterFunction f_cmd;
  const functions::TriggerFunction f_imatrix_reset;
  const functions::TriggerFunction f_imatrix_sync;

  const functions::SR74HCT595 scaling_register;
  const functions::TriggerFunction scaling_register_sync;

public:
  explicit IBlockHAL_V_1_2_0(bus::addr_t block_address);

  bool write_outputs(const std::array<uint32_t, 16> &outputs) override;

  bool write_upscaling(std::bitset<32> upscaling) override;
};

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
  // Entity hardware identifier information.
  static constexpr auto CLASS_ = entities::EntityClass::I_BLOCK;

  entities::EntityClass get_entity_class() const final { return CLASS_; }

  static IBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

public:
  static constexpr uint8_t BLOCK_IDX = bus::I_BLOCK_IDX;

  static constexpr uint32_t INPUT_BITMASK(uint8_t input_idx) { return IBlockHAL::INPUT_BITMASK(input_idx); }

  static constexpr uint8_t NUM_INPUTS = IBlockHAL::NUM_INPUTS;
  static constexpr uint8_t NUM_OUTPUTS = IBlockHAL::NUM_OUTPUTS;

  static constexpr std::array<uint8_t, NUM_INPUTS> INPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

  static constexpr std::array<uint8_t, NUM_OUTPUTS> OUTPUT_IDX_RANGE() {
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  };

protected:
  bool _connect_from_json(const JsonVariantConst &input_spec, uint8_t output);

  void config_self_to_json(JsonObject &cfg) override;

  bool _is_connected(uint8_t input, uint8_t output) const;
  bool _is_output_connected(uint8_t output) const;

  IBlockHAL *hardware;

  std::array<uint32_t, NUM_OUTPUTS> outputs;
  std::bitset<NUM_INPUTS> scaling_factors = 0;

public:
  explicit IBlock(const bus::addr_t block_address, IBlockHAL* hardware) : FunctionBlock("I", block_address), hardware(hardware), outputs{0} {}

  IBlock() : IBlock(bus::NULL_ADDRESS, new IBlockHALDummy(bus::NULL_ADDRESS)) {}

  [[nodiscard]] bool write_to_hardware() override;

  bool init() override;

  void reset_outputs();

  void reset(bool keep_calibration) override;

  const std::array<uint32_t, NUM_OUTPUTS> &get_outputs() const;

  void set_outputs(const std::array<uint32_t, NUM_OUTPUTS> &outputs_);

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
  bool is_connected(uint8_t input, uint8_t output) const;

  bool is_anything_connected() const;

  //! Disconnect one input from an output. Fails for invalid arguments or if no input is connected.
  bool disconnect(uint8_t input, uint8_t output);

  //! Disconnect all inputs from an output. Fails for invalid arguments.
  bool disconnect(uint8_t output);

  //! Sets the input scale of the corresponding output. If upscale is false, a factor of 1.0 is applied, if
  //! upscale is true, a factor of 10.0 will be used.
  bool set_upscaling(uint8_t input, bool upscale);

  //! Sets all 32 input scales. If the corresponding bit is false, a factor of 1.0 is applied, if true, a
  //! factor of 10.0 will be used.
  void set_upscaling(std::bitset<NUM_INPUTS> scales);

  //! Sets all 32 input scales to the default 1.0.
  void reset_upscaling();

  //! Returns the input scale of the corresponding output.
  bool get_upscaling(uint8_t output) const;

  //! Returns all input scales. A low bit indicates a factor of 1.0, a high bit indicates a factor of 10.0.
  const std::bitset<NUM_INPUTS> &get_upscales() const { return scaling_factors; }

  bool config_self_from_json(JsonObjectConst cfg) override;
};

} // namespace blocks
