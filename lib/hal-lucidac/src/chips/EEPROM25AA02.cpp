// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "EEPROM25AA02.h"

const SPISettings functions::EEPROM25AA02::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE0};

void functions::EEPROM25AA02::set_write_enable() const {
  // Set write enable by sending 0b0000'0110 and cycling chip select
  // See datasheet for details
  transfer8(COMMAND_WRITE_ENABLE);
}

void functions::EEPROM25AA02::unset_write_enable() const {
  // Unset write enable by sending 0b0000'0100 and cycling chip select
  // Possibly unnecessary, datasheet says it is reset after successful write
  // See datasheet for details
  transfer8(COMMAND_WRITE_DISABLE);
}

size_t functions::EEPROM25AA02::read(size_t address, size_t length, uint8_t *buffer) const {
  if (!_is_address_valid(address) || !_is_address_valid(address + length - 1))
    return false;

  // Begin SPI communication
  begin_communication();

  auto spi = get_raw_spi();
  // Send READ command B00000011
  spi.transfer(COMMAND_READ);
  // Send byte offset / address
  spi.transfer(address);
  // Read data
  spi.transfer(nullptr, buffer, length);

  // Release address and stop SPI transaction
  end_communication();

  return length;
}

bool functions::EEPROM25AA02::read8(size_t address, uint8_t *data_out) const {
  return read(address, 1, data_out) == 1;
}

bool functions::EEPROM25AA02::read16(size_t address, uint16_t *data_out) const {
  return read(address, 2, (uint8_t *)data_out) == 2;
}

bool functions::EEPROM25AA02::read32(size_t address, uint32_t *data_out) const {
  return read(address, 4, (uint8_t *)data_out) == 4;
}

size_t functions::EEPROM25AA02::write(size_t address, size_t length, const uint8_t *buffer) const {
  set_write_enable();

#ifdef ANABRID_PEDANTIC
  auto read_status = read_status_register();
  if (!read_status.is_write_enabled())
    return false;
#endif

  begin_communication();
  auto spi = get_raw_spi();

  // Check if the last byte of the buffer can fit in the EEPROM
  const auto last_byte = address + length - 1;
  if (!_is_address_valid(last_byte))
    return false;

  auto page_start = address;
  auto page_end = std::min(
      last_byte,
      page_start | ADRESS_MASK); // Write till we hit the end of the current page or the end of the buffer

  while (true) {
    set_write_enable();
    begin_communication();

    // Write current page to chip
    spi.transfer(COMMAND_WRITE);
    spi.transfer(page_start);
    spi.transfer(buffer + page_start - address, nullptr, page_end - page_start + 1);

    end_communication();

    // Move to the next page
    page_start = page_end + 1;
    if (page_start > last_byte)
      break; // The whole buffer was written and we are done

    page_end = std::min(
        last_byte,
        page_start | ADRESS_MASK); // Write till we hit the end of the current page or the end of the buffer

    // Wait for chip to handle input, before we write more data. Abort write if we timeout
    if (!await_write_complete())
      return false;
  }

  if (!await_write_complete())
    return false;

#ifdef ANABRID_PEDANTIC
  uint8_t written_data[length];

  if (!read(address, length, written_data))
    return false;
  return memcmp(written_data, buffer, length) == 0;
#endif

  return length;
}

bool functions::EEPROM25AA02::write8(size_t address, uint8_t data) const {
  set_write_enable();

#ifdef ANABRID_PEDANTIC
  auto read_status = read_status_register();
  if (!read_status.is_write_enabled())
    return false;
#endif
  // Send WRITE command, byte offset and then data
  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(COMMAND_WRITE);
  spi.transfer(address);
  spi.transfer(data);
  end_communication();

  if (!await_write_complete())
    return false;

#ifdef ANABRID_PEDANTIC
  uint8_t written_data;
  if (!read8(address, &written_data))
    return false;
  return written_data == data;
#endif
  return true;
}

bool functions::EEPROM25AA02::write16(size_t address, uint16_t data) const {
  uint8_t buffer[2];
  *(uint16_t *)buffer = data;
  return write(address, 2, buffer);
}

bool functions::EEPROM25AA02::write32(size_t address, uint32_t data) const {
  uint8_t buffer[4];
  *(uint32_t *)buffer = data;
  return write(address, 4, buffer);
}

functions::EEPROM25AA02::Status functions::EEPROM25AA02::read_status_register() const {
  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(COMMAND_READ_STATUS_REGISTER);
  auto data = spi.transfer(0);
  end_communication();
  return Status(data);
}

bool functions::EEPROM25AA02::await_write_complete() const {
  // According to datasheet, internal write times are up to 5ms,
  // and it's recommended to poll the write_in_progress flag
  uint8_t tries = 0;
  do {
    delay(1);
    if (tries++ > 20)
      return false;
  } while (read_status_register().is_write_in_progress());
  return true;
}

bool functions::EEPROM25AA02::write_status_register(const Status status) const {
  set_write_enable();

  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(COMMAND_WRITE_STATUS_REGISTER);
  spi.transfer(static_cast<uint8_t>(status) & 0b0000'1100);
  end_communication();

  return await_write_complete();
}
