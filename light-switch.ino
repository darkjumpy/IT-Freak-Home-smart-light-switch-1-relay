#include <ESP8266WiFi.h>    
#include <PubSubClient.h>   
#include <ESP8266mDNS.h>   
#include <WiFiUdp.h>        
#include <ArduinoOTA.h>     
#include <EEPROM.h>

// WiFi configuration
#define wifi_ssid "Your WiFi name"
#define wifi_password "Your WiFi password"

// MQTT configuration
#define mqtt_server "MQTT broker IP (example: 192.168.1.11)"
#define mqtt_user "MQTT username"
#define mqtt_password "MQTT password"

#define RELAY 14
#define SWITCH 13

void mqttCallback(char* topic, byte* payload, unsigned int length);

// Start MQTT client
WiFiClient espClient;
PubSubClient mqtt_client(mqtt_server, 1883, mqttCallback, espClient);

//WiFiServer TelnetServer(8266);

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.print(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("OK");
  Serial.print("   IP address: ");
  Serial.println(WiFi.localIP());
}

boolean isLightOn=false, prevSwitchState, isOtaUploading=false;
String nodeId, msg, command_topic, state_topic;
unsigned long prevMillisMqttReconnect=0, intervalMqttReconnect=5000;
int mqttReconnectCounter=0, addr = 0;

void setup() 
{
  pinMode(RELAY, OUTPUT);
  EEPROM.begin(512);
  isLightOn=EEPROM.read(addr);
  digitalWrite(RELAY, isLightOn);
  if(isLightOn==true)
  {
    EEPROM.write(addr, false);
    EEPROM.commit(); 
  } 

  pinMode(SWITCH, INPUT_PULLUP);
  
  Serial.begin(115200);
  Serial.println("\r\nSetting up....");
  nodeId=String(ESP.getChipId());
  String hostName="LightSwitch-"+nodeId;
  ArduinoOTA.setHostname(hostName.c_str());
 
  setup_wifi();

  Serial.print("Configuring OTA device...");
//  TelnetServer.begin();   
  ArduinoOTA.onStart([]() {Serial.println("OTA starting...");isOtaUploading=true;});
  ArduinoOTA.onEnd([]() {Serial.println("OTA update finished!");Serial.println("Rebooting...");});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {Serial.printf("OTA in progress: %u%%\r\n", (progress / (total / 100)));}); 
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OK");

  Serial.println("Configuring MQTT server...");
  Serial.printf("   Server IP: %s\r\n",mqtt_server); 
  Serial.printf("   Username:  %s\r\n",mqtt_user);
  Serial.println("   MQTT configured!");

  Serial.println("Setup complete. Running loop...");
  
  command_topic="Home/"+nodeId;
  state_topic="Home/"+nodeId+"/state";
  Serial.println("==================================================");
  Serial.print("MQTT command topic: ");Serial.println(command_topic);
  Serial.print("MQTT state topic: ");Serial.println(state_topic);
  Serial.println("==================================================");
  prevSwitchState=digitalRead(SWITCH);
}

void mqtt_reconnect() 
{
  if(!mqtt_client.connected() && millis()-prevMillisMqttReconnect >= intervalMqttReconnect) 
    {
      Serial.print("Attempting MQTT connection...");   
      mqttReconnectCounter++;
      if (mqtt_client.connect(nodeId.c_str(), mqtt_user, mqtt_password)) 
      {
        Serial.println("connected");
        mqtt_client.subscribe(command_topic.c_str());
        mqtt_client.subscribe("homeassistant/status");
        msg = isLightOn ? "ON":"OFF";
        mqtt_client.publish(state_topic.c_str(), msg.c_str());
        mqttReconnectCounter=0;
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(mqtt_client.state());
        Serial.println(" try again in 5 seconds");
        prevMillisMqttReconnect=millis();
        if(mqttReconnectCounter>=5)
        {
          WiFi.disconnect();
          Serial.println("Connection test failed. Restarting");
          if(isLightOn!=EEPROM.read(addr))
          {
            EEPROM.write(addr, isLightOn);
            EEPROM.commit();
          }
          delay(20);
          ESP.restart();
        }
      }
    }
}

void loop() 
{
  do{ArduinoOTA.handle();}while(isOtaUploading==true);
  mqtt_reconnect();
  mqtt_client.loop();

  if(digitalRead(SWITCH) != prevSwitchState)
  {
    isLightOn = !isLightOn;
    digitalWrite(RELAY, isLightOn);
    msg = isLightOn ? "ON":"OFF";
    mqtt_client.publish(state_topic.c_str(), msg.c_str());
    delay(100);
    prevSwitchState=digitalRead(SWITCH);
  }  
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) 
{
  char* cleanPayload = (char*)malloc(length+1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length+1);
  String msg = String(cleanPayload);
  free(cleanPayload);
  String Topic = topic;
  Serial.println("recived "+msg);
  if(Topic=="homeassistant/status" && msg=="online")
  {
    msg = isLightOn ? "ON":"OFF";
    mqtt_client.publish(state_topic.c_str(), msg.c_str());
  }
  else if(msg=="ON")
  {
    isLightOn=true;
    msg = "ON";
    mqtt_client.publish(state_topic.c_str(), msg.c_str());
    digitalWrite(RELAY, HIGH);
  }
  else if(msg=="OFF")
  {
    isLightOn=false;
    msg = "OFF";
    mqtt_client.publish(state_topic.c_str(), msg.c_str());
    digitalWrite(RELAY, LOW);
  }
}
 
