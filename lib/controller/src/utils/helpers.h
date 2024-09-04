// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <tuple>

// Helper to check if all values in a tuple are unique
template <typename Tuple, std::size_t... Is>
constexpr bool all_unique_impl(const Tuple& t, std::index_sequence<Is...>) {
  return ((std::get<Is>(t) != std::get<Is + 1>(t)) && ...);
}

template <typename... Args>
constexpr bool all_unique(const std::tuple<Args...>& t) {
  return all_unique_impl(t, std::make_index_sequence<sizeof...(Args) - 1>{});
}
