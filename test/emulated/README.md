# Emulated Tests

Emulated tests mock the low-level hardware calls to verify higher-level code calls them as expected.
That includes `delay*`, any `SPI` communication and some other things provided by the Arduino framework.

These tests are primarily intended to check the correctness of chip implementations
and the logic behind generating SPI data.
See [examples/test_arduino_fake](examples/test_arduino_fake) for an example.

Native and emulated tests can be debugged on the host machine.
See [../native/examples/test_debugging](`../native/examples/test_debugging`) for an example and follow the [platformio documentation on how to debug](https://docs.platformio.org/en/latest/plus/debugging.html).
