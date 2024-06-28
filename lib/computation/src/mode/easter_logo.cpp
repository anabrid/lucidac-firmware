// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>

#include "carrier/carrier.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace platform;
using namespace mode;

namespace easter_egg {

struct DrawAnabridLogo {
  Cluster &cluster;
  MIntBlock *mBlock;
  OneshotDAQ daq_{};

  const uint32_t TquaterCirc = 162;

  DrawAnabridLogo(carrier::Carrier &c) : cluster(c.clusters[0]), mBlock((MIntBlock *)(cluster.m1block)) {}

  const uint8_t one = mBlock->M2_OUTPUT(4);

  uint8_t x = 0;
  const uint8_t x_in = mBlock->M1_INPUT(0);
  const uint8_t x_out = mBlock->M1_OUTPUT(0);

  uint8_t y = 1;
  const uint8_t y_in = mBlock->M1_INPUT(1);
  const uint8_t y_out = mBlock->M1_OUTPUT(1);

  uint8_t z = 2;
  const uint8_t z_in = mBlock->M1_INPUT(2);
  const uint8_t z_out = mBlock->M1_OUTPUT(2);

  void draw_circ(float pos_x, float pos_y, float c_x, float c_y, float size, uint64_t t,
                 bool reverse = false) {
    mBlock->set_ic(x, pos_x);
    mBlock->set_ic(y, pos_y);
    mBlock->write_to_hardware();

    int rev = 1;

    if (reverse) {
      rev = -1;
    }

    cluster.cblock->set_factor(0, -rev);
    cluster.cblock->set_factor(1, rev * c_x);
    cluster.cblock->set_factor(2, rev);
    cluster.cblock->set_factor(3, -rev * c_y);
    cluster.cblock->set_factor(4, 0);
    cluster.cblock->set_factor(5, 0);
    cluster.cblock->set_factor(6, 0);
    cluster.cblock->write_to_hardware();

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

    cluster.cblock->set_factor(0, 0);
    cluster.cblock->set_factor(1, 0);
    cluster.cblock->set_factor(2, 0);
    cluster.cblock->set_factor(3, length);
    cluster.cblock->set_factor(4, 0);
    cluster.cblock->set_factor(5, 0);
    cluster.cblock->set_factor(6, 0);
    cluster.cblock->write_to_hardware();

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

    cluster.cblock->set_factor(0, 0);
    cluster.cblock->set_factor(1, length);
    cluster.cblock->set_factor(2, 0);
    cluster.cblock->set_factor(3, 0);
    cluster.cblock->set_factor(4, 0);
    cluster.cblock->set_factor(5, 0);
    cluster.cblock->set_factor(6, 0);
    cluster.cblock->write_to_hardware();

    ManualControl::to_ic();
    delayMicroseconds(5);
    ManualControl::to_op();
    delayMicroseconds(100);
    ManualControl::to_halt();
  }

  void draw_a(float pos_x, float pos_y, float size) {
    float c_x = pos_x;
    float c_y = pos_y + size;
    draw_circ(pos_x, pos_y, c_x, c_y, size, 3 * TquaterCirc);
    draw_line_vert(pos_x + size, pos_y + size, -size);
    draw_line_horiz(pos_x + size, pos_y, -size);
  }

  void draw_n(float pos_x, float pos_y, float size) {
    float c_x = pos_x;
    float c_y = pos_y + size;
    draw_line_horiz(pos_x, pos_y, -size);
    draw_line_vert(pos_x - size, pos_y, 2 * size);
    draw_line_horiz(pos_x - size, pos_y + 2 * size, size);
    draw_circ(pos_x, pos_y + 2 * size, c_x, c_y, size, TquaterCirc);
    draw_line_vert(pos_x + size, pos_y + size, -size);
    draw_line_horiz(pos_x + size, pos_y, -size);
  }

  void draw_b(float pos_x, float pos_y, float size) {
    float c_x = pos_x;
    float c_y = pos_y + size;
    draw_circ(pos_x, pos_y, c_x, c_y, size, TquaterCirc);
    draw_line_vert(pos_x - size, pos_y + size, 2.5 * size);
    draw_line_vert(pos_x - size, pos_y + 3.5 * size, -1.5 * size);
    draw_line_horiz(pos_x - size, pos_y + 2 * size, size);
    draw_circ(pos_x, pos_y + 2 * size, c_x, c_y, size, 2 * TquaterCirc);
  }

  void draw_r(float pos_x, float pos_y, float size) {
    float c_x = pos_x;
    float c_y = pos_y + size;
    draw_line_horiz(pos_x, pos_y, -size);
    draw_line_vert(pos_x - size, pos_y, size);
    draw_circ(pos_x - size, pos_y + size, c_x, c_y, size, 1.5 * TquaterCirc);
    draw_circ(pos_x + 1 / sqrt(2) * size, pos_y + (1 + 1 / sqrt(2)) * size, c_x, c_y, size, 1.5 * TquaterCirc,
              true);
    draw_line_vert(pos_x - size, pos_y + size, -size);
    draw_line_horiz(pos_x - size, pos_y, size);
  }

  void draw_i(float pos_x, float pos_y, float size) {
    draw_line_vert(pos_x, pos_y, 2 * size);
    draw_line_vert(pos_x, pos_y + 2 * size, -2 * size);
  }

  void draw_d(float pos_x, float pos_y, float size) {
    float c_x = pos_x;
    float c_y = pos_y + size;
    draw_circ(pos_x, pos_y, c_x, c_y, size, 2 * TquaterCirc);
    draw_line_horiz(pos_x, pos_y + 2 * size, size);
    draw_line_vert(pos_x + size, pos_y + 2 * size, 1.5 * size);
    draw_line_vert(pos_x + size, pos_y + 3.5 * size, -2.5 * size);
    draw_circ(pos_x + size, pos_y + size, c_x, c_y, size, TquaterCirc);
  }

  void draw_anabrid() {
    cluster.reset(true);

    cluster.ublock->connect(x_out, UBlock::IDX_RANGE_TO_ACL_OUT(5));
    cluster.ublock->connect(y_out, UBlock::IDX_RANGE_TO_ACL_OUT(6));

    cluster.route(x_out, 0, 0, y_in);
    cluster.route(one, 1, 0, y_in);

    cluster.route(y_out, 2, 0, x_in);
    cluster.route(one, 3, 0, x_in);

    cluster.write_to_hardware();
    delayMicroseconds(500);

    float size = 0.13;

    float gap = 2.5 * size;
    float start = -0.829;

    for (;;) {
      draw_a(start, 0, size);
      draw_line_horiz(start, 0, gap);
      draw_n(start + gap, 0, size);
      draw_line_horiz(start + gap, 0, gap);
      draw_a(start + 2 * gap, 0, size);
      draw_line_horiz(start + 2 * gap, 0, gap);
      draw_b(start + 3 * gap, 0, size);
      draw_line_horiz(start + 3 * gap, 0, gap);
      draw_r(start + 4 * gap, 0, size);
      draw_line_horiz(start + 4 * gap, 0, 0.5 * gap);
      draw_i(start + 4.5 * gap, 0, size);
      draw_line_horiz(start + 4.5 * gap, 0, 0.6 * gap);
      draw_d(start + 5.1 * gap, 0, size);
      draw_line_horiz(start + 5.1 * gap, 0, -5.1 * gap);
    }

    cluster.reset(true);
    cluster.write_to_hardware();
  }

  void draw_damped_sin() {
    mBlock->set_ic(x, -1);
    mBlock->set_ic(y, -0);
    mBlock->set_ic(z, 1);
    mBlock->write_to_hardware();

    cluster.cblock->set_factor(0, 0);
    cluster.cblock->set_factor(1, 0);
    cluster.cblock->set_factor(2, 0);
    cluster.cblock->set_factor(3, 0.116);
    cluster.cblock->set_factor(4, -0.2);
    cluster.cblock->set_factor(5, 1);
    cluster.cblock->set_factor(6, -1);
    cluster.cblock->write_to_hardware();

    ManualControl::to_ic();
    delayMicroseconds(50);
    ManualControl::to_op();
    delayMicroseconds(1980);
    ManualControl::to_halt();
  }

  void draw_logo() {
    cluster.reset(true);

    cluster.ublock->connect(x_out, UBlock::IDX_RANGE_TO_ACL_OUT(5));
    cluster.ublock->connect(y_out, UBlock::IDX_RANGE_TO_ACL_OUT(6));

    cluster.route(x_out, 0, 0, y_in);
    cluster.route(one, 1, 0, y_in);

    cluster.route(y_out, 2, 0, x_in);
    cluster.route(one, 3, 0, x_in);

    cluster.route(y_out, 4, 0, y_in);
    cluster.route(y_out, 5, 0, z_in);
    cluster.route(z_out, 6, 0, y_in);

    cluster.write_to_hardware();
    delayMicroseconds(500);

    // TODO: Have some finite runtime like 10sec instead of an infinite loop
    for (;;) {
      draw_circ(-1, 0, 0, 0, 1, 4 * TquaterCirc);
      draw_damped_sin();
      draw_line_horiz(1, 0, -2);
    }

    cluster.reset(true);
    cluster.write_to_hardware();
  }

}; // end of class

void draw_anabrid_logo(carrier::Carrier &c) { DrawAnabridLogo(c).draw_logo(); }

} // namespace easter_egg
