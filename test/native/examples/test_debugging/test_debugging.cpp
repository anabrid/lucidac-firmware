// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <unity.h>

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_a_test() {
  // This is a simple test
  // Set a breakpoint to debug it.
  bool result = true;
  TEST_ASSERT(result);
}

int main(int argc, char **argv) {
  // The native platform does not use setup/loop functions, but a single main() function
  // When using "debug_test = native/examples/test_debugging" in platformio.ini, the debugger will start here.
  UNITY_BEGIN();
  RUN_TEST(test_a_test);
  UNITY_END();
}
