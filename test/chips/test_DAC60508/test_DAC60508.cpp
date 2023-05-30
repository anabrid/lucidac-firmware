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

#define protected public
#include "DAC60508.h"
#include "functions.h"

using namespace functions;

auto addr = bus::idx_to_addr(0, bus::M2_BLOCK_IDX, 1);

DataFunction f{addr, functions::DAC60508::DEFAULT_SPI_SETTINGS};
functions::DAC60508 dac{addr};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_float_to_raw() {
  TEST_ASSERT_EQUAL(DAC60508::RAW_PLUS_ONE, DAC60508::float_to_raw(+1.0f));
  TEST_ASSERT_EQUAL(DAC60508::RAW_MINUS_ONE, DAC60508::float_to_raw(-1.0f));
}

void test_raw_read() {
  f.begin_communication();
  auto &raw_spi = functions::DataFunction::get_raw_spi();

  // Read command is
  //  1. /CS low
  //  2. send 24bit = [1bit RW=1][3bit reserved=0][4bit ADDR MSB][16bit ignored]
  //  3. /CS high
  //  4. send/read 24bit = [1bit ECHO RW][3bit ECHO reserved=0][4bit ECHO ADDR][16bit data]

  // Read DEVICE_ID register, with addr = 0b0001;
  raw_spi.transfer(0b1'000'0001);
  raw_spi.transfer(0);
  raw_spi.transfer(0);
  f.end_communication();
  f.begin_communication();
  // First 8bit read back echo what we send above
  TEST_ASSERT_EQUAL(0b1'000'0001, raw_spi.transfer(0));
  // Next 16 are the DEVICE_ID register, which is checked according to datasheet
  TEST_ASSERT_EQUAL(0b0'010'1000, raw_spi.transfer(0));
  TEST_ASSERT_EQUAL(0b1'00101'10, raw_spi.transfer(0));
  f.end_communication();
}

void test_read_register() {
  TEST_ASSERT_EQUAL(0b0'010'1000'1'00101'10, dac.read_register(DAC60508::REG_DEVICE_ID));
}

void test_set_noop_register() {
  TEST_ASSERT(dac.write_register(0, 0b1111'0000'0000'0011));
  TEST_ASSERT_EQUAL(0, dac.read_register(0));
}

void test_set_registers_as_they_should_be_later() {
  // TODO: Change back to external reference when working again
  uint16_t data_reg_config = 0b0000'0000'0000'0000;
  TEST_ASSERT(dac.write_register(DAC60508::REG_CONFIG, data_reg_config));
  TEST_ASSERT_EQUAL(data_reg_config, dac.read_register(DAC60508::REG_CONFIG));

  uint16_t data_reg_gain = 0b0000'0000'1111'1111;
  TEST_ASSERT(dac.write_register(DAC60508::REG_GAIN, data_reg_gain));
  TEST_ASSERT_EQUAL(data_reg_gain, dac.read_register(DAC60508::REG_GAIN));
}

void test_set_dac_outputs() {
  //TEST_ASSERT(dac.write_register(functions::DAC60508::REG_DAC(0), 0b0000'0000'0000'0000));
  TEST_ASSERT(dac.write_register(functions::DAC60508::REG_DAC(0), 0b0000'1111'0000'0000));
  TEST_ASSERT_EQUAL(0x0F00, dac.read_register(DAC60508::REG_DAC(0)));
  //TEST_ASSERT(dac.write_register(functions::DAC60508::REG_DAC(0), 0b1111'1111'1111'0000));
}

void test_set_dac_outputs_raw() {
  dac.set_channel(0, DAC60508::RAW_MINUS_ONE);
  //dac.set_channel(0, DAC60508::RAW_PLUS_ONE);
}

void test_set_dac_outputs_float() {
  TEST_ASSERT(dac.set_channel(0, DAC60508::float_to_raw(-0.65f)));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_float_to_raw);
  RUN_TEST(test_raw_read);
  RUN_TEST(test_read_register);
  RUN_TEST(test_set_noop_register);
  RUN_TEST(test_set_registers_as_they_should_be_later);
  //RUN_TEST(test_set_dac_outputs);
  RUN_TEST(test_set_dac_outputs_raw);
  RUN_TEST(test_set_dac_outputs_float);
  UNITY_END();
}

void loop() {
}