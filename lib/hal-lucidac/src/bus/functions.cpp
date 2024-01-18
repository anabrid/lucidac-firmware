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

#include "functions.h"

functions::Function::Function(const bus::addr_t address) : address(address) {}

void functions::Function::set_address() const { bus::address_function(address); }

void functions::Function::release_address() const { bus::release_address(); }

void functions::TriggerFunction::trigger() const {
  set_address();
  delayNanoseconds(2 * 42);
  release_address();
}

functions::DataFunction::DataFunction(bus::addr_t address, const SPISettings &spiSettings)
    : Function(address), spi_settings(spiSettings) {}

void functions::DataFunction::begin_communication() const {
  bus::spi.beginTransaction(spi_settings);
  set_address();
}

void functions::DataFunction::end_communication() const {
  release_address();
  bus::spi.endTransaction();
}

uint8_t functions::DataFunction::transfer(uint8_t data) const {
  begin_communication();
  auto ret = bus::spi.transfer(data);
  end_communication();
  return ret;
}

void functions::DataFunction::transfer(const void *mosi_buf, void *miso_buf, size_t count) const {
  begin_communication();
  bus::spi.transfer(mosi_buf, miso_buf, count);
  end_communication();
}

uint16_t functions::DataFunction::transfer16(uint16_t data) const {
  begin_communication();
  auto ret = bus::spi.transfer16(data);
  end_communication();
  return ret;
}

uint32_t functions::DataFunction::transfer32(uint32_t data) const {
  begin_communication();
  auto ret = bus::spi.transfer32(data);
  end_communication();
  return ret;
}

SPIClass &functions::DataFunction::get_raw_spi() { return bus::spi; }
