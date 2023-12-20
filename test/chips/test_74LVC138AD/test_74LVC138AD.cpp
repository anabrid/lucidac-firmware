// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "74LVC138AD.tpl.hpp"

MetadataMemory74LVC138AD chip{bus::board_function_to_addr(0)};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_read_eui64() {
  std::array<uint8_t, 8> eui64_is{1,1,1,1,1,1,1,1};
  // For development carrier board I use
  std::array<uint8_t, 8> eui64_should{0x00, 0x04, 0xA3, 0x0B, 0x00, 0x14, 0x6F, 0xD5};

  // Read eui64
  chip.read_from_hardware(offsetof(metadata::MetadataMemoryLayoutV1, eui64),
                          sizeof(metadata::MetadataMemoryLayoutV1::eui64), eui64_is.data());

  // Check eui64
  TEST_ASSERT_EQUAL_UINT8_ARRAY(eui64_should.data(), eui64_is.data(), eui64_should.size());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_read_eui64);
  UNITY_END();
}

void loop() {
}