OSD
===

No "cool" name here yet.

ATMEGA328p firmware for controlling an SPI-attached MAX7456 analog OSD overlay chip via simple text commands over UART connection.

I wrote this up because the existing OSD projects I found:

 - Required implementations of special protocols like MAVLink to be used
 - Were hacky decade-old abandonware I couldn't even get to build

So I made my own.  It works for me, and I hope it may be useful to others too.

