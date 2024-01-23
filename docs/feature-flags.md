Use build flags to turn on/off features
=======================================

The following keywords can be used in a `platform.ini` as in the following example line:

```
build_flags = -DANABRID_DEBUG_INIT -DANABRID_UNSAFE_INTERNET -DANABRID_SKIP_DHCP
```

Here is a (not yet comprehensive) list of keywords:

* `ANABRID_UNSAFE_INTERNET`: Skips to build any user/password authentification into the code
* `ANABRID_SKIP_DHCP`: Skips to build the DHCP client at startup (use this if you want to
  enforce using static IPv4)
* `ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER`: Enables the global storage plugin loader, which reserves
  a few kB of precious global variable storage for the plugin feature.
  This option currently also enables the plugin system as-is.
* `ANABRID_ENABLE_OTA_FLASHING`: Enables over-the-air flashing, i.e. firmware update over TCP/IP
  instead of traditional Teensy USB.
* `ANABRID_DEBUG_INIT`: Print more debugging messages at startup
* `ANABRID_DEBUG_...` also for various other modules

\warning
If you enable the plugin system or OTA flashing without enabling the authentification system,
anybody with network access can run arbitrary code on your microcontroller. That is, the
combination `-DANABRID_UNSAFE_INTERNET -DANABRID_ENABLE_GLOBAL_PLUGIN_LOADER` and/or
`-DANABRID_UNSAFE_INTERNET -DANABRID_ENABLE_OTA_FLASHING` is dangerous.
