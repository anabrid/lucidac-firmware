# Native Tests

Native tests must be able to run without calling any hardware-related functions.
That includes `delay*`, any `SPI` communication and some other things provided by the Arduino framework.

These tests are intended to test the "business logic" side of the firmware.
While it would be good practice to separate business logic completely from hardware related code,
you don't have to, since the `ArduinoTeensyFake` library is still included and hardware functions can be part of the compiled source files (though they must not be called!)

Native and emulated tests can be debugged on the host machine.
See [examples/test_debugging](`examples/test_debugging`) for an example and follow the [platformio documentation on how to debug](https://docs.platformio.org/en/latest/plus/debugging.html).
