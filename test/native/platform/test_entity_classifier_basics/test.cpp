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
using namespace entities;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_cblock_v1_variants() {
  CBlockHAL_V_1_0_0_SequentialCoeffsCS cblock_seq(bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0));
  TEST_ASSERT_EQUAL(cblock_seq.f_coeffs[0].address, bus::idx_to_addr(0, CBlock::BLOCK_IDX, 1));
  TEST_ASSERT_EQUAL(cblock_seq.f_coeffs[15].address, bus::idx_to_addr(0, CBlock::BLOCK_IDX, 16));
  TEST_ASSERT_EQUAL(cblock_seq.f_coeffs[16].address, bus::idx_to_addr(0, CBlock::BLOCK_IDX, 17));
  TEST_ASSERT_EQUAL(cblock_seq.f_coeffs[31].address, bus::idx_to_addr(0, CBlock::BLOCK_IDX, 32));

  CBlockHAL_V_1_0_0_MixedCoeffsCS cblock_mixed(bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0));
  TEST_ASSERT_EQUAL(cblock_mixed.f_coeffs[0].address, bus::idx_to_addr(0, CBlock::BLOCK_IDX, 1));
  TEST_ASSERT_EQUAL(cblock_mixed.f_coeffs[15].address, bus::idx_to_addr(0, CBlock::BLOCK_IDX, 32));
  TEST_ASSERT_EQUAL(cblock_mixed.f_coeffs[16].address, bus::idx_to_addr(0, CBlock::BLOCK_IDX, 33));
  TEST_ASSERT_EQUAL(cblock_mixed.f_coeffs[31].address, bus::idx_to_addr(0, CBlock::BLOCK_IDX, 16));
}

void test_cblock_from_entity_classifier() {
  auto cblock_unknown = CBlock::from_entity_classifier(EntityClassifier{CBlock::CLASS_, CBlock::TYPE, 0, 0},
                                                       bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0));
  TEST_ASSERT_EQUAL(nullptr, cblock_unknown);

  auto cblock_seq = CBlock::from_entity_classifier(EntityClassifier{CBlock::CLASS_, CBlock::TYPE, 1, 1},
                                                   bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0));
  TEST_ASSERT_NOT_EQUAL(nullptr, cblock_seq);
  TEST_ASSERT_TRUE(dynamic_cast<CBlockHAL_V_1_0_0_SequentialCoeffsCS *>(cblock_seq->hardware));
  TEST_ASSERT_FALSE(dynamic_cast<CBlockHAL_V_1_0_0_MixedCoeffsCS *>(cblock_seq->hardware));

  auto cblock_mixed = CBlock::from_entity_classifier(EntityClassifier{CBlock::CLASS_, CBlock::TYPE, 2, 1},
                                                     bus::idx_to_addr(0, CBlock::BLOCK_IDX, 0));
  TEST_ASSERT_NOT_EQUAL(nullptr, cblock_mixed);
  TEST_ASSERT_FALSE(dynamic_cast<CBlockHAL_V_1_0_0_SequentialCoeffsCS *>(cblock_mixed->hardware));
  TEST_ASSERT_TRUE(dynamic_cast<CBlockHAL_V_1_0_0_MixedCoeffsCS *>(cblock_mixed->hardware));
}

void test_mblock_from_entity_classifier() {
  auto unknown_mblock = MBlock::from_entity_classifier(
      EntityClassifier{EntityClass::M_BLOCK, static_cast<const uint8_t>(MBlock::TYPES::UNKNOWN), 0, 0},
      bus::idx_to_addr(0, MBlock::M0_IDX, 0));
  TEST_ASSERT_EQUAL(nullptr, unknown_mblock);
  TEST_ASSERT_FALSE(dynamic_cast<MIntBlock *>(unknown_mblock));
  TEST_ASSERT_FALSE(dynamic_cast<MMulBlock *>(unknown_mblock));

  auto m_int8_block = MBlock::from_entity_classifier(
      EntityClassifier{EntityClass::M_BLOCK, static_cast<const uint8_t>(MBlock::TYPES::M_INT8_BLOCK), 1, 1},
      bus::idx_to_addr(0, MBlock::M0_IDX, 0));
  TEST_ASSERT_NOT_EQUAL(nullptr, m_int8_block);
  TEST_ASSERT_TRUE(dynamic_cast<MIntBlock *>(m_int8_block));
  TEST_ASSERT_FALSE(dynamic_cast<MMulBlock *>(m_int8_block));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_mblock_from_entity_classifier);
  RUN_TEST(test_cblock_from_entity_classifier);
  RUN_TEST(test_cblock_v1_variants);
  UNITY_END();
}
