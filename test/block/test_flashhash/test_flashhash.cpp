// Copyright (c) 2022 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/


#include <Arduino.h>
#include <unity.h>

#include "flashhash.h"

void test_flashhash() {
    Serial.begin(512000);
    Serial.println("Running FlashHash test once");
    compute_flash_crc32();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_flashhash);
  UNITY_END();
}


void loop() {
    delay(500);
}