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

/*
 * ASSUMPTIONS
 *  - MUL block is inserted into M2 slot
 */

#include <Arduino.h>
#include <unity.h>

#include "metadata/74LVC138AD.tpl.hpp"

MetadataMemory74LVC138AD chip{bus::idx_to_addr(0, bus::M2_BLOCK_IDX, 0)};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_uuid_address() {
  // M2_BLOCk_IDX is 4, but block idx start at 1, so address is 5.
  TEST_ASSERT_EQUAL(0b000000'0101, chip.address);
}

void test_read_uuid() {
  std::array<uint8_t, 8> uuid_is{1, 1, 1, 1, 1, 1, 1, 1};
  // For development carrier board I use
  std::array<uint8_t, 8> uuid_should{0, 0b00000100, 0b10100011, 0b00001011,
                                     0, 0b00010100, 0b01110000, 0b11111101};

  // Read UUID
  chip.read_from_hardware(offsetof(metadata::MetadataMemoryLayoutV1, uuid),
                          sizeof(metadata::MetadataMemoryLayoutV1::uuid), uuid_is.data());

  // Check UUID
  TEST_ASSERT_EQUAL_UINT8_ARRAY(uuid_should.data(), uuid_is.data(), uuid_should.size());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_uuid_address);
  RUN_TEST(test_read_uuid);
  UNITY_END();
}

void loop() {}