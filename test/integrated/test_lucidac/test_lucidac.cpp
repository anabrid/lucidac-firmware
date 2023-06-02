// Copyright (c) 2023 anabrid GmbH
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

#include "lucidac.h"
#include "daq.h"

using namespace lucidac;
using namespace daq;

LUCIDAC luci{};
OneshotDAQ daq_{};

void setUp() {
  // set stuff up here
  luci.init();
  daq_.init(0);
}

void tearDown() {
  // clean stuff up here
}

void test_lucidac() {
  TEST_ASSERT_EQUAL(0b00000'0011, luci.ublock->get_block_address());
}

void test_calibrate() {
  luci.calibrate(&daq_);
  while (true) {
    auto data = daq_.sample_raw();
    TEST_MESSAGE("X =|################|, bisher fehlt ein bit");
    for (unsigned int i_ = 0; i_ < data.size(); i_++) {
      char buffer[4 + 33] = {' ', ' ', '=', ' '};
      buffer[0] = '0' + i_;
      itoa(data[i_], buffer + 4, 2);
      TEST_MESSAGE(buffer);
      itoa(data[i_], buffer + 4, 10);
      TEST_MESSAGE(buffer);
    }
    delay(1000);
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_lucidac);
  RUN_TEST(test_calibrate);
  UNITY_END();
}

void loop() {}
