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

#include "block.h"
#include "local_bus.h"

blocks::UMatrixFunction umatrix{bus::idx_to_addr(0, 2, blocks::UBlock::UMATRIX_FUNC_IDX),
                                SPISettings(4'000'000, MSBFIRST, SPI_MODE2),
                                /*{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}*/
                                {16}
};

bus::TriggerFunction umatrix_reset{bus::idx_to_addr(0, 2, blocks::UBlock::UMATRIX_RESET_FUNC_IDX)};

bus::TriggerFunction umatrix_sync{bus::idx_to_addr(0, 2, blocks::UBlock::UMATRIX_SYNC_FUNC_IDX)};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_func_idx() { TEST_ASSERT_EQUAL(1, blocks::UBlock::UMATRIX_FUNC_IDX); }

void test_communication() {
  // umatrix_reset.trigger();
  umatrix.sync_to_hardware();
  umatrix_sync.trigger();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_func_idx);
  RUN_TEST(test_communication);
  UNITY_END();
}

void loop() {}
