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

#include "bus.h"

namespace functions {

/**
 * Function classes encapsulate digital bus calls.
 *
 * This base class stores the bus address while child classes
 * may store additional configuration data or even communication states.
 *
 **/
class Function {
public:
  const bus::addr_t address;
  virtual void set_address() const;
  virtual void release_address() const;

  // explicit keyword shall most certainly avoid accidental downcasting
  explicit Function(bus::addr_t address);
};

/**
 * A trigger function exploits the fact that setting an address on
 * the bus triggers an action (TODO: Is that description right?)
 **/
class TriggerFunction : public Function {
public:
  void trigger() const;

  using Function::Function;
};

/**
 * A DataFunction class wraps SPI communication over the digital bus.
 *
 * The communication is configured at constructor level, while the transfer
 * functions basically decorate the Arduino/Teensyduino SPIClass access
 * with lucidac bus adressing.
 **/
class DataFunction : public Function {
protected:
  static SPIClass &get_raw_spi();

public:
  const SPISettings &spi_settings;

  /*
   * Constructor for a new DataFunction.
   * CARE: In my goal to optimize resource usage,
   *       I did not want each instance of DataFunction to hold onto its own SPISettings object,
   *       as e.g. all N ~= 100 coefficients use the same.
   *       Instead, a reference to e.g. a static member variable of the subclass should be passed.
   *       If the constructor is called like DataFunction(address, SPISettings(...)),
   *       the reference to the cleaned-up SPISettings object will be invalid and things will break.
   */
  DataFunction(bus::addr_t address, const SPISettings &spiSettings);

  void begin_communication() const;
  void end_communication() const;

  void transfer(const void *mosi_buf, void *miso_buf, size_t count) const;
  uint8_t transfer(uint8_t data) const;
  uint16_t transfer16(uint16_t data) const;
  uint32_t transfer32(uint32_t data) const;
};

} // namespace functions