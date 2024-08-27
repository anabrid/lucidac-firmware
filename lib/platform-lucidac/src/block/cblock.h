// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "block/base.h"
#include "bus/bus.h"
#include "chips/AD5452.h"
#include "chips/SR74HCT595.h"

namespace platform {
class Calibration;
}

namespace blocks {

// ██   ██  █████  ██
// ██   ██ ██   ██ ██
// ███████ ███████ ██
// ██   ██ ██   ██ ██
// ██   ██ ██   ██ ███████

class CBlockHAL : public FunctionBlockHAL {
public:
  virtual bool write_factor(uint8_t idx, float value) = 0;
};

class CBlockHALDummy : public CBlockHAL {
public:
  bool write_factor(uint8_t idx, float value) override { return true; }
};

class CBlockHAL_Common : public CBlockHAL {
protected:
  std::array<const functions::AD5452, 32> f_coeffs;

public:
  static std::array<const functions::AD5452, 32> make_f_coeffs(bus::addr_t block_address,
                                                               std::array<const uint8_t, 32> f_coeffs_cs);

  CBlockHAL_Common(bus::addr_t block_address, std::array<const uint8_t, 32> f_coeffs_cs);

  bool write_factor(uint8_t idx, float value) override;
};

class CBlockHAL_V_1_1_X : public CBlockHAL_Common {
public:
  explicit CBlockHAL_V_1_1_X(bus::addr_t block_address);
};

class CBlockHAL_V_1_0_X : public CBlockHAL_Common {
public:
  explicit CBlockHAL_V_1_0_X(bus::addr_t block_address);
};

// ██████   █████  ███████ ███████      ██████ ██       █████  ███████ ███████
// ██   ██ ██   ██ ██      ██          ██      ██      ██   ██ ██      ██
// ██████  ███████ ███████ █████       ██      ██      ███████ ███████ ███████
// ██   ██ ██   ██      ██ ██          ██      ██      ██   ██      ██      ██
// ██████  ██   ██ ███████ ███████      ██████ ███████ ██   ██ ███████ ███████

/**
 * The Lucidac Coefficient Block (C-Block) is represented by this class.
 *
 * This class provides a neat interface for setting digital potentiometers
 * without having to worry for "raw" DPT values or upscaling factors
 * (allowing the DPTs to take values between [-20,+20] instead of only
 * [-1,+1]).
 *
 * As a Lucidac can only have a single C-Block, this is kind of a singleton.
 * Typical usage happens via the Lucidac class.
 *
 **/
class CBlock : public FunctionBlock {
public:
  // Entity hardware identifier information.
  // For CBlocks, there is only one entity type possible currently.
  static constexpr auto CLASS_ = entities::EntityClass::C_BLOCK;
  static constexpr uint8_t TYPE = 1;

  static CBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

public:
  static constexpr uint8_t BLOCK_IDX = bus::C_BLOCK_IDX;

  static constexpr uint8_t COEFF_BASE_FUNC_IDX = 1;

  static constexpr uint8_t NUM_COEFF = 32;

  static constexpr std::array<uint8_t, NUM_COEFF> INPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

  static constexpr std::array<uint8_t, NUM_COEFF> OUTPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

  static constexpr float MIN_FACTOR = -1.01f;
  static constexpr float MAX_FACTOR = +1.01f;
  static constexpr float MAX_GAIN_CORRECTION_ABS = 0.1f;

protected:
  CBlockHAL *hardware;

  std::array<float, NUM_COEFF> factors_{{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                                         1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
                                         1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}};
  std::array<float, NUM_COEFF> gain_corrections_{
      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
       1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}};

  [[nodiscard]] bool write_factors_to_hardware();

public:
  CBlock(bus::addr_t block_address, CBlockHAL *hardware);
  CBlock();

  entities::EntityClass get_entity_class() const final { return entities::EntityClass::C_BLOCK; }

  // Variants exist, force them to differentiate themselves
  // TODO: This is now handled via sub classes of CBlockHAL
  // uint8_t get_entity_variant() const override = 0;

  /**
   * Set a particular digital potentiometer.
   *
   * idx means the coefficient ID and is between [0,31].
   * factor is the actual analog value between [-20,20].
   *
   * Note that calling this function only stores the value
   * in the in-memory representation of the hybrid controller.
   * The in-memory representation is flushed to hardware by
   * calling write_to_hardware().
   *
   * @returns false in case of invalid input, true else.
   **/

  float get_factor(uint8_t idx);
  const std::array<float, NUM_COEFF> &get_factors() const;

  void set_factors(const std::array<float, NUM_COEFF> &factors);
  [[nodiscard]] bool set_factor(uint8_t idx, float factor);

  float get_gain_correction(uint8_t idx) const;
  const std::array<float, NUM_COEFF> &get_gain_corrections() const;

  bool set_gain_correction(uint8_t coeff_idx, const float correction);
  void set_gain_corrections(const std::array<float, NUM_COEFF> &corrections);

  void reset_gain_corrections();

  [[nodiscard]] bool write_to_hardware() override;
  void reset(bool keep_calibration) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;

  bool _config_elements_form_json(const JsonVariantConst &cfg);

  friend class ::platform::Calibration;
};

} // namespace blocks
