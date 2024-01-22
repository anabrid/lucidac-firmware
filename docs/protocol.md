# REDAC communication protocol

The firmware uses a [JSON Lines](https://jsonlines.org/) plain text protocol for communication.
It follows a client/server idiom where the microcontroller is the server
and the host computer is the client. It implements an extended request-response paradigm, where
"extended" means that the server is allowed to send messages any time (*out of band* messages).

The protocol is used on a TCP/IP service as well as on the USB Serial connection. The protocol
is standarized by a JSON schema file (part of the firmware repository) and also covered in detail
by the Python client reference impelemntation. A good client protcol implementation makes extensively
use of typing. In contrast, the server implementation uses [ArduinoJson](https://arduinojson.org/)
and is therefore not strictly implemented with strong types.

## A short primer into the protocol
The request-response based protocol follows the general structure *envelope*

```
{ "id": "some-uuid", "type": "something", "msg": { ... } }
```

Such messages are initiated by the client, which chooses a message identifier
([UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier) as string, which is however
currently not enforced by the server). It always contains a *type* which indicates the type of
the nested message. Here is an example for a hello world message is the following:

```
me@localhost$ pio device monitor
--- Terminal on /dev/ttyACM1 | 9600 8-N-1
--- Available filters and text transformations: colorize, debug, default, direct, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at https://bit.ly/pio-monitor-filters
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H
{'type':'help'}
{"id":"null","type":"help","msg":{"human_readable_info":"This is a JSON-Lines protocol described at https://anabrid.dev/docs/pyanabrid-redac/redac/protocol.html","available_types":["get_config","get_entities","get_settings","hack","help","load_plugin","login","one_shot_daq","ota_update_abort","ota_update_complete","ota_update_init","ota_update_stream","ping","reset","reset_settings","set_config","start_run","status","unload_plugin","update_settings"]},"success":true}
```

Here, the message `{'type':'help'}` produced an answer and also listed all available types.

