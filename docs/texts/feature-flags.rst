.. _feature-flags:

Build/Feature flags
===================

The following keywords can be used in the ``platform.ini`` file as in the following example line:

::

   build_flags = -DANABRID_DEBUG_INIT -DANABRID_UNSAFE_INTERNET -DANABRID_SKIP_DHCP


On this page is a list of all keywords used so far. Note that all our flags are
prepended with ``ANABRID_`` in order to distinguish them for instance from PlatformIO
flags (``PIO_...``) or flags related to third party libraries.

Note that PlatformIO recompiles the complete code base
if it detects a change in the ``platform.ini``.

Flags related to turning on/off networking features
---------------------------------------------------

Using these flags makes the system *ignore* whatever is currently set by
the :ref:`nvmconf`. When calling a ``reset`` on the permanent settings, the
presence of this flags determines the permanent settings. Otherwise, the 
permanent settings and these flags do not touch.

``ANABRID_UNSAFE_INTERNET`` (Equivalent setting: ``auth::Gatekeeper::enable_auth``)
  Disables all authentification mechanisms in the code. Firmware images
  built this way will treat any ingoing connection with *admin* privileges.
  This can be very practical in situations which are known to be safe or 
  when you don't care about the microcontroller security at all.

``ANABRID_SKIP_DHCP`` (Equivalent setting: ``net::StartupConfig::enable_dhcp``)
  Skips to build the DHCP client at startup. Use this if you want to
  enforce using static IPv4.
  
``ANABRID_SKIP_ETHERNET`` (Equivalent setting: ``net::StartupConfig::enable_ethernet``)
  Skips ethernet setup at startup. Use this if you have problems with the
  Ethernet you cannot otherwise solve.
    
Flags related to loading code
-----------------------------

These flags allow to generally turn on/off the availability of certain
critical code which is very sensitive to :ref:`security` but also has
a lot of impact about how to load new code on the microcontroller. Using
these features has the possibility to make the system unstable, unreachable
or un-flashable (only over OTA). If you change these flags from the default,
make sure you know what you do.

.. warning::

  If you enable the plugin system or OTA flashing without enabling the authentification system,
  anybody with network access can run arbitrary code on your microcontroller. That is, the
  combination ``-DANABRID_UNSAFE_INTERNET -DANABRID_ENABLE_GLOBAL_PLUGIN_LOADER`` and/or
  ``-DANABRID_UNSAFE_INTERNET -DANABRID_ENABLE_OTA_FLASHING`` is dangerous.
  
``ANABRID_ENABLE_GLOBAL_PLUGIN_LOADER``
  Enables the global storage plugin loader, which reserves
  a few kB of precious global variable storage for the plugin feature.
  This option currently also enables the plugin system as-is.

``ANABRID_ENABLE_OTA_FLASHING``
  Enables over-the-air flashing, i.e. firmware update over TCP/IP
  instead of traditional Teensy USB.

Flags related to shipping/vendoring
-----------------------------------

``ANABRID_WRITE_EEPROM``
  Enables certain protocol-exposed calls for writing permanent settings on
  different EEPROMs (both the MCU emulated EEPROM as well as the entitiy
  eeproms). This is by default not enabled in release builds and used only
  during device manufacturing.

  
Flags related to debugging
--------------------------

``ANABRID_PEDANTIC``
  In pedantic mode, the firmware reads some configuration written out to chips
  over SPI after writing. This takes more time but ensures the write out has
  happened correctly. We use this only for some pio tests and not in the general
  production code (main firmware). In the main firmware we assume that the chip
  communication was properly debugged and is working.

``ANABRID_DEBUG``
  The debugging macro system is (still) the primary way to increase the logging
  level. It works assuming that in production code there is not much need for
  debugging. Next to this general flag, there exist various sub-flags to enable
  more debugging printing in certain sub-systems. Note that all this affects are
  serial console printouts.
  
``ANABRID_DEBUG_INIT``
  More debugging information at startup.
  
``ANABRID_DEBUG_DAQ``
  More debugging information at data aquisition.
  
``ANABRID_DEBUG_STATE``
  More debugging information at FlexIO state managament.
  
``ANABRID_DEBUG_CALIBRATION``
  More debugging information at self-calibration.

There might be even more ``ANABRID_DEBUG_...`` flags not covered here.
