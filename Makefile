# This Makefile collects some handy commands for the lazy.
# However, you are encouraged to use platformio directly without mezzing around
# with this Makefile. Also, please read the REDME for useful hints.

build:
	pio run

docs:
	cd docs && make

ifdef SUDO
prepend=sudo -E env
else
prepend=
endif

test:
# USAGE: make test SUDO=TRUE FILTER=hardware/integrated/u-c-i-int/test_sinus
	PLATFORMIO_FORCE_ANSI=true $(prepend) pio test -v -e teensy41 --filter $(FILTER)

.PHONY: docs build test
