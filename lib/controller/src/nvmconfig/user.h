// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <list>

#include "nvmconfig/persistent.h"

namespace nvmconfig {

    /**
     * Permanent *untyped* arbitrary information which a device user wants to store.
     * This can be, for instance, additional information in an enterprise network
     * (such as device location or contact name), but also index informaiton for
     * location the lucidac in a larger array of similiar devices.
     * 
     * This class is actually just a thin layer over @see PersistentSettings.
     * 
     * Please don't store too much information, @see nvmconfig::PersistentSettingsWriter
     * for maximum storage sizes (around 4kB).
     */
    struct PermanentUserDefinedStuff : nvmconfig::PersistentSettings {
        int max_doc_bytes = 600;
        DynamicJsonDocument *doc;

        std::string name() const { return "user"; }
        void reset_defaults() { delete doc; }
        void clear() { delete doc; }
        void fromJson(JsonObjectConst src) {
            if(!doc) doc = new DynamicJsonDocument(max_doc_bytes);
            else doc->clear();
            *doc = src;
        }
        void toJson(JsonObject target) const {
            if(doc) target = doc->as<JsonObject>();
        }

        static PermanentUserDefinedStuff& get() {
            static PermanentUserDefinedStuff instance;
            return instance;
        }
    };

    //JSON_CONVERT_SUGAR(VendorOTP);
} // ns nvmconfig
