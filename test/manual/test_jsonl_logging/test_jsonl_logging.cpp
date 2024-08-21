// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "protocol/jsonl_logging.h"

void setup_logging() {
  msg::Log::get().sinks.add(&Serial);
  msg::Log::get().sinks.add(&msg::StartupLog::get());
  //TEST_ASSERT(msg::Log::get().sinks.targets.size() == 2);
}

void direct_streamlogger() {
  msg::StreamLogger log(Serial);
  log.println("This is a direct_streamlogger testline");
  log.println("And another one");
}

void direct_startuplog() {
  msg::StartupLog::get().println("This is a logline");
  msg::StartupLog::get().println("This is another line");
  msg::StartupLog::get().print("This is another\nand another\n");
}

void print_multiplexing() {
  utils::PrintMultiplexer mul;
  mul.add(&Serial);
  mul.add(&Serial);
  mul.println("This comes two times");
  mul.println("As does this");
}

void log_central() {
  msg::Log::get().println("This is a logline");
  msg::Log::get().println("This is another line");
  msg::Log::get().print("This is another\nand another\n");
}

void recap_logging() {
  TEST_ASSERT(msg::StartupLog::get().buf.data().size() >= 1);
  TEST_MESSAGE("Now printing the current log buffer to Serial:");
  msg::StartupLog::get().stream_to_json(Serial);
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  UNITY_BEGIN();
  RUN_TEST(setup_logging);
  RUN_TEST(direct_startuplog);
  RUN_TEST(recap_logging);
  RUN_TEST(direct_streamlogger);
  RUN_TEST(print_multiplexing);
  RUN_TEST(log_central);
  RUN_TEST(recap_logging);
  UNITY_END();
}

void loop() { delay(100); }