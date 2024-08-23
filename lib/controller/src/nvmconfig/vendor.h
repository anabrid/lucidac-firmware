// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <list>

#include "nvmconfig/persistent.h"
#include "utils/uuid.h"

namespace nvmconfig {

    /**
     * Pseudo one-time-programmable information provided by vendor/manufacturer,
     * supposed to be stored in the persistent Settings in order to make them
     * independent from the Firmware image itself.
     * 
     * This kind of information was previously stored in @file(../build/distrubutor.h).
     * 
     */
    struct VendorOTP : nvmconfig::PersistentSettings {
        constexpr static uint16_t invalid_serial_number = 0;
        uint16_t serial_number = invalid_serial_number;
        utils::UUID serial_uuid;
        std::string default_admin_password;

        std::string name() const { return "immutable"; }

        bool is_valid() const { return serial_number != invalid_serial_number; }

        void reset_defaults() { /* No-OP by definition */ }
        void fromJson(JsonObjectConst src, Context c = Context::Flash) override {
            if(c == Context::User) return;
            JSON_GET(src, serial_number);
            JSON_GET(src, serial_uuid);
            JSON_GET_AS(src, default_admin_password, std::string);
        }
        void toJson(JsonObject target, Context c = Context::Flash) const override {
            if(c == Context::User) return;
            JSON_SET(target, serial_number);
            JSON_SET(target, serial_uuid);
            JSON_SET(target, default_admin_password);
        }
        static VendorOTP& get() {
            static VendorOTP instance;
            return instance;
        }
    };

    JSON_CONVERT_SUGAR(VendorOTP);
} // ns nvmconfig
