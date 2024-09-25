.. _style-guide:

Style Guide
===========

This page is a code style guide for the HybridController Firmware.

Entrypoints and Seperations
---------------------------

* ``src/hybrid-controller.cpp`` is the main entry point for regular usage.
* All (unit/integration/etc) tests below ``test/`` are also entry points on its own.
* Code is highly capsulated in different "modules" within the ``lib/`` directory.

Formatting
----------

* The code generally uses 2 whitespaces as intend. However, `clang-format <https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/blob/main/.clang-format?ref_type=heads>`_ shall be used.
* Use C++ features wisely. Don't end up in template hell.

Communication Concepts
----------------------

* `JSON lines <https://jsonlines.org/>`_ (in short: JSONL) as *universal protocol* for communicating with the microcontroller (basically for anything: Logging, Remote Procedure Calls, etc.). We therefore also stick to ASCII whenever possible.
* The USB serial console is always treated as an *administrative interface*. Reason: Whoever has access to the serial console also has physical access to the microcontroller.
* In production code, the USB serial terminal also speaks valid JSONL.

Strings
-------

* Prefer STL over Arduino libraries wherever possible, for instance prefer ``std::string``(https://cplusplus.com/reference/string/string/) over Arduinos ``String`` (https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/).
* Prefer Arduinos ``Printable`` (https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Printable.h) interface over memory-intensive string serialization.
* Prefer to use `JSON Custom Converters <https://arduinojson.org/news/2021/05/04/version-6-18-0/#custom-converters>`_ in favour of some ``toJSON`` methods. However, if multiple JSON representations exist for a given data structure, prefer ``toJSON(withOptions)``.

The Heap in general
-------------------

This code does not save on `new` and `malloc` calls. One reason for that is convenience, another is that we run out of RAM1 stack space anyway.

Strings/Buffer vs. Streams
--------------------------

TODO, write about

* Buffers such as ArduinoJSON with their document concept
* Streaming data which avoids in-memory preparation
