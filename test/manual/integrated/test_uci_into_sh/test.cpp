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
  // SET UBLOCK OUTPUT TO +2V
  ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF);

  for (auto output : UBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(ublock.connect(output / 2, output));
  }
  ublock.write_to_hardware();
}

void configure_iblock() {
  // Set I Block to n to n connection
  for (auto output : IBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(iblock.connect(output, output));
  }
  iblock.write_to_hardware();
}

void setup() {
  bus::init();
  pinMode(29, INPUT_PULLUP);

  UNITY_BEGIN();
  // INIT TEST;
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
      return 1; // Timeout
    }
  }
  pressTime = millis();
  if (pressTime - releaseTime < debounceTime) return 0;
  while (!digitalReadFast(29)) {
    if (timeout != 0 && millis() - startTime >= timeout) {
      return 1; // Timeout
    }
  }
  releaseTime = millis();
  unsigned long pressDuration = releaseTime - pressTime;
  if (pressDuration < shortPressTime) {
    return 1;
  } else if (pressDuration >= shortPressTime && pressDuration <= longPressTime) {
    return 2;
  }
  else return 0;
}

void writeOffsetValue() {
    for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * offsetval));
  }
  cblock.write_to_hardware();

  delay(10);
}

void writeTestValue() {
  for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * (testvalue + offsetval)));
    }
  cblock.write_to_hardware();

  delay(10);
}
  
void enterTrackMode() {
  shblock.set_track.trigger();

  //min 1s
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
  // TRIGGER !!!!
  enterInjectMode();  
  writeTestValue();
  wait4Button(60000);  
}

void loop() {
  // CHOSE YOUR OFFSET AND TEST VALUES AND PUT IT IN THE ARRAYS
  float testvalues[] = {0};
  float offsetvals[] = {-1.0f, -0.5f, -0.2f, -0.1f, -0.05f, -0.02f, -0.01f, 0.0f, 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.0f};  
  int countTestvalues = sizeof(testvalues) / sizeof(testvalues[0]);
  int countOffsetvals = sizeof(offsetvals) / sizeof(offsetvals[0]);
  for(int i = 0; (i < countTestvalues && i >= 0); i=(direction +i) % countTestvalues) {
    for(int j = 0; (j < countOffsetvals && j >= 0); j=(direction +j) % countOffsetvals) {
      testvalue = testvalues[i];
      offsetval = offsetvals[j];
      test();
      delay(1000);
    }  
  }    
}
