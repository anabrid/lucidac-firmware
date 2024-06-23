#include <ArduinoJson.h>
#include <StreamUtils.hpp>

#include "utils/logging.h"
#include "nvmconfig/persistent.h"

void nvmconfig::PersistentSettingsWriter::info(JsonObject msg_out) {
//    read_from_json(serialized_conf);
    //write_to_json(msg_out.createNestedObject("updated_config"));
    msg_out["available_bytes"] = eeprom_size;
    msg_out["consumed_bytes"] = write_to_eeprom();
}

void nvmconfig::PersistentSettingsWriter::toJson(JsonObject target) {
  for(auto const& sys : subsystems)
    sys->toJson(target.createNestedObject(sys->name()));
}

void nvmconfig::PersistentSettingsWriter::fromJson(JsonObjectConst target) {
  for(auto const& sys : subsystems)
    sys->fromJson(target[sys->name()]);
}

void nvmconfig::PersistentSettingsWriter::reset_defaults(bool do_write_to_eeprom) {
  for(auto const& sys : subsystems)
    sys->reset_defaults();
  if(do_write_to_eeprom)
    write_to_eeprom();
}

void nvmconfig::PersistentSettingsWriter::read_from_eeprom() {
    StaticJsonDocument<eeprom_size> deserialized_conf;
    StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
    auto error = 
        use_messagepack
        ? deserializeJson(deserialized_conf, eepromStream)
        : deserializeMsgPack(deserialized_conf, eepromStream);
    if(error) {
        LOG_ERROR("nvmconfig::PersistentSettingsWriter::read_from_eeprom(): Failure, will fall back to default values.");
        LOG4("DeserializationError code: ", error.code(), " and explanation: ", error.c_str());
    }

    version = deserialized_conf["version"];

    if(!error && version >= required_magic) {
        fromJson(deserialized_conf.as<JsonObjectConst>());
    } else {
        reset_defaults(/*write_to_eeprom*/ false);
    }
}

size_t nvmconfig::PersistentSettingsWriter::write_to_eeprom() {
    StaticJsonDocument<eeprom_size> serialized_conf;
    toJson(serialized_conf.to<JsonObject>());
    StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
    size_t consumed_size =
        use_messagepack
        ? serializeMsgPack(serialized_conf, eepromStream)
        : serializeJson(serialized_conf, eepromStream);

    LOGMEV("Consumed %d Bytes from %d available ones (%.2f%%); Serialization: %s\n",
      consumed_size, eeprom_size, 100.0 * consumed_size/eeprom_size,
      use_messagepack ? "Messagepack" : "JSON"
    );

    return consumed_size;
}
