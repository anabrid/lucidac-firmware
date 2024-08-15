// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifdef ANABRID_PEDANTIC
#error "Emulated test cases expect pedantic mode to be disabled."
#endif

using namespace fakeit;

#include "block/blocks.h"
#include "metadata/74LVC138AD.tpl.hpp"
#include "metadata/metadata.h"

using namespace blocks;
using namespace entities;
using namespace metadata;

MetadataMemory74LVC138AD meta_memory{bus::NULL_ADDRESS};

MetadataMemoryLayoutV1 fake_data{.version = LayoutVersion::V1,
                                 .size = 256,
                                 .classifier{EntityClass::UNKNOWN, 0, Version(0), 0},
                                 .uuid{0, 1, 2, 3, 4, 5, 6, 7}};

void setUp() {
  // This is called before *each* test.
  // Reset all mocking instances
  ArduinoFakeReset();

  // Allow all delay functions as no-ops
  When(Method(ArduinoFake(Function), delay)).AlwaysReturn();
  When(Method(ArduinoFake(Function), delayMicroseconds)).AlwaysReturn();
  When(Method(ArduinoFake(Function), delayNanoseconds)).AlwaysReturn();

  // Allow all GPIO access as no-ops
  When(OverloadedMethod(ArduinoFake(Function), pinMode, void(uint8_t, uint8_t))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(Function), digitalWrite, void(uint8_t, uint8_t))).AlwaysReturn();

  // Allow SPI configuration as no-ops
  When(Method(ArduinoFake(SPI), begin)).AlwaysReturn();
  When(Method(ArduinoFake(SPI), end)).AlwaysReturn();
  When(Method(ArduinoFake(SPI), beginTransaction)).AlwaysReturn();
  When(Method(ArduinoFake(SPI), endTransaction)).AlwaysReturn();
}

void tearDown() {
  // This is called after *each* test.
}

void test_class_properties() {
  TEST_ASSERT_EQUAL(6, sizeof(EntityClassifier));
  TEST_ASSERT_EQUAL(256, sizeof(MetadataMemoryLayoutV1));
}

void test_readout() {
  // Underlying DataFunction::begin/end_communication does two transfer16
  // read_from_hardware does two 8bit transfers (command, offset) and then reads
  When(Method(ArduinoFake(SPI), transfer16)).Return(0, 0);
  When(OverloadedMethod(ArduinoFake(SPI), transfer, uint8_t(uint8_t))).Return(0, 0);
  When(OverloadedMethod(ArduinoFake(SPI), transfer, void(const void *, void *, size_t)))
      .Do([](auto mosi, auto miso, size_t count) { memcpy(miso, &fake_data, count); });
  meta_memory.read_from_hardware();

  auto read_data = meta_memory.as<MetadataMemoryLayoutV1>();
  TEST_ASSERT_EQUAL_UINT8_ARRAY(fake_data.uuid, read_data.uuid, sizeof(fake_data.uuid));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_class_properties);
  RUN_TEST(test_readout);
  UNITY_END();
}
