// Copyright (c) 2022 anabrid GmbH
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

#include "mode.h"

using namespace mode;

void setUp() {
  // This is called before *each* test.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);
}

void tearDown() {
  // This is called after *each* test.
  digitalWriteFast(LED_BUILTIN, LOW);
}

void test_simple_run() {
  TEST_ASSERT(true);
  unsigned int ic_time = 230, op_time=1000;
  TEST_ASSERT(FlexIOControl::init(ic_time, op_time));
  for (auto i: {0/*,1,2,3,4*/}) {
    FlexIOControl::force_start();
    delayNanoseconds(ic_time+op_time+100);
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_simple_run);
  UNITY_END();
}

void loop() { delay(500); }
