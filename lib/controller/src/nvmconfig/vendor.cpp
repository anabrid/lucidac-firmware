#include "nvmconfig/vendor.h"
#include <Arduino.h> // FLASHMEM

FLASHMEM
void nvmconfig::VendorOTP::fromJson(JsonObjectConst src, Context c) {
#ifndef ANABRID_WRITE_EEPROM
    if(c == Context::User) return;
#endif
    JSON_GET(src, serial_number);
    JSON_GET(src, serial_uuid);
    JSON_GET_AS(src, default_admin_password, std::string);
    JSON_GET_AS(src, default_user_password, std::string);
}

FLASHMEM
void nvmconfig::VendorOTP::toJson(JsonObject target, Context c) const{
#ifndef ANABRID_WRITE_EEPROM
    if(c == Context::User) return;
#endif
    JSON_SET(target, serial_number);
    JSON_SET(target, serial_uuid);
    JSON_SET(target, default_admin_password);
    JSON_SET(target, default_user_password);
}
