// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <algorithm>

#include "bus/functions.h"

namespace functions {

class EEPROM25AA02 : public DataFunction {
protected:
  static constexpr uint8_t COMMAND_READ = 0b0000'0011;
  static constexpr uint8_t COMMAND_READ_STATUS_REGISTER = 0b0000'0101;
  static constexpr uint8_t COMMAND_WRITE = 0b0000'0010;
  static constexpr uint8_t COMMAND_WRITE_STATUS_REGISTER = 0b0000'0001;
  static constexpr uint8_t COMMAND_WRITE_ENABLE = 0b0000'0110;
  static constexpr uint8_t COMMAND_WRITE_DISABLE = 0b0000'0100;

  static constexpr uint8_t ADRESS_MASK = 0x0f;

  static constexpr size_t MAX_ADDRESS = 255;

  static constexpr bool _is_address_valid(size_t address) { return address <= MAX_ADDRESS; };

public:
  static const SPISettings DEFAULT_SPI_SETTINGS;
  static constexpr uint8_t ADDRESS_UUID = 0xF8;

  class Status {
  private:
    uint8_t data;

  public:
    explicit Status(const uint8_t data_) : data(data_ & 0b0000'1111) {}

    explicit operator uint8_t() const { return data; }

    bool is_write_in_progress() const { return data & 0b0000'0001; };

    bool is_write_enabled() const { return data & 0b0000'0010; };

    bool is_block_zero_protected() const { return data & 0b0000'0100; };

    void set_block_zero_protection(bool on = true) {
      if (on)
        data |= 0b0000'0100;
      else
        data &= ~0b0000'0100;
    }

    bool is_block_one_protected() const { return data & 0b0000'1000; };

    void set_block_one_protection(bool on = true) {
      if (on)
        data |= 0b0000'1000;
      else
        data &= ~0b0000'1000;
    }

    void set_block_protection(bool on = true) {
      set_block_zero_protection(on);
      set_block_one_protection(on);
    }

    void unset_block_protection() { set_block_protection(false); }

    bool is_any_block_protected() const { return is_block_zero_protected() or is_block_one_protected(); }
  };

  explicit EEPROM25AA02(bus::addr_t address) : DataFunction(address, DEFAULT_SPI_SETTINGS) {}

  Status read_status_register() const;
  bool write_status_register(const Status status) const;

  //! Reads the specified number of bytes starting at address into the specified buffer. Returns number of read
  //! bytes on success, 0 on failure.
  size_t read(size_t address, size_t length, uint8_t *buffer) const;
  //! Reads one byte at the specified address into the provided pointer. Returns 0 on failure.
  bool read8(size_t address, uint8_t *data_out) const;
  //! Reads two bytes starting at the specified address into the provided pointer. Returns 0 on failure
  bool read16(size_t address, uint16_t *data_out) const;
  //! Reads four bytes starting at the specified address into the provided pointer. Returns 0 on failure
  bool read32(size_t address, uint32_t *data_out) const;

  //! Writes the specified number of bytes starting at address into the specified buffer. Returns number of
  //! written bytes on success, 0 on failure.
  size_t write(size_t address, size_t length, uint8_t *buffer) const;
  //! Writes one byte at the specified address Returns 0 on failure.
  bool write8(size_t address, uint8_t data) const;
  //! Writes two bytes starting at the specified address Returns 0 on failure.
  bool write16(size_t address, uint16_t data) const;
  //! Writes four bytes starting at the specified address Returns 0 on failure.
  bool write32(size_t address, uint32_t data) const;

protected:
  void set_write_enable() const;
  void unset_write_enable() const;
  bool await_write_complete() const;
};

} // namespace functions
