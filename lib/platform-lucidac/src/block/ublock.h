// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "block/base.h"
#include "bus/bus.h"
#include "bus/functions.h"
#include "carrier/analog.h"
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

  static UBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);
  ;

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

  static constexpr uint8_t TRANSMISSION_MODE_FUNC_IDX = 2;
  static constexpr uint8_t TRANSMISSION_MODE_SYNC_FUNC_IDX = 4;

  static constexpr uint8_t UMATRIX_FUNC_IDX = 5;
  static constexpr uint8_t UMATRIX_SYNC_FUNC_IDX = 6;
  static constexpr uint8_t UMATRIX_RESET_FUNC_IDX = 7;

  enum Transmission_Mode : uint8_t {
    ANALOG_INPUT = 0b000,
    POS_BIG_REF = 0b010,
    POS_SMALL_REF = 0b100,
    NEG_BIG_REF = 0b011,
    NEG_SMALL_REF = 0b101,
    GROUND = 0b110
  };

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

  uint8_t transmission_mode_byte = 0;
  Transmission_Mode a_side_mode = ANALOG_INPUT;
  Transmission_Mode b_side_mode = ANALOG_INPUT;

  // Default sanity checks for input and output indizes
  static bool _i_sanity_check(const uint8_t input);
  static bool _o_sanity_check(const uint8_t output);
  static bool _io_sanity_check(const uint8_t input, const uint8_t output);

  //! Changes the transmission mode of the regular ublock switches. Returns the shift register value for
  //! debugging purposes
  uint8_t change_a_side_transmission_mode(const Transmission_Mode mode);
  //! Changes the transmission mode of the alternative ublock switches. Returns the shift register value for
  //! debugging purposes
  uint8_t change_b_side_transmission_mode(const Transmission_Mode mode);
  //! Changes the transmission mode for all ublock switches. Returns the shift register value for debugging
  //! purposes
  uint8_t change_all_transmission_modes(const Transmission_Mode mode);
  //! Changes the transmission mode for all ublock switches. Returns the shift register value for debugging
  //! purposes
  uint8_t change_all_transmission_modes(const std::pair<Transmission_Mode, Transmission_Mode> modes);

  std::pair<Transmission_Mode, Transmission_Mode> get_all_transmission_modes() const;

private:
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
  explicit UBlock(bus::addr_t block_address);

  UBlock() : UBlock(bus::idx_to_addr(0, bus::U_BLOCK_IDX, 0)) {}

  entities::EntityClass get_entity_class() const final { return entities::EntityClass::U_BLOCK; }

  void reset(const bool keep_offsets) override;

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

  [[nodiscard]] bool write_matrix_to_hardware() const;
  [[nodiscard]] bool write_transmission_mode_to_hardware() const;
  [[nodiscard]] bool write_to_hardware() override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;

  friend class ::platform::Cluster;
};

} // namespace blocks

// Include template definitions
#include "ublock.tpl.h"
