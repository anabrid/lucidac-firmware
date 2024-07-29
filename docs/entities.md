# REDAC entitiy concept                                       {#entities}

In REDAC/LUCIDAC, *entities* are subsystems which contain a EEPROM with a MAC address
(EUI-64 identifier). They can be autodetected on a digital bus and thus allow for
building different configurations of computing elements. Do not confuse this concept
with real reconfigurability which happens without replacing actual electronics.

Typically, entities are within a modular PCB. For instance, the different blocks
(U/I/C/SH/M/Ctrl/Front/etc) are entities.

The [JSON communication protocol](#protocol) differs between general configuration
options and entities. Both can be nested hierarchically. However, typically the
entitiy can be recognized by its `/` slash prefix in the notation.

The `get_entities` call is the entry place for hardware scanning and provides a tree.
The following example lists a LUCIDAC (identified by its MAC address) with its single
cluster (indicated by `/0`) which holds three a couple of modules (`M1, U, C, I, SH`).
Next to the cluster is the front panel (indicated by `FP`):

```js
{
  "04-E9-E5-16-0A-15": {
    "class": 1,
    "type": 1,
    "variant": 1,
    "version": 1,
    "/0": {
      "class": 2,
      "type": 1,
      "variant": 1,
      "version": 1,
      "/M1": {
        "class": 3,
        "type": 2,
        "variant": 1,
        "version": 1
      },
      "/U": {
        "class": 4,
        "type": 1,
        "variant": 1,
        "version": 1
      },
      "/C": {
        "class": 5,
        "type": 1,
        "variant": 1,
        "version": 1
      },
      "/I": {
        "class": 6,
        "type": 1,
        "variant": 1,
        "version": 1
      },
      "/SH": {
        "class": 7,
        "type": 1,
        "variant": 1,
        "version": 1
      }
    }
  },
  "/FP": {
    "class": 8,
    "type": 1,
    "variant": 1,
    "version": 1
  }
}
```

## Entity properties

Entities are identified by a 4-tuple `(class, type, variant, version)` of integers.
Furthermore, entities hold 64bit of unique identification (EUI-64, like an Ethernet
MAC Address).

Anabrid will release a registry/database of possible entities in the future.

## Entity configuration

Entities can be configured with the `set_config` (`set_circuit` in later firmware
versions) JSONL type. The following two ways of setting the configuration are
equivalent:

Method 1: Directly setting the entitiy path

```
outer_config = {
    "entity": ["04-E9-E5-16-09-92", "0"],
    "config": config
}
```

Method 2: Only setting some parent entity and then descending

```
outer_config = {
    "entity": ["04-E9-E5-16-09-92"],
    "config": { "/0": config }
}
```

In both ways, the resulting structure `outer_config` can be fed as a message into the
`set_config` call (such as with `hc.query("set_config", outer_config)`).


Keep in mind that LUCIDAC entity configuration is non-ephermal and thus is lost
at every reboot/power loss.

## Entitiy functions

Entities expose *functions* which can be steered... please write here more.
