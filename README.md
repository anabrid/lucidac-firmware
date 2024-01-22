# LUCIDAC Hybrid Controller

This repository holds the firmware for the hybrid controller of the LUCIDAC/REDAC. It
lives at https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller.

It is supposed to run on a [Teensy 4.1](https://www.pjrc.com/teensy/), i.e. a 32bit
600MHz Cortex-M7 with Ethernet chip.

This project uses [PlatformIO](https://platformio.org/) for the build system (`pio` in
short). PlatformIO helps for building, flashing and can also be used within various IDEs.

\note
A good point to start reading the documentation is the guide [getting started as a user](#getting-started).

<!--
  This project uses Doxygen for documentation. Please read the generated documentation,
  either locally by running "make docs && open docs/html/index.html" or the published
  docs in the internet.
  
  You can also read the individual markdown files at the docs/ directory.
-->
