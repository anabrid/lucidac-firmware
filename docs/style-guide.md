# Style Guide for the REDAC HybridController Firmware

## Entrypoints and Seperations
* `src/main.cpp` is the main entry point for regular usage.
* All (unit/integration/etc) tests below `test/` are also entry points on its own.
* Code is highly capsulated in different "modules" within the `lib/` directory.

## Formatting
* The code generally uses 2 whitespaces as intend. However, [clang-format](https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/blob/main/.clang-format?ref_type=heads) shall be used.
* Use C++ features wisely. Don't end up in template hell.

## Concepts
* [JSON lines](https://jsonlines.org/) as *universal protocol* for communicating with the microcontroller (basically for anything: Logging, Remote Procedure Calls, etc.). We therefore also stick to ASCII whenever possible.
* The USB serial console is always treated as an *administrative interface*. Reason: Whoever has access to the serial console also has physical access to the microcontroller.

## Strings
* Prefer STL to Arduino libraries wherever possible, for instance prefer [std::string](https://cplusplus.com/reference/string/string/) over Arduinos [String](https://www.arduino.cc/reference/en/language/variables/data-types/stringobject/).
* Prefer Arduinos [Printable](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Printable.h) interface over memory-intensive string serialization.
* Prefer to use [JSON Custom Converters](https://arduinojson.org/news/2021/05/04/version-6-18-0/#custom-converters) in favour of some `toJSON` methods.