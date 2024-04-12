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

#include "block/ublock.h"

using namespace blocks;

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_block() {
  UBlock ublock(0);

  TEST_ASSERT(ublock.connect(0, 0));
  TEST_ASSERT(ublock.connect(1, 1));
  TEST_ASSERT(ublock.connect(2, 2));
  TEST_ASSERT(ublock.connect(3, 3));
  TEST_ASSERT(ublock.connect(4, 4));
  TEST_ASSERT(ublock.connect(5, 5));
  TEST_ASSERT(ublock.connect(6, 6));
  TEST_ASSERT(ublock.connect(7, 7));
  TEST_ASSERT(ublock.connect(8, 8));
  TEST_ASSERT(ublock.connect(9, 9));
  TEST_ASSERT(ublock.connect(10, 10));
  TEST_ASSERT(ublock.connect(11, 11));
  TEST_ASSERT(ublock.connect(12, 12));
  TEST_ASSERT(ublock.connect(13, 13));
  TEST_ASSERT(ublock.connect(14, 14));
  TEST_ASSERT(ublock.connect(15, 15));

  TEST_ASSERT(ublock.connect(0, 16));
  TEST_ASSERT(ublock.connect(1, 17));
  TEST_ASSERT(ublock.connect(2, 18));
  TEST_ASSERT(ublock.connect(3, 19));
  TEST_ASSERT(ublock.connect(4, 20));
  TEST_ASSERT(ublock.connect(5, 21));
  TEST_ASSERT(ublock.connect(6, 22));
  TEST_ASSERT(ublock.connect(7, 23));
  TEST_ASSERT(ublock.connect(8, 24));
  TEST_ASSERT(ublock.connect(9, 25));
  TEST_ASSERT(ublock.connect(10, 26));
  TEST_ASSERT(ublock.connect(11, 27));
  TEST_ASSERT(ublock.connect(12, 28));
  TEST_ASSERT(ublock.connect(13, 29));
  TEST_ASSERT(ublock.connect(14, 30));
  TEST_ASSERT(ublock.connect(15, 31));

  ublock.write_to_hardware();

  TEST_ASSERT(ublock.is_connected(0, 0));
  TEST_ASSERT(ublock.is_connected(1, 1));
  TEST_ASSERT(ublock.is_connected(2, 2));
  TEST_ASSERT(ublock.is_connected(3, 3));
  TEST_ASSERT(ublock.is_connected(4, 4));
  TEST_ASSERT(ublock.is_connected(5, 5));
  TEST_ASSERT(ublock.is_connected(6, 6));
  TEST_ASSERT(ublock.is_connected(7, 7));
  TEST_ASSERT(ublock.is_connected(8, 8));
  TEST_ASSERT(ublock.is_connected(9, 9));
  TEST_ASSERT(ublock.is_connected(10, 10));
  TEST_ASSERT(ublock.is_connected(11, 11));
  TEST_ASSERT(ublock.is_connected(12, 12));
  TEST_ASSERT(ublock.is_connected(13, 13));
  TEST_ASSERT(ublock.is_connected(14, 14));
  TEST_ASSERT(ublock.is_connected(15, 15));

  TEST_ASSERT(ublock.is_connected(0, 16));
  TEST_ASSERT(ublock.is_connected(1, 17));
  TEST_ASSERT(ublock.is_connected(2, 18));
  TEST_ASSERT(ublock.is_connected(3, 19));
  TEST_ASSERT(ublock.is_connected(4, 20));
  TEST_ASSERT(ublock.is_connected(5, 21));
  TEST_ASSERT(ublock.is_connected(6, 22));
  TEST_ASSERT(ublock.is_connected(7, 23));
  TEST_ASSERT(ublock.is_connected(8, 24));
  TEST_ASSERT(ublock.is_connected(9, 25));
  TEST_ASSERT(ublock.is_connected(10, 26));
  TEST_ASSERT(ublock.is_connected(11, 27));
  TEST_ASSERT(ublock.is_connected(12, 28));
  TEST_ASSERT(ublock.is_connected(13, 29));
  TEST_ASSERT(ublock.is_connected(14, 30));
  TEST_ASSERT(ublock.is_connected(15, 31));

  ublock.write_to_hardware();

  TEST_ASSERT(ublock.disconnect(0, 0));
  TEST_ASSERT(ublock.disconnect(1, 1));
  TEST_ASSERT(ublock.disconnect(2, 2));
  TEST_ASSERT(ublock.disconnect(3, 3));
  TEST_ASSERT(ublock.disconnect(4, 4));
  TEST_ASSERT(ublock.disconnect(5, 5));
  TEST_ASSERT(ublock.disconnect(6, 6));
  TEST_ASSERT(ublock.disconnect(7, 7));
  TEST_ASSERT(ublock.disconnect(8, 8));
  TEST_ASSERT(ublock.disconnect(9, 9));
  TEST_ASSERT(ublock.disconnect(10, 10));
  TEST_ASSERT(ublock.disconnect(11, 11));
  TEST_ASSERT(ublock.disconnect(12, 12));
  TEST_ASSERT(ublock.disconnect(13, 13));
  TEST_ASSERT(ublock.disconnect(14, 14));
  TEST_ASSERT(ublock.disconnect(15, 15));

  TEST_ASSERT(ublock.disconnect(0, 16));
  TEST_ASSERT(ublock.disconnect(1, 17));
  TEST_ASSERT(ublock.disconnect(2, 18));
  TEST_ASSERT(ublock.disconnect(3, 19));
  TEST_ASSERT(ublock.disconnect(4, 20));
  TEST_ASSERT(ublock.disconnect(5, 21));
  TEST_ASSERT(ublock.disconnect(6, 22));
  TEST_ASSERT(ublock.disconnect(7, 23));
  TEST_ASSERT(ublock.disconnect(8, 24));
  TEST_ASSERT(ublock.disconnect(9, 25));
  TEST_ASSERT(ublock.disconnect(10, 26));
  TEST_ASSERT(ublock.disconnect(11, 27));
  TEST_ASSERT(ublock.disconnect(12, 28));
  TEST_ASSERT(ublock.disconnect(13, 29));
  TEST_ASSERT(ublock.disconnect(14, 30));
  TEST_ASSERT(ublock.disconnect(15, 31));

  ublock.write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_block);
  UNITY_END();
}

void loop() {}
