

# this is a POST script, cf. https://docs.platformio.org/en/latest/scripting/launch_types.html#scripting-launch-types

# cf. ~/.platformio/platforms/teensy/builder/main.py

print("Hi from plugin_build_system")

from os.path import join, basename
from SCons.Script import AlwaysBuild

Import("env")


fh = open(join("/tmp/", "dump.txt"), "w")
fh.write(env.Dump())
fh.close()

fh = open(join("/tmp/", "expanded.txt"), "w")
for k,v in env.Dictionary().items():
    fh.write("%s = %s = %s\n" % (k, v, env.subst('$'+k)))
fh.close()



###############################################################################
# Job 1) Build firmware.elf.a
###############################################################################

# The following makes sure something like firmware.elf.a is produced,
# which is required to have something to link against, because the static compiled
# executable (firmware) contains no more symbol table.
target_elf = join("$BUILD_DIR", "${PROGNAME}.elf")
target_elf_a = target_elf + ".a"
env.Append(
    LINKFLAGS=[
        "-Wl,--out-implib," + target_elf_a
    ],
)
print("lib/loader/plugin_build_system.py will also build ", basename(env.subst(target_elf_a)))

###############################################################################
# Job 2) Build firmware.bin
###############################################################################

# The following is a makefile'y way to execute something like
#    objcopy --input-target=ihex --output-target=binary firmware.hex firmware.bin
# at the correct time.

# target_bin = env.ElfToBin(join("$BUILD_DIR", "${PROGNAME}"), target_elf) # doesn't work
# AlwaysBuild(target_bin) # doesn't work
  
env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(" ".join([
        "$OBJCOPY", "-O", "binary", "-I", "ihex",
        "$BUILD_DIR/${PROGNAME}.elf", "$BUILD_DIR/${PROGNAME}.bin"
    ]), "Building $BUILD_DIR/${PROGNAME}.bin")
)

###############################################################################
# Job 3) Extract CFlags, etc for a custom Makefile
###############################################################################

expand = lambda var: env.subst('$'+var)

# still missing: The crazy amount of include paths

fh = open(join("/tmp/", "Makefile.txt"), "w")
for var in "CCFLAGS CXXFLAGS CPPFLAGS _CPPDEFFLAGS _CPPINCFLAGS CC CXX OBJCOPY PROGPATH".split():
    fh.write("%s = %s\n" % (var, expand(var)))
  
# path contains path to arm-none-eabi-gcc and friends
path = env['ENV']['PATH'] # same as os.environ['PATH']
fh.write("PATH = "+path)

fh.close()
