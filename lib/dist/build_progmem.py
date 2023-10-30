# This is a pio build script (see also scons.org)
# and can also be used as a standlone python3 script.

# The purpose of this script is to build distributor-specific
# data into the ROM. These data are not subject to be changed
# by users or their settings.

# Within the Pio build process,
# this python script is not invoked regularly as a seperate process but
# within the pio code. Therefore, magic such as this one can be called:

try:
  # the following code demonstrates how to access build system variables.
  # we don't make use of them right now.
  Import("env")
  #print(env.Dump())
  interesting_fields = ["BOARD", "BOARD_MCU", "BOARD_F_CPU", "BUILD_TYPE", "UPLOAD_PROTOCOL"]
  build_system = { k: env.Dictionary(k) for k in interesting_fields }
  build_flags = env.Dictionary('BUILD_FLAGS') # is a list
except NameError:
  # pure python does not know 'Import' (is not defined in pure Python).
  build_system = {}
  build_flags = []

import pathlib, json, textwrap, uuid, pprint

rel_filename = "src/distributor_values.cpp"
abs_filename = "lib/dist/" + rel_filename


try:
  project_dir = pathlib.Path(__file__).parent.resolve().parent.parent
  abs_filename = project_dir / abs_filename
except NameError:
  # pio environment does not know __file__. However, in PIO, all paths
  # are relative to the project folder, therefore things are fine.
  pass

print(f"OEM distribution generating infos at {abs_filename}")

# The following data are exemplaric and will be replaced with actual data one day.

item = {
    "OEM": "anabrid",
    "model": {
        "name": "LUCIDAC",
    },
    "build_system": {
      "name": "pio",
      "env": build_system,
      "build_flags": build_flags,
    },
    "device_serials": {
        "serial_number": "123",
        "uuid": "26051bb5-7cb7-43fd-8d64-e24fdfa14489",
        "registration_link": "https://anabrid.com/sn/lucidac/123/26051bb5-7cb7-43fd-8d64-e24fdfa14489"
    },
    "versions": { # semantic versioning
        "firmware": [ 0, 0, 1 ],
        "protocol": [ 0, 0, 1 ]
    },
    "passwords": {
        "admin": "Iesh8Sae"
    },
}

# uncomment this line to inspect the full data
# pprint.pprint(item)

nl = "\n"
esc = lambda s: s.replace('"', '\\"')
sugar = lambda fun: lambda **kv: "".join(fun(k,v) for k,v in kv.items())
cfunc = lambda retval, body: sugar(lambda funcname, v: f"{retval} dist::Distributor::{funcname}() const "+"{ return "+body(v)+"; }")

cstring = cfunc("String", lambda s: '"'+s.replace('"', '\\"')+'"')
cuuid = cfunc("utils::UUID", lambda uuid_as_string: "utils::UUID{"+",".join(hex(b) for b in uuid.UUID(uuid_as_string).bytes)+"}")
cversion = cfunc("utils::Version", lambda version_list: "utils::Version{"+', '.join(map(str,version_list))+"}")

#cstring = lambda **kv: "".join(k+" = F(\""+esc(v)+"\");"+nl for k,v in kv.items())
#cuuid = lambda **kv: "".join(k+" = UUID{"+",".join(hex(b) for b in uuid.UUID(v).bytes)+"};"+nl for k,v in kv.items())
#cversion = lambda **kv: "".join(k+" = Version{"+', '.join(map(str,v))+"};"+nl for k,v in kv.items())


cpp_code = \
"""// This file was written by dist/build_progmem.py and is not supposed to be git-versioned

#include <avr/pgmspace.h> // F() macro, PROGMEM

#include "distributor.h"

""" + "\n".join([
  cstring(oem_name=item["OEM"]),
  cstring(model_name=item["model"]["name"]),
  cstring(appliance_serial_number=item["device_serials"]["serial_number"]),
  cuuid(appliance_serial_uuid=item["device_serials"]["uuid"]),
  cstring(appliance_registration_link=item["device_serials"]["registration_link"]),
  cversion(firmware_version=item["versions"]["firmware"]),
  cversion(protocol_version=item["versions"]["protocol"]),
  cstring(default_admin_password=item["passwords"]["admin"]),
]) + nl


pathlib.Path(abs_filename).write_text(cpp_code)

# just to make sure the generated code does not touch the git repository.
dist_src = pathlib.Path(abs_filename).parent.parent
(dist_src / ".gitignore").write_text(rel_filename + "\n")
