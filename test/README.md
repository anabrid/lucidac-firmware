# Tests

This directory contains test cases for the firmware.
The tests are structured in three categories:

* `native` tests, which can be executed completely independent on any hardware-related code.
* `emulated` tests, which use the `ArduinoTeensyFake` and `fakeit` library to mock low-level hardware calls and can verify that the higher-level code sends the expected data.
* `hardware` tests, which can only be run on actual hardware.
* `manual` tests, which can also only be run on actual hardware, but are not designed to be run automatically. These generally need additional external test equipment and human attention during testing. They are mostly used during development.

See the respective README files in the subdirectories for more details.
