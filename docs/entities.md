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
