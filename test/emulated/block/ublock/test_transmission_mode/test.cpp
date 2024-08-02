// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifdef ANABRID_PEDANTIC
#error "Emulated test cases expect pedantic mode to be disabled."
#endif

#define private public
#define protected public
#include "block/ublock.h"

using namespace blocks;
using namespace fakeit;

UBlock ublock(0, new UBlockHAL_V_1_2_0(0));

void setUp() {
  // This is called before *each* test.
  ArduinoFakeReset();

  // Mock bus::init calls
  When(Method(ArduinoFake(SPI), begin)).AlwaysReturn();
  When(Method(ArduinoFake(Function), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(Function), digitalWrite)).AlwaysReturn();

  // Mock all delay* calls
  When(Method(ArduinoFake(Function), delay)).AlwaysReturn();
  When(Method(ArduinoFake(Function), delayMicroseconds)).AlwaysReturn();
  When(Method(ArduinoFake(Function), delayNanoseconds)).AlwaysReturn();

  // Mock GPIO calls
  When(OverloadedMethod(ArduinoFake(Function), pinMode, void(uint8_t, uint8_t))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(Function), digitalWrite, void(uint8_t, uint8_t))).AlwaysReturn();
  When(Method(ArduinoFake(Function), digitalRead)).AlwaysReturn(0);
}

void tearDown() {
  // This is called after *each* test.
}

void test() {
  // Mock SPI calls
  // Ignore transaction handling
  When(Method(ArduinoFake(SPI), beginTransaction)).AlwaysReturn();
  When(Method(ArduinoFake(SPI), endTransaction)).AlwaysReturn();
  // Ignore transfer16 calls, which here are used only for setting address and triggering things
  When(Method(ArduinoFake(SPI), transfer16)).AlwaysReturn();
  // Sending the transmission modes and reference magnitude uses one transfer8 call
  When(OverloadedMethod(ArduinoFake(SPI), transfer, uint8_t(uint8_t))).AlwaysReturn(0);

  using Transmission_Mode = UBlock::Transmission_Mode;
  using Reference_Magnitude = UBlock::Reference_Magnitude;

  ublock.hardware->write_transmission_modes_and_ref({Transmission_Mode::POS_REF, Transmission_Mode::NEG_REF},
                                                    Reference_Magnitude::ONE_TENTH);
  Verify(OverloadedMethod(ArduinoFake(SPI), transfer, uint8_t(uint8_t)).Using(0b000'10'01'1));

  ublock.hardware->write_transmission_modes_and_ref({Transmission_Mode::ANALOG_INPUT, Transmission_Mode::ANALOG_INPUT},
                                                    Reference_Magnitude::ONE);
  Unverified.Verify(OverloadedMethod(ArduinoFake(SPI), transfer, uint8_t(uint8_t)).Using(0b000'00'00'0));

  ublock.hardware->write_transmission_modes_and_ref({Transmission_Mode::POS_REF, Transmission_Mode::GROUND},
                                                    Reference_Magnitude::ONE_TENTH);
  Unverified.Verify(OverloadedMethod(ArduinoFake(SPI), transfer, uint8_t(uint8_t)).Using(0b000'11'01'1));
}

int main(int argc, char **argv) {
  // The native platform does not use setup/loop functions, but a single main() function
  // When using "debug_test = native/examples/test_debugging" in platformio.ini, the debugger will start here.

  // Call setUp to set up mocking
  setUp();

  // Mock SPI configuration calls
  When(OverloadedMethod(ArduinoFake(SPI), begin, void(void))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(SPI), end, void(void))).AlwaysReturn();
  bus::init();

  UNITY_BEGIN();
  RUN_TEST(test);
  UNITY_END();
}
