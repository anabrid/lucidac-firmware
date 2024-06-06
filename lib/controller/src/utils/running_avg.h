// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>

namespace utils {

template <unsigned int N> class RunningAverageVec {
public:
  typedef std::array<float, N> data_t;

private:
  size_t n = 0;
  std::array<float, N> average{{0}};

public:
  data_t add(data_t data);
  const data_t &get_average() const;
};

template <unsigned int N> typename RunningAverageVec<N>::data_t RunningAverageVec<N>::add(RunningAverageVec<N>::data_t data) {
  n += 1;
  for (size_t i = 0; i < N; i++) {
    average[i] = (average[i] * static_cast<float>(n - 1) + data[i]) / static_cast<float>(n);
  }
  return average;
}

template <unsigned int N> const typename RunningAverageVec<N>::data_t &RunningAverageVec<N>::get_average() const {
  return average;
}

} // namespace utils