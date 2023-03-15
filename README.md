## Introduction
This project will be an open source ESP32 based Wifi MQTT light switch. My interntion is to make a device that will fit into a "Decor" style switch box. Sonoff devices are interesting but they do not fit into a regular box allong side other non-smart switches. 

Right now this is the beginning. There is much more work to be done.

## Todo
1. Finalize the hardware design and order boards and parts for testing
~~ 2. Define the software requirements
3. Complete the software for the ESP32-S3
4. Design the enclosure in Fusion 360
5. Print and test enclosure

## Features to add
1. Over the Air updates
2. Web server avilable at all time to look at status, variables and configuration
3. Light switch works no matter what even before being configured
4. MQTT tiggering via Nodered
5. Of course fits into a standard wall box in North America
6. Standard Normmaly open switching and three way switching
7. If I can figure it out, I would like the switch to know if the switched circuit is active. Meaning, if the switch is on. This will require a means to measure the current on the switched side.
8. LED colors. Green - WIFI and MQTT connected, Red - Boot up, Yellow - WIFI connected
