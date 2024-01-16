# This is a pio build script (see also scons.org)
# and can also be used as a standlone python3 script.
#
# The purpose of this script is to build distributor-specific
# data into the ROM. These data are not subject to be changed
# by users or their settings. The data is stored as a simple
# key-value "database"/dictionary.
#
# Note that Teensy 3 is 32bit ARM and any string literals defined
# with "const" go automatically into flash memory, there is no more
# need for PROGMEM, F() and similar.
#
# Note that we want to foster reproducable builds (deterministic
# compilation), thus things as build time, build system paths or
# the build system hostname is not included.
#
# Within the Pio build process, this python script is not invoked
# regularly as a seperate process but within the pio code.
# Therefore, magic such as Import("env") can be evaluated within pio.

try:
  # the following code demonstrates how to access build system variables.
  # we don't make use of them right now.
  Import("env")
  #print(env.Dump())
  interesting_fields = ["BOARD", "BOARD_MCU", "BUILD_TYPE", "UPLOAD_PROTOCOL"]
  build_system = { k: env.Dictionary(k) for k in interesting_fields }
  build_flags = env.Dictionary('BUILD_FLAGS') # is a list
except (NameError, KeyError):
  # NameError: pure python does not know 'Import' (is not defined in pure Python).
  # KeyError: doesn't know some of the interesting_fields keys
  build_system = {}
  build_flags = []    

import pathlib, json, textwrap, uuid, pprint, subprocess, datetime

lib_dir = pathlib.Path("./").resolve() # use this if called from library.json
#lib_dir = pathlib.Path("./lib/controller/").resolve() # use this if called from platform.ini
rel_src_dir = "src/build" # without trailing slash
abs_src_dir = pathlib.Path(lib_dir) / rel_src_dir

try:
  lib_dir = pathlib.Path(__file__).parent.resolve()
  abs_src_dir = lib_dir / rel_src_dir
except NameError:
  # pio environment does not know __file__. However, in PIO, all paths
  # are relative to... well, where? the project folder, therefore things are fine.
  pass

print(f"OEM distribution generating infos at {abs_src_dir}")
warn = lambda msg: print("StandaloneENV/build_progmem.py: WARN ", msg)

firmware_version = subprocess.getoutput("which git >/dev/null && git describe --tags || echo no-git-avail").strip()
# For alternatives, see https://setuptools-git-versioning.readthedocs.io/en/stable/differences.html
# but in the end they all call external git and parse the output.

# Use the current commit time as date approximator.  
try:
  unix_timestamp = subprocess.getoutput("which git >/dev/null && git log -1 --format=%ct || echo failure").strip()
  firmware_version_date = datetime.datetime.fromtimestamp(int(unix_timestamp)).isoformat()
except ValueError:
  print("Read: ", unix_timestamp)
  warn("No git available, have no information about version and date at all.")
  firmware_version_date = 'unavailable'

rename_keys = lambda dct, prefix='',suffix='': {prefix+k+suffix:v for k,v in dct.items()}

# The following data are exemplaric and will be replaced with actual data one day.

distdb = dict(
    OEM = "anabrid",
    OEM_MODEL_NAME = "LUCIDAC",

    # hardware revision should actually be detectable from the boards but if
    # it is not, this would be a place to describe it

    OEM_HARDWARE_REVISION = "LUCIDAC-v1.2.3",

    BUILD_SYSTEM_NAME = "pio",
    **rename_keys(build_system, prefix="BUILD_SYSTEM_"),
    BUILD_FLAGS = " ".join(build_flags),

    # These values are supposed to be determined when the device finishes
    # self-tests and leaves the house, similar to THAT serial number procedure.
    DEVICE_SERIAL_NUMBER = "123",
    DEVICE_SERIAL_UUID = "26051bb5-7cb7-43fd-8d64-e24fdfa14489",
    DEVICE_SERIAL_REGISTRATION_LINK = "https://anabrid.com/sn/lucidac/123/26051bb5-7cb7-43fd-8d64-e24fdfa14489",
    DEFAULT_ADMIN_PASSWORD = "Iesh8Sae",

    # the previous fields are actually secrets and should only be told on a secret channel.
    SENSITIVE_FIELDS = 'DEVICE_SERIAL_UUID DEVICE_SERIAL_REGISTRATION_LINK DEFAULT_ADMIN_PASSWORD',

    FIRMWARE_VERSION = firmware_version,
    FIRMWARE_DATE = firmware_version_date,

    PROTOCOL_VERSION = "0.0.1", # FIXME
    PROTOCOL_DATE = firmware_version_date, # FIXME
)

# uncomment this line to inspect the full data
# pprint.pprint(item)

for k,v in distdb.items():
  assert isinstance(v,str), f"Expected {k} to be string but it is: {v}"

nl, quote = "\n", '"'
esc = lambda s: s.replace(quote, '\\"')

distdb_as_variables = "\n".join(f"const char {k}[] PROGMEM = {quote}{esc(v)}{quote};" for k,v in distdb.items())
distdb_variable_lookup = "\n".join(f"  if(strcmp_P(key, (PGM_P)F({quote}{k}{quote}) )) return {k};" for k,v in distdb.items())
distdb_as_defines = "\n".join(f"#define {k} {quote}{esc(v)}{quote}" for k,v in distdb.items())

distdb_as_json_string_esc = esc(json.dumps(distdb));

public_distdb = { k:v for k,v in distdb.items() if not k in distdb["SENSITIVE_FIELDS"].split() }
secret_distdb = { k:v for k,v in distdb.items() if     k in distdb["SENSITIVE_FIELDS"].split() }
distdb_public_as_json_string_esc = esc(json.dumps(public_distdb))

distdb_public_json_defines = "\n".join(f"  target[{quote}{k}{quote}] = {k};" for k,v in public_distdb.items())
distdb_secret_json_defines = "\n".join(f"    target[{quote}{k}{quote}] = {k};" for k,v in secret_distdb.items())

code_files = {}

code_files["distributor_generated.h"] = \
"""
// This file was written by dist/build_progmem.py and is not supposed to be git-versioned

// This file is not supposed to be included directly. Include "distributor.h" instead.

%(distdb_as_defines)s

#define distdb_AS_JSON "%(distdb_as_json_string_esc)s"

#define distdb_PUBLIC_AS_JSON "%(distdb_public_as_json_string_esc)s"

"""

code_files["distributor_generated.cpp"] = \
"""
#include "build/distributor.h"

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
%(distdb_public_json_defines)s
  if(include_secrets) {
%(distdb_secret_json_defines)s
  }
}
"""

for fname, content in code_files.items():
  pathlib.Path(abs_src_dir / fname).write_text(content % locals())

# just to make sure the generated code does not touch the git repository.
(lib_dir / ".gitignore").write_text("".join( f"{rel_src_dir}/{fname}\n" for fname in code_files.keys()))
