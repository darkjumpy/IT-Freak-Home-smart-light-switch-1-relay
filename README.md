# IT Freak Home smart light switch (1 relay)
DIY, smart and relatively chip light switch.<br/>
(This project supports OTA)
## -What IT Freak Home exactly is?
It was a private project of DIY smart home devices which I decide to publish as an open-source project.<br/>
I would like to make an IT Freak Home easy to build, relatively chip alternative for smart home products.
IT Freak Home works based on MQTT communication. I recommend using [Home Assistant](https://www.home-assistant.io/) for this project but if you would like to use something else go free. You can even make your own program for your devices. Initially, my project was to be based on a mesh network but there were some issues with a stable connection.<br/>

In my free time, I'm working on more smart devices for IT Freak Home so if you are interested you can check my GitHub:octocat: from time to time for more devices.

## -What do you need for a smart light switch?
You will need:
1. ESP8266 Wemos D1 MINI
1. 5V SSR relay 
1. HI-LINK 5V power supply
1. Screw terminal with 3 pins
1. Universal PCB (Project of custom PCB soon) 

## -How to build it?
You can connect parts like this:<br/>
**(N-Neutral wire, LI-Live wire, LO-Light out)**<br/>
![Sketch](/images/sketch.jpg)

You can put your parts on universal PCB like this:<br/>
![Parts on universal PCB](/images/partsOnPCB.jpg)

## -How to program it?
Libraries you will need:
1. [PubSubClient.h](https://github.com/knolleary/pubsubclient) 
1. ESP8266WiFi.h
1. ESP8266mDNS.h
1. WiFiUdp.h
1. [ArduinoOTA.h](https://www.arduino.cc/reference/en/libraries/arduinoota/)
1. [EEPROM.h](https://www.arduino.cc/en/Reference/EEPROM)   

Download "light-switch.ino"and set your WiFi and MQTT broker credentials. Now you can upload this program on your Wemos D1 MINI.<br/>
(If you don't have ESP8266 boards installed in your Arduino IDE check this https://github.com/esp8266/Arduino)<br/>
Open serial monitor on 115200 baud and note somewhere state topic and command topic.
On these topics, your node will publish state and receive commands.

## -How to put it into the wall?
(I recommend first configure your smart switch with [Home Assistant](https://www.home-assistant.io/) or with your MQTT broker and see if it works)
* **Turn off the power in your home**
* Disassemble your normal light switch
* If you don't have a neutral wire in your electrical box ask the electrician about adding one
* Connect electric wires to screw terminal (On sketch you can see where which wire should go)
* Put the smart switch to the electrical box and connect the normal light switch to the smart switch (Like on the sketch)
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
    
Add your smart light switch to the lovelace card.<br/>
**Now your device is ready to use!!!** :tada:<br/>

## -If you would like you can buy me a coffee :D
<a href="https://www.buymeacoffee.com/itfreakjake" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>
