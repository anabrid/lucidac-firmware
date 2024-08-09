\page entities REDAC entitiy concept

In REDAC/LUCIDAC, *entities* are hardware subsystems which allow autodetection with a EEPROM
that holds a custom information next to a MAC address (EUI-64 identifier,
see metadata::MetadataMemory).
Entities can be discovered on a digital bus and thus allow for
quickly building different setups of modular computing elements. This might be called
*modularity* and should not be confused with *reconfigurability* which happens purely
in software by configuring interconnection matrices without replacing actual hardware.

Typically, entities are within a modular PCB. For instance, the different blocks
(U/I/C/SH/M/Ctrl/Front/etc) are entities. The rest of this page will concentrate on the
the *sofware* aspect of entities, i.e. their representation in firmware and in the
communication protocol.

In general, entities hold configurable stateful hardware such as analog switches,
potentiometers, etc.. One of the main job of the entity concept is to be able to 
address these *elements*.

## General idea of entities in REDAC

Entities are organized hierarchically, they are the primary data structure to allow
to setup a large analog supercomputer based on smallest components. For instance, one
REDAC will hold

* a number of iREDAC chassis which hold
* a number of mREDAC motherboards which hold
* a number of clusters which hold
* a number of DIMM modules which hold
* a number of reconfigurable computing elements

The entity tree will allow to address each element in a path like manner, for 
instance `/2/3/0/C/3` might represent the 2nd iREDAC, 3rd mREDAC, zeroth cluster,
C block, 3rd potentiometer.

Further details will be written when the REDAC is released. At the time being, it is
important to keep in mind that a single cluster is equivalent to the LUCIDAC computer.

## General idea of entities in LUCIDAC

The LUCIDAC entity tree looks as following:

* LUCIDAC itself, identified by its Ctrl Eth MAC address
  * `/FP` Front panel
  * `/0` Cluster 0 (the only cluster)
    * `/U` U block (reconfigurable part of interconnection matrix: Fanout)
    * `/C` C block (reconfigurable part of interconnection matrix: Coefficients)
    * `/I` I block (reconfigurable part of interconnection matrix: Fanin and summation)
    * `/SH` SH block (autonomous and transparent part for callibration)
    * `/M0` Math block (exchangable)
    * `/M1` Math block (exchangable)

Except of the Math blocks, all other entities are *predefined* and can, in principle,
not be replaced by something completely different. This is because the hardware wiring
is not flexible enough to allow something else happening at that place. For instance,
the position of the `U`, `C` and `I` blocks is fixed.

## Entity modeling

The metadata::MetadataMemory holds information which look like the following:

```
pos   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  
     +--------+---------------------------------------+
0x00 |version | storage size                          |
     +--------+-------------+-------------------------+
0x02 | class                | type                    |
     +----------------------+-------------------------+
0x04 | variant              | version                 |
     +----------------------+-------------------------+
0x06 | element specific configuration and             |
0x08 | calibration data                               |
...  |                                                |
0xF6 |                                                |
     +------------------------------------------------+
0xF8 | preprogrammed EUI-64 identifier                |
0xFA |                                                |
0xFC |                                                |
0xFE |                                                |
     +------------------------------------------------+
```

\note
Relevant are the definitions in the code, the figure above is purely for illustration

Entities are identified by a 4-tuple `(class, type, variant, version)` of integers.
Furthermore, entities hold 64bit of unique identification (EUI-64, like an Ethernet
MAC Address).

Anabrid will release a registry/database of possible entities in the future.


## Firmware representation

The firmware uses traditional object oriented inheritance with virtual methods to represent
entities (even the predefined non-swappable ones). The base class of all entities is
\ref entities::Entity (the inheritance graph on that documentation page is paritcularly interesting).

entities::Entity is an abstract base class which defines the contract all entities have
to fulfill. The actual entity tree is realized as a single-linked list with various
methods such as entities::Entity::get_child_entities() or 
entities::Entity::resolve_child_entity().
Configuration is (de)serialized with entities::Entity::config_from_json() and
entities::Entity::config_to_json().


## Protocol implementation

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

## JSON Protocol: Entity configuration

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
