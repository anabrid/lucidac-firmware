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
  Import("env")
  #print(env.Dump())
  interesting_fields = ["BOARD", "BUILD_TYPE", "UPLOAD_PROTOCOL"]
  build_system = { k: env.Dictionary(k) for k in interesting_fields }
  build_flags = env.Dictionary('BUILD_FLAGS') # is a list
except (NameError, KeyError):
  # NameError: pure python does not know 'Import' (is not defined in pure Python).
  # KeyError: doesn't know some of the interesting_fields keys
  build_system = {}
  build_flags = []

# all the following libs are python built-ins:
import pathlib, json, textwrap, uuid, pprint, subprocess, datetime, re

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
warn = lambda msg: print("controller/build_distributor.py: WARN ", msg)

# Read off the current firmware version from the latest git tag. There are alternatives, see
# https://setuptools-git-versioning.readthedocs.io/en/stable/differences.html for a list, but
# in the end they all call external git and parse the output.
firmware_version = subprocess.getoutput("which git >/dev/null && git describe --tags || echo no-git-avail").strip()
firmware_version_useful = bool(re.match(r"\d+\.\d+-\d+.*", firmware_version)) # a proper string is for instance "0.2-80-g302f016"
not_available = "N/A" # placeholder string used instead
if not firmware_version_useful: firmware_version = not_available # instead of subprocess garbage

# Use the current commit time as firmware date approximator.
# Do not use the current time, as we want to have reproducable builds.
try:
  unix_timestamp = subprocess.getoutput("which git >/dev/null && git log -1 --format=%ct || echo failure").strip()
  firmware_version_date = datetime.datetime.fromtimestamp(int(unix_timestamp)).isoformat()
except ValueError:
  print("Read: ", unix_timestamp)
  warn("No git available, have no information about version and date at all.")
  firmware_version_date = not_available

rename_keys = lambda dct, prefix='',suffix='': {prefix+k+suffix:v for k,v in dct.items()}

# Given that firmware builds are supposed to be published for allowing users
# to update their LUCIDAC on their own, do not include things like device
# serial keys here. Instead of the firmware, they shall be stored in "EEPROM",
# cf. the src/nvmconfig/ subsystem.

distdb = dict(
    OEM = "anabrid",
    OEM_MODEL_NAME = "LUCIDAC",

    BUILD_SYSTEM_NAME = "pio",
    **rename_keys(build_system, prefix="BUILD_SYSTEM_"),
    BUILD_FLAGS = " ".join(build_flags),

    FIRMWARE_VERSION = firmware_version,
    FIRMWARE_DATE = firmware_version_date,

    PROTOCOL_VERSION = "0.0.2", # FIXME
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
distdb_as_json_defines = "\n".join(f"  target[{quote}{k}{quote}] = {k};" for k,v in distdb.items())

code_files = {}

code_files["distributor_generated.h"] = \
"""
// This file was written by dist/build_progmem.py and is not supposed to be git-versioned

// This file is not supposed to be included directly. Include "distributor.h" instead.

%(distdb_as_defines)s

#define distdb_AS_JSON "%(distdb_as_json_string_esc)s"

"""

code_files["distributor_generated.cpp"] = \
"""
#ifdef ARDUINO
#include "build/distributor.h"

// Interestingly, FLASHMEM and PROGMEM *do* have an effect in Teensy,
// which is whether functions are copied into ICTM RAM or not. If not provided,
// it is up to the compiler (linker) to decide where to put stuff.

FLASHMEM const char* dist::ident() {
  return OEM_MODEL_NAME " Hybrid Controller (" FIRMWARE_VERSION ")";
}

FLASHMEM const char* dist::as_json() {
  return distdb_AS_JSON;
}

FLASHMEM void dist::write_to_json(JsonObject target) {
%(distdb_as_json_defines)s
}
#endif /* ARDUINO */
"""

for fname, content in code_files.items():
  pathlib.Path(abs_src_dir / fname).write_text(content % locals())

# just to make sure the generated code does not touch the git repository.
(lib_dir / ".gitignore").write_text("".join( f"{rel_src_dir}/{fname}\n" for fname in code_files.keys()))
