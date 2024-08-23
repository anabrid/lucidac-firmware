# This is a Python SCons script loading other scripts

Import("env")
env.SConscript([
    "build_distributor.py",
    "build_plugin_system.py"
], exports="env")