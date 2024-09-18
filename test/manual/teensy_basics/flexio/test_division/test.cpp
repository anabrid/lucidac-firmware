// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>
#include <utility>

#include "utils/running_avg.h"

const long long acceptable_delta = 10;

std::ostream &operator<<(std::ostream &os, const std::pair<long long, long long> &pair) {
  os << pair.first << ", " << pair.second;
  return os;
}

long max_duration = 0;
utils::RunningAverage<uint_fast64_t> runtime;

std::pair<long long, long long> find_factors(long long input) {
  std::cout << "Searching for " << input << std::endl;
  long long root = sqrt(input);
  auto start = micros();

  if (root * root > input - acceptable_delta)
    return std::make_pair(root, root);

  long long x = root, y = root;

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
    goto end_calc;

end_calc:
  auto end = micros();

  uint32_t duration = end - start;
  std::cout << "Duration: " << duration << std::endl;
  if (duration > max_duration)
    max_duration = duration;
  runtime.add(duration);

  return std::make_pair(x, y);
}

void divide() {
  for (long long i = 0; i < 10000; i++)
    std::cout << find_factors(10'000'000'000 + i * 200) << std::endl;
  auto max_duration_first = max_duration;
  max_duration = 0;

  std::cout << "Max Duration first: " << max_duration_first << " second: " << max_duration << std::endl;
  std::cout << runtime.get_average() << std::endl;
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(divide);
  UNITY_END();
}

void loop() {}

/*
 long long main( long long argc, char **argv) {
Duration: 274
41742, 33781
Max Duration: 1692
}

Max Duration first: 830455 second: 840305

//Without root:
Max Duration first: 860003 second: 840303

//With root:
Max Duration first: 830454 second: 840305

//with root without floor
Max Duration first: 840305 second: 840305
*/

/*
Max Duration first: 860004 second: 860003
2545

100:
Max Duration first: 610107 second: 610107
1974

half even:
Max Duration first: 828202 second: 828202
2803

only odd:
Max Duration first: 336150 second: 336150
1997


Max Duration first: 193828 second: 193829
1645
*/
