// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

using namespace fakeit;

void setUp() {
  // This is called before *each* test.
  ArduinoFakeReset();
}

void tearDown() {
  // This is called after *each* test.
}

void test_spi() {
  SPISettings settings(1000000, MSBFIRST, SPI_MODE3);
  uint8_t mosi = 0b10101010;

  When(Method(ArduinoFake(SPI), begin)).AlwaysReturn();
  When(Method(ArduinoFake(SPI), end)).AlwaysReturn();
  When(Method(ArduinoFake(SPI), beginTransaction).Using(settings)).AlwaysReturn();
  When(Method(ArduinoFake(SPI), endTransaction)).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(SPI), transfer, uint8_t(uint8_t)).Using(mosi)).AlwaysReturn(0b00001111);

  SPI.begin();
  SPI.beginTransaction(settings);
  auto miso = SPI.transfer(mosi);
  SPI.endTransaction();
  SPI.end();

  TEST_ASSERT_EQUAL(0b00001111, miso);
  Verify(Method(ArduinoFake(SPI), begin)).Once();
  Verify(Method(ArduinoFake(SPI), end)).Once();
  Verify(Method(ArduinoFake(SPI), beginTransaction)).Once();
  Verify(Method(ArduinoFake(SPI), endTransaction)).Once();
  Verify(OverloadedMethod(ArduinoFake(SPI), transfer, uint8_t(uint8_t))).Once();
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_spi);
  UNITY_END();
}
