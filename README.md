This project will be an open source ESP32 based Wifi MQTT light switch. 

Right now this is the beginning. There is much more work to be done.

Switched to PlatformIO as I find the updates to the VScode Arduino addon are not helpful now that they are depreciating the GUI version before Arduino 2.0. PlatformIO is more stable and I am beginning to like it more and more.

#Todo
1. Finalize the hardware design and order boards and parts for testing
2. Define the software requirements
3. Complete the software for the ESP32-S3
4. Design the enclosure in Fusion 360
5. Print and test enclosure

#Features to add
1. Over the Air updates
2. Web server avilable at all time to look at status, veriables and configuration
3. Light switch works no matter what even before being configured
4. MQTT tiggering via Nodered
5. Of course fits into a standard wall box in North America
6. Standard Normmaly open switching and three way switching
7. If I can figure it out, I would like the switch to know if the switched circuit is active. Meaning, if the sawitch is on. This will require a means to measure the current on the switched side.
8.
