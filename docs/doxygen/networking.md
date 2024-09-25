\page networking Networking

The LUCIDAC Hybrid Controller has a 10/100mbit (10BASE-Te, 100BASE-TX) Ethernet controller. The firmware currently
only supports IPv4, not IPv6. The firmware uses the [lwIP](https://en.wikipedia.org/wiki/LwIP)-based
[QNEthernet](https://github.com/ssilverman/QNEthernet) library.


## How to find the hybrid controller in the network

By default, the hybrid controller configures its IPv4 network address via DHCP.
You need to know it's assigned IP address in order to connect to it. You can also
disable DHCP or change the configuration if you need to do so, see below for details.

The following services are active by default:

* The JSONL protocol server listening on TCP port `5732`
* A webserver listening on TCP port `80`
* The MDNS zeroconf announcement acting in the local broadcast domain

You can also find the IP address by asking your system administrator
or watching your DHCP logs. You can also perform a network scan such as 
`nmap -v -A -p 5732 192.168.1.1-255` to search for computers in your (exemplaric)
`192.168.1.1/24` network listening on port `5732`.

## DHCP client vs Static IP address
By default, the Hybrid Controller acts as a *DHCP client*. That means at startup it tries to
connect to a DHCP server and get a DHCP lease. 

The ethernet configuration can be customized at runtime with the user configuration messages.
The actual behaviour is implemented in the net::StartupConfig class.
In particular, for the DHCP client, the hostname can be configured.

If the DHCP client option is turned off,the IPv4 configuration can be set by passing the systems IP address,
netmask and gateway as well as DNS server address.
DHCP client support can be permanently disabled at firmware build-time with
the @ref feature-flags `ANABRID_SKIP_DHCP`.

\remark 
Any change to the network configuration requries a restart (reboot) of the teensy to take part. This
can be triggered with a `reboot` message type.

## Networking limitations

The following limitations apply to the current networking stack:

* The QNEthernet library does not update the lease after it elapsed. This is important 
  for DHCP servers with a short lease time.
* No support for static routes (routing tables), only a single default gateway.

## Changing the networking settings

The class net::StartupConfig has a large number of fields which are exposed via the JSON protocol
(msg::handlers::SetNetworkSettingsHandler). For details see \ref protocol.

## The USB virtual serial terminal

As a design decision, the USB serial terminal is a **fallback** or backup option to connect to the microcontroller.
For instance, the network can be easily reconfigured from a USB Serial connection
if no other access is possible.

The virtual terminal speaks the JSONL protocol and provides an admin context, so there is no further
authentification neccessary/implemented. Most clients can easily connect to the serial
console the same way as they do to TCP/IP targets.

The hybrid controller will send debug information, including its IP address,
via the USB-serial connection. 
After a restart, use `cat /dev/ttyACM0` or similar to access the debug output.

\note
The debug output is not buffered and may not be visible if you are too slow. In such
a case, you can query the log at a later time.
