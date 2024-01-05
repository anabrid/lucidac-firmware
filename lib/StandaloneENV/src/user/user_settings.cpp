#include <ArduinoJson.h>
#include <StreamUtils.hpp>

#include "utils/logging.h"
#include "user/user_settings.h"

// global singleton storage
user::settings::UserSettingsImplementation user::UserSettings;


// Teensy EEPROM is 4KB in size. This class claims the beginning
// of this storage. The required size is generously estimated, may be much less.
// Remember increasing the storage when more data have to be stored.
static constexpr int
    eeprom_address = 0,
    eeprom_size = 2'000;

// This number checks as a proof that the EEPROM stores something senseful. If it
// does not store a number >= this constant, it is resetted.
// Must not start from 0 in order to distinguish from "value not found" errors,
// thus the arbitrary nonzero 0xAA prefix.
static constexpr uint64_t required_magic = 0xAA00;

void user::settings::JsonFlashUserSettings::reset_defaults() {
    LOGMEV("Resetting settings, EEPROM version was:  %X, Firmware version is %X.", version, HEX, required_magic, HEX);

    ethernet.reset_defaults();
    auth.reset_defaults();

    version = required_magic;
    write_to_eeprom();
}

void user::settings::JsonFlashUserSettings::read_from_json(JsonObjectConst serialized_conf) {
    if(serialized_conf.containsKey("ethernet"))
        ethernet.read_from_json(serialized_conf["ethernet"]);
    
    if(serialized_conf.containsKey("passwords"))
        auth.read_from_json(serialized_conf["passwords"]);

}

void user::settings::JsonFlashUserSettings::write_to_json(JsonObject target) {
    target["version"] = version;

    ethernet.write_to_json(target.createNestedObject("ethernet"));
    auth.write_to_json(target.createNestedObject("passwords"));
}

void user::settings::JsonFlashUserSettings::read_from_eeprom() {
    StaticJsonDocument<eeprom_size> deserialized_conf;
    StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
    auto error = deserializeJson(deserialized_conf, eepromStream);
    if(error) {
        Serial.print("Error while reading configuration from EEPROM:");
        Serial.println(error.c_str());
    }

    version = deserialized_conf["version"];

    if(!error && version >= required_magic) {
        read_from_json(deserialized_conf.as<JsonObjectConst>());
    } else {
        reset_defaults();
    }
}

size_t user::settings::JsonFlashUserSettings::write_to_eeprom() {
    StaticJsonDocument<eeprom_size> serialized_conf;
    write_to_json(serialized_conf.to<JsonObject>());
    StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
    size_t consumed_size = serializeJson(serialized_conf, eepromStream);

    LOGMEV("Consumed %d Bytes from %d Available ones (%.2f%%)\n",
      consumed_size, eeprom_size, 100.0 * consumed_size/eeprom_size);

    return consumed_size;
}

int msg::handlers::GetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    user::UserSettings.write_to_json(msg_out);
    return success;
}

int msg::handlers::SetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    user::UserSettings.read_from_json(msg_in);
    user::UserSettings.write_to_json(msg_out.createNestedObject("updated_config"));
    msg_out["available_bytes"] = eeprom_size;
    msg_out["consumed_bytes"] = user::UserSettings.write_to_eeprom();
    return success;
}

int msg::handlers::ResetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    user::UserSettings.reset_defaults();
    return success;
}
