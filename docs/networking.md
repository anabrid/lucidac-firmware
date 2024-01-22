# Networking

The LUCIDAC Hybrid Controller has a 10/100mbit Ethernet controller. The firmware currently
only supports IPv4, not IPv6. The firmware uses the [QNEthernet](https://github.com/ssilverman/QNEthernet)
library.

Currently the firmware does not support changing the MAC address.

## DHCP client vs Static IP address
By default, the Hybrid Controller acts as a *DHCP client*. That means at startup it tries to
connect to a DHCP server and get a DHCP lease. Note for DHCP servers with a short lease time:
The QNEthernet library does not update the lease after it elapsed.

The ethernet configuration can be customized at runtime with the user configuration messages.
The actual behaviour is implemented [UserDefinedEthernet](@ref user::ethernet::UserDefinedEthernet)
class. In particular, for the DHCP client, the hostname can be configured.

If the DHCP client option is turned off, the static IP address can be set by giving the IP address,
netmask and gateway address. DHCP client support can be permanently disabled at firmware build-time with
the [Feature flag](feature-flags.md) `ANABRID_SKIP_DHCP`.

\remark 
Any change to the network configuration requries a restart (reboot) of the teensy to take part. This
can be triggered with a `reboot` message type.

## Connecting via USB

The USB Serial console can be used as a backup connection line in case of problems with the network
configuration. For instance, the network can be easily reconfigured from a USB Serial connection
if no other access is possible.
