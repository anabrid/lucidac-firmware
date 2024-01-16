// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <SPI.h>
#include <array>
#include <core_pins.h>
#include <cstdint>

#include "bus/bus.h"

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

class _old_DataFunction : public Function {
  // TODO: Split in Reading/WritingFunction and ReadWriteFunction
public:
  void begin_communication() const;
  void end_communication() const;

  const SPISettings spi_settings;

  _old_DataFunction(bus::addr_t address, const SPISettings &spiSettings);
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

  DataFunction(bus::addr_t address, const SPISettings &spiSettings);

  void begin_communication() const;
  void end_communication() const;

  void transfer(const void *mosi_buf, void *miso_buf, size_t count);
  uint8_t transfer(uint8_t data) const;
  uint16_t transfer16(uint16_t data) const;
  uint32_t transfer32(uint32_t data) const;
};

} // namespace functions