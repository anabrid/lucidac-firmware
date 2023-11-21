# LUCIDAC Hybrid Controller

This repository holds the firmware for the hybrid controller of the LUCIDAC.

## Users: Getting started

This section is intended for users of the LUCIDAC,
who want to use it as is.

###  Flashing the latest firmware

If necessary, you can download the latest stable firmware from the releases page
or the latest development version from the build artifacts.
In the moment, this is *not* the [Gitlab project releases](https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/releases)
but instead the [Gitlab project jobs](https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/jobs) built
artifacts, i.e. you should go to https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/artifacts
and look for the latest artifact from the *dev branch*. You can download
the `artifacts.zip` and lookup the file in `.pio/build/teensy41/firmware.hex`

Install the teensy flashing utility `teensy-loader-cli`
and the udev rules from https://www.pjrc.com/teensy/00-teensy.rules
to allow flashing the firmware image without prior building at your computer.
If you have the platformio teensy package installed, you can also find the flashing utility in `$HOME/.platformio/packages/tool-teensy/teensy_loader_cli`.

Connect your hybrid controller via the USB port to your computer
and press the small flash button on the teensy board.

Execute `teensy_loader_cli --mcu=TEENSY41 firmware.hex` to flash your hybrid controller.



### Find the hybrid controller in the network

By default, the hybrid controller configures its network address via DHCP.
You will need that network address to connect to it.

The hybrid controller will send debug information, including its IP address,
via the USB-serial connection.
After a restart, use `cat /dev/ttyACM0` or similar to access the debug output.
Please note that debug output is not buffered and may not be visible if you are too slow.

You can also find the IP address by asking your system administrator
or watching your DHCP logs.

### Using your LUCIDAC

Use the `pyanabrid-redac` python package to interact with your LUCIDAC. It currently lives
at https://lab.analogparadigm.com/redac/software/pyanabrid-redac/
as well as https://lab.analogparadigm.com/pyanabrid/.

## Developers: Getting started

This section is intended for developers of the LUCIDAC,
who are looking to adapt and build the firmware themselves.

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

### Hello World from the test suite
When you start with `pio`, make sure to *always* run it from the root directory of this repository. You can make
a first *hello world* with this command:

```
pio test -v --filter integrated/test_sinusoidal
```

For other tests, try `pio test --list-tests`.

## Using the actual TCP/IP main code
Make sure the IP adress written in `src/main.cpp` is connectable for you (i.e. you are in
the same network).

Use the IDE plugin or run `pio run --target=upload`.

You can monitor the serial output for debugging at `pio device monitor` which prints the IP
and Mac address the device is listening on. Use this way or
`arp` to detect the Mac address of your teensy network interface.

For testing the network access ("hello world"), you can ping the Teensy and also use the following code in your local terminal:

```
(echo '{ "_id": 42, "_type": "get_config", "msg": { "entity": ["04-E9-E5-14-74-BF"], "recursive": true } }'; sleep 1) | nc 123.456.789.123 5732
```

In order to use this code, you have to replace the Mac addrress `04-E9-E5-14-74-BF` by the one from your teensy and the IP adress by the one you are using. You can leave the TCP port `5732` as is.

You should get some output like this one:

```
{"id":"null","type":"get_config","msg":{"entity":["04-E9-E5-0D-CB-93"],"config":{"/0":{"/M0":{"elements":[{"ic":-1,"k":10000},{"ic":-1,"k":10000},{"ic":-1,"k":10000},{"ic":-1,"k":10000},{"ic":-1,"k":10000},{"ic":-1,"k":10000},{"ic":-1,"k":10000},{"ic":-1,"k":10000}]},"/M1":{},"/U":{"outputs":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null]},"/C":{"elements":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]},"/I":{"outputs":[null,null,null,null,null,null,null,null,null,null,null,null,null,null,null,null]}}}}}
```
