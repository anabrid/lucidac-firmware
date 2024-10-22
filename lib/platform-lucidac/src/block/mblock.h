// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <bitset>
#include <cstdint>

#include "block/base.h"
#include "chips/DAC60508.h"
#include "chips/SR74HCT595.h"
#include "chips/SR74HC16X.h"
#include "daq/base.h"

namespace platform {
class Cluster;
}

namespace carrier {
class Carrier;
}

namespace blocks {

// ██████   █████  ███████ ███████      ██████ ██       █████  ███████ ███████
// ██   ██ ██   ██ ██      ██          ██      ██      ██   ██ ██      ██
// ██████  ███████ ███████ █████       ██      ██      ███████ ███████ ███████
// ██   ██ ██   ██      ██ ██          ██      ██      ██   ██      ██      ██
// ██████  ██   ██ ███████ ███████      ██████ ███████ ██   ██ ███████ ███████

class MBlockHAL : public FunctionBlockHAL {
public:
  virtual std::bitset<8> read_overload_flags() = 0;
  virtual void reset_overload_flags() = 0;

  bool init() override;
};

/**
 * A Lucidac Math block (M-Block) is represented by this class.
 * Lucidac currently has two Math blocks, referred to as M1 and M2.
 * Both are represented by instances of this class or a suitable subclass.
 *
 * This base class provides convenient static functions to compute the correct
 * index (for usage in the U and I block) for M block computing elements.
 *
 **/
class MBlock : public FunctionBlock {
public:
  // Entity hardware identifier information.
  static constexpr auto CLASS_ = entities::EntityClass::M_BLOCK;

  enum class TYPES : uint8_t { UNKNOWN = 0, M_INT8_BLOCK = 1, M_MUL4_BLOCK = 2 };

  static MBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

  bool is_entity_type(TYPES type_) { return entities::Entity::is_entity_type(static_cast<uint8_t>(type_)); }

public:
  static constexpr uint8_t M0_IDX = bus::M0_BLOCK_IDX;
  static constexpr uint8_t M1_IDX = bus::M1_BLOCK_IDX;

  enum class SLOT : uint8_t { M0 = 0, M1 = 1 };

  static constexpr std::array<uint8_t, 8> SLOT_INPUT_IDX_RANGE() { return {0, 1, 2, 3, 4, 5, 6, 7}; };

  static constexpr std::array<uint8_t, 8> SLOT_OUTPUT_IDX_RANGE() { return {0, 1, 2, 3, 4, 5, 6, 7}; };

  //! M0 input signal specifier for hard-coded usage, like MBlock::M0_INPUT<3>().
  template <int n> static constexpr uint8_t M0_INPUT() {
    static_assert(n < 8, "MBlock input must be less than 8.");
    return n + 8;
  }

  //! M0 input signal specifier for dynamic usage, like MBlock::M0_INPUT(variable).
  static constexpr uint8_t M0_INPUT(uint8_t idx) { return idx + 8; }

  //! M0 output signal specifier for hard-coded usage, like MBlock::M0_OUTPUT<3>().
  template <int n> static constexpr uint8_t M0_OUTPUT() {
    static_assert(n < 8, "MBlock output must be less than 8.");
    return n + 8;
  }

  //! M0 output signal specifier for dynamic usage, like MBlock::M0_OUTPUT(variable).
  static constexpr uint8_t M0_OUTPUT(uint8_t idx) { return idx + 8; }

  //! M1 input signal specifier for hard-coded usage, like MBlock::M1_INPUT<3>().
  template <int n> static constexpr uint8_t M1_INPUT() {
    static_assert(n < 8, "MBlock input must be less than 8.");
    return n;
  }

  //! M1 input signal specifier for dynamic usage, like MBlock::M1_INPUT(variable).
  static constexpr uint8_t M1_INPUT(uint8_t idx) { return idx; }

  //! M1 output signal specifier for hard-coded usage, like MBlock::M1_OUTPUT<3>().
  template <int n> static constexpr uint8_t M1_OUTPUT() {
    static_assert(n < 8, "MBlock output must be less than 8.");
    return n;
  }

  //! M1 output signal specifier for dynamic usage, like MBlock::M1_OUTPUT(variable).
  static constexpr uint8_t M1_OUTPUT(uint8_t idx) { return idx; }

public:
  const SLOT slot;
  MBlockHAL* hardware;

public:
  explicit MBlock(bus::addr_t block_address, MBlockHAL* hardware);

  explicit MBlock(SLOT slot, MBlockHAL *hardware)
      : MBlock(bus::idx_to_addr(0, slot == SLOT::M0 ? bus::M0_BLOCK_IDX : bus::M1_BLOCK_IDX, 0), hardware) {}

  entities::EntityClass get_entity_class() const final { return entities::EntityClass::M_BLOCK; }

  // Types exist, force them to differentiate themselves
  uint8_t get_entity_type() const override = 0;

  uint8_t slot_to_global_io_index(uint8_t local) const;

  // M Blocks generaly can't calibrate themselfs on their own, so they need the cluster they are installed in.
  virtual bool calibrate(daq::BaseDAQ *daq_, platform::Cluster *cluster) { return true; }

  void overload_flags_to_json(JsonArray msg_out);
};

class EmptyMBlock : public MBlock {
public:
  utils::status write_to_hardware() override;
  uint8_t get_entity_type() const override;

  using MBlock::MBlock;

protected:
  utils::status config_self_from_json(JsonObjectConst cfg) override;
};

// ██ ███    ██ ████████  █████       ██████ ██       █████  ███████ ███████
// ██ ████   ██    ██    ██   ██     ██      ██      ██   ██ ██      ██
// ██ ██ ██  ██    ██     █████      ██      ██      ███████ ███████ ███████
// ██ ██  ██ ██    ██    ██   ██     ██      ██      ██   ██      ██      ██
// ██ ██   ████    ██     █████       ██████ ███████ ██   ██ ███████ ███████

class MIntBlockHAL : public MBlockHAL {
public:
  virtual bool write_ic(uint8_t idx, float ic) = 0;
  virtual bool write_time_factor_switches(std::bitset<8> switches) = 0;
};

class MIntBlockHAL_Dummy : public MIntBlockHAL {
public:
  bool write_ic(uint8_t idx, float ic) override { return true; }

  bool write_time_factor_switches(std::bitset<8> switches) override { return true; }

  std::bitset<8> read_overload_flags() override { return {0}; }
  void reset_overload_flags() override {}

  explicit MIntBlockHAL_Dummy(bus::addr_t) {}
};

class MIntBlockHAL_V_1_0_X : public MIntBlockHAL {
protected:
  const functions::DAC60508 f_ic_dac;
  const functions::SR74HCT595 f_time_factor;
  const functions::TriggerFunction f_time_factor_sync;
  const functions::TriggerFunction f_time_factor_reset;
  const functions::SR74HC16X f_overload_flags;
  const functions::TriggerFunction f_overload_flags_reset;

public:
  explicit MIntBlockHAL_V_1_0_X(bus::addr_t block_address);

  bool init() override;

  bool write_ic(uint8_t idx, float ic) override;
  bool write_time_factor_switches(std::bitset<8> switches) override;

  std::bitset<8> read_overload_flags() override;
  void reset_overload_flags() override;
};

// HINT: Consider renaming this MBlockInt
//       in particular if we get some MBlockMult (MMultBlock reads weird)
class MIntBlock : public MBlock {
public:
  // Entity hardware identifier information.
  static constexpr auto TYPE = MBlock::TYPES::M_INT8_BLOCK;

  static MIntBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

public:
  static constexpr uint8_t NUM_INTEGRATORS = 8;
  static constexpr unsigned int DEFAULT_TIME_FACTOR = 10000;

  static constexpr std::array<uint8_t, NUM_INTEGRATORS> INTEGRATORS_INPUT_RANGE() {
    return {0, 1, 2, 3, 4, 5, 6, 7};
  };

  static constexpr std::array<uint8_t, NUM_INTEGRATORS> INTEGRATORS_OUTPUT_RANGE() {
    return {0, 1, 2, 3, 4, 5, 6, 7};
  };

protected:
  MIntBlockHAL *hardware;

  std::array<float, NUM_INTEGRATORS> ic_values;
  std::array<unsigned int, NUM_INTEGRATORS> time_factors;

  utils::status _config_elements_from_json(const JsonVariantConst &cfg);

public:
  explicit MIntBlock(bus::addr_t block_address, MIntBlockHAL *hardware);

  explicit MIntBlock(SLOT slot, MIntBlockHAL *hardware)
      : MIntBlock(bus::idx_to_addr(0, slot == SLOT::M0 ? bus::M0_BLOCK_IDX : bus::M1_BLOCK_IDX, 0), hardware) {
  }

  explicit MIntBlock(SLOT slot)
      : MIntBlock(bus::idx_to_addr(0, slot == SLOT::M0 ? bus::M0_BLOCK_IDX : bus::M1_BLOCK_IDX, 0),
                  new MIntBlockHAL_Dummy(bus::NULL_ADDRESS)) {}

  uint8_t get_entity_type() const final { return static_cast<uint8_t>(TYPE); }

  bool init() override;
  void reset(entities::ResetAction action) override;

  [[nodiscard]] const std::array<float, 8> &get_ic_values() const;
  [[nodiscard]] float get_ic_value(uint8_t idx) const;
  bool set_ic_values(float value);
  bool set_ic_values(const std::array<float, 8> &ic_values_);
  bool set_ic_value(uint8_t idx, float value);
  void reset_ic_values();

  [[nodiscard]] const std::array<unsigned int, 8> &get_time_factors() const;
  unsigned int get_time_factor(uint8_t idx) const;
  bool set_time_factors(unsigned int k);
  bool set_time_factors(const std::array<unsigned int, 8> &time_factors_);
  bool set_time_factor(uint8_t int_idx, unsigned int k);
  void reset_time_factors();

  [[nodiscard]] utils::status write_to_hardware() override;

  utils::status config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;
};

// TODO: Change this comment -- This is rather a MUL4 not MUL8
//
// ███    ███ ██    ██ ██       █████       ██████ ██       █████  ███████ ███████
// ████  ████ ██    ██ ██      ██   ██     ██      ██      ██   ██ ██      ██
// ██ ████ ██ ██    ██ ██       █████      ██      ██      ███████ ███████ ███████
// ██  ██  ██ ██    ██ ██      ██   ██     ██      ██      ██   ██      ██      ██
// ██      ██  ██████  ███████  █████       ██████ ███████ ██   ██ ███████ ███████

class MMulBlockHAL : public MBlockHAL {
public:
  virtual bool write_calibration_input_offsets(uint8_t idx, float offset_x, float offset_y) = 0;
  virtual bool reset_calibration_input_offsets() = 0;

  virtual bool write_calibration_output_offset(uint8_t idx, float offset_z) = 0;
  virtual bool reset_calibration_output_offsets() = 0;

  bool init() override;
};

class MMulBlockHAL_V_1_0_X : public MMulBlockHAL {
protected:
  const functions::TriggerFunction f_overload_flags_reset;
  const functions::SR74HC16X f_overload_flags;
  const functions::DAC60508 f_calibration_dac_0;
  const functions::DAC60508 f_calibration_dac_1;

public:
  MMulBlockHAL_V_1_0_X(bus::addr_t block_address);

  bool init() override;

public:
  bool write_calibration_input_offsets(uint8_t idx, float offset_x, float offset_y) override;
  bool reset_calibration_input_offsets() override;

  bool write_calibration_output_offset(uint8_t idx, float offset_z) override;
  bool reset_calibration_output_offsets() override;

  std::bitset<8> read_overload_flags() override;
  void reset_overload_flags() override;
};

/**
 * Calibration data per each multiplier. There are NUM_MULTIPLIERS on an MMulBlock.
 **/
struct __attribute__((packed)) MultiplierCalibration {
  float offset_x = 0.0f, offset_y = 0.0f, offset_z = 0.0f;
};

struct __attribute__((packed)) ManualMultiplierCalibrationMetadata {
  MultiplierCalibration cal[4];
  uint8_t checksum;
  uint8_t compute_checksum() const;
};

/**
 * Multplier Math Block (or: Math Multplier Block, hence MMulBlock).
 * Holds 4 analog multpliers. Furthermore, the first inputs are always mapped on the
 * last four outputs as identity elements. Therefore this block can "implicitely"
 * serve as a "Math identity block" (something which also exists for 8-to-8 identities,
 * but purely passive). The identity part is not configurable.
 * The multipliers itself are also not user-configurable. They are, however,
 * digitally configurable with respect to their offset calibration.
 **/
class MMulBlock : public MBlock::MBlock {
public:
  // Entity hardware identifier information.
  static constexpr auto TYPE = MBlock::TYPES::M_MUL4_BLOCK;

  static MMulBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

public:
  static constexpr uint8_t NUM_MULTIPLIERS = 4;

  static constexpr std::array<uint8_t, NUM_MULTIPLIERS * 2> MULTIPLIERS_INPUT_RANGE() {
    return {0, 1, 2, 3, 4, 5, 6, 7};
  };

  static constexpr std::array<uint8_t, NUM_MULTIPLIERS> MULTIPLIERS_OUTPUT_RANGE() { return {0, 1, 2, 3}; };

protected:
  MMulBlockHAL *hardware;

  std::array<MultiplierCalibration, NUM_MULTIPLIERS> calibration{};

public:
  using MBlock::MBlock;
  MMulBlock(bus::addr_t block_address, MMulBlockHAL *hardware);

  uint8_t get_entity_type() const final { return static_cast<uint8_t>(TYPE); }

  bool init() override;

  /// Attention, this does nothing!
  /// @see write_calibration_to_hardware() for actual code.
  [[nodiscard]] utils::status write_to_hardware() override;

  bool calibrate(daq::BaseDAQ *daq_, platform::Cluster *cluster) override;

  utils::status read_calibration_from_eeprom(); ///< does not write_to_hardware
  utils::status write_calibration_to_eeprom();

  void reset(entities::ResetAction action);

  [[nodiscard]] const std::array<MultiplierCalibration, NUM_MULTIPLIERS> &get_calibration() const;
  [[nodiscard]] blocks::MultiplierCalibration get_calibration(uint8_t mul_idx) const;
  utils::status write_calibration_to_hardware();

protected:
  utils::status config_self_from_json(JsonObjectConst cfg) override;
  utils::status _config_elements_from_json(const JsonVariantConst &cfg);
  void config_self_to_json(JsonObject &cfg) override;
};

} // namespace blocks
