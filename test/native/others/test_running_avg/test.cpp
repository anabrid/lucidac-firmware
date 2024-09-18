// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <iostream>

#include <Arduino.h>
#include <unity.h>

#include "utils/running_avg.h"

template <class T> void print_array(const T &arr) {
  std::cout << "{ ";
  for (int i = 0; i < arr.size() - 1; i++)
    std::cout << arr[i] << ", ";

  std::cout << arr.back() << " }" << std::endl;
}

std::array<float, 4> a1 = {-2.0f, 1.0f, 3.0f, -8.0f};
std::array<float, 4> a2 = {2.0f, 1.0f, 3.0f, 5.0f};

std::array<int, 4> a3 = {2, 1, 3, 5};
std::array<int, 4> a4 = {-2, 1, 4, -8};

void test() {
  utils::RunningAverage<std::array<float, 4>> avg;
  avg.add(a1);
  avg.add(a2);
  print_array(avg.get_average());
}

void test2() {
  int a = 2;
  int b = 5;

  utils::RunningAverage<int> avg;
  avg.add(a);
  avg.add(b);
  std::cout << avg.get_average() << std::endl;
}

void test3() {
  utils::RunningAverage<std::array<int, 4>> avg;
  avg.add(a3);
  avg.add(a4);
  print_array(avg.get_average());
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test);
  std::cout << std::endl;
  std::cout << std::endl;
  RUN_TEST(test2);
  std::cout << std::endl;
  std::cout << std::endl;
  RUN_TEST(test3);
  UNITY_END();
}
