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

#include <ArduinoJson.h>
#include <Arduino.h>
#include <unity.h>
#include <math.h>
#include <array>

#define private public
#define protected public
#include "block/iblock.h"
#include "bus/functions.h"

blocks::IBlock iblockdonor{0};
blocks::IBlock iblockreciever{0};

void test_object2json2object () {
    iblockdonor.init();
    iblockdonor.reset_outputs();
    u_int8_t num_connections = iblockdonor.NUM_INPUTS;
    u_int8_t max_factor = iblockdonor.NUM_OUTPUTS;
    u_int8_t min_factor = 0;

    for (u_int8_t i = 0; i<num_connections; i++) {
        u_int8_t curr_connection = (max_factor*i + (num_connections-i)*min_factor)/num_connections;

        TEST_ASSERT(iblockdonor.connect(i, curr_connection, false, false) );
        TEST_ASSERT(iblockdonor.is_connected(i,curr_connection));
    };

    StaticJsonDocument<2048> doc;
    JsonObject test_cfg = doc.to<JsonObject>();
    iblockdonor.config_self_to_json(test_cfg);

    iblockreciever.init();
    iblockreciever.config_self_from_json(test_cfg);

    for (auto idx = 0; idx < num_connections; idx++) {
        for (auto jdx = 0; jdx < num_connections; jdx++) {
            TEST_ASSERT_EQUAL(iblockdonor.is_connected(idx, jdx), iblockreciever.is_connected(idx, jdx));
        }
    }
}



void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_object2json2object);
  UNITY_END();
}



void loop() {}


