#include "nvmconfig/persistent.h"

#ifdef ARDUINO

#include <ArduinoJson.h>
#include <StreamUtils.hpp>

#include "utils/logging.h"
#include "utils/streaming_json.h" // only for debugging

FLASHMEM void debug_print_json(JsonObjectConst thing) {
  utils::StreamingJson s(Serial);
  s.begin_dict();
  s.kv("type", "log");
  s.kv("source", "manual_debug_log");
  s.key("thing");
  serializeJson(thing, Serial);
  s.end_dict();
  s.endln();
}

FLASHMEM void nvmconfig::PersistentSettingsWriter::info(JsonObject msg_out) {
  //    read_from_json(serialized_conf);
  // write_to_json(msg_out.createNestedObject("updated_config"));
  msg_out["available_bytes"] = eeprom_size;
  msg_out["consumed_bytes"] = write_to_eeprom();
}

FLASHMEM void nvmconfig::PersistentSettingsWriter::toJson(JsonObject target, Context c) {
  target["version"] = version;
  for (auto const &sys : subsystems) {
    sys->toJson(target.createNestedObject(sys->name()), c);
    if (!target[sys->name()].size())
      target.remove(sys->name());
  }
}

FLASHMEM void nvmconfig::PersistentSettingsWriter::fromJson(JsonObjectConst target, Context c) {
  for (auto const &sys : subsystems) {
    if (target.containsKey(sys->name()))
      sys->fromJson(target[sys->name()], c);
  }
}

FLASHMEM void nvmconfig::PersistentSettingsWriter::reset_defaults(bool do_write_to_eeprom) {
  version = required_magic;
  for (auto const &sys : subsystems)
    sys->reset_defaults();
  if (do_write_to_eeprom)
    write_to_eeprom();
}

FLASHMEM void nvmconfig::PersistentSettingsWriter::read_from_eeprom() {
  DynamicJsonDocument deserialized_conf_doc(eeprom_size);
  StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
  auto error = use_messagepack ? deserializeMsgPack(deserialized_conf_doc, eepromStream)
                               : deserializeJson(deserialized_conf_doc, eepromStream);
  if (error) {
    LOG_ERROR(
        "nvmconfig::PersistentSettingsWriter::read_from_eeprom(): Failure, will fall back to default values.");
    LOG4("DeserializationError code: ", error.code(), " and explanation: ", error.c_str());
  }

  if(deserialized_conf_doc.isNull()) {
    LOG_ERROR("EEPROM JSON Document is empty");
  }

  auto deserialized_conf = deserialized_conf_doc.as<JsonObject>();

  //LOG_ALWAYS("PersistentSettingsWriter::read_from_eeprom has read this data...");
  //debug_print_json(deserialized_conf);

  version = deserialized_conf["version"];

  if (!error && version >= required_magic) {
    fromJson(deserialized_conf, Context::Flash);
  } else {
    LOG_ERROR("PersistentSettingsWriter::read_from_eeprom(): Invalid Magic, falling back to default values.");
    LOG4("Required magic byte values: ", required_magic, " Read magic byte values: ", version);
    reset_defaults(/*write_to_eeprom*/ false);
  }
}

FLASHMEM size_t nvmconfig::PersistentSettingsWriter::write_to_eeprom() {
  DynamicJsonDocument serialized_conf_doc(eeprom_size);
  auto serialized_conf = serialized_conf_doc.to<JsonObject>();
  toJson(serialized_conf, Context::Flash);
  StreamUtils::EepromStream eepromStream(eeprom_address, eeprom_size);
  size_t consumed_size = use_messagepack ? serializeMsgPack(serialized_conf, eepromStream)
                                         : serializeJson(serialized_conf, eepromStream);
  eepromStream.flush();

  //LOG_ALWAYS("PersistentSettingsWriter::write_to_eeprom has written to EEPROM...");
  //debug_print_json(serialized_conf);

  LOGMEV("Consumed %d Bytes from %d available ones (%.2f%%); Serialization: %s\n", consumed_size, eeprom_size,
         100.0 * consumed_size / eeprom_size, use_messagepack ? "Messagepack" : "JSON");

  return consumed_size;
}

#endif // ARDUINO
