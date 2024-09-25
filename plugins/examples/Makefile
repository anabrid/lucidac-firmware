# This is an exemplaric standalone GNU Makefile which does not use PlatformIOs SCons build
# system but extracts all the paths out from there. This way, it mimics the build flags of
# the "native" firmware code.
#
# It is an ongoing process to correctly extract the actual used build flags at build time
# and store them somewhere for use when building plugins, see lib/controller/build_plugin_system.py.
#
# Until that is working, this Makefile does its job.

# Detect the PlatformIO core directory for accessing packages.
# typically something like $HOME/.platformio
PLATFORMIO_CORE_DIR=$(shell pio system info --json-output | jq -r '.core_dir.value')

# the CFLAGS have to match how the host code was built. For instance, mixing -O2 and -O0 is
# a bad idea (as it is also for two compilation units in a simpler setup) and typically results
# in weird linker errors.
PREFIX=$(PLATFORMIO_CORE_DIR)/packages/toolchain-gccarmnoneeabi-teensy/bin/
CORE_INC=-L$(PLATFORMIO_CORE_DIR)/packages/framework-arduinoteensy/cores/teensy4
CFLAGS=-fno-exceptions -felide-constructors -fno-rtti -std=gnu++14 -Wno-error=narrowing -fpermissive -fno-threadsafe-statics -fno-use-cxa-atexit -Wall -ffunction-sections -fdata-sections -mthumb -mcpu=cortex-m7 -nostdlib -mfloat-abi=hard -mfpu=fpv5-d16 -O2 -DPLATFORMIO=60107 -D__IMXRT1062__ -DARDUINO_TEENSY41 -DUSB_SERIAL -DARDUINO=10805 -DTEENSYDUINO=158 -DCORE_TEENSY -DF_CPU=600000000 -DLAYOUT_US_ENGLISH -I$(PLATFORMIO_CORE_DIR)/packages/framework-arduinoteensy/cores/teensy4
INC = $(CORE_INC) $(FIRMWARE_INC) $(LIBDEPS_INC)

FIRMWARE_ROOT=../..
PIO_BUILD=$(FIRMWARE_ROOT)/.pio

# Actually lookup for headers in these standard places
FIRMWARE_LIBS=$(wildcard $(FIRMWARE_ROOT)/lib/*/src)
FIRMWARE_INC=$(addprefix -I,$(FIRMWARE_LIBS))

# PlatformIO provides us even a convenient way to access the external library
# headers such as QNEthernet, ETL, etc.
LIBDEPS=$(wildcard $(PIO_BUILD)/libdeps/teensy41/*/src) $(wildcard $(PIO_BUILD)/libdeps/teensy41/*/include)
LIBDEPS_INC=$(addprefix -I,$(LIBDEPS))

# The linker script needs to have the correct LOAD_ADDR.
ldscript=ramload.ld

# name of your function to be called at entrance. Required for Elf format header, but not for stripped binary.
# You can basically extract these functions from your elf symbol table if you feel to do so.
ENTRY_POINT_SYM="plugin_setup"

# Note for linker scripts:
#   See also `arm-none-ebai-ld --verbose` for the default linker script
#   as well as the Teensy one at https://github.com/PaulStoffregen/cores/blob/master/teensy4/imxrt1062_t41.ld

# Just for the nice tooling, we automatically read the appropriate code load address
# and entry point from the respective sources. Note that Makefiles variables are lazy
# evaluated, so files are always there at variable evaluation time.
# Also note, remove $$NF to $NF when extracting from makefile to regular shell script
#
# Note that GlobalPluginLoader_Storage is only defined if code was built with ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER
LOAD_ADDR=0x$(shell nm -gC firmware.elf.a | grep GlobalPluginLoader_Storage | cut -f1 -d' ')
ENTRY_POINT=$(shell readelf -h payload.elf  | grep "Entry point" | awk '{print $$NF}')

all: payload.h

# The variant of the firmware which contains linkable symbols (i.e. non-stripped).
# Has to be provided by the time the original host firmware was linked.
# To be created by build_plugin_system.py.
firmware.elf.a:
	ln -s $(PIO_BUILD)/build/teensy41/firmware.elf.a

firmware.elf:
	ln -s $(PIO_BUILD)/build/teensy41/firmware.elf

firmware.bin: firmware.elf
	$(PREFIX)arm-none-eabi-objcopy -O binary  $< $@

# going the .o -> .a way only for making the linker entry point calm.
payload.o: payload.cpp
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -c payload.cpp
	
payload.a: payload.o
	$(PREFIX)arm-none-eabi-gcc-ar rc payload.a payload.o

payload.elf: payload.a
	sed -i "s/^LOAD_ADDR.*/LOAD_ADDR = $(LOAD_ADDR);/" $(ldscript)
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) -o $@ -T $(ldscript) \
		-nostartfiles -nodefaultlibs -nostdlib  --static-pie \
		-l:firmware.elf.a -L. -l:payload.a -g -Wl,-e,$(ENTRY_POINT_SYM) payload.a
#	$(PREFIX)arm-none-eabi-strip --strip-unneeded  payload.elf

# the contents of *this* file are actually ramloaded
payload.bin: payload.elf
	$(PREFIX)arm-none-eabi-objcopy --dump-section .text=payload.bin  payload.elf

clean:
	rm -f *.elf *.a *.o

# Makefile hacking: Print variables, for instance "make PRINT_LOAD_ADDR"
PRINT_% : ; $(info $($*)) @true

# show size statistics:
#    nm --print-size --size-sort --radix=d payload.elf
#    arm-none-eabi-size --common payload.elf
