// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <utility>

namespace utils {

// This function takes an input and divides it into two number x and y, that multiply to the input number
// within the given delta range. This is used for FlexIO timing
std::pair<unsigned long long, unsigned long long> factorize(unsigned long long input,
                                                            unsigned int acceptable_delta = 10);

} // namespace utils
