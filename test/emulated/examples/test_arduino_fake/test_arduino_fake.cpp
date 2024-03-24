// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

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
