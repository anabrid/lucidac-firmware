// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <iostream>

#include <Arduino.h>
#include <unity.h>

#include "versions.h"

using namespace entities;

void test() {
  entities::Version v1{1, 0, 0}, v2{1, 0, 1};
  TEST_ASSERT((Version{1, 3, 2} == Version{1, 3, 2}));
  TEST_ASSERT((Version{1, 3, 2} <= Version{1, 3, 2}));
  TEST_ASSERT((Version{1, 2, 2} != Version{1, 3, 2}));

  TEST_ASSERT((Version{1, 3, 2} > Version{1, 3, 1}));
  TEST_ASSERT_FALSE((Version{1, 3, 2} < Version{1, 3, 1}));

  TEST_ASSERT((Version{1, 3, 2} >= Version{1, 3, 1}));
  TEST_ASSERT_FALSE((Version{1, 3, 2} <= Version{1, 3, 1}));

  TEST_ASSERT((Version{2, 3, 2} > Version{1, 30, 10}));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test);
  UNITY_END();
}
