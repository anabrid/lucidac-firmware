// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifndef ANABRID_PEDANTIC
#error "This test requires pedantic mode."
#endif

#include "block/blocks.h"
#include "chips/EEPROM25AA02.h"
#include "entity/entity.h"
#include "lucidac/front_panel.h"

using namespace blocks;
using namespace functions;
using namespace metadata;
using namespace entities;

void test_write_classifier(const EntityClassifier &classifier, const bus::addr_t target_address) {
  MetadataEditor writer(target_address);
  TEST_ASSERT(writer.write_entity_classifier(classifier));
}

/*
        Test case for reading and writing the entity classifier of a block. This is a lightweight alternative
   to the test_dynamic_block_detection file. Entity classifiers must be adjusted manually
   before executing this test.
*/

Version u_version(1, 2, 0);
Version c_version(1, 0, 0);
Version i_version(1, 2, 0);
Version sh_version(0, 1, 0);
Version m0_version(1, 0, 0);
Version m1_version(1, 0, 0);
Version ctrl_version(1, 0, 2);
Version fp_version(1, 1, 2);

void write_eeproms() {
  test_write_classifier(EntityClassifier(EntityClass::U_BLOCK, EntityClassifier::DEFAULT_, u_version),
                        bus::idx_to_addr(0, UBlock::BLOCK_IDX, 0));

  test_write_classifier(EntityClassifier(EntityClass::C_BLOCK, EntityClassifier::DEFAULT_, c_version),
                        bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0));

  test_write_classifier(EntityClassifier(EntityClass::I_BLOCK, EntityClassifier::DEFAULT_, i_version),
                        bus::idx_to_addr(0, IBlock::BLOCK_IDX, 0));

  test_write_classifier(EntityClassifier(EntityClass::SH_BLOCK, EntityClassifier::DEFAULT_, sh_version),
                        bus::idx_to_addr(0, SHBlock::BLOCK_IDX, 0));

  test_write_classifier(
      EntityClassifier(EntityClass::M_BLOCK, static_cast<uint8_t>(MBlock::TYPES::M_INT8_BLOCK), m0_version),
      bus::idx_to_addr(0, MBlock::M0_IDX, 0));

  test_write_classifier(
      EntityClassifier(EntityClass::M_BLOCK, static_cast<uint8_t>(MBlock::TYPES::M_MUL4_BLOCK), m1_version),
      bus::idx_to_addr(0, MBlock::M1_IDX, 0));

  test_write_classifier(EntityClassifier(EntityClass::CTRL_BLOCK, EntityClassifier::DEFAULT_, ctrl_version),
                        bus::address_from_tuple(1, 0));

  test_write_classifier(EntityClassifier(EntityClass::FRONT_PANEL, EntityClassifier::DEFAULT_, fp_version),
                        bus::address_from_tuple(2, 0));
}

void test_detection() {
  TEST_ASSERT(entities::detect<UBlock>(bus::idx_to_addr(0, UBlock::BLOCK_IDX, 0)));
  TEST_ASSERT(entities::detect<CBlock>(bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0)));
  TEST_ASSERT(entities::detect<IBlock>(bus::idx_to_addr(0, IBlock::BLOCK_IDX, 0)));
  TEST_ASSERT(entities::detect<SHBlock>(bus::idx_to_addr(0, SHBlock::BLOCK_IDX, 0)));
  TEST_ASSERT(entities::detect<MBlock>(bus::idx_to_addr(0, MBlock::M0_IDX, 0)));
  TEST_ASSERT(entities::detect<MBlock>(bus::idx_to_addr(0, MBlock::M1_IDX, 0)));
  TEST_ASSERT(entities::detect<CTRLBlock>(bus::address_from_tuple(1, 0)));
  TEST_ASSERT(entities::detect<platform::LUCIDACFrontPanel>(bus::address_from_tuple(2, 0)));
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  RUN_TEST(write_eeproms);
  RUN_TEST(test_detection);
  UNITY_END();
}

void loop() { delay(100); }
