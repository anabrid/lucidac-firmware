# This PlatformIO Scons script is supposed to gather all information to make plugins
# work (i.e. compile but also actually work) against the firmware built at the time
# this Scons script you are currently viewing runs.
#
# That means, ideally this file does the following:
#  - Make sure an import library file (*.elf.a) is built from the static firmware
#    because it is needed to link the plugin against.
#  - "Freeze" the full build environment, ideally including all Cflags *and* all
#    relevant header files with their correct path structure. That is, this script
#    is supposed to prepare/set up the build system for plugins that can compile
#    and run against exactly this firmware version. Basically that is part of the
#    definition of a Compilation Database.
#
# Currently, the script only creates the *.elf.a file, nothing else.

# this is a POST script, cf. https://docs.platformio.org/en/latest/scripting/launch_types.html#scripting-launch-types
# cf. ~/.platformio/platforms/teensy/builder/main.py

import os
from os.path import join, basename
from SCons.Script import AlwaysBuild

Import("env")

understand_the_build_system = True
if understand_the_build_system:

    fh = open(join("/tmp/", "dump.txt"), "w")
    fh.write(env.Dump())
    fh.close()

    fh = open(join("/tmp/", "expanded.txt"), "w")
    for k,v in env.Dictionary().items():
        fh.write("%s = %s = %s\n" % (k, v, env.subst('$'+k)))
    fh.close()

me = "controller/build_plugin_system.py"

###############################################################################
# Job 1) Build firmware.elf.a
###############################################################################

target_elf = join("$BUILD_DIR", "${PROGNAME}.elf")
target_elf_a = target_elf + ".a"
base_target_elf_a = basename(env.subst(target_elf_a))
if "BUILD_ELF_A" in os.environ:
    # The following makes sure something like firmware.elf.a is produced,
    # which is required to have something to link against, because the static compiled
    # executable (firmware) contains no more symbol table.
    #
    # Note: This is GCC Syntax and won't work with clang. Typically this means
    #       that this isn't working on mac but linux only.
    env.Append(
        LINKFLAGS=[
            "-Wl,--out-implib," + target_elf_a
        ],
    )
    print(me, "will also build", base_target_elf_a)
else:
    print(me, "skips building", base_target_elf_a)

###############################################################################
# Job 2) Build firmware.bin
###############################################################################

# Note:
#   This step is actually *not* needed for the plugin system but instead for OTA updating.
#   It is, however, pretty easy to do this anytime after build time since it just requires
#   running an objcopy on the elf or (i)hex file.

if 0:
    # The following is a makefile'y way to execute something like
    #    objcopy --input-target=ihex --output-target=binary firmware.hex firmware.bin
    # at the correct time.

    # target_bin = env.ElfToBin(join("$BUILD_DIR", "${PROGNAME}"), target_elf) # doesn't work
    # AlwaysBuild(target_bin) # doesn't work

    Import("projenv") # defines projenv

    global_env = DefaultEnvironment()

    # well, this used to work as an extra_scripts = post:lib/loader/...py
    # within the platformio.ini, but it doesn't from library.json.
    global_env.AddPostAction(
        "$BUILD_DIR/${PROGNAME}.elf",
        global_env.VerboseAction(" ".join([
            "$OBJCOPY", "-O", "binary", "-I", "ihex",
            "$BUILD_DIR/${PROGNAME}.elf", "$BUILD_DIR/${PROGNAME}.bin"
        ]), "Building $BUILD_DIR/${PROGNAME}.bin")
    )

    def post_program_action(source, target, env):
        print("Program has been built, yeah")
        program_path = target[0].get_abspath()
        print("Program path ", target)

    global_env.AddPostAction("$PROGPATH", post_program_action)

    # arghl, I hate pio.

    print(me, "will also TRY to build", "firmware.bin")

###############################################################################
# Job 3) Extract CFlags, etc for a custom Makefile
###############################################################################

if understand_the_build_system:
    expand = lambda var: env.subst('$'+var)

    # still missing: The crazy amount of include paths

    fh = open(join("/tmp/", "Makefile.txt"), "w")
    for var in "CCFLAGS CXXFLAGS CPPFLAGS _CPPDEFFLAGS _CPPINCFLAGS CC CXX OBJCOPY PROGPATH".split():
        fh.write("%s = %s\n" % (var, expand(var)))

    # path contains path to arm-none-eabi-gcc and friends
    path = env['ENV']['PATH'] # same as os.environ['PATH']
    fh.write("PATH = "+path)

    fh.close()
