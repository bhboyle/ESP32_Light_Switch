## Introduction
This project will be an open source ESP32 based Wifi MQTT light switch. My intention is to make a device that will fit into a "Decor" style switch box. Sonoff devices are interesting but they do not fit into a regular box along side other non-smart switches. 

The circuit boards are split into two. There is a power board that handles the conversion of the AC in to DC and provides both 5 volts DC and 3.3 volts DC to the controller board. The controller board has the ESP32 for control and the buttons for triggering. There is a "light" button and a "factory" button. The light button is of course used to turn the light on and off while the factory button is used to do a factory reset. There is a Neopixel on the control board to indicate the status of the device in addition to a dedicated power led.

The Preferences library is used to store the variables in the NV ram for retrieval after a power down.

Right now this is the beginning. There is much more work to be done.

## Todo
1. ~~Finalize the hardware design and order boards and parts for testing.~~ **Boards have been ordered**
2. ~~Define the software requirements~~
3. Complete the software for the ESP32-S3
4. Design the enclosure in Fusion 360
5. Print and test the enclosure

## Features to add
1. Over the Air updates if possible
2. Web server available at all times to look at status, variables and configuration
3. Light switch works no matter what even before being configured
4. MQTT triggering via node-red
5. Of course fits into a standard wall box in North America
6. Standard Normally open switching and three way switching
7. If I can figure it out, I would like the switch to know if the switched circuit is active. Meaning, if the switch is on. This will require a means to measure the current on the switched side.
8. LED colors. Green - WIFI and MQTT connected, Red - Boot up, Yellow - WIFI connected
9. Remember power state for power failures using preferences

