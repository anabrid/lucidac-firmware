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
the nested message. For instance, a simple hellow world message

\todo
continue here
