.. REDAC-Firmware documentation master file, created by
   sphinx-quickstart on Tue Sep 24 14:48:58 2024.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

REDAC-Firmware documentation
============================

This documentation covers the C++ firmware for the hybrid controller (microcontroller on Ctrl-block) of the
`analog-digital hybrid computer LUCIDAC <https://anabrid.com/lucidac>`_) (and REDAC) made
by `anabrid <https://anabrid.com/>`_.

The primary audience for this documentation are people who want to program right **on** the
microcontroller or who need to understand the functionality of the digital part of the
system. As a user who primarily works via USB/network with the device, you do not need to
read this documentation. You better look into the `LUCIDAC User Manual (booklet)
<https://www.anabrid.com/lucidac-user-manual.pdf>`_ or the relevant client
documentations. See the documentation portal for details... (\todo fixme). For the time being,
https://github.com/anabrid can be a good starting point.

This documentation is generated with *Doxygen* and *Sphinx*. You are currently viewing the
main entry page of the documentation.

.. 

   This tool builds primarily an in-depth crossreference
   for the actual code which can be browsed either by [Namespace](namespace.html), by
   [Class name](annotated.html) or by [Folder structure](files.html). Next to this extracted
   documentation, the documentation is supplemented with various introductive and overview
   documents:


.. toctree::
   :maxdepth: 2
   :caption: User Guide
   
   texts/users.rst
   texts/networking.rst
   texts/permanent_settings.rst
   texts/security.rst
   texts/licensing.rst

.. toctree::
   :maxdepth: 2
   :caption: Developer Guide
   
   texts/developer.rst
   texts/getting-started.rst
   texts/feature-flags.rst
   texts/style-guide.rst
   texts/docs.rst
   texts/wording.rst

.. toctree::
   :maxdepth: 2
   :caption: Architecture
   
   texts/architecture.rst
   texts/entities.rst
   texts/protocol.rst
   texts/nvmconf.rst
   texts/calibration.rst

.. toctree::
   :maxdepth: 2
   :caption: Firmware API

   lib-communication.rst
   lib-computation.rst
   lib-controller.rst
   lib-hal-lucidac.rst
   lib-platform-lucidac.rst
