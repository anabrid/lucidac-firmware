// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <unity.h>

#ifdef ANABRID_PEDANTIC
#error "Emulated test cases expect pedantic mode to be disabled."
#endif

#include "bus/bus.h"

using namespace bus;
using namespace fakeit;

void setUp() {
  // This is called before *each* test.
  ArduinoFakeReset();

  // mock all delay* calls
  When(Method(ArduinoFake(Function), delay)).AlwaysReturn();
  When(Method(ArduinoFake(Function), delayMicroseconds)).AlwaysReturn();
  When(Method(ArduinoFake(Function), delayNanoseconds)).AlwaysReturn();

  // mock SPI configuration calls
  When(OverloadedMethod(ArduinoFake(SPI), begin, void(void))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(SPI), end, void(void))).AlwaysReturn();

  // mock GPIO calls
  When(OverloadedMethod(ArduinoFake(Function), pinMode, void(uint8_t, uint8_t))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(Function), digitalWrite, void(uint8_t, uint8_t))).AlwaysReturn();

  bus::init();
}

void tearDown() {
  // This is called after *each* test.
}

void test_address_via_shift_register() {
  SPISettings settings(4'000'000, MSBFIRST, SPI_MODE2);
  When(Method(ArduinoFake(SPI), beginTransaction).Using(settings)).Return();
  When(Method(ArduinoFake(SPI), transfer16)).Return();
  When(Method(ArduinoFake(SPI), endTransaction)).Return();

  bus::address_function(1, 3, 7);

  Verify(Method(ArduinoFake(SPI), transfer16).Using(0b00000111'00010011)).Once();
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_address_via_shift_register);
  UNITY_END();
}
