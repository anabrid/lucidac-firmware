.. _users:

Flashing and PIO
================

This section is intended for users of the LUCIDAC, who want to use it *as is*.

If your need is to make use of a given microcontroller, inspect a suitable client documentation
instead.

Flashing the firmware via OTP
-----------------------------

The firmware also provides mechanisms for self-updates over ethernet, without USB cable.

How to flash the latest firmware via the USB Bootloader
-------------------------------------------------------

If necessary, you can download the latest stable firmware from the releases page
at Github: https://github.com/anabrid/lucidac-firmware/releases . In this guide, 
you need the ``firmware.hex`` file from the most recent release.

Install the teensy flashing utility from https://www.pjrc.com/teensy/loader_cli.html
to allow flashing the firmware image without prior building at your computer.
Don't forget to apply the udev rules from https://www.pjrc.com/teensy/00-teensy.rules if
you work on a Linux computer which was not connected to a Teensy before.
If you have the platformio teensy package installed, you can also find the
flashing utility in ``$HOME/.platformio/packages/tool-teensy/teensy_loader_cli``.

Connect your LUCIDAC via the USB port to your computer.
Execute ``teensy_loader_cli --mcu=TEENSY41 firmware.hex`` to flash your embedded
microcontroller. Hint: You might have to run this command two or three times.

Alternatively, you can use the graphical Teensy Loader Application at https://www.pjrc.com/teensy/loader.html)
or consider alternative tools such as https://koromix.dev/tytools.

How to get started with platformio
----------------------------------

In order to develop on the firmware, you have to install *platformio*. We recommend not to install
the version from your package repository (such as ``apt install platformio``) but instead install the
most recent version with ``pip install platformio``. This also solves errors such as
"error detected unknown package".

Furthermore, the ``teensy_loader_cli`` bundled with the platformio teensy package is known to be
broken, cf. https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/issues/76.
The workaround is to build and use your own uploader:

::

    % wget https://github.com/PaulStoffregen/teensy_loader_cli/raw/master/teensy_loader_cli.c
    % apt install gcc libusb-dev  # probably have to install some dependencies
    % gcc -DUSE_LIBUSB teensy_loader_cli.c -oteensy_loader_cli-v2.3 -lusb
    % grep upload platformio.ini  # change accordingly or add lines:
    upload_protocol = custom
    upload_command = ./teensy_loader_cli-v2.3 -mmcu=teensy41 -w -s -v .pio/build/teensy41/firmware.hex
    % pio run -t upload # then this works again

