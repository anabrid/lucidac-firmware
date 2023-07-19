# LUCIDAC Teensy hybrid-controller

This repository holds the firmware for the **Teensy** hybrid controller for the LUCIDAC.

## Getting started

Follow the respective platformio (`pio` in short) documentations to install the required dependencies and tools for developement.

Follow https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html#installation-installer-script
for Linux & Commandline. You can also use the *platformio*  from your respective plattform (for instance
`apt-get install platformio` on Ubuntu). For the VSCode IDE integration, see
https://docs.platformio.org/en/latest/integration/ide/vscode.html

### Linux-relevant hints for Teensy via USB
When you start on a fresh Linux PC, you need to make sure to be able to reach out to the Teensy via USB
via the `/dev/ttyACM0` file. You can grab them at https://www.pjrc.com/teensy/00-teensy.rules . You should
also ensure to be in the correct user group to access this file; after adding yourself to the group (such as
`sudo gpasswd -a YOUR-LOCAL-USERNAME dialout`), you have to sign out and sign in again on your desktop.

### Hello World from LUCIDAC
When you start with `pio`, make sure to *always* run it from the root directory of this repository. You can make
a first *hello world* with this command:

```
pio test -v --filter integrated/test_sinusoidal
```

For other tests, try `pio test --list-tests`.

## Flashing the firmware
This section is basically not relevant as there are currently only unit tests and no defacto-standard firmware.

Use the IDE plugin or run `pio run --target=upload`.
