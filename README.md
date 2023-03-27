## Introduction
This project will be an open source ESP32 based Wifi MQTT light switch. My intention is to make a device that will fit into a "Decor" style switch box. Sonoff devices are interesting but they do not fit into a regular box along side other non-smart switches. 

The circuit boards are split into two. There is a power board that handles the SPDT relay for AC switching, the conversion of the AC in to DC and provides both 5 volts DC and 3.3 volts DC to the controller board. The controller board has the ESP32 for control and the buttons for triggering. There is a "light" button and a "factory" button. The light button is of course used to turn the light on and off while the factory button is used to do a factory reset. There is a Neopixel on the control board to indicate the status of the device in addition to a dedicated power led.

For safety and simplicity sake, I used the [Mean Well IRM-02-5](https://www.digikey.ca/en/products/detail/mean-well-usa-inc/IRM-02-5/7704628?s=N4IgTCBcDaIIwA4BsSC0BmADJgnKgcgCIgC6AvkA) 2 watt AC-DC converter. As with most things it is a compromise. In this case between simplicity, size, safety and cost. It is a little on the pricy side at $10 USD each but it makes it very simple to build the power supply and because it is a sealed housing it is nice and safe. This module creates 5 volts DC and I use a 1 amp LDO linear regulator the generate the 3.3 volts the ESP32 needs to operate.

The Preferences library is used to store the variables in the NV ram for retrieval after a power down.

Right now this is the beginning. There is much more work to be done.

## Todo
1. ~~Finalize the hardware design and order boards and parts for testing.~~ **Boards have been ordered**
2. ~~Define the software requirements~~ **Done (at least for now)**
3. Complete the software for the ESP32-S3 **This is almost done**
4. Design the enclosure in Fusion 360 **I will not start this until I have the boards in hand and can measure them in the real world**
5. Print and test the enclosure. **I have ordered some PETG to print the initial housings from. I hope it has the characteristics I am looking for.**

## Features to add
1. ~~Over the Air updates~~ ** added using the [ElegantOTA lib](https://github.com/ayushsharma82/ElegantOTA). **Tested and working**
2. ~~Web server available at all times to look at status, variables and configuration~~ **Tested and working**
3. ~~Light switch works no matter what even before being configured~~ **Tested and working**
4. ~~MQTT triggering via node-red~~ **Tested and working** 
5. ~~Triggering via Wed interface~~ **Tested and working**
6. Fits into a standard wall box in North America
7. ~~Standard Normally open switching and three way switching~~
8. If I can figure it out, I would like the switch to know if the switched circuit is active. Meaning, if the switch is on. This will require a means to measure the current on the switched side. This is only useful in the 3-way mode as it does not necessarily matter what state the relay is in for the circuit to be on.

I have found the ACS712 this is a very interesting device that uses the Hall effect to measure the AC current without directly connecting its output to the AC voltage. There are quite a few variants of this device but this one runs on 3.3 volts so it is compatible with the ESP32 and it measures up to 10 amps so it is right in the ball park with the capacity of the relay I am using. I am updating the schematic for the next version that I will eventually order.

So after some more research I have discovered that the ACS712 may well be perfect for this project. it has a maximum current handling capacity of 55 amps, way more than I would ever need. which I like in terms of having lots of overhead for a safety margin. Also it can be used to determine how much power is being used over time. This is huge.

I have updated the schematics to include the new device but the original boards have not yet arrived. 

9. LED colors. Red - Boot up, Yellow - WIFI connected, Green - WIFI and MQTT connected, Blue = AP is active **Done**
10. Remember power state for power failures using preferences **Tested and working**

### Build environment
Just in case it is not obvious, I wanted to mention that the project is being working on in VScode with the PlatformIO plugin. If you choose to compile the code in the Arduino IDE you simply need to get the main.cpp file from the SRC folder and put it in to your sketch folder. You will also need to gather and install the needed libraries.
