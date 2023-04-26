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

#include <stdint.h>
#include <array>
#include <core_pins.h>


namespace local_bus {

    constexpr std::array<uint8_t, 4> PINS_BADDR = {14, 15, 40, 41};
    constexpr uint8_t PINS_BADDR_BITS_OFFSET = CORE_PIN14_BIT;
    constexpr std::array<uint8_t, 6> PINS_FADDR = {17, 16, 22, 23, 20, 21};
    constexpr uint8_t PINS_FADDR_BITS_OFFSET = CORE_PIN17_BIT;

    void init() {
        for (const auto pin: PINS_BADDR) {
            pinMode(pin, OUTPUT);
        }
        for (const auto pin: PINS_FADDR) {
            pinMode(pin, OUTPUT);
        }
    }

    /**
     * Address an index (0-15) on the local bus.
     * @timing ~25ns (full function call and propagation to final destination)
     * @param idx The index of the block (1-15) or the carrier board (0).
     */
    void address_block_idx(uint8_t idx) {
        uint32_t mask = (idx & 0xF) << PINS_BADDR_BITS_OFFSET;
        GPIO6_DR = (GPIO6_DR & ~(0xF << PINS_BADDR_BITS_OFFSET)) | mask;
    }

    /**
     * Address a block inside a cluster.
     * @timing see address_block_idx
     * @param cluster_idx The index of the cluster, from 0 to 2.
     * @param block_idx The index of the block (0=I, 1=C, 2=U, 3=M1, 4=M2).
     */
    void address_block(uint8_t cluster_idx, uint8_t block_idx) {
        address_block_idx(cluster_idx * 5 + block_idx);
    }

    /**
     * Address a function without changing the current block address.
     * @timing see address_block_idx
     * @param func_idx The index of the function, from 0 to 63.
     */
    void address_function_raw(uint8_t func_idx) {
        uint32_t mask = (func_idx & 0x3F) << PINS_FADDR_BITS_OFFSET;
        GPIO6_DR = (GPIO6_DR & ~(0x3F << PINS_FADDR_BITS_OFFSET)) | mask;
    }

    /**
     * Address a function of a block of a cluster.
     * @timing see address_block_idx
     * @param cluster_idx The index of the cluster, from 0 to 2.
     * @param block_idx The index of the block, from 0 to 4 (0=I, 1=C, 2=U, 3=M1, 4=M2).
     * @param func_idx The index of the function, from 0 to 63.
     */
    void address_function(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
        uint32_t mask = (((cluster_idx * 5 + block_idx) & 0xF) << PINS_BADDR_BITS_OFFSET) |
                        ((func_idx & 0x3F) << PINS_FADDR_BITS_OFFSET);
        GPIO6_DR = (GPIO6_DR & ~(0x3FF << PINS_BADDR_BITS_OFFSET)) | mask;
    }

}
