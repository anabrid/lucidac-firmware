 #!/usr/bin/env python3

import subprocess, hashlib, base64, pathlib # batteries included
P = pathlib.Path
sh = lambda arg: subprocess.check_output(arg, shell=True)

from simplehc import HybridController # real dependency

sh("make firmware.bin")
firmware_bin = P("firmware.bin").read_bytes()
firmware_len = len(firmware_bin)
firmware_sha256 = hashlib.sha256(firmware_bin).hexdigest()
load_addr = int(sh("make PRINT_LOAD_ADDR"), 16) # could also directly use some elf reader

#sh("make")
entry_point = int(sh("make PRINT_ENTRY_POINT"), 16)
payload = base64.b64encode(P("payload.bin").read_bytes())

# check whether can load plugins and for compatibility
hc = simplehc.HybridController("192.168.68.60", 5732)

status = hc.query("status", {"flashimage": True, "plugin": True})
assert status.flashimage.size == firmware_len
assert status.flashimage.sha256sum == firmware_sha256

assert status.plugin.type == "GlobalPluginLoader"
assert status.plugin.can_load
assert status.plugin.load_addr == load_addr
assert status.plugin.memsize >= len(payload)

# exit_point = ...
immediately_unload = True

entry_ret_val = hc.query("load_plugin", dict(
    entry=entry_point,
    load_addr=load_addr,
    immediately_unload=immediately_unload,
    payload=payload,
    firmware_sha256=firmware_sha256
))

# interact, for instance with custom queries.
# or make use of the return value in case of nontrivial plugins

