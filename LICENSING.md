Licensing
=========

This is an open source code which is licensed as following:

> Copyright (c) 2024 anabrid GmbH
>
> Contact: https://www.anabrid.com/licensing/
>
> SPDX-License-Identifier: MIT OR GPL-2.0-or-later

We are sensitive for not using incompatible licensed open source libraries. Therefore, in the following
all dependencies and used code snippets are listed.

External libraries
------------------

The following external libraries are used (explicitely included via `platformio.ini`):

* https://github.com/bblanchon/ArduinoJson (MIT licensed)
* https://github.com/ssilverman/QNEthernet (APGL-3.0 licensed)
* https://github.com/bblanchon/ArduinoStreamUtils (MIT licensed)
* https://github.com/ETLCPP/etl (MIT licensed)

The following external libraries are implicitely included by PlatformIO:

* https://registry.platformio.org/tools/platformio/framework-arduinoteensy
  (mixed open source licenses, cf https://github.com/PaulStoffregen/cores/issues/67)
* https://registry.platformio.org/platforms/platformio/teensy (Apache 2.0 licensed)
* https://registry.platformio.org/tools/platformio/toolchain-gccarmnoneeabi-teensy (GPL-2.0-or-later licensed)

These packages depend on even more libraries such as https://github.com/SCons/scons
(MIT licensed) which are however all released as open source.


External code snippets distributed with this code
-------------------------------------------------

* `/lib/controller/src/ota/flasher.cpp` is public domain (references therein)
* `/lib/controller/src/utils/dcp.cpp` is BSD-3 licensed, see https://github.com/nxp-mcuxpresso/mcux-sdk