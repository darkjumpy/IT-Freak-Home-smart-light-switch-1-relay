# IT Freak Home smart light switch (1 relay)
DIY, smart and relatively cheap light switch.

## -What IT Freak Home exactly is?
It was a private project of DIY smart home devices which I decide to publish as an open-source project.<br/>
I would like to make an IT Freak Home easy to build, relatively chip alternative for smart home products.
IT Freak Home works based on MQTT communication. I recommend using [Home Assistant](https://www.home-assistant.io/) for this project but if you would like to use something else go free.<br/>This project supports Arduino IDE OTA and has a web interface for a better user experience.

In my free time, I'm working on more smart devices for IT Freak Home so if you are interested you can check my GitHub:octocat: from time to time for more devices.

## -What do you need for a smart light switch?
You will need:
1. ESP8266 (ESP-12E)
1. AMS1117 3.3V
1. Capacitor 470uF 10V (6.3mm x 11mm)
1. Varistor (JVR14N391K)
1. Thermal fuse (TZ-P115/2)
1. 5V SSR relay (Omron G3MB-202P)
1. HI-LINK 5V power supply
1. Screw terminal with 3 pins
1. Custom PCB (You can download gerber files from this repository)
1. 3D printed casing (You can download STL files from this repository)

## -How to build it?
Put all of your parts on the PCB like on the picture:<br/>
<img src="/images/partsOnPcb.jpg" width="400px"><br/>
On the bottom side of the PCB, you have named where witch cable should go.
**(N-Neutral wire, LI-Live wire, LO-Light out)**<br/>

Attach the bottom part of the casing to the PCB. The plastic thing on the middle should go into a crack in PCB.<br/>
<img src="/images/bottomCasing.jpg" width="400px"><br/>

Now attach the top part of the casing. It should be attached to the power supply casing.<br/>
<img src="/images/topCasing.jpg" width="400px">

## -How to program it?
You can simply upload the binary file or compile it on your own.<br/><br/>

If you would like to compile it:<br/>

Libraries you will need:
1. StreamString.h
1. modified-WiFiManager.h (You can download this library from this repository)
1. MQTT.h
1. ArduinoJson.h
1. ArduinoOTA.h
1. FS.h  

Download "light-switch.ino", "IT_Freak_home_essentials.h" and modified WiFiManager library. Connect D0 to GND, programming pins to USB-UART converter and USB-UART converter to the PC. Now you can upload this program on your ESP-12E.<br/>
(If you don't have ESP8266 boards installed in your Arduino IDE check this https://github.com/esp8266/Arduino)

## -How to put it behind normal light switch?
(I recommend first configure your smart switch and see if it works)
* **Turn off the power in your home**
* Disassemble your normal light switch
* If you don't have a neutral wire in your electrical box ask the electrician about adding one
* Connect electric wires to screw terminal (On bottom of the PCB you can see where which wire should go)
* Put the smart switch to the electrical box and connect the normal light switch to the smart switch (To the "SWITCH" pins on PCB)
* Assemble your normal light switch to the wall 

## -How to configure it with [Home Assistant](https://www.home-assistant.io/)?
If you have already configured home assistant you should install and configure mosquitto MQTT broker.<br/>
[Here you have how to do it](https://www.home-assistant.io/docs/mqtt/broker#public-broker)<br/><br/>

After this connect to the WiFi access point of your smart switch (password: admin123) and in web browser write 192.168.4.1<br/>
Your browser should show you a configuration page. Go to the setup and complete the form.<br/>
Next, go to the configure WiFi and put here the credentials of your WiFi. (After connecting to WiFi you can find this page under device's IP)<br/>
<img src="/images/webConfig.jpg" width="700px">
<br/><br/>
If you have enabled MQTT auto discovery your device will show up in [Home Assistant](https://www.home-assistant.io/) under mosquitto broker integration.
<br/>
**Now your device is ready to use!!!** :tada:<br/>

## -If you would like you can buy me a coffee :D
<a href="https://www.buymeacoffee.com/itfreakjake" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 60px !important;width: 217px !important;" ></a>
