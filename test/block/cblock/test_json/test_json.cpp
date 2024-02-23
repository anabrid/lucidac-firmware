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
#include "block/cblock.h"
#include "bus/functions.h"

blocks::CBlock cblockdonor{0};
blocks::CBlock cblockreciever{0};

void test_object2json2object () {
    cblockdonor.init();
    int num_coeff = cblockdonor.NUM_COEFF;
    float max_factor = cblockdonor.MAX_FACTOR;
    float min_factor = cblockdonor.MIN_FACTOR;

    for (int i = 0; i<num_coeff; i++) {
        float coeff_in_range = (max_factor*i + (num_coeff-i)*min_factor)/num_coeff;
        TEST_ASSERT(cblockdonor.set_factor(i, coeff_in_range));
        TEST_ASSERT_FLOAT_WITHIN(0.02, coeff_in_range, cblockdonor.get_factor(i));
    };

    TEST_ASSERT(cblockdonor.set_factor(0, 0.0));

    StaticJsonDocument<2048> doc;
    JsonObject test_cfg = doc.to<JsonObject>();
    cblockdonor.config_self_to_json(test_cfg);

    cblockreciever.init();
    cblockreciever.config_self_from_json(test_cfg);

    for (auto idx = 0; idx < cblockdonor.factors_.size(); idx++) {
      TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.02, cblockdonor.get_factor(idx), cblockreciever.get_factor(idx), std::to_string(idx).c_str());
    }
}



void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_object2json2object);
  UNITY_END();
}



void loop() {}



