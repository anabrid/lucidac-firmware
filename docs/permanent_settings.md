\page nvmconf Permanent Settings

The Teensy supports persistent, permanent settings which are stored on an virtualized
EEPROM realized on the Flash memory. In the LUCIDAC, this is used to store primarily
settings around the digital networking I/O. In contrast, all analog *settings*,
technically refered to as analog *configuration* or *circuit* configuration, is
intentionally ephermal and lost when the Teensy or LUCIDAC looses power.

As a design decision, a change in permanent settings normally requires a reboot in
order to take change. This makes sense for network-related settings which are typically
only made use of at startup.

In the code, the settings is implemented by nvmconfig::PersistentSettingsWriter.
In particular, there is a small registry at net::register_settings which collects
all subsystems which store permanent settings.

Following the general idiom of the JSON-based LUCIDAC communication, also the
permanent settings are just a JSON dictionary. In order to save space on the EEPROM
area, which is only about 2kB in size, [MessagePack](https://msgpack.org/) can be used
for serialization. In any way, the [ArduinoJSON](https://arduinojson.org/) library
is used the same way as in the [JSONL protocol implementation](#protocol). Therefore,
the same conversion rules apply.

## String casting rules
In case you want to write a minimal client which can modify settings, you might want
to use the simple key-value idiom which assumes all (non-list/object) values to be *strings*,
despite JSON also knows boolean and numbers. The casting is transparently performed
by ArduinoJSON.

The following rules are handy to remember:

* When trying to set a boolean value, do not pass the strings, as any string will be
  interpreted as truth value *true*. That is, `"false"`, `"0"` and even the empty
  string `""` evaluate to `true`. However, the numerical value `0` as well as the
  JSON `Null` type cast to `false`.
* When trying to set a numeric value, string parsing *does* work. That is, ýou can
  pass a `"123"` and it is properly scanned as `123`. This is because ArduinoJSON
  ships [with a sophisticated string to number parser](https://github.com/bblanchon/ArduinoJson/blob/7.x/src/ArduinoJson/Numbers/parseNumber.hpp).

The other way around is untypical but anyway reported here: When you pass a number
or boolean where a string is expected, the following happens:

* numbers will be easily casted into strings, but
* any boolean as well as the `Null` value results in an empty string.

Given these casting rules, it is suggested to do basic preprocessing of strings
on the client side to deal with booleans.
