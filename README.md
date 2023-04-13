## Introduction
This project will be an open source ESP32 based Wifi MQTT light switch. My intention is to make a device that will fit into a "Decor" style switch box. Sonoff devices are interesting but they do not fit into a regular box along side other non-smart switches.

I have gone through the first iteration of the design that does not include current sensing on the output. This version would work fine of a single on off switch but I would like this switch design to work for both single on/off switches and for three way switches as well. This requires current sensing that will be explained later

## Objective
My objective for this project was explained at a high level in the introduction section. But that did not explain my motivation for this project. I had installed several TP-link smart switches in my home all within a month of each other. Five years later they all died within a month of each other. That seemed like too much of a coincidence to me so I decided to make my own. This project is the idea come to life.

## Features
I will use an integrated web server for manage the following features
 * Web based control of the switch
 * Web based status of switch
 * Web based OTA updates of the switch firmware
 * Web based settings update
 * Stand alown AP mode for initial configuration
 * Remember power state for power failures using preferences lib
 * LED colors. Red - Boot up, Yellow - WIFI connected, Green - WIFI and MQTT connected, Blue = AP is active.
 * Fits into a standard wall box in North America as a Decor style switch

 In addition to these features, there will be MQTT control of the switch as well as MQTT status updates of the switch.

## Design
At the heart of the project are two custom designed PCBs. There is a power board that handles the SPDT relay for AC switching, the conversion of the AC in to DC and provides both 5 volts DC and 3.3 volts DC to the controller board. The controller board has the ESP32 for control, communications and the buttons for triggering. There is a "light" button and a "factory" button. The light button is of course used to turn the light on and off while the factory button is used to do a factory reset. There is a Neopixel on the control board to indicate the status of the device in addition to a dedicated power led. [Here is a link](https://oshwlab.com/bhboyle/esp32-light-switch) to the EasyEDA project.

For safety and simplicity sake, I used the [Mean Well IRM-02-5](https://www.digikey.ca/en/products/detail/mean-well-usa-inc/IRM-02-5/7704628?s=N4IgTCBcDaIIwA4BsSC0BmADJgnKgcgCIgC6AvkA) 2 watt AC-DC converter. As with most things it is a compromise. In this case between simplicity, size, safety and cost. It is a little on the pricy side at $10 USD each but it makes it very simple to build the power supply and because it is a sealed housing it is nice and safe. This module creates 5 volts DC that drives the relay and the Neopixel and I use an LDO linear regulator to generate the 3.3 volts the ESP32 needs to operate.

The Preferences library is used to store the variables in the NV ram for retrieval after a power down.

## Todo
The biggest remaining item on the list of things to do at this point for version one of this project is docimentation.

### Build environment
Just in case it is not obvious, I wanted to mention that the project is being working on in VScode with the PlatformIO plugin. If you choose to compile the code in the Arduino IDE you simply need to get the main.cpp file from the SRC folder and put it in to your sketch folder. You will also need to gather and install the needed libraries.

### DISCLAMER
Please understand that this project uses mains power and if you do not know what your doing you can seriously harm or even kill yourself. In some places you may not even be legally allowed to install electrical equipment if you are not a licensed electrician. PLEASE be careful and if you are not confident do not do electrical work yourself. **You have been warned**

![first image](Images/First_picture.jpg)