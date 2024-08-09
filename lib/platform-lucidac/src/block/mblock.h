// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <cstdint>

#include "block/base.h"
#include "bus/bus.h"
#include "chips/DAC60508.h"
#include "chips/SR74HCT595.h"
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

  //! M1 input signal specifier for hard-coded usage, like MBlock::M1_INPUT<3>().
  template <int n> static constexpr uint8_t M1_INPUT() {
    static_assert(n < 8, "MBlock input must be less than 8.");
    return n + 8;
  }

  //! M1 input signal specifier for dynamic usage, like MBlock::M1_INPUT(variable).
  static constexpr uint8_t M1_INPUT(uint8_t idx) { return idx + 8; }

  //! M1 output signal specifier for hard-coded usage, like MBlock::M1_OUTPUT<3>().
  template <int n> static constexpr uint8_t M1_OUTPUT() {
    static_assert(n < 8, "MBlock output must be less than 8.");
    return n + 8;
  }

  //! M1 output signal specifier for dynamic usage, like MBlock::M1_OUTPUT(variable).
  static constexpr uint8_t M1_OUTPUT(uint8_t idx) { return idx + 8; }

  //! M2 input signal specifier for hard-coded usage, like MBlock::M2_INPUT<3>().
  template <int n> static constexpr uint8_t M2_INPUT() {
    static_assert(n < 8, "MBlock input must be less than 8.");
    return n;
  }

  //! M2 input signal specifier for dynamic usage, like MBlock::M2_INPUT(variable).
  static constexpr uint8_t M2_INPUT(uint8_t idx) { return idx; }

  //! M2 output signal specifier for hard-coded usage, like MBlock::M2_OUTPUT<3>().
  template <int n> static constexpr uint8_t M2_OUTPUT() {
    static_assert(n < 8, "MBlock output must be less than 8.");
    return n;
  }

  //! M2 output signal specifier for dynamic usage, like MBlock::M2_OUTPUT(variable).
  static constexpr uint8_t M2_OUTPUT(uint8_t idx) { return idx; }

public:
  const SLOT slot;

public:
  explicit MBlock(bus::addr_t block_address);

  explicit MBlock(SLOT slot)
      : MBlock(bus::idx_to_addr(0, slot == SLOT::M0 ? bus::M0_BLOCK_IDX : bus::M1_BLOCK_IDX, 0)) {}

  entities::EntityClass get_entity_class() const final { return entities::EntityClass::M_BLOCK; }

  // Types exist, force them to differentiate themselves
  uint8_t get_entity_type() const override = 0;

  uint8_t slot_to_global_io_index(uint8_t local) const;

  virtual bool calibrate(daq::BaseDAQ *daq_, carrier::Carrier &carrier_, platform::Cluster &cluster) {
    return true;
  }
};

class EmptyMBlock : public MBlock {
public:
  bool write_to_hardware() override;
  uint8_t get_entity_type() const override;

  using MBlock::MBlock;

protected:
  bool config_self_from_json(JsonObjectConst cfg) override;
};

// ██ ███    ██ ████████  █████       ██████ ██       █████  ███████ ███████
// ██ ████   ██    ██    ██   ██     ██      ██      ██   ██ ██      ██
// ██ ██ ██  ██    ██     █████      ██      ██      ███████ ███████ ███████
// ██ ██  ██ ██    ██    ██   ██     ██      ██      ██   ██      ██      ██
// ██ ██   ████    ██     █████       ██████ ███████ ██   ██ ███████ ███████

// HINT: Consider renaming this MBlockInt
//       in particular if we get some MBlockMult (MMultBlock reads weird)
class MIntBlock : public MBlock {
public:
  // Entity hardware identifier information.
  static constexpr auto TYPE = MBlock::TYPES::M_INT8_BLOCK;

  static MIntBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

public:
  static constexpr uint8_t IC_FUNC_IDX = 1;
  static constexpr uint8_t TIME_FACTOR_FUNC_IDX = 3;
  static constexpr uint8_t TIME_FACTOR_SYNC_FUNC_IDX = 4;
  static constexpr uint8_t TIME_FACTOR_RESET_FUNC_IDX = 5;

  static constexpr uint8_t NUM_INTEGRATORS = 8;
  static constexpr unsigned int DEFAULT_TIME_FACTOR = 10000;

private:
  const functions::DAC60508 f_ic_dac;
  const functions::SR74HCT595 f_time_factor;
  const functions::TriggerFunction f_time_factor_sync;
  std::array<uint16_t, 8> ic_raw;
  std::array<unsigned int, 8> time_factors;

  bool write_time_factors_to_hardware();

public:
  explicit MIntBlock(bus::addr_t block_address);

  explicit MIntBlock(SLOT slot)
      : MIntBlock(bus::idx_to_addr(0, slot == SLOT::M0 ? bus::M0_BLOCK_IDX : bus::M1_BLOCK_IDX, 0)) {}

  uint8_t get_entity_type() const final { return static_cast<uint8_t>(TYPE); }

  bool init() override;

  bool set_ic(uint8_t idx, float value);

  bool set_time_factor(uint8_t int_idx, unsigned int k);

  [[nodiscard]] bool write_to_hardware() override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;
};

// ███    ███ ██    ██ ██       █████       ██████ ██       █████  ███████ ███████
// ████  ████ ██    ██ ██      ██   ██     ██      ██      ██   ██ ██      ██
// ██ ████ ██ ██    ██ ██       █████      ██      ██      ███████ ███████ ███████
// ██  ██  ██ ██    ██ ██      ██   ██     ██      ██      ██   ██      ██      ██
// ██      ██  ██████  ███████  █████       ██████ ███████ ██   ██ ███████ ███████

class MMulBlockHAL : public FunctionBlockHAL {
public:
  virtual bool write_calibration_input_offsets(uint8_t idx, float offset_x, float offset_y) = 0;
  virtual bool reset_calibration_input_offsets() = 0;

  virtual bool write_calibration_output_offset(uint8_t idx, float offset_z) = 0;
  virtual bool reset_calibration_output_offsets() = 0;

  bool init() override;
};

class MMulBlockHAL_V_1_0_1 : public MMulBlockHAL {
protected:
  const functions::DAC60508 f_calibration_dac_0;
  const functions::DAC60508 f_calibration_dac_1;

public:
  MMulBlockHAL_V_1_0_1(bus::addr_t block_address);

  bool init() override;

public:
  bool write_calibration_input_offsets(uint8_t idx, float offset_x, float offset_y) override;
  bool reset_calibration_input_offsets() override;

  bool write_calibration_output_offset(uint8_t idx, float offset_z) override;
  bool reset_calibration_output_offsets() override;
};

typedef struct MultiplierCalibration {
  float offset_x = 0.0f, offset_y = 0.0f, offset_z = 0.0f;
} MultiplierCalibration;

class MMulBlock : public MBlock {
public:
  // Entity hardware identifier information.
  static constexpr auto TYPE = MBlock::TYPES::M_MUL4_BLOCK;

  static MMulBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

public:
  static constexpr uint8_t NUM_MULTIPLIERS = 4;

protected:
  MMulBlockHAL *hardware;

  std::array<MultiplierCalibration, NUM_MULTIPLIERS> calibration{};

public:
  using MBlock::MBlock;
  MMulBlock(bus::addr_t block_address, MMulBlockHAL *hardware);

  uint8_t get_entity_type() const final { return static_cast<uint8_t>(TYPE); }

  bool init() override;

  [[nodiscard]] bool write_to_hardware() override;

  bool calibrate(daq::BaseDAQ *daq_, carrier::Carrier &carrier_, platform::Cluster &cluster) override;

  [[nodiscard]] const std::array<MultiplierCalibration, NUM_MULTIPLIERS> &get_calibration() const;
  [[nodiscard]] blocks::MultiplierCalibration get_calibration(uint8_t mul_idx) const;

protected:
  bool config_self_from_json(JsonObjectConst cfg) override;
};

} // namespace blocks
