# This is a pio build script, building distributor-specific
# data into the ROM. These data are not subject to be changed
# by users or their settings.

# this python script is not invoked regularly as a seperate process but
# within the pio code. Therefore, magic such as this one can be called:
# Import("env")

import pathlib, json
dist_dir = pathlib.Path(__file__).parent.resolve()
relative_filename = "src/dist_progmem.h"
absolute_filename = dist_dir / relative_filename

print(f"OEM DISTRIBUTION generating {absolute_filename}")

# The following data are exemplaric and will be replaced with actual data one day.

dist_data = {
    "OEM": "anabrid",
    "model": {
        "name": "LUCIDAC",
    },
    "device_serials": {
        "serial_number": "123",
        "uuid": "26051bb5-7cb7-43fd-8d64-e24fdfa14489",
        "registration_link": "https://anabrid.com/sn/lucidac/123/26051bb5-7cb7-43fd-8d64-e24fdfa14489"
    },
    "versions": { # semantic versioning
        "firmware": "0.0.1",
        "protocol": "0.0.1"
    },
    "passwords": {
        "admin": "Iesh8Sae"
    },
}

serialized = json.dumps(dist_data, sort_keys=True)
serialized_escaped_json = serialized.replace('"', '\\"')

cpp_code = \
"""// This file was written by dist/build_progmem.py and is not supposed to be git-versioned

#include <avr/pgmspace.h>

namespace dist {
    char* get_stanza() {
        const char data[] PROGMEM = {"%(serialized_escaped_json)s"};
        return data;
    }
}
""" % { 'serialized_escaped_json': serialized_escaped_json }

pathlib.Path(absolute_filename).write_text(cpp_code)

# just to make sure the generated code does not touch the git repository.
pathlib.Path(dist_dir / ".gitignore").write_text(relative_filename + "\n")
