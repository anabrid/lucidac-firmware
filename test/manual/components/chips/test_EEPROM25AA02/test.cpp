// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define private public
#define protected public
#include "chips/EEPROM25AA02.h"
#include "metadata/metadata.h"

using namespace functions;
using namespace metadata;

//! Change this arbitrary seed for reliable test results. If the seed stayes the same during multiple test
//! cycles, a meaningful test result can not be guaranteed.
const int seed = 4;

// This reads the chip on the U-Block as an example
EEPROM25AA02 chip{bus::idx_to_addr(0, bus::U_BLOCK_IDX, bus::METADATA_FUNC_IDX)};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_metadata_offset() {
  TEST_ASSERT_EQUAL(EEPROM25AA02::ADDRESS_UUID, offsetof(MetadataMemoryLayoutV1, uuid));
}

void test_status_register_class() {
  EEPROM25AA02::Status status_all{0b1111'1111};
  TEST_ASSERT_EQUAL(0b0000'1111, static_cast<uint8_t>(status_all));
  TEST_ASSERT(status_all.is_write_in_progress());
  TEST_ASSERT(status_all.is_write_enabled());
  TEST_ASSERT(status_all.is_block_zero_protected());
  TEST_ASSERT(status_all.is_block_one_protected());

  EEPROM25AA02::Status status_some{0b0000'1001};
  TEST_ASSERT(status_some.is_write_in_progress());
  TEST_ASSERT_FALSE(status_some.is_write_enabled());
  TEST_ASSERT_FALSE(status_some.is_block_zero_protected());
  TEST_ASSERT(status_some.is_block_one_protected());
  TEST_ASSERT_BITS_HIGH(0b0000'1001, static_cast<uint8_t>(status_some));

  status_some.set_block_zero_protection();
  status_some.set_block_one_protection(false);
  TEST_ASSERT(status_some.is_block_zero_protected());
  TEST_ASSERT_FALSE(status_some.is_block_one_protected());
  TEST_ASSERT_BITS_HIGH(0b0000'0101, static_cast<uint8_t>(status_some));

  status_some.set_block_zero_protection(false);
  status_some.set_block_one_protection(true);
  TEST_ASSERT_FALSE(status_some.is_block_zero_protected());
  TEST_ASSERT(status_some.is_block_one_protected());
  TEST_ASSERT_BITS_HIGH(0b0000'1001, static_cast<uint8_t>(status_some));
}

void test_status_register_manipulations() {
  auto status = chip.read_status_register();
  TEST_ASSERT_FALSE(status.is_write_enabled());
  TEST_ASSERT_FALSE(status.is_write_in_progress());

  status.set_block_protection();
  TEST_ASSERT(chip.write_status_register(status));
  delay(10);
  auto new_status = chip.read_status_register();
  Serial.print("Status: ");
  Serial.println(new_status.data, BIN);
  TEST_ASSERT_FALSE(new_status.is_write_in_progress());
  TEST_ASSERT_TRUE(new_status.is_block_zero_protected());
  TEST_ASSERT_TRUE(new_status.is_block_one_protected());

  new_status.unset_block_protection();
  TEST_ASSERT(chip.write_status_register(new_status));
  auto newer_status = chip.read_status_register();
  TEST_ASSERT_FALSE(newer_status.is_any_block_protected());
  TEST_ASSERT_FALSE(newer_status.is_write_in_progress());
}

void test_read_uuid() {
  // Read UUID
  std::array<uint8_t, 8> uuid{0, 0, 0, 0, 0, 0, 0, 0};
  chip.read(EEPROM25AA02::ADDRESS_UUID, uuid.size(), uuid.data());

  // Check UUID
  // The uuid is different in each chip obviously, so we can only check certain aspects.
  // The first three bytes are the fixed Organizationally Unique Identifier (OUI).
  // This may change for future production series though.
  std::array<uint8_t, 3> oui{0, 0x04, 0xA3};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(oui.data(), uuid.data(), oui.size());
  // For all chips I saw until now, uuid[4] and uuid[5] were equal as well.
  // This may not be true though
  TEST_ASSERT_EQUAL(0, uuid[4]);
  TEST_ASSERT_EQUAL(0b00010100, uuid[5]);
  // uuid[6] & uuid[7] should probably not be zero, though that's not certain
  TEST_ASSERT_NOT_EQUAL(0, uuid[6]);
  TEST_ASSERT_NOT_EQUAL(0, uuid[7]);
}

void test_write8_and_read() {
  size_t offset = offsetof(MetadataMemoryLayoutV1, entity_data);

  uint8_t test_value = rand();
  TEST_ASSERT(chip.write8(offset, test_value));

  uint8_t data_readback;

  TEST_ASSERT(chip.read8(offset, &data_readback));
  TEST_ASSERT_EQUAL(test_value, data_readback);
}

void test_write16_and_read() {
  size_t offset = offsetof(MetadataMemoryLayoutV1, entity_data) + 1;

  uint16_t test_value = rand();
  TEST_ASSERT(chip.write16(offset, test_value));

  uint16_t data_readback;
  TEST_ASSERT(chip.read16(offset, &data_readback));

  TEST_ASSERT_EQUAL(test_value, data_readback);
}

void test_write32_and_read() {
  size_t offset = offsetof(MetadataMemoryLayoutV1, entity_data) + 3;

  uint32_t test_value = rand();
  TEST_ASSERT(chip.write32(offset, test_value));

  uint32_t data_readback;
  TEST_ASSERT(chip.read32(offset, &data_readback));

  TEST_ASSERT_EQUAL(test_value, data_readback);
}

void test_write_and_read() {
  size_t offset = offsetof(MetadataMemoryLayoutV1, entity_data) + 7;

  uint8_t test_data[35];
  for (int i = 0; i < 35; i++)
    test_data[i] = rand();

  uint8_t read_data[35];

  TEST_ASSERT(chip.write(offset, 35, test_data));
  TEST_ASSERT(chip.read(offset, 35, read_data) == 35);
  TEST_ASSERT(memcmp(test_data, read_data, 35) == 0);
}

#ifdef ARDUINO_TEENSY41
void setup() {
  bus::init();
#else
int main() {
#endif

  srand(seed);

  UNITY_BEGIN();

  RUN_TEST(test_status_register_class);
  RUN_TEST(test_status_register_manipulations);
  RUN_TEST(test_read_uuid);

  RUN_TEST(test_write8_and_read);
  RUN_TEST(test_write16_and_read);
  RUN_TEST(test_write32_and_read);

  RUN_TEST(test_write_and_read);

  UNITY_END();
}

void loop() {}
