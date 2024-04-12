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

  static constexpr uint8_t UMATRIX_FUNC_IDX = 1;
  static constexpr uint8_t UMATRIX_SYNC_FUNC_IDX = 2;
  static constexpr uint8_t UMATRIX_RESET_FUNC_IDX = 3;

protected:
  const functions::UMatrixFunction f_umatrix;
  const functions::TriggerFunction f_umatrix_sync;
  // Reset disables all output, but rest of logic is unchanged according to datasheet.
  // But I don't really know what that means. Data is still shifted out after a reset
  // and the enable-bits in the data are still set.
  // The datasheet calls the RESET pin OUTPUT ENABLE, so it probably is simply that.
  // Meaning it is completely useless.
  const functions::TriggerFunction f_umatrix_reset;

  std::array<uint8_t, NUM_OF_OUTPUTS> output_input_map;

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

  void write_matrix_to_hardware() const;
  void write_to_hardware() override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;
};

} // namespace blocks

// Include template definitions
#include "ublock.tpl.h"
