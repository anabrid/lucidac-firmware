#include <ArduinoJson.h>
#include <StreamUtils.hpp>

#include "logging.h"
#include "eeprom.h"


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

void eeprom::UserSettings::reset_defaults() {
    Serial.print("Resetting settings, EEPROM version was: ");
    Serial.print(version, HEX);
    Serial.print(", Firmware version is");
    Serial.print(required_magic, HEX);
    Serial.println(".");

    ethernet.reset_defaults();
    auth.reset_defaults();

    version = required_magic;
    write_to_eeprom();
}

void eeprom::UserSettings::read_from_json(JsonObjectConst serialized_conf) {
    if(serialized_conf.containsKey("ethernet"))
        ethernet.read_from_json(serialized_conf["ethernet"]);
    
    if(serialized_conf.containsKey("passwords"))
        auth.read_from_json(serialized_conf["passwords"]);

}

void eeprom::UserSettings::write_to_json(JsonObject target) {
    target["version"] = version;

    ethernet.write_to_json(target.createNestedObject("ethernet"));
    auth.write_to_json(target.createNestedObject("passwords"));

    //LOG(ANABRID_DEBUG_INIT, "UserSettings::write_to_json produced this serialization:");
    serializeJson(target, Serial);
}

void eeprom::UserSettings::read_from_eeprom() {
    StaticJsonDocument<eeprom_size> deserialized_conf;
    StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
    //LOG(ANABRID_DEBUG_INIT, "Initialized EEPROM Stream");
    auto error = deserializeJson(deserialized_conf, eepromStream);
    if(error) {
        Serial.print("Error while reading configuration from EEPROM:");
        Serial.println(error.c_str());
    }

    //LOG(ANABRID_DEBUG_INIT, "Finished reading EEPROM Stream");

    version = deserialized_conf["version"];

    if(!error && version >= required_magic) {
        read_from_json(deserialized_conf.as<JsonObjectConst>());
    } else {
        reset_defaults();
    }
}

void eeprom::UserSettings::write_to_eeprom() {
    //LOG(ANABRID_DEBUG_INIT, "Dumping configuration to EEPROM...");
    StaticJsonDocument<eeprom_size> serialized_conf;
    write_to_json(serialized_conf.as<JsonObject>());
    //LOG(ANABRID_DEBUG_INIT, "Finished a JSON document in RAM, now writing to EEPROM");
    StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
    serializeJson(serialized_conf, eepromStream);
    //LOG(ANABRID_DEBUG_INIT, "Finished Dumping configuration to EEPROM.");
}

bool msg::handlers::GetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    settings.write_to_json(msg_out);
    return true;
}

bool msg::handlers::SetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    settings.read_from_json(msg_in);
    settings.write_to_eeprom();
    return true;
}

bool msg::handlers::ResetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    settings.reset_defaults();
    return true;
}
