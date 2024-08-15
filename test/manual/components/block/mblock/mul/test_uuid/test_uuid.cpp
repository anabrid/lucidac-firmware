// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

/*
 * ASSUMPTIONS
 *  - MUL block is inserted into M1 slot
 */

#include <Arduino.h>
#include <unity.h>

#include "metadata/74LVC138AD.tpl.hpp"

MetadataMemory74LVC138AD chip{bus::idx_to_addr(0, bus::M1_BLOCK_IDX, 0)};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_uuid_address() {
  // M1_BLOCk_IDX is 4, but block idx start at 1, so address is 5.
  TEST_ASSERT_EQUAL(0b000000'0101, chip.address);
}

void test_read_uuid() {
  std::array<uint8_t, 8> uuid_is{1, 1, 1, 1, 1, 1, 1, 1};
  // For development carrier board I use
  std::array<uint8_t, 8> uuid_should{0, 0b00000100, 0b10100011, 0b00001011,
                                     0, 0b00010100, 0b01110000, 0b11111101};

  // Read UUID
  chip.read_from_hardware(offsetof(metadata::MetadataMemoryLayoutV1, uuid),
                          sizeof(metadata::MetadataMemoryLayoutV1::uuid), uuid_is.data());

  // Check UUID
  TEST_ASSERT_EQUAL_UINT8_ARRAY(uuid_should.data(), uuid_is.data(), uuid_should.size());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_uuid_address);
  RUN_TEST(test_read_uuid);
  UNITY_END();
}

void loop() {}
