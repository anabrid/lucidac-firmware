// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "block/base.h"
#include "bus/functions.h"
#include "chips/SR74HCT595.h"

namespace platform {
class Cluster;
}

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
  [[nodiscard]] bool transfer(const std::array<int8_t, num_of_outputs> &outputs) const;
};

} // namespace functions

namespace blocks {

class MBlock;

class UBlockHAL : public FunctionBlockHAL {
public:
  enum class Reference_Magnitude : uint8_t { ONE = 0, ONE_TENTH = 1 };
  enum class Transmission_Mode : uint8_t {
    ANALOG_INPUT = 0b00,
    POS_REF = 0b01,
    NEG_REF = 0b10,
    GROUND = 0b11
  };

  virtual bool write_outputs(std::array<int8_t, 32> outputs) = 0;
  virtual bool write_transmission_modes_and_ref(std::pair<Transmission_Mode, Transmission_Mode> modes,
                                                Reference_Magnitude ref) = 0;
  virtual void reset_transmission_modes_and_ref() = 0;
};

class UBlockHAL_Dummy : public UBlockHAL {
public:
  bool write_outputs(std::array<int8_t, 32> outputs) override;
  bool write_transmission_modes_and_ref(std::pair<Transmission_Mode, Transmission_Mode> modes,
                                        Reference_Magnitude ref) override;
  void reset_transmission_modes_and_ref() override;
};

class UBlockHAL_Common : public UBlockHAL {
protected:
  const functions::UMatrixFunction f_umatrix;
  const functions::TriggerFunction f_umatrix_sync;
  // Reset disables all output, but rest of logic is unchanged according to datasheet.
  // But I don't really know what that means. Data is still shifted out after a reset
  // and the enable-bits in the data are still set.
  // The datasheet calls the RESET pin OUTPUT ENABLE, so it probably is simply that.
  // Meaning it is completely useless.
  // const functions::TriggerFunction f_umatrix_reset;

  const functions::SR74HCT595 f_transmission_mode_register;
  const functions::TriggerFunction f_transmission_mode_sync;
  const functions::TriggerFunction f_transmission_mode_reset;

public:
  explicit UBlockHAL_Common(bus::addr_t block_address, uint8_t f_umatrix_cs, uint8_t f_umatrix_sync_cs,
                            uint8_t f_transmission_mode_register_cs, uint8_t f_transmission_mode_sync_cs,
                            uint8_t f_transmission_mode_reset_cs);

  bool write_outputs(std::array<int8_t, 32> outputs) override;
  bool write_transmission_modes_and_ref(std::pair<Transmission_Mode, Transmission_Mode> modes,
                                        Reference_Magnitude ref) override;
  void reset_transmission_modes_and_ref() override;
};

class UBlockHAL_V_1_2_X : public UBlockHAL_Common {
public:
  explicit UBlockHAL_V_1_2_X(const bus::addr_t block_address)
      : UBlockHAL_Common(block_address, 5, 6, 2, 3, 4){};
};

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
  // Entity hardware identifier information.
  static constexpr auto CLASS_ = entities::EntityClass::U_BLOCK;
  static constexpr uint8_t TYPE = 1;

  static UBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

public:
  static constexpr uint8_t BLOCK_IDX = bus::U_BLOCK_IDX;

  static constexpr uint8_t NUM_OF_INPUTS = 16;
  static constexpr uint8_t NUM_OF_OUTPUTS = 32;

  static constexpr std::array<uint8_t, NUM_OF_INPUTS> INPUT_IDX_RANGE() {
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  };

  static constexpr std::array<uint8_t, NUM_OF_OUTPUTS> OUTPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

  using Transmission_Mode = UBlockHAL::Transmission_Mode;
  using Reference_Magnitude = UBlockHAL::Reference_Magnitude;

protected:
  UBlockHAL *hardware;

  std::array<int8_t, NUM_OF_OUTPUTS> output_input_map;
  Reference_Magnitude ref_magnitude = Reference_Magnitude::ONE;
  Transmission_Mode a_side_mode = Transmission_Mode::ANALOG_INPUT;
  Transmission_Mode b_side_mode = Transmission_Mode::ANALOG_INPUT;

  // Default sanity checks for input and output indizes
  static bool _i_sanity_check(const uint8_t input);
  static bool _o_sanity_check(const uint8_t output);
  static bool _io_sanity_check(const uint8_t input, const uint8_t output);

  //! Changes the transmission mode of the regular ublock switches.
  void change_a_side_transmission_mode(const Transmission_Mode mode);
  //! Changes the transmission mode of the alternative ublock switches.
  void change_b_side_transmission_mode(const Transmission_Mode mode);
  //! Changes the transmission mode for all ublock switches.
  void change_all_transmission_modes(const Transmission_Mode mode);
  //! Changes the transmission mode for all ublock switches.
  void change_all_transmission_modes(const std::pair<Transmission_Mode, Transmission_Mode> modes);

  Reference_Magnitude get_reference_magnitude();

  void change_reference_magnitude(Reference_Magnitude ref);

  void reset_reference_magnitude();

  std::pair<Transmission_Mode, Transmission_Mode> get_all_transmission_modes() const;
  //! Check whether an input is connected to an output, without sanity checks.
  bool _is_connected(const uint8_t input, const uint8_t output) const;

  //! Connects output with given input, without sanity checks or disconnection prevention.
  void _connect(const uint8_t input, const uint8_t output);

  //! Disconnects output, without sanity checks.
  void _disconnect(const uint8_t output);

  //! Check whether an output is connected to any input, without sanity checks.
  bool _is_output_connected(const uint8_t output) const;

  bool _is_input_connected(const uint8_t input) const;

public:
  UBlock(bus::addr_t block_address, UBlockHAL *hardware);
  UBlock() : UBlock(0, new UBlockHAL_Dummy()){};

  entities::EntityClass get_entity_class() const final { return entities::EntityClass::U_BLOCK; }

  void reset(entities::ResetAction action) override;

  void reset_connections();

  //! Connects a block input to a block output. If the current transmission mode would make the connection
  //! impossible but could be changed without consequences, the mode will be adjusted. If the specified output
  //! is already connected or the transmission mode can't be corrected, the function fails. If force is true
  //! the output will be overwritten and the transmission mode will be adjusted.
  bool connect(const uint8_t input, const uint8_t output, const bool force = false);

  //! Connects an alternative input / non block input specified by signal_mode from the a- or b-side to a block
  //! output. a-side inputs will be mapped one to one on the chip input if possible. Fallback chip is input 0.
  //! If the current transmission mode would make the connection impossible but could be changed without
  //! consequences, the mode will be adjusted. If the specified output is already connected or the transmission
  //! mode can't be corrected, the function fails. If force is true the output will be overwritten and the
  //! transmission mode will be adjusted.
  bool connect_alternative(Transmission_Mode signal_type, const uint8_t output, const bool force = false,
                           bool use_a_side = false);

  //! Disconnect an input from an output, if they are connected. Both input and output are zero-based indizes.
  bool disconnect(const uint8_t input, const uint8_t output);

  //! Disconnect all inputs from an output. Fails for invalid arguments.
  bool disconnect(const uint8_t output);

  //! Check whether an chip input is connected to an chip / block output.
  bool is_connected(const uint8_t input, const uint8_t output) const;

  //! Check whether an chip / block output is connected to any chip input.
  bool is_output_connected(const uint8_t output) const;

  //! Check whether an chip input is connected to any output.
  bool is_input_connected(const uint8_t input) const;

  bool is_anything_connected() const;

  [[nodiscard]] utils::status write_to_hardware() override;

  utils::status config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;

  utils::status _config_outputs_from_json(const JsonVariantConst &cfg);
  utils::status _config_constants_from_json(const JsonVariantConst &cfg);

  friend class ::platform::Cluster;
  friend class ::blocks::MBlock;
};
} // namespace blocks

// Include template definitions
#include "ublock.tpl.h"
