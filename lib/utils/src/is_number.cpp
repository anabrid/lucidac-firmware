// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "is_number.h"

bool is_number(const std::string::const_iterator &start, const std::string::const_iterator &end) {
  std::string::const_iterator it = start;
  while (it != end && std::isdigit(*it))
    ++it;
  return it == end;
}
