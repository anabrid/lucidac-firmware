# Getting started                                              {#getting-started}

This section is intended for users of the LUCIDAC, who want to use it *as is*. Typical user
needs are

* given a "ready to use" Teensy microcontroller, how to connect and use the device
* how to upgrade to a given newer firmware release.

These needs are described in the following

## Find the hybrid controller in the network

By default, the hybrid controller configures its IPv4 network address via DHCP.
You need to know it's assigned IP address in order to connect to it. You can also
[disable DHCP or change the configuration](networking.md) if you need to do so.

By default, the device listens on TCP port `5732`.

The hybrid controller will send debug information, including its IP address,
via the USB-serial connection.
After a restart, use `cat /dev/ttyACM0` or similar to access the debug output.
Please note that debug output is not buffered and may not be visible if you are too slow.

You can also find the IP address by asking your system administrator
or watching your DHCP logs. You can also perform a network scan such as 
`nmap -v -A -p 5732 192.168.1.1-255` to search for computers in your (exemplaric)
`192.168.1.1/24` network listening on port `5732`.

## Using your LUCIDAC

Use the `pyanabrid-redac` python package to interact with your LUCIDAC. It currently lives
at https://lab.analogparadigm.com/redac/software/pyanabrid-redac/
as well as https://lab.analogparadigm.com/pyanabrid/. 

##  Flashing the latest firmware

If necessary, you can download the latest stable firmware from the releases page
or the latest development version from the build artifacts.
In the moment, this is *not* the [Gitlab project releases](https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/releases)
but instead the [Gitlab project jobs](https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/jobs) built
artifacts, i.e. you should go to https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/artifacts
and look for the latest artifact from the *dev branch*. You can download
the `artifacts.zip` and lookup the file in `.pio/build/teensy41/firmware.hex`

Install the [teensy flashing utility](https://www.pjrc.com/teensy/loader_cli.html)
`teensy-loader-cli` to allow flashing the firmware image without prior building at your computer.
Don't forget to apply the [udev rules](https://www.pjrc.com/teensy/00-teensy.rules) if
you work on a Linux computer which was not connected to a Teensy before.
If you have the platformio teensy package installed, you can also find the flashing utility in `$HOME/.platformio/packages/tool-teensy/teensy_loader_cli`.

Connect your hybrid controller via the USB port to your computer
and press the small flash button on the teensy board.

Execute `teensy_loader_cli --mcu=TEENSY41 firmware.hex` to flash your hybrid controller.

## Developing with platformio

In order to develop on the firmware, you have to install *platformio*. We recommend not to install
the version from your package repository (such as `apt install platformio`) but instead install the
most recent version with `pip install platformio`. This also solves errors such as
"error detected unknown package".

Furthermore, the `teensy_loader_cli` bundled with the platformio teensy package is known to be
broken, cf. https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/issues/76.
The workaround is to build and use your own uploader:

```
% wget https://github.com/PaulStoffregen/teensy_loader_cli/raw/master/teensy_loader_cli.c
% gcc -DUSE_LIBUSB teensy_loader_cli.c -oteensy_loader_cli-v2.3 -lusb
% grep upload platformio.ini  # change accordingly or add lines:
upload_protocol = custom
upload_command = ./teensy_loader_cli-v2.3 -mmcu=teensy41 -w -s -v .pio/build/teensy41/firmware.hex
% pio run -t upload # then this works again
```