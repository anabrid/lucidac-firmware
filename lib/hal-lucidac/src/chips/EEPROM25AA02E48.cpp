// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "EEPROM25AA02E64.h"

const SPISettings functions::EEPROM25AA02E64::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE0};

bool functions::EEPROM25AA02E64::read8(size_t byte_offset, uint8_t *data_out) const {
  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(COMMAND_READ);
  spi.transfer(byte_offset);
  auto data = spi.transfer(0);
  end_communication();

  if (data_out)
    *data_out = data;

  // TODO: Implement error handling, though it may be unnecessary for 1-byte reads.
  //       Possible errors are address overflow, ...?
  return true;
}

size_t functions::EEPROM25AA02E64::read(size_t byte_offset, size_t length, uint8_t *buffer) const {
  // Begin SPI communication
  begin_communication();

  auto spi = get_raw_spi();
  // Send READ command B00000011
  spi.transfer(COMMAND_READ);
  // Send byte offset
  spi.transfer(byte_offset);
  // Read data
  spi.transfer(nullptr, buffer, length);

  // Release address and stop SPI transaction
  end_communication();

  return length;
}

void functions::EEPROM25AA02E64::set_write_enable() const {
  // Set write enable by sending 0b0000'0110 and cycling chip select
  // See datasheet for details
  transfer8(COMMAND_WRITE_ENABLE);
}

void functions::EEPROM25AA02E64::unset_write_enable() const {
  // Unset write enable by sending 0b0000'0100 and cycling chep select
  // Possibly unnecessary, datasheet says it is reset after successful write
  // See datasheet for details
  transfer8(COMMAND_WRITE_DISABLE);
}

bool functions::EEPROM25AA02E64::write(size_t byte_offset, size_t length, uint8_t *buffer) const {
  // TODO: The chip only support writing by pages, with a maximum of 16 bytes (see datasheet).
  //       This means we need to check length and byte_offset and potentially split the write.
  //       See https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/issues/94
  //       See https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/issues/96
  // FIXME

  /*
  set_write_enable();

  // Send WRITE command, byte offset and then data
  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(COMMAND_WRITE);
  spi.transfer(byte_offset);
  spi.transfer(buffer, nullptr, length);
  end_communication();

  unset_write_enable();
  return await_write_complete();
  */
  return false;
}

bool functions::EEPROM25AA02E64::write8(size_t byte_offset, uint8_t data) const {
  set_write_enable();

  // Send WRITE command, byte offset and then data
  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(0b0000'0010);
  spi.transfer(byte_offset);
  spi.transfer(data);
  end_communication();

  unset_write_enable();
  // TODO: Implement error handling
  //       - maybe check for address overflow
  //       - add ANABRID_PEDANTIC mode
  //       - in ANABRID_PEDANTIC mode, check for write protection
  return await_write_complete();
}

bool functions::EEPROM25AA02E64::write16(size_t byte_offset, uint16_t data) const {
  set_write_enable();

  // Send WRITE command, byte offset and then data
  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(0b0000'0010);
  spi.transfer(byte_offset);
  spi.transfer16(data);
  end_communication();

  unset_write_enable();
  // TODO: Implement error handling like in write8
  return await_write_complete();
}

functions::EEPROM25AA02E64::Status functions::EEPROM25AA02E64::read_status_register() const {
  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(COMMAND_READ_STATUS_REGISTER);
  auto data = spi.transfer(0);
  end_communication();
  return Status(data);
}

bool functions::EEPROM25AA02E64::await_write_complete() const {
  // According to datasheet, internal write times are up to 5ms,
  // and it's recommended to poll the write_in_progress flag
  delay(1);
  uint8_t tries = 0;
  while (read_status_register().is_write_in_progress()) {
    delay(1);
    if (tries++ > 20)
      return false;
  }
  return true;
}

bool functions::EEPROM25AA02E64::write_status_register(const Status status) const {
  set_write_enable();

  begin_communication();
  auto spi = get_raw_spi();
  spi.transfer(COMMAND_WRITE_STATUS_REGISTER);
  spi.transfer(static_cast<uint8_t>(status) & 0b0000'1100);
  end_communication();

  unset_write_enable();
  return await_write_complete();
}
