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

size_t eeprom::UserSettings::write_to_eeprom() {
    StaticJsonDocument<eeprom_size> serialized_conf;
    write_to_json(serialized_conf.to<JsonObject>());
    StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
    size_t consumed_size = serializeJson(serialized_conf, eepromStream);

    Serial.printf("eeprom::UserSettings: Consumed %d Bytes from %d Available ones (%.2f%%)\n",
      consumed_size, eeprom_size, 100.0 * consumed_size/eeprom_size);

    return consumed_size;
}

bool msg::handlers::GetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    settings.write_to_json(msg_out);
    return true;
}

bool msg::handlers::SetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    settings.read_from_json(msg_in);
    settings.write_to_json(msg_out.createNestedObject("updated_config"));
    msg_out["available_bytes"] = eeprom_size;
    msg_out["consumed_bytes"] = settings.write_to_eeprom();
    return true;
}

bool msg::handlers::ResetSettingsHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    settings.reset_defaults();
    return true;
}
