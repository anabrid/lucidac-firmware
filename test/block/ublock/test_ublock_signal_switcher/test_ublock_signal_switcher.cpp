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

blocks::USignalSwitchFunction switcher{bus::idx_to_addr(0, bus::U_BLOCK_IDX, blocks::UBlock::SIGNAL_SWITCHER),
                                       SPISettings(4'000'000, MSBFIRST, SPI_MODE2)};

bus::TriggerFunction switcher_sync{bus::idx_to_addr(0,bus::U_BLOCK_IDX, blocks::UBlock::SIGNAL_SWITCHER_SYNC)};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_communication() {
  switcher.data = 0b00000000'00000000;
  switcher.write_to_hardware();
  switcher_sync.trigger();

  delayMicroseconds(1);
  switcher.data = 0b00000000'00001000;
  switcher.write_to_hardware();
  switcher_sync.trigger();

  delayMicroseconds(1);
  switcher.data = 0b00000011'00011000;
  // TODO: Currently this leads to HIGH on ACL {0, 4, 5, 9, 10}
  switcher.write_to_hardware();
  switcher_sync.trigger();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_communication);
  UNITY_END();
}

void loop() {}
