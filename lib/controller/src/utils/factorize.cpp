// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "factorize.h"

#include <math.h>

std::pair<unsigned long long, unsigned long long> utils::factorize(unsigned long long input,
                                                                   unsigned int acceptable_delta) {
  unsigned long long root = sqrt(input);
  if (root * root > input - acceptable_delta)
    return std::make_pair(root, root);

  unsigned long long x = root, y = root;

begin:
  if (x * y > input + acceptable_delta) {
    y--;
    x = input / y;
    goto begin;
  } else if (x * y < input - acceptable_delta) {
    x++;
    y = input / x;
    goto begin;
  } else
    return std::make_pair(x, y);
}
