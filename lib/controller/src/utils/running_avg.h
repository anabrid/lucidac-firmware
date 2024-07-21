// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>

namespace utils {

template <class T, std::size_t N>
std::array<T, N> operator+(const std::array<T, N> &l, const std::array<T, N> &r) {
  std::array<T, N> result;
  for (std::size_t i = 0; i < N; i++)
    result[i] = l[i] + r[i];
  return result;
}

template <class T, std::size_t N> std::array<T, N> operator*(const std::array<T, N> &l, const float &r) {
  std::array<T, N> result;
  for (std::size_t i = 0; i < N; i++)
    result[i] = l[i] * r;
  return result;
}

template <class T, std::size_t N> std::array<T, N> operator*(const float &l, const std::array<T, N> &r) {
  return r * l;
}

template <unsigned int N> class RunningAverageVec {
public:
  typedef std::array<float, N> data_t;

private:
  size_t n = 0;
  std::array<float, N> average{{0}};

public:
  data_t add(data_t data) {
    n += 1;
    for (size_t i = 0; i < N; i++) {
      average[i] = (average[i] * static_cast<float>(n - 1) + data[i]) / static_cast<float>(n);
    }
    return average;
  };

  const data_t &get_average() const { return average; }
};

template <class T> class RunningAverageNew {
private:
  std::size_t n = 0;
  T sum{};

public:
  void add(const T &data) {
    n++;
    sum = sum + data;
  };

  T get_average() const { return 1.0f / static_cast<float>(n) * sum; }
};

} // namespace utils
