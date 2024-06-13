# Networking

The LUCIDAC Hybrid Controller has a 10/100mbit (10BASE-Te, 100BASE-TX) Ethernet controller. The firmware currently
only supports IPv4, not IPv6. The firmware uses the [lwIP](https://en.wikipedia.org/wiki/LwIP)-based
[QNEthernet](https://github.com/ssilverman/QNEthernet) library.

## DHCP client vs Static IP address
By default, the Hybrid Controller acts as a *DHCP client*. That means at startup it tries to
connect to a DHCP server and get a DHCP lease. 

The ethernet configuration can be customized at runtime with the user configuration messages.
The actual behaviour is implemented [UserDefinedEthernet](@ref net::ethernet::UserDefinedEthernet)
class. In particular, for the DHCP client, the hostname can be configured.

If the DHCP client option is turned off,the IPv4 configuration can be set by passing the systems IP address,
netmask and gateway as well as DNS server address.
DHCP client support can be permanently disabled at firmware build-time with
the [Feature flag](feature-flags.md) `ANABRID_SKIP_DHCP`.

\remark 
Any change to the network configuration requries a restart (reboot) of the teensy to take part. This
can be triggered with a `reboot` message type.

## Networking limitations

The following limitations apply to the current networking stack:

* The QNEthernet library does not update the lease after it elapsed. This is important 
  for DHCP servers with a short lease time.
* No support for static routes (routing tables), only a single default gateway.
* Currently, the code implements only a "non-forking", blocking TCP/IP Server. That means only
  one (ingoing) TCP connection at a time is allowed.

## Connecting via USB

The USB Serial console can be used as a backup connection line in case of problems with the network
configuration. For instance, the network can be easily reconfigured from a USB Serial connection
if no other access is possible.

\todo
Show demo how to set the network to static ip via USB.
