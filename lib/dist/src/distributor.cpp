#include "distributor.h"

void dist::Distributor::write_to_json(JsonObject target, bool include_secrets) const {
  target["OEM"] = oem_name();

  auto model = target.createNestedObject("model");
  model["name"] = model_name();
  
  auto appliance = target.createNestedObject("appliance");
  appliance["serial_number"] = appliance_serial_number();
  if(include_secrets) {
    appliance["serial_uuid"] = appliance_serial_uuid().toString();
    appliance["registration_link"] = appliance_registration_link();
  }

  auto versions = target.createNestedObject("versions");
  for(int i=0; i<3; i++) {
    versions["firmware"][i] = firmware_version().at(i);
    versions["protocol"][i] = protocol_version().at(i);
  }

  if(include_secrets) {
    auto passwords = target.createNestedObject("passwords");
    passwords["admin"] = default_admin_password();
  }

}