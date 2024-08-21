// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "net/settings.h"
#include "nvmconfig/vendor.h"

void to_be_written(nvmconfig::VendorOTP& conf) {
    conf.serial_number = 1234;
    conf.serial_uuid.fromString("acdfeab1-cc87-4a92-91bb-1fb0d81b3f43");
    conf.default_admin_password = "MaFi2ief";
};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

// This function was only for debugging
void info_persistent_writer() {
  Serial.println("info_persistent_writer()");
  int i=0;
  for(auto const& sys : nvmconfig::PersistentSettingsWriter::get().subsystems) {
    Serial.printf("Pers.Subsystem[%s] = ", sys->name().c_str());
    StaticJsonDocument<2000> doc;
    sys->toJson(doc.to<JsonObject>());
    serializeJson(doc, Serial);
    Serial.println("");
  }
}

// This function was only for debugging
void test_dump_eeprom() {
  Serial.println("test_dump_eeprom:");
  StaticJsonDocument<2000> serialized_conf;
  auto foo = serialized_conf.to<JsonObject>();
  nvmconfig::PersistentSettingsWriter::get().toJson(foo);
  serializeJsonPretty(foo, Serial);
  Serial.println(""); // add newline
}

void test_prepare_vendorflash() {
  auto& persistent = nvmconfig::PersistentSettingsWriter::get();
  auto& vendor = nvmconfig::VendorOTP::get();
  persistent.read_from_eeprom(); // at first run, this will result in default values
  test_dump_eeprom();
  TEST_ASSERT(!vendor.is_valid()); // This test is for being written once.
  to_be_written(vendor);
  test_dump_eeprom();
  info_persistent_writer();
  persistent.write_to_eeprom();
}

void test_read_vendorflash() {
  nvmconfig::VendorOTP a;
  to_be_written(a);

  nvmconfig::PersistentSettingsWriter::get().read_from_eeprom();
  auto& b = nvmconfig::VendorOTP::get();

  TEST_ASSERT(a.serial_number == b.serial_number);
  TEST_ASSERT(a.serial_uuid == b.serial_uuid);
  TEST_ASSERT(a.default_admin_password == b.default_admin_password);

  test_dump_eeprom();
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("test_write_config startup");
  net::register_settings();
  info_persistent_writer();

  UNITY_BEGIN();
  RUN_TEST(test_prepare_vendorflash);
  RUN_TEST(test_read_vendorflash);
  UNITY_END();
}

void loop() { delay(100); }
