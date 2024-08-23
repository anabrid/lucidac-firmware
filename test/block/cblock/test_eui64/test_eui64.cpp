// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "metadata/74LVC138AD.tpl.hpp"

MetadataMemory74LVC138AD chip{bus::idx_to_addr(0, bus::C_BLOCK_IDX, bus::METADATA_FUNC_IDX)};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_eui64_address() {
  // C_BLOCK_IDX is 1, but block idx start at 1, so address is 2.
  TEST_ASSERT_EQUAL(0b000000'0010, chip.address);
}

void test_read_eui64() {
  std::array<uint8_t, 8> eui64_is{1, 1, 1, 1, 1, 1, 1, 1};
  // For development carrier board I use
  std::array<uint8_t, 8> eui64_should{0, 0b00000100, 0b10100011, 0b00001011,
                                     0, 0b00010100, 0b01110011, 0b10000000};

  // Read eui64
  chip.read_from_hardware(offsetof(metadata::MetadataMemoryLayoutV1, eui64),
                          sizeof(metadata::MetadataMemoryLayoutV1::eui64), eui64_is.data());

  // Check eui64
  TEST_ASSERT_EQUAL_UINT8_ARRAY(eui64_should.data(), eui64_is.data(), eui64_should.size());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_eui64_address);
  RUN_TEST(test_read_eui64);
  UNITY_END();
}

void loop() {}