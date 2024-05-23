# REDAC communication protocol

The firmware uses a [JSON Lines](https://jsonlines.org/) plain text protocol for communication.
It *mostly* follows a client/server idiom where the microcontroller is the server
and the host computer is the client. It implements an extended request-response paradigm, where
"extended" means that the server is allowed to send messages any time (*out of band* messages).

The protocol is used on a TCP/IP service as well as on the USB Serial connection. The protocol
is standarized by a JSON schema file (part of the firmware repository) and also covered in detail
by the *pybrid* Python client reference impelemntation (see also the [Protocol documentation in the Pybrid
documentation](https://anabrid.dev/docs/pyanabrid-redac/redac/protocol.html)).
A good client protcol implementation makes extensively
use of typing. In contrast, the server implementation uses [ArduinoJson](https://arduinojson.org/)
and is therefore not strictly implemented with strong types.

The protocol is *not* formally specified but rather described within this documentation for humans.
It is not particularly complicated but the documentation shall help any interested person to get
a start.

## A short primer into the protocol
The protocol basically implements an *asynchronous remote procedure calls* (RPC). They follow in
general the structure of an *envelope* which is used both by client requests and server responses:


```
{ "id": "some-uuid", "type": "something", "msg": { ... } }
```

The client should come up with a message identifier 
([UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier) as a string and the server
takes up the UUID in any reply. It is not guaranteed that replies come directly after the response
(this is the asynchronous aspect, see more on that below). The envelope always contains a *type*
which can be thought as a *method name* in the RPC picture.
An example for a hello world message is the following:

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

## On synchronous and asynchronous messaging
As a design principle, the protocol implements asynchronous procedure calls. This is best mapped
with asynchronous subroutines as provided in many modern programming languages and frequently used
in async I/O. Fully implementing a client requires some kind of lookup mechanism to pick up suitable
[futures/promises](https://en.wikipedia.org/wiki/Futures_and_promises) when the corresponding server
reply comes in. However, it is also possible to come up with a *synchronous client* which is much
easier to implement but will not be able to deal with any kind of out-of-band messages. When
implementing such a client, you can make the following assumptions:

* Making a request shall result immediately in the corresponding reply message.
* Ignore any kind of incoming messages which do not fit to the request id and type.
* If you want to make runs/data aquisition, you can encapsulate the asynchronous logic by
  slurping all suitable server responses until the run/aquisition ends.

There are synchronous clients [available in Python](https://lab.analogparadigm.com/lucidac/software/simplehc.py)
and [C++](https://lab.analogparadigm.com/lucidac/software/lucisim) which demonstrate how to easily
write such a client. In contrast, there are asynchronous clients available in Julia, Typescript and
other languages which naturally implement async functions.

## Message types design principles

Technically, there is only a message type string field and no more differentiation. This allows
to implement access control on message types and easily extending for user-defined types.
We did not decide for a REST-like verb-object semantics despite extending the protocol
into such an orthogonal design may be possible in the future. Also the categorization in
the following overview table is only for documentation purpose:

| Category           | Type                                    | Synchronous? |
|--------------------|-----------------------------------------|-----------|
| Basics             | ping                                    | yes        |
|                    | help                                    | yes        |
|                    | reset                                   | yes        |
| Carrier/RunManager | {set,get,reset}_config                  | yes        |
|                    | get_entities                            | yes        |
|                    | {start,reset}_run                       | NO         |
| Manual HW access   | one_shot_daq                            | yes        |
|                    | manual_mode                             | yes        |
| HC-Specific        | {get,update,reset}_settings             | yes        |
|                    | status                                  | yes        |
|                    | login                                   | yes        |
|                    | {load,unload}_plugin                    | yes        |
|                    | ota_update_{init,stream,abort,complete} | yes        |

Note that one can imagine writing a client which focusses on administrative issues
(probably also only implementing synchronous communication) while another type of client
might focus on actual computer usage (data science oriented)

\todo
Say something about stability of method calls. Also say about the structure of the different
message types (in terms of request and response)

## Protocol elevation

The concept of *protocol elevation* means the transport/encapsulation of the JSONL protocol over
some other protocol, for instance HTTP or websockets. This is supported by the firmware
and details are supposed to be written here (TODO).

## Connection endpoint URIs

For convenience, it is suggested that clients encode all parameters for connecting to a LUCIDAC
in a uniform resource identifier (URI). The following connection types are suggested:

* `tcp://123.123.123.123:5732` for a regular JSONL over TCP connection to a host given by
  IPv4 address or hostname.
* `usb:/dev/ttyACM0` for a serial connection over USB serial console
* `ws://1.2.3.4/websocket` for a typical JSONL over websocket connection
* `tcp://user:password@1.2.3.4:5732` for the way of embedding access credentials into the URI

Note that the usage of these URIs is out of scope for the protocol but a good practice. For
instance, the NI-VISA standard proposes a similar but way more complex
[resource syntax](https://www.ni.com/docs/en-US/bundle/ni-visa/page/visa-resource-syntax-and-examples.html)
