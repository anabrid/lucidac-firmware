// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

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

template <size_t N> typename RunningAverageVec<N>::data_t RunningAverageVec<N>::add(RunningAverageVec::data_t data) {
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