# IT Freak Home smart light switch (1 relay)
DIY, smart and relatively chip light switch 
## -What IT Freak Home exactly is?
It was a private project of DIY smart home which I decide to publish as open source project.<br/>
I would like to make an IT Freak Home easy to build, relatively chip alternative for smart home products.
IT Freak Home works on a mesh network ([PainlessMesh](https://gitlab.com/painlessMesh/painlessMesh)) which has 1 root node publishing/receiving data to/from MQTT broker. I recommend using [Home Assistant](https://www.home-assistant.io/) for this project but if you would like to use something else go free. You can even make your own program for your devices and connect them directly to the router if you don't wont to use a bridge. I think using a mesh network with a bridge is a good solution because you have only 1 device connected to the router which makes devices list on the router cleaner and you don't have to worry about the distance between your smart devices and router.<br/><br/>

In my free time, I'm working on more smart devices for IT Freak Home so if you are interested you can check my GitHub:octocat: from time to time for more devices.

## -What do you need for a smart light switch?
You will need:
1. ESP8266 Wemos D1 MINI
1. 5V SSR relay 
1. HI-LINK 5V power supply
1. Screw terminal with 3 pins
1. Universal PCB (Project of custom PCB soon)
1. Already working IT Freak Home bridge

## -How to build it?
You can connect parts like this:<br/>
**(N-Neutral wire, LI-Light in, LO-Light out)**<br/>
![Sketch](/images/sketch.jpg)

You can put your parts on universal PCB like this:<br/>
![Parts on universal PCB](/images/partsOnPCB.jpg)

## -How to program it?
Libraries you will need:
1. [PainlessMesh](https://gitlab.com/painlessMesh/painlessMesh) 
1. [ArduinoJson](https://github.com/bblanchon/ArduinoJson) (For PainlessMesh)
1. [TaskSheduler](https://github.com/arkhipenko/TaskScheduler) (For PainlessMesh)
1. [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP) (For PainlessMesh)
1. [EEPROM.h](https://www.arduino.cc/en/Reference/EEPROM) 

Download "light-switch.ino" and upload this program on your Wemos D1 MINI.<br/>
(If you don't have ESP8266 boards installed in your Arduino IDE check this https://github.com/esp8266/Arduino)<br/>
Open serial monitor on 115200 baud and note somewhere state topic and command topic.
On these topics, your node will publish state and receive commands.

## -How to put it into the wall?
(I recommend first configure your smart switch with [Home Assistant](https://www.home-assistant.io/) or with your MQTT broker and see if it works)
* **Turn off the power in your home**
* Disassemble your normal light switch
* If you don't have a neutral wire in your electrical box ask the electrician about adding one
* Connect electric wires to screw terminal (On sketch you can see where which wire should go)
* Put the smart switch to the electrical box and connect the normal light switch to the smart switch (Like on sketch)
* Assemble your normal light switch to the wall 

## -How to configure it with [Home Assistant](https://www.home-assistant.io/)?
If you have already configured home assistant you should install and configure mosquitto MQTT broker.<br/>
[Here you have how to do it](https://www.home-assistant.io/docs/mqtt/broker#public-broker)<br/><br/>

After this put this in your configuration.yaml<br/>
```yaml
switch HerePutYourDeviceName:
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
