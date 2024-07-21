// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <unity.h>

#undef B1
#undef isinf
#undef isnan
#include <fmt/ranges.h>

template <typename ...Args> void TEST_MESSAGE_FORMAT(Args && ...args)
{
  TEST_MESSAGE(fmt::format(std::forward<Args>(args)...).c_str());
}
