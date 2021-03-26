//I know the code is a little mess but it's waiting for cleanup ;)
#include "StreamString.h"
#include <modified-WiFiManager.h> 
#include <MQTT.h>   
#include <ArduinoJson.h>  
#include <ArduinoOTA.h> 
#include <FS.h> 

#define mark "IT Freak Home"

void resendAttributes();
void resendConfig();
void pinsSetup();
void mqttTopicsSetup();
void mqttStatusResend();
void reloadConfigSettings();
void saveParamsCallback();
void apStarted(WiFiManager *myWiFiManager);
bool connectMqtt();
bool connectMqttOptions();
void mqtt_message_handle(String topic, String payload);
void mqttMessageReceived(String &topic, String &payload);

static const char HTTP_FIRMWARE[] PROGMEM =
  R"(<!DOCTYPE html>
     <html lang='en'>
     <head>
         <meta charset='utf-8'>
         <meta name='viewport' content='width=device-width,initial-scale=1'/>
     </head>
     <body>
     <h3>Firmware info</h3>
     <hr/>
     <dt><strong>Manufacturer:</strong></dt>
     <dd>{c}</dd>
     <dt><strong>Device:</strong></dt> 
     <dd>{r}</dd>
     <dt><strong>Firmware verison:</strong></dt> 
     <dd>{v}</dd>
     <dt><strong>GitHub project:</strong></dt> 
     <dd><em><a href="{h}" target="_blank">GitHub link</a></em></dd>
     <br/>
     <h3>Upload firmware</h3>
     <hr/>
     <form method='POST' action='' enctype='multipart/form-data'>
         Firmware:<br>
         <input type='file' accept='.bin,.bin.gz' name='firmware'>
         <input type='submit' value='Update firmware'>
     </form>
     </body>
     </html>)";
static const char successResponse[] PROGMEM = 
  "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting...";

StaticJsonDocument<confSize> conf;
String serializedConf;

File confFile;
bool isOtaUploading=false, isOtaStarted=false, isDisconnected=true, needMqttConnect=false, isServerOnline=true, needRestart=false, isApStarted=false, disableMqtt=false;
String nodeID, msg, topic, apName;
unsigned long prevMillisTimeToReset=0, intervalTimeToReset=10000, prevMillisReconnect=0, intervalReconnect=480000, 
lastMqttConnectionAttempt=0, intervalMqttReconnect=5000, prevSetMqttProps=0, intervalSetMqttProps=20000;

WiFiClient net;
MQTTClient mqttClient(700);

char device_user_name[64];
char mqtt_server[48];
char mqtt_user[64];
char mqtt_password[64];
char mqtt_port[5]="1883";
char mqtt_prefix[16]="Home";
char serverStatusTopic[64]="homeassistant/status";
char serverOnlineStatusPayload[64]="online";
char serverOfflineStatusPayload[64]="offline";
boolean isDiscoveryOn=true;
boolean isMqttEnabled=true;
char discoveryPrefix[64]="homeassistant";
boolean isArduinoOtaEnabled=false;

WiFiManager wm;

WiFiManagerParameter custom_device_user_name("device_user_name", "Device name", device_user_name, 63, device_name_placeholder.c_str());
WiFiManagerParameter custom_enable_mqtt("enable_mqtt", "Enable MQTT client", "t", 2, "type=\"checkbox\" checked style=\"width: 20px\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_mqtt_server("server", "MQTT server", mqtt_server, 47, "placeholder=\"IP address or domain\"");
WiFiManagerParameter custom_mqtt_user("user", "MQTT user", mqtt_user, 63, "placeholder=\"Login\"");
WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT password", mqtt_password, 63, "type=password placeholder=\"Password\"");
WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", mqtt_port, 4, "placeholder=\"Default: 1883\"");
WiFiManagerParameter custom_mqtt_prefix("mqtt_prefix", "MQTT device prefix", mqtt_prefix, 15, "placeholder=\"Default: Home\"");
WiFiManagerParameter custom_server_status_topic("server_status_topic", "Server status topic", serverStatusTopic, 63, "placeholder=\"Default: homeassistant/status\"");
WiFiManagerParameter custom_server_online_payload("server_online_status_payload", "Server online payload", serverOnlineStatusPayload, 63, "placeholder=\"Default: online\"");
WiFiManagerParameter custom_server_offline_payload("server_offline_status_payload", "Server offline payload", serverOfflineStatusPayload, 63, "placeholder=\"Default: offline\"");
WiFiManagerParameter custom_enable_auto_discovery("enable_auto_discovery", "Enable MQTT auto discovery", "t", 2, "type=\"checkbox\" checked style=\"width: 20px\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_br("<br/>");
WiFiManagerParameter custom_discovery_prefix("discovery_prefix", "MQTT auto discovery prefix", discoveryPrefix, 63, "placeholder=\"Default: homeassistant\"");
WiFiManagerParameter custom_enable_arduino_ota("enable_arduino_ota", "Enable Arduino IDE OTA", "t", 2, "type=\"checkbox\" style=\"width: 20px\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_js("<script>document.getElementById(\"enable_mqtt\").checked = document.getElementById(\"enable_mqtt\").value==\"t\";document.getElementById(\"enable_auto_discovery\").checked = document.getElementById(\"enable_auto_discovery\").value==\"t\";document.getElementById(\"enable_arduino_ota\").checked = document.getElementById(\"enable_arduino_ota\").value==\"t\";</script>");

bool isCharArrayDigit(char *charArray)
{
  bool isCharArrayDigit=true;
  for(int i=0; i<sizeof(charArray) && isCharArrayDigit; i++)
  {
    isCharArrayDigit=isDigit(charArray[i]);
  }
  return isCharArrayDigit;
}

void setupProcess()
{
    WiFi.mode(WIFI_STA);
    Serial.begin(115200);
    SPIFFS.begin();
    delay(1000);
    if(SPIFFS.exists("/config.txt"))
    {
      SPIFFS.rename("/config.txt", "/config.json"); 
    }
    if(SPIFFS.exists("/config.json"))
    {
      Serial.println("Config exist");
      reloadConfigSettings();
    }
    custom_device_user_name.setValue(device_user_name, 64);
    custom_mqtt_server.setValue(mqtt_server, 48);
    custom_mqtt_user.setValue(mqtt_user, 64);
    //custom_mqtt_password.setValue(mqtt_password, 64);
    custom_mqtt_port.setValue(mqtt_port, 5);
    custom_mqtt_prefix.setValue(mqtt_prefix, 16);
    custom_server_status_topic.setValue(serverStatusTopic, 64);
    custom_server_online_payload.setValue(serverOnlineStatusPayload, 64);
    custom_server_offline_payload.setValue(serverOfflineStatusPayload, 64);
    custom_discovery_prefix.setValue(discoveryPrefix, 64);
    custom_enable_auto_discovery.setValue(isDiscoveryOn?"t":"f",2);
    custom_enable_mqtt.setValue(isMqttEnabled?"t":"f",2);
    custom_enable_arduino_ota.setValue(isArduinoOtaEnabled?"t":"f",2);
    nodeID=String(ESP.getChipId());
    pinsSetup();
    wm.setDeviceName(host_name, device_user_name);
    wm.addParameter(&custom_device_user_name);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_enable_mqtt);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_mqtt_server);
    wm.addParameter(&custom_mqtt_user);
    wm.addParameter(&custom_mqtt_password);
    wm.addParameter(&custom_mqtt_port);
    wm.addParameter(&custom_mqtt_prefix);
    wm.addParameter(&custom_server_status_topic);
    wm.addParameter(&custom_server_online_payload);
    wm.addParameter(&custom_server_offline_payload);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_enable_auto_discovery);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_discovery_prefix);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_enable_arduino_ota);
    wm.addParameter(&custom_js);
    std::vector<const char *> menu = {"control","wifi","param","sep","firmware","restart","info"};
    wm.setMenu(menu);
    wm.setMinimumSignalQuality(10);
    wm.setRemoveDuplicateAPs(true);
    wm.setConfigPortalBlocking(false);
    wm.setClass("invert");
    wm.setWiFiAutoReconnect(true);
    wm.setHostname(host_name.c_str());
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setSaveConfigCallback(saveParamsCallback);
    wm.setAPCallback(apStarted);
    mqttTopicsSetup();
    if(mqtt_server[0]!='\0' && mqtt_port[0]!='\0' && mqtt_prefix[0]!='\0' && isCharArrayDigit(mqtt_port) && isMqttEnabled)
    {
      if((isDiscoveryOn && discoveryPrefix[0]=='\0') || !isDiscoveryOn)
      {
        Serial.println("Disabling MQTT autodicovery");
        isDiscoveryOn=false;
      }
      mqttClient.begin(mqtt_server, atol(mqtt_port), net);
      mqttClient.onMessage(mqttMessageReceived);
      wm.setMqttProps(true, mqttClient.connected(), mqtt_server, mqtt_port);
      prevSetMqttProps=millis();
    }
    else
    {
      Serial.println("Disabling MQTT");
      disableMqtt=true;
      isServerOnline=false;
      wm.setMqttProps(disableMqtt, mqttClient.connected(), mqtt_server, mqtt_port);
      prevSetMqttProps=millis();
    }
    if(!isArduinoOtaEnabled)Serial.println("Disabling Arduino IDE OTA");
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    apName = device_user_name[0]!='\0'?device_user_name:host_name.c_str();
    if(apName=='\0') apName = nodeID;
    if(wm.autoConnect(apName.c_str(), "admin123"))
    {
        Serial.println("connected...yeey :)");
        wm.startWebPortal();
        wm.setup_web_ota();
    }
    else 
    {
        Serial.println("Configportal running");
        wm.setup_web_ota();
    } 
}

void wifiConnected()
{
  if(!disableMqtt)needMqttConnect=true;
  isApStarted=false;
  if(!isOtaStarted && isArduinoOtaEnabled)
  {
  Serial.println("Setting up OTA");
  ArduinoOTA.setHostname(host_name.c_str());  
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
  Serial.println("Done!");
  ArduinoOTA.begin();
  }
  isOtaStarted=true;
}

void wifi_handle()
{
  if(isApStarted && millis()-prevMillisReconnect >= intervalReconnect)
  {
    Serial.println("AP callback reconnect");
    wm.autoConnect(host_name.c_str(), "admin123");
    prevMillisReconnect=millis();
  }
  wm.process();
  if(needRestart && millis()-prevMillisTimeToReset >= intervalTimeToReset)
  {
    ESP.restart();
  }
  if(isDisconnected && WiFi.status()==WL_CONNECTED)
  {
    isDisconnected=false;
    wifiConnected();
  }
  else if(!isDisconnected && WiFi.status()!=WL_CONNECTED)
  {
    isDisconnected=true;
    needMqttConnect=false;
  }
}

void mqtt_handle()
{
  if(!disableMqtt)
  {
    mqttClient.loop();
    delay(5);
    if(millis() - prevSetMqttProps<intervalSetMqttProps && !isDisconnected)
    {
      wm.setMqttProps(disableMqtt, mqttClient.connected(), mqtt_server, mqtt_port);
      prevSetMqttProps=millis();
    }
    if(needMqttConnect)
    {
      if(connectMqtt())
      {
        needMqttConnect = false;
        wm.setMqttProps(disableMqtt, mqttClient.connected(), mqtt_server, mqtt_port);
      }
    }
    else if (WiFi.status()==WL_CONNECTED && !mqttClient.connected())
    {
      Serial.println("MQTT reconnect");
      needMqttConnect=true;
      wm.setMqttProps(disableMqtt, mqttClient.connected(), mqtt_server, mqtt_port);
    }
  }
}

bool connectMqtt() 
{
  unsigned long Now = millis();
  if (millis() - lastMqttConnectionAttempt<intervalMqttReconnect)
  {
    // Do not repeat within 1 sec.
    return false;
  }
  Serial.println("Connecting to MQTT server...");
  if (!connectMqttOptions()) 
  {
    Serial.println("Not connected");
    lastMqttConnectionAttempt = Now;
    if(disableMqtt)Serial.println("MQTT disbled");
    return false;
  }
  else
  {
    Serial.println("Connected!");

    mqttClient.subscribe(command_topic);
    mqttClient.subscribe(serverStatusTopic);
    mqttStatusResend();
    return true;
  }
  
}

bool connectMqttOptions()
{
  bool result;
  if (mqtt_password[0] != '\0' && mqtt_user[0] != '\0' && mqtt_server[0] != '\0')
  {
    Serial.println("Connect option: with password");
    result = mqttClient.connect(nodeID.c_str(), mqtt_user, mqtt_password);
  }
  else if (mqtt_user[0] != '\0' && mqtt_server[0] != '\0')
  {
    Serial.println("Connect option: without password");
    result = mqttClient.connect(nodeID.c_str(), custom_mqtt_user.getValue());
  }
  else if (mqtt_server[0] != '\0')
  {
    Serial.println("Connect option: without password and user name");
    result = mqttClient.connect(nodeID.c_str());
  }
  else
  {
    Serial.println("Connect option: ERROR, invalid data");
    disableMqtt=true;
    result=false;
  }
  Serial.print("Mqtt connection result: ");
  Serial.println(result);
  return result;
}

void resendAttributes()
{
  StaticJsonDocument<128> doc;
  String obj;
  obj="";
  doc["IP_address"] = WiFi.localIP().toString();
  doc["MAC_address"] = WiFi.macAddress();
  doc["ESP_chip_ID"] = nodeID;
  serializeJson(doc, obj);
  Serial.println(attributes_topic);
  Serial.print("Sending attributes: "); Serial.println(obj);
  mqttClient.publish(attributes_topic, obj);
}

void ota_handle()
{
  if(isOtaStarted && isArduinoOtaEnabled){do{ArduinoOTA.handle();}while(isOtaUploading==true);}
}

void WiFiManager::_setUpdaterError()
{
  Update.printError(Serial);
  StreamString str;
  Update.printError(str);
  _updaterError = str.c_str();
}

void WiFiManager::setup_web_ota()
{
  server->on("/firmware", HTTP_GET, [&]()
  {
      String page = getHTTPHead("Firmware"); // @token options @todo replace options with title
      page += FPSTR(HTTP_FIRMWARE);
      if(_showBack) page += FPSTR(HTTP_BACKBTN);
      reportStatus(page);
      reportMqttStatus(page);
      page += FPSTR(HTTP_END);
      page.replace(FPSTR(T_c),mark);
      page.replace(FPSTR(T_r),compatible_device);
      page.replace(FPSTR(T_v),software_version);
      page.replace(FPSTR(T_h),github_project_link);
      server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
      server->send(200, FPSTR(HTTP_HEAD_CT), page);
    });

    // handler for the /update form POST (once file upload finishes)
    server->on("/firmware", HTTP_POST, [&](){
      if (Update.hasError()) {
        server->send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
      } else {
        server->client().setNoDelay(true);
        server->send(200, PSTR("text/html"), successResponse);
        delay(100);
        server->client().stop();
        ESP.restart();
      }
    },[&](){
      // handler for the file upload, get's the sketch bytes, and writes
      // them through the Update object
      HTTPUpload& upload = server->upload();

      if(upload.status == UPLOAD_FILE_START){
        _updaterError.clear();
          Serial.setDebugOutput(true);

        WiFiUDP::stopAll();
          Serial.printf("Update: %s\n", upload.filename.c_str());
          uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if (!Update.begin(maxSketchSpace, U_FLASH)){//start with max available size
            _setUpdaterError();
          }
        
      } else if(upload.status == UPLOAD_FILE_WRITE && !_updaterError.length()){
        Serial.printf(".");
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          _setUpdaterError();
        }
      } else if(upload.status == UPLOAD_FILE_END && !_updaterError.length()){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          _setUpdaterError();
        }
        Serial.setDebugOutput(false);
      } else if(upload.status == UPLOAD_FILE_ABORTED){
        Update.end();
        Serial.println("Update was aborted");
      }
      delay(0);
    });
}

void mqttMessageReceived(String &topic, String &payload)
{
  Serial.println("Incoming: " + topic + " - " + payload);
  if(topic==serverStatusTopic && payload==serverOnlineStatusPayload)
  {
    isServerOnline=true;
    mqttStatusResend();
  }
  else if(topic==serverStatusTopic && payload==serverOfflineStatusPayload)
  {
    isServerOnline=false;
  }
  else if(isServerOnline)
  {
    mqtt_message_handle(topic, payload);
  }
}

void reloadConfigSettings()
{
  Serial.println("Getting config settings");
  confFile = SPIFFS.open("/config.json", "r");
  String readedConf = confFile.readStringUntil('\n');
  Serial.println(readedConf);
  deserializeJson(conf, readedConf);
  String stringBuffer;
  if(conf.containsKey("deviceUserName"))
  {
    stringBuffer = conf["deviceUserName"].as<String>();
    stringBuffer.toCharArray(device_user_name, 64);
  }
  if(conf.containsKey("isMqttEnabled"))
  {
    isMqttEnabled = conf["isMqttEnabled"];
  }
  if(conf.containsKey("mqtt_server"))
  {
    stringBuffer = conf["mqtt_server"].as<String>();
    stringBuffer.toCharArray(mqtt_server, 48);
  }
  if(conf.containsKey("mqtt_user"))
  {
    stringBuffer = conf["mqtt_user"].as<String>();
    stringBuffer.toCharArray(mqtt_user, 64);
  }
  if(conf.containsKey("mqtt_password"))
  {
    stringBuffer = conf["mqtt_password"].as<String>();
    stringBuffer.toCharArray(mqtt_password, 64);
  }
  if(conf.containsKey("mqtt_port"))
  {
    stringBuffer = conf["mqtt_port"].as<String>();
    stringBuffer.toCharArray(mqtt_port, 5);
  }
  if(conf.containsKey("mqtt_prefix"))
  {
    stringBuffer = conf["mqtt_prefix"].as<String>();
    stringBuffer.toCharArray(mqtt_prefix, 16);
  }
  if(conf.containsKey("serverStatusTopic"))
  {
    stringBuffer = conf["serverStatusTopic"].as<String>();
    stringBuffer.toCharArray(serverStatusTopic, 64);
  }
  if(conf.containsKey("serverOnlineStatusPayload"))
  {
    stringBuffer = conf["serverOnlineStatusPayload"].as<String>();
    stringBuffer.toCharArray(serverOnlineStatusPayload, 64);
  }
  if(conf.containsKey("serverOfflineStatusPayload"))
  {
    stringBuffer = conf["serverOfflineStatusPayload"].as<String>();
    stringBuffer.toCharArray(serverOfflineStatusPayload, 64);
  }
  if(conf.containsKey("isAutoDiscoveryOn"))
  {
    isDiscoveryOn = conf["isAutoDiscoveryOn"];
  }
  if(conf.containsKey("discoveryPrefix"))
  {
    stringBuffer = conf["discoveryPrefix"].as<String>();
    stringBuffer.toCharArray(discoveryPrefix, 64);
  }
  if(conf.containsKey("isArduinoOtaEnabled"))
  {
    isArduinoOtaEnabled = conf["isArduinoOtaEnabled"];
  }
  confFile.close();
}

void saveParamsCallback() 
{
  Serial.println("Get Params:");
  Serial.print(custom_device_user_name.getID());
  Serial.print(" : ");
  Serial.println(custom_device_user_name.getValue());
  Serial.print(custom_mqtt_server.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_server.getValue());
  Serial.print(custom_mqtt_user.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_user.getValue());
  Serial.print(custom_mqtt_password.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_password.getValue());
  Serial.print(custom_mqtt_port.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_port.getValue());
  Serial.print(custom_mqtt_prefix.getID());
  Serial.print(" : ");
  Serial.println(custom_mqtt_prefix.getValue());
  Serial.print(custom_server_status_topic.getID());
  Serial.print(" : ");
  Serial.println(custom_server_status_topic.getValue());
  Serial.print(custom_server_online_payload.getID());
  Serial.print(" : ");
  Serial.println(custom_server_online_payload.getValue());
  Serial.print(custom_server_offline_payload.getID());
  Serial.print(" : ");
  Serial.println(custom_server_offline_payload.getValue());
  Serial.print(custom_discovery_prefix.getID());
  Serial.print(" : ");
  Serial.println(custom_discovery_prefix.getValue());

  Serial.println("JSON:");
  conf["deviceUserName"] = custom_device_user_name.getValue();
  Serial.println(conf["deviceUserName"].as<String>());
  if((strncmp(custom_enable_mqtt.getValue(), "f", 1) == 0)==true || (strncmp(custom_enable_mqtt.getValue(), "t", 1) == 0)==true)
  {
    conf["isMqttEnabled"]=true;
  }
  else
  {
    conf["isMqttEnabled"]=false;
  }
  conf["mqtt_server"] = custom_mqtt_server.getValue(); 
  Serial.println(conf["mqtt_server"].as<String>());
  conf["mqtt_user"] = custom_mqtt_user.getValue();
  Serial.println(conf["mqtt_user"].as<String>());
  conf["mqtt_password"] = custom_mqtt_password.getValue();
  Serial.println(conf["mqtt_password"].as<String>());
  conf["mqtt_port"] = custom_mqtt_port.getValue();
  Serial.println(conf["mqtt_port"].as<String>());
  conf["serverStatusTopic"] = custom_server_status_topic.getValue();
  Serial.println(conf["serverStatusTopic"].as<String>());
  conf["serverOnlineStatusPayload"] = custom_server_online_payload.getValue();
  Serial.println(conf["serverOnlineStatusPayload"].as<String>());
  conf["serverOfflineStatusPayload"] = custom_server_offline_payload.getValue();
  Serial.println(conf["serverOfflineStatusPayload"].as<String>());
  if((strncmp(custom_enable_auto_discovery.getValue(), "f", 1) == 0)==true || (strncmp(custom_enable_auto_discovery.getValue(), "t", 1) == 0)==true)
  {
    conf["isAutoDiscoveryOn"]=true;
  }
  else
  {
    conf["isAutoDiscoveryOn"]=false;
  }
  Serial.println(conf["isAutoDiscoveryOn"].as<String>());
  conf["discoveryPrefix"] = custom_discovery_prefix.getValue();
  Serial.println(conf["discoveryPrefix"].as<String>());
  if((strncmp(custom_enable_arduino_ota.getValue(), "f", 1) == 0)==true || (strncmp(custom_enable_arduino_ota.getValue(), "t", 1) == 0)==true)
  {
    conf["isArduinoOtaEnabled"]=true;
  }
  else
  {
    conf["isArduinoOtaEnabled"]=false;
  }
  Serial.println(conf["isArduinoOtaEnabled"].as<String>());

  serializeJson(conf, serializedConf);
  Serial.println("Serialized conf:");
  confFile = SPIFFS.open("/config.json", "w");
  confFile.print(serializedConf);
  Serial.println(serializedConf);
  confFile.close();
  reloadConfigSettings();
  needRestart=true;
  prevMillisTimeToReset=millis();
  intervalTimeToReset=10000;
  Serial.print("Restart in ");
  Serial.print(intervalTimeToReset/1000);
  Serial.println("s");
}

void apStarted(WiFiManager *myWiFiManager)
{
  Serial.println("AP callback");
  isApStarted=true;
  prevMillisReconnect=millis();
}