// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifndef ANABRID_PEDANTIC
#error "This test case requires PEDANTIC mode."
#endif

#include "block/blocks.h"
#include "chips/EEPROM25AA02.h"
#include "entity/entity.h"
#include "lucidac/front_plane.h"

#include "test_fmtlib.h"

using namespace blocks;
using namespace functions;
using namespace metadata;
using platform::lucidac::FrontPlane;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

bool
  setup_mblocks   = false,
  setup_ublock    = true,
  setup_iblock    = true,
  setup_cblock    = true,
  setup_shblock   = false,
  setup_ctrlblock = true,
  setup_fp        = true;

void test_prepare_eeprom() {
  // Write to metadata memory for preparing the test case.
  // Can be run only once and it should persist.

  /*
  EEPROM25AA02 m0_eeprom(bus::idx_to_addr(0, MBlock::M0_IDX, 0));
  TEST_ASSERT(m0_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 0,
                               static_cast<uint8_t>(MBlock::CLASS_)));
  TEST_ASSERT(m0_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 1,
                               static_cast<uint8_t>(MBlock::TYPES::M_INT8_BLOCK)));
  TEST_ASSERT(m0_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 2, 1));
  TEST_ASSERT(m0_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 3, 1));
  */

  EEPROM25AA02 m1_eeprom(bus::idx_to_addr(0, MBlock::M1_IDX, 0));
  TEST_ASSERT(m1_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 0,
                               static_cast<uint8_t>(MBlock::CLASS_)));
  TEST_ASSERT(m1_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 1,
                               static_cast<uint8_t>(MBlock::TYPES::M_MUL4_BLOCK)));
  TEST_ASSERT(m1_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 2, 1));
  TEST_ASSERT(m1_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 3, 1));

  if(setup_ublock) {
    EEPROM25AA02 u_eeprom(bus::idx_to_addr(0, UBlock::BLOCK_IDX, 0));
    TEST_ASSERT(u_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 0,
                                static_cast<uint8_t>(entities::EntityClass::U_BLOCK)));
    TEST_ASSERT(u_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 1, 1));
    TEST_ASSERT(u_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 2, 1));
    TEST_ASSERT(u_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 3, 1));
  }

  EEPROM25AA02 c_eeprom(bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0));
  TEST_ASSERT(
      c_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 0, static_cast<uint8_t>(CBlock::CLASS_)));
  TEST_ASSERT(
      c_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 1, static_cast<uint8_t>(CBlock::TYPE)));
  TEST_ASSERT(c_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 2, 2));
  TEST_ASSERT(c_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 3, 1));

  if(setup_iblock) {
    EEPROM25AA02 i_eeprom(bus::idx_to_addr(0, IBlock::BLOCK_IDX, 0));
    TEST_ASSERT(i_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 0,
                                static_cast<uint8_t>(entities::EntityClass::I_BLOCK)));
    TEST_ASSERT(i_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 1, 1));
    TEST_ASSERT(i_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 2, 1));
    TEST_ASSERT(i_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 3, 1));
  }

  if(setup_shblock) {
    EEPROM25AA02 sh_eeprom(bus::idx_to_addr(0, SHBlock::BLOCK_IDX, 0));
    TEST_ASSERT(sh_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 0,
                                static_cast<uint8_t>(entities::EntityClass::SH_BLOCK)));
    TEST_ASSERT(sh_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 1, 1));
    TEST_ASSERT(sh_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 2, 1));
    TEST_ASSERT(sh_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 3, 1));
  }

  EEPROM25AA02 ctrl_eeprom(bus::address_from_tuple(1, 0));
  TEST_ASSERT(ctrl_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 0,
                                 static_cast<uint8_t>(entities::EntityClass::CTRL_BLOCK)));
  TEST_ASSERT(ctrl_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 1, 1));
  TEST_ASSERT(ctrl_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 2, 1));
  TEST_ASSERT(ctrl_eeprom.write8(offsetof(MetadataMemoryLayoutV1, classifier) + 3, 1));
}

void test_detect_block() {
  // TEST_ASSERT(entities::detect<MBlock>(bus::idx_to_addr(0, MBlock::M0_IDX, 0)));
  TEST_ASSERT(entities::detect<MBlock>(bus::idx_to_addr(0, MBlock::M1_IDX, 0)));
  TEST_ASSERT(entities::detect<UBlock>(bus::idx_to_addr(0, UBlock::BLOCK_IDX, 0)));
  TEST_ASSERT(entities::detect<CBlock>(bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0)));
  TEST_ASSERT(entities::detect<IBlock>(bus::idx_to_addr(0, IBlock::BLOCK_IDX, 0)));
  TEST_ASSERT(entities::detect<SHBlock>(bus::idx_to_addr(0, SHBlock::BLOCK_IDX, 0)));
  TEST_ASSERT(entities::detect<CTRLBlock>(bus::address_from_tuple(bus::CTRL_BLOCK_BADDR, 0)));
}

void test_read_euis() {
  for (auto block_idx : {bus::M0_BLOCK_IDX, bus::M1_BLOCK_IDX, UBlock::BLOCK_IDX, CBlock::BLOCK_IDX,
                         IBlock::BLOCK_IDX, SHBlock::BLOCK_IDX}) {
    MetadataReader reader{bus::idx_to_addr(0, block_idx, 0)};
    TEST_MESSAGE_FORMAT("{}", reader.read_eui());
  }
  // ctrl block
  MetadataReader reader{bus::address_from_tuple(1, 0)};
  TEST_MESSAGE_FORMAT("{}", reader.read_eui());
  // fp
  MetadataReader reader_fp{bus::address_from_tuple(2, 0)};
  TEST_MESSAGE_FORMAT("{}", reader_fp.read_eui());
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  RUN_TEST(test_prepare_eeprom);
  RUN_TEST(test_detect_block);
  RUN_TEST(test_read_euis);
  UNITY_END();
}

void loop() { delay(100); }