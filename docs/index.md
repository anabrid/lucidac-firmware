\mainpage Documentation 

This documentation covers the C++ firmware for the hybrid controller (microcontroller on Ctrl-block) of the
[analog-digital hybrid computer LUCIDAC](https://anabrid.com/lucidac) (and REDAC) made
by [anabrid](https://anabrid.com/).

The primary audience for this documentation are people who want to program **on** the
microcontroller or who need to understand the functionality of the digital part of the
system. As a user who primarily works via USB/network with the device, you do not need to
read this documentation. You better look into the *lucidac user docs* or the relevant client
documentations. See the documentation portal for details... (\todo fixme). For the time being,
https://github.com/anabrid can be a good starting point.

This documentation is generated with *Doxygen*. This tool builds primarily an in-depth crossreference
for the actual code which can be browsed either by [Namespace](namespace.html), by
[Class name](annotated.html) or by [Folder structure](files.html). Next to this extracted
documentation, the documentation is supplemented with various introductive and overview
documents:

- \subpage usage - Simple instructions how to start working on/with the Firmware.
- \subpage devs - Actual in-depth explanations on how to start serious development with the Firmware.
- \subpage concepts - Explanations of design decisions and conceptual architecture
- \subpage various - Misc other topics


![LUCIDAC Breakdown picture](LUCI-Isometric.png)

\page usage First Steps

This section will teach you the **Usage** and first step developer tasks in a
tutorial/how-to manner. These topics are organized on the following sub-pages:

- \subpage users
- \subpage networking

\page devs Development

- \subpage getting-started-dev
- \subpage feature-flags
- \subpage style-guide
- \subpage docs
- \subpage wording


\page concepts Concepts

- \subpage architecture
- \subpage entities
- \subpage protocol
- \subpage nvmconf

\page various Various topics

- \subpage security
- \subpage licensing  (see https://github.com/anabrid/lucidac-firmware/blob/lucidac/LICENSING.md)
