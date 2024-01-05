
#include "dist/distributor.h"

// Interestingly, FLASHMEM and PROGMEM *do* have an effect in Teensy,
// which is whether functions are copied into ICTM RAM or not. If not provided,
// it is up to the compiler (linker) to decide where to put stuff.

FLASHMEM const char* dist::ident() {
  return OEM_MODEL_NAME " Hybrid Controller (" FIRMWARE_VERSION ")";
}

FLASHMEM const char* dist::as_json(bool include_secrets) {
  return include_secrets ? distdb_AS_JSON : distdb_PUBLIC_AS_JSON;
}

FLASHMEM void dist::write_to_json(JsonObject target, bool include_secrets) {
  target["OEM"] = OEM;
  target["OEM_MODEL_NAME"] = OEM_MODEL_NAME;
  target["OEM_HARDWARE_REVISION"] = OEM_HARDWARE_REVISION;
  target["BUILD_SYSTEM_NAME"] = BUILD_SYSTEM_NAME;
  target["BUILD_SYSTEM_BOARD"] = BUILD_SYSTEM_BOARD;
  target["BUILD_SYSTEM_BOARD_MCU"] = BUILD_SYSTEM_BOARD_MCU;
  target["BUILD_SYSTEM_BUILD_TYPE"] = BUILD_SYSTEM_BUILD_TYPE;
  target["BUILD_SYSTEM_UPLOAD_PROTOCOL"] = BUILD_SYSTEM_UPLOAD_PROTOCOL;
  target["BUILD_FLAGS"] = BUILD_FLAGS;
  target["DEVICE_SERIAL_NUMBER"] = DEVICE_SERIAL_NUMBER;
  target["SENSITIVE_FIELDS"] = SENSITIVE_FIELDS;
  target["FIRMWARE_VERSION"] = FIRMWARE_VERSION;
  target["FIRMWARE_DATE"] = FIRMWARE_DATE;
  target["PROTOCOL_VERSION"] = PROTOCOL_VERSION;
  target["PROTOCOL_DATE"] = PROTOCOL_DATE;
  if(include_secrets) {
    target["DEVICE_SERIAL_UUID"] = DEVICE_SERIAL_UUID;
    target["DEVICE_SERIAL_REGISTRATION_LINK"] = DEVICE_SERIAL_REGISTRATION_LINK;
    target["DEFAULT_ADMIN_PASSWORD"] = DEFAULT_ADMIN_PASSWORD;
  }
}
