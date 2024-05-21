// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define private public
#define protected public
#include "block/blocks.h"

using namespace blocks;

auto start=millis();
static float offsetval;
static float testvalue;
int direction = 1;
const unsigned long debounceTime = 40;
const unsigned long shortPressTime = 500;
const unsigned long longPressTime = 5000;

UBlock ublock{0};
CBlock cblock{0};
IBlock iblock{0};
SHBlock shblock{0};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void configure_ublock() {
  // SET UBLOCK OUTPUT TO +0.2V
  ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF);

  for (auto output : UBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(ublock.connect(output / 2, output));
  }
  TEST_ASSERT(ublock.write_to_hardware());
}

void configure_iblock() {
  // SET IBLOCK TO N TO N CONNECTION
  for (auto output : IBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(iblock.connect(output, output));
  }
  TEST_ASSERT(iblock.write_to_hardware());
}

void setup() {
  bus::init();
  pinMode(29, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, LOW);
  Serial.begin(9600);
  for(int k = 0; k < 20; k++) {
    digitalToggleFast(LED_BUILTIN);
    delay(50);
  }

 // INIT TEST;
  UNITY_BEGIN();
  RUN_TEST(configure_ublock);
  RUN_TEST(configure_iblock);
  shblock.set_track.trigger();
  UNITY_END();
}

int readButton(unsigned long timeout) {
  static unsigned long pressTime;
  static unsigned long releaseTime;
  unsigned long startTime = millis();
  while (digitalReadFast(29)) {
    if (timeout != 0 && millis() - startTime >= timeout) {
      return 1; // Timeout is like short button
    }
  }
  pressTime = millis();
  if (pressTime - releaseTime < debounceTime) return 0;
  while (!digitalReadFast(29)) {
    if (timeout != 0 && millis() - startTime >= timeout) {
      return 1; // Timeout is like short button
    }
  }
  releaseTime = millis();
  unsigned long pressDuration = releaseTime - pressTime;
  if (pressDuration < shortPressTime) {
    return 1; // short button
  } else if (pressDuration >= shortPressTime && pressDuration <= longPressTime) {
    return 2; // long button
  }
  else return 0; // ignore tooo long buttons
}

void writeOffsetValue() {
    for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * offsetval));
  }
  TEST_ASSERT(cblock.write_to_hardware());

  delay(10);
}

void writeTestValue() {
  for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * (testvalue + offsetval)));
    }
  TEST_ASSERT(cblock.write_to_hardware());

  delay(10);
}

void enterTrackMode() {
  shblock.set_track.trigger();

  // MIN 1sec
  delay(1000);
}

void enterInjectMode() {
  shblock.set_inject.trigger();

  delay(10);
}

void wait4Button(unsigned long timeout) {
  int buttonState = 0;
  while (buttonState == 0) {
    buttonState = readButton(timeout);
  }
  if (buttonState == 2) direction = -direction;
}

void test() {
  writeOffsetValue();
  enterTrackMode();
  wait4Button(0);
  // TRIGGER AND SHOW IT!!!!
  digitalWriteFast(LED_BUILTIN, HIGH);
  enterInjectMode();
  writeTestValue();
  wait4Button(60000);
  digitalWriteFast(LED_BUILTIN, LOW);
}

void loop() {
  // CHOSE YOUR OFFSET AND TEST VALUES AND PUT IT IN THE ARRAYS
  float offsetvals[] = {-0.5f, 0, 0.5f};
  float testvalues[] = {-0.01f, -0.0033f, 0, 0.0033f, 0.01f};
  int countOffsetvals = sizeof(offsetvals) / sizeof(offsetvals[0]);
  int countTestvalues = sizeof(testvalues) / sizeof(testvalues[0]);
  // 2 NESTED BIDIRECTIONAL LOOPS (PRESS LONG 4 CHANGE DIRECTION)
  for(int i = direction > 0 ? 0 : countOffsetvals - 1; (i < countOffsetvals && i >= 0); i += direction) {
    for(int j = direction > 0 ? 0 : countTestvalues - 1; (j < countTestvalues && j >= 0); j += direction) {
      offsetval = offsetvals[i];
      testvalue = testvalues[j];
      Serial.print("offsetval for next test = ");
      Serial.println(offsetval, 4);
      Serial.print("testvalue for next test = ");
      Serial.println(testvalue, 4);
      Serial.print("direction = ");
      Serial.println(direction);
      Serial.println();

      test();
      delay(1000);
    }
  }
}
