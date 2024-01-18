==============
carbixenergymonitor.local
==============

How does it work?
-------------------

The CarbixEnergyMonitor connects to your local area network via WiFi.
The procedure for connecting is outlined `here <connectWiFi.html>`_.
When your router accepts the connection, it assigns an IP address
and necessary routing information to the CarbixEnergyMonitor using the
Dynamic Host Configuration Protocol (DHCP).
Depending on your router settings, that IP address will either be
a fixed local IP address, or a semi-random IP address chosen from
a pool of available addresses.

When you enter **carbixenergymonitor.local** into your browser,
your computer uses one of several similar
`zeroconf <https://en.wikipedia.org/wiki/Zero-configuration_networking>`_ 
protocols to discover the IP address that has been assigned to the CarbixEnergyMonitor.
These protocols go under several names: Bonjour (Apple) and LLMNR (Microsoft).
See the `zeroconf <https://en.wikipedia.org/wiki/Zero-configuration_networking>`_ 
link for the detailed WiKi.

Essentially, when you type carbixenergymonitor.local intro your browser,
the underlying networking layer in your computer broadcasts a 
datagram message available to all members of the LAN, asking 
if there is anyone out there that called carbixenergymonitor.

At startup, CarbixEnergyMonitor creates a process that listens for those datagrams.
When it hears it's name, it responds to the sender saying "I'm carbixenergymonitor
and my IP address is xx.xx.xx.xx".  The requestor makes a note of
this address and uses it to send subsequent transactions to carbixenergymonitor.local.

How does that *not* work?
-------------------------

Sounds simple doesn't it?  What could possibly go wrong?

Plenty.  First off, these zeroconf protocols are not part of the
standardized internet. If your computer doesn't have some version of
the protocol installed, it won't work.

There are other issues.  Remember that your computer resolves
the name and then stores the IP for future use?  Well, if the
CarbixEnergyMonitor gets assigned an IP address from a pool of addresses,
it can change without your computer knowing it.  That is a
common problem.  Your computer can talk to the CarbixEnergyMonitor for
days or weeks and then suddenly it stops.  It's possible the
CarbixEnergyMonitor restarted or it's DHCP lease expired and it got a
different IP address.  Some computers will simply never think
to broadcast a new query, insisting on unsuccessfully retrying the old IP
address forever.

How to make it work better.
---------------------------

It is recommended that you to assign a 
static IP to the CarbixEnergyMonitor right after installation.  You do this
in your router and should assign an IP address that is
not in the DHCP pool, so there is no opportunity for conflict.
Your router associates the specified IP address with the MAC
address of the CarbixEnergyMonitor and always gives it that address during
the DHCP handshake at startup.
Write it down.  If you subsequently find that you can't access
via carbixenergymonitor.local, you can use the IP address by
typing HTTP://xx.xx.xx.xx as a URL in the browser.

If you are reading this because you didn't assign a static IP
and now can't access your CarbixEnergyMonitor with carbixenergymonitor.local,
you may need to turn off your computer and the CarbixEnergyMonitor, restart
your router, then restart your computer and the CarbixEnergyMonitor.

What else?
----------

Throughout this section, we have been using the name carbixenergymonitor.local.
If you changed the name of the CarbixEnergyMonitor using the Device setup
of the configuration app, you would appended .local to that name.  
If you changed the name to *ttawatoi* you would use
ttawatoi.local.

The last word
-------------

Fix the IP and write it down.