# IT Freak Home smart light switch (1 relay)
DIY, smart and relatively cheap light switch.<br/>
(This project supports OTA)
## -What IT Freak Home exactly is?
It was a private project of DIY smart home devices which I decide to publish as an open-source project.<br/>
I would like to make an IT Freak Home easy to build, relatively chip alternative for smart home products.
IT Freak Home works based on MQTT communication. I recommend using [Home Assistant](https://www.home-assistant.io/) for this project but if you would like to use something else go free. You can even make your own program for your devices. Initially, my project was to be based on a mesh network but there were some issues with a stable connection.<br/>

In my free time, I'm working on more smart devices for IT Freak Home so if you are interested you can check my GitHub:octocat: from time to time for more devices.

## -What do you need for a smart light switch?
You will need:
1. ESP8266 ESP-12E
1. AMS1117 3.3V
1. Capacitor 470uF 10V (6.3mm x 11mm)
1. 5V SSR relay 
1. HI-LINK 5V power supply
1. Screw terminal with 3 pins
1. Custom PCB (You can download gerber files from this repository)
1. 3D printed casing (You can download STL files from this repository)

## -How to build it?
Put all of your parts on the PCB like on the picture:<br/>
<img src="/images/partsOnPcb.jpg" width="400px"><br/>
(In the picture is an older version of the PCB. Your PCB should have a place for a capacitor)
On the bottom side of the PCB, you have named where witch cable should go.
**(N-Neutral wire, LI-Live wire, LO-Light out)**<br/>

Attach the bottom part of the casing to the PCB. The plastic thing on the middle should go into a crack in PCB.<br/>
<img src="/images/bottomCasing.jpg" width="400px"><br/>

Now attach the top part of the casing. It should be attached to the power supply casing.<br/>
<img src="/images/topCasing.jpg" width="400px">

## -How to program it?
Libraries you will need:
1. [PubSubClient.h](https://github.com/knolleary/pubsubclient) 
1. ESP8266WiFi.h
1. ESP8266mDNS.h
1. WiFiUdp.h
1. [ArduinoOTA.h](https://www.arduino.cc/reference/en/libraries/arduinoota/)
1. [EEPROM.h](https://www.arduino.cc/en/Reference/EEPROM)   

Download "light-switch.ino"and set your WiFi and MQTT broker credentials. Connect D0 to GND, programming pins to USB-UART converter and USB-UART converter to the PC. Now you can upload this program on your ESP-12E.<br/>
(If you don't have ESP8266 boards installed in your Arduino IDE check this https://github.com/esp8266/Arduino)<br/>
Open serial monitor on 115200 baud and note somewhere state topic and command topic.
On these topics, your node will publish state and receive commands.

## -How to put it into the wall?
(I recommend first configure your smart switch with [Home Assistant](https://www.home-assistant.io/) or with your MQTT broker and see if it works)
* **Turn off the power in your home**
* Disassemble your normal light switch
* If you don't have a neutral wire in your electrical box ask the electrician about adding one
* Connect electric wires to screw terminal (On bottom of the PCB you can see where which wire should go)
* Put the smart switch to the electrical box and connect the normal light switch to the smart switch (To the switch pins on PCB)
* Assemble your normal light switch to the wall 

## -How to configure it with [Home Assistant](https://www.home-assistant.io/)?
If you have already configured home assistant you should install and configure mosquitto MQTT broker.<br/>
[Here you have how to do it](https://www.home-assistant.io/docs/mqtt/broker#public-broker)<br/><br/>

After this put this in your configuration.yaml<br/>
```yaml
switch:
 - platform: mqtt
   name: "NameForYourSwitch"
   unique_id: "PutHereSomethingUnique"
   command_topic: "HereShouldBeYourNodeCommandTopic"
   state_topic: "HereShouldBeYourNodeStateTopic"
   retain: false
   payload_on: "ON"
   payload_off: "OFF"
   state_on: "ON"
   state_off: "OFF"
   icon: "mdi:lightbulb"
   ```
    
Restart Home Assistant and
add your smart light switch to the lovelace card.<br/>
**Now your device is ready to use!!!** :tada:<br/>

## -If you would like you can buy me a coffee :D
<a href="https://www.buymeacoffee.com/itfreakjake" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>
