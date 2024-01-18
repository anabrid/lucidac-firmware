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

#include "carrier/carrier.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace lucidac;
using namespace mode;

LUCIDAC luci{};
OneshotDAQ daq_{};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Initialize mode controller (currently separate thing)
  ManualControl::init();

  // Put LUCIDAC start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(luci.init());
  
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(luci.calibrate(&daq_));
  delayMicroseconds(200);
}

auto *mBlock = (MIntBlock *)(luci.m1block);

uint32_t TquaterCirc = 162;

auto one = mBlock->M2_OUTPUT(4);

uint8_t x = 0;
auto x_in = mBlock->M1_INPUT(0);
auto x_out = mBlock->M1_OUTPUT(0);

uint8_t y = 1;
auto y_in = mBlock->M1_INPUT(1);
auto y_out = mBlock->M1_OUTPUT(1);

uint8_t z = 2;
auto z_in = mBlock->M1_INPUT(2);
auto z_out = mBlock->M1_OUTPUT(2);

void draw_circ(float pos_x, float pos_y, float c_x, float c_y, float size, uint64_t t, bool reverse = false) {
    mBlock->set_ic(x, pos_x);
    mBlock->set_ic(y, pos_y);
    mBlock->write_to_hardware();

    int rev = 1;

    if (reverse) {
      rev = -1;
    }

    luci.cblock->set_factor(0, -rev);
    luci.cblock->set_factor(1, rev*c_x);
    luci.cblock->set_factor(2, rev);
    luci.cblock->set_factor(3, -rev*c_y);
    luci.cblock->set_factor(4, 0);
    luci.cblock->set_factor(5, 0);
    luci.cblock->set_factor(6, 0);
    luci.cblock->write_to_hardware();

    ManualControl::to_ic();
    delayMicroseconds(5);
    ManualControl::to_op();
    delayMicroseconds(t);
    ManualControl::to_halt();
}

void draw_line_horiz(float pos_x, float pos_y, float length) {
    mBlock->set_ic(x, pos_x);
    mBlock->set_ic(y, pos_y);
    mBlock->write_to_hardware();

    luci.cblock->set_factor(0, 0);
    luci.cblock->set_factor(1, 0);
    luci.cblock->set_factor(2, 0);
    luci.cblock->set_factor(3, length);
    luci.cblock->set_factor(4, 0);
    luci.cblock->set_factor(5, 0);
    luci.cblock->set_factor(6, 0);
    luci.cblock->write_to_hardware();

    ManualControl::to_ic();
    delayMicroseconds(5);
    ManualControl::to_op();
    delayMicroseconds(100);
    ManualControl::to_halt();
}

void draw_line_vert(float pos_x, float pos_y, float length) {
    mBlock->set_ic(x, pos_x);
    mBlock->set_ic(y, pos_y);
    mBlock->write_to_hardware();

    luci.cblock->set_factor(0, 0);
    luci.cblock->set_factor(1, length);
    luci.cblock->set_factor(2, 0);
    luci.cblock->set_factor(3, 0);
    luci.cblock->set_factor(4, 0);
    luci.cblock->set_factor(5, 0);
    luci.cblock->set_factor(6, 0);
    luci.cblock->write_to_hardware();

    ManualControl::to_ic();
    delayMicroseconds(5);
    ManualControl::to_op();
    delayMicroseconds(100);
    ManualControl::to_halt();
}

void draw_a(float pos_x, float pos_y, float size) {
  float c_x = pos_x;
  float c_y = pos_y + size;
  draw_circ(pos_x, pos_y, c_x, c_y, size, 3*TquaterCirc);
  draw_line_vert(pos_x+size, pos_y+size, -size);
  draw_line_horiz(pos_x+size, pos_y, -size);
}

void draw_n(float pos_x, float pos_y, float size) {
  float c_x = pos_x;
  float c_y = pos_y + size;
  draw_line_horiz(pos_x, pos_y, -size);
  draw_line_vert(pos_x-size, pos_y, 2*size);
  draw_line_horiz(pos_x-size, pos_y+2*size, size);
  draw_circ(pos_x, pos_y+2*size, c_x, c_y, size, TquaterCirc);
  draw_line_vert(pos_x+size, pos_y+size, -size);
  draw_line_horiz(pos_x+size, pos_y, -size);
}

void draw_b(float pos_x, float pos_y, float size) {
  float c_x = pos_x;
  float c_y = pos_y + size;
  draw_circ(pos_x, pos_y, c_x, c_y, size, TquaterCirc);
  draw_line_vert(pos_x-size, pos_y+size, 2.5*size);
  draw_line_vert(pos_x-size, pos_y+3.5*size, -1.5*size);
  draw_line_horiz(pos_x-size, pos_y+2*size, size);
  draw_circ(pos_x, pos_y+2*size, c_x, c_y, size, 2*TquaterCirc);
}

void draw_r(float pos_x, float pos_y, float size) {  
  float c_x = pos_x;
  float c_y = pos_y + size;
  draw_line_horiz(pos_x, pos_y, -size);
  draw_line_vert(pos_x-size, pos_y, size);
  draw_circ(pos_x-size, pos_y+size, c_x, c_y, size, 1.5*TquaterCirc);
  draw_circ(pos_x+1/sqrt(2)*size, pos_y+(1+1/sqrt(2))*size, c_x, c_y, size, 1.5*TquaterCirc, true);
  draw_line_vert(pos_x-size, pos_y+size, -size);
  draw_line_horiz(pos_x-size, pos_y, size);
}

void draw_i(float pos_x, float pos_y, float size) {
  draw_line_vert(pos_x, pos_y, 2*size);
  draw_line_vert(pos_x, pos_y+2*size, -2*size);
}

void draw_d(float pos_x, float pos_y, float size) {
  float c_x = pos_x;
  float c_y = pos_y + size;
  draw_circ(pos_x, pos_y, c_x, c_y, size, 2*TquaterCirc);
  draw_line_horiz(pos_x, pos_y+2*size, size);
  draw_line_vert(pos_x+size, pos_y+2*size, 1.5*size);
  draw_line_vert(pos_x+size, pos_y+3.5*size, -2.5*size);
  draw_circ(pos_x+size, pos_y+size, c_x, c_y, size, TquaterCirc);
}

void draw_anabrid() {
  luci.reset(true);

  luci.ublock->connect(x_out, UBlock::IDX_RANGE_TO_ACL_OUT(5));
  luci.ublock->connect(y_out, UBlock::IDX_RANGE_TO_ACL_OUT(6));

  luci.route(x_out, 0, 0, y_in);
  luci.route(one, 1, 0, y_in);

  luci.route(y_out, 2, 0, x_in);
  luci.route(one, 3, 0, x_in);

  luci.write_to_hardware();
  delayMicroseconds(500);

  float size = 0.13;
  
  float gap = 2.5*size;
  float start = -0.829;

  for (;;){
    draw_a(start, 0, size);
    draw_line_horiz(start, 0, gap);
    draw_n(start+gap, 0, size);
    draw_line_horiz(start+gap, 0, gap);
    draw_a(start+2*gap, 0, size);
    draw_line_horiz(start+2*gap, 0, gap);
    draw_b(start+3*gap, 0, size);
    draw_line_horiz(start+3*gap, 0, gap);
    draw_r(start+4*gap,0,size);
    draw_line_horiz(start+4*gap, 0, 0.5*gap);
    draw_i(start+4.5*gap,0,size);
    draw_line_horiz(start+4.5*gap, 0, 0.6*gap);
    draw_d(start+5.1*gap,0,size);
    draw_line_horiz(start+5.1*gap, 0, -5.1*gap);
  }

  luci.reset(true);
  luci.write_to_hardware();
}

void draw_damped_sin() {
  mBlock->set_ic(x, -1);
  mBlock->set_ic(y, -0);
  mBlock->set_ic(z, 1);
  mBlock->write_to_hardware();

  luci.cblock->set_factor(0, 0);
  luci.cblock->set_factor(1, 0);
  luci.cblock->set_factor(2, 0);
  luci.cblock->set_factor(3, 0.116);
  luci.cblock->set_factor(4, -0.2);
  luci.cblock->set_factor(5, 1);
  luci.cblock->set_factor(6, -1);
  luci.cblock->write_to_hardware();

  ManualControl::to_ic();
  delayMicroseconds(50);
  ManualControl::to_op();
  delayMicroseconds(1980);
  ManualControl::to_halt();
}

void draw_logo() {
  luci.reset(true);

  luci.ublock->connect(x_out, UBlock::IDX_RANGE_TO_ACL_OUT(5));
  luci.ublock->connect(y_out, UBlock::IDX_RANGE_TO_ACL_OUT(6));

  luci.route(x_out, 0, 0, y_in);
  luci.route(one, 1, 0, y_in);

  luci.route(y_out, 2, 0, x_in);
  luci.route(one, 3, 0, x_in);

  luci.route(y_out, 4, 0, y_in);
  luci.route(y_out, 5, 0, z_in);
  luci.route(z_out, 6, 0, y_in);

  luci.write_to_hardware();
  delayMicroseconds(500);

  for (;;){
    draw_circ(-1,0,0,0,1,4*TquaterCirc);
    draw_damped_sin();
    draw_line_horiz(1,0,-2);
  }

  luci.reset(true);
  luci.write_to_hardware();
}


void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  //RUN_TEST(draw_anabrid);
  RUN_TEST(draw_logo);
  UNITY_END();
}

void loop() {}
