// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>

namespace utils {

// Some helping functions for array conversions
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

template <typename Type, typename OtherType, std::size_t Size, typename... Types,
          class = typename std::enable_if<sizeof...(Types) != Size>::type>
constexpr std::array<Type, Size> convert(const std::array<OtherType, Size> source, const Types... data);

template <typename Type, typename OtherType, std::size_t Size, typename... Types,
          class = typename std::enable_if<sizeof...(Types) == Size>::type, class = void>
constexpr std::array<Type, Size> convert(const std::array<OtherType, Size> source, const Types... data);

template <typename Type, typename OtherType, std::size_t Size, typename... Types, class>
constexpr std::array<Type, Size> convert(const std::array<OtherType, Size> source, const Types... data) {
  return convert<Type>(source, data..., static_cast<const Type>(source[sizeof...(data)]));
}

template <typename Type, typename OtherType, std::size_t Size, typename... Types, class, class>
constexpr std::array<Type, Size> convert(const std::array<OtherType, Size> source, const Types... data) {
  return std::array<Type, Size>{{data...}};
}

template <class T> class RunningAverage {
private:
  std::size_t n = 0;
  T sum{};

public:
  void add(const T &data) {
    n++;
    sum = sum + data;
  };

  void clear() {
    n = 0;
    sum = {};
  }

  T get_average() const { return 1.0f / static_cast<float>(n) * sum; }
};

} // namespace utils
