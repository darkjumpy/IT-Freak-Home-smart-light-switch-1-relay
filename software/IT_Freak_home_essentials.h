//I know the code is a little mess but it's waiting for cleanup ;)
#include "StreamString.h" 
#include <MQTT.h>   
#include <ArduinoJson.h>  
#include <ArduinoOTA.h> 
#include <FS.h> 

#define mark "IT Freak Home"

void addAdditionalParameters();
void setAdditionalParameters();
void reloadAdditionalParameters();
void saveAdditionalParameters();
void resendAttributes();
void resendConfig();
void pinsSetup();
void mqttTopicsSetup();
void mqttStatusResend();
void reloadConfigSettings();
void saveParamsCallback();
void saveConfigCallback();
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
unsigned long prevMillisTimeToReset=0, intervalTimeToReset=10000, prevMillisReconnect=0, intervalReconnect=240000, 
lastMqttConnectionAttempt=0, intervalMqttReconnect=15000, prevSetMqttProps=0, intervalSetMqttProps=20000;

WiFiClient net;
MQTTClient mqttClient(mqttBufferSize);

char device_user_name[64];
char mqtt_server[48];
char mqtt_user[64];
char mqtt_password[64];
char mqtt_client_name[16];
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

String device_name_placeholder="placeholder=\""+host_name+"\"",
mqtt_client_name_placeholder="placeholder=\"Default: IFH-"+String(ESP.getChipId())+"\"";
WiFiManagerParameter custom_device_user_name("device_user_name", "Device name", device_user_name, 63, device_name_placeholder.c_str());
WiFiManagerParameter custom_enable_mqtt("enable_mqtt", "Enable MQTT client", "t", 2, "type=\"checkbox\" checked style=\"width: 20px\" onchange=\"changeMqtt()\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server, 47, "placeholder=\"IP address or domain\"");
WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT user", mqtt_user, 63, "placeholder=\"Login\"");
WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT password", mqtt_password, 63, "type=password placeholder=\"Password\"");
WiFiManagerParameter custom_mqtt_client_name("mqtt_client_name", "MQTT client name", mqtt_client_name, 15, mqtt_client_name_placeholder.c_str());
WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", mqtt_port, 4, "placeholder=\"Default: 1883\"");
WiFiManagerParameter custom_mqtt_prefix("mqtt_prefix", "MQTT device prefix", mqtt_prefix, 15, "placeholder=\"Default: Home\"");
WiFiManagerParameter custom_server_status_topic("server_status_topic", "Server status topic", serverStatusTopic, 63, "placeholder=\"Default: homeassistant/status\"");
WiFiManagerParameter custom_server_online_payload("server_online_status_payload", "Server online payload", serverOnlineStatusPayload, 63, "placeholder=\"Default: online\"");
WiFiManagerParameter custom_server_offline_payload("server_offline_status_payload", "Server offline payload", serverOfflineStatusPayload, 63, "placeholder=\"Default: offline\"");
WiFiManagerParameter custom_enable_auto_discovery("enable_auto_discovery", "Enable MQTT auto discovery", "t", 2, "type=\"checkbox\" checked style=\"width: 20px\" onchange=\"changeDiscovery()\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_br("<br/>");
WiFiManagerParameter custom_hr("<hr/>");
WiFiManagerParameter custom_mqtt_div("<div id=\"mqttSetup\" style=\"padding: 0px\">");
WiFiManagerParameter custom_auto_discovery_div("<div id=\"autoDiscoverySetup\" style=\"padding: 0px\">");
WiFiManagerParameter custom_end_div("</div>");
WiFiManagerParameter custom_discovery_prefix("discovery_prefix", "MQTT auto discovery prefix", discoveryPrefix, 63, "placeholder=\"Default: homeassistant\"");
WiFiManagerParameter custom_enable_arduino_ota("enable_arduino_ota", "Enable Arduino IDE OTA", "f", 2, "type=\"checkbox\" style=\"width: 20px\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_js(R"(<script>
document.getElementById("enable_mqtt").checked = document.getElementById("enable_mqtt").value=="t";
document.getElementById("enable_auto_discovery").checked = document.getElementById("enable_auto_discovery").value=="t";
document.getElementById("enable_arduino_ota").checked = document.getElementById("enable_arduino_ota").value=="t";

changeDiscovery();
changeMqtt();

function changeDiscovery()
{
  const isEnableAutoDiscoveryChecked=document.getElementById("enable_auto_discovery").checked;
  if(isEnableAutoDiscoveryChecked)
  {
    document.getElementById("autoDiscoverySetup").style.display = "initial";
  }
  else
  {
    document.getElementById("autoDiscoverySetup").style.display = "none";
  }
}

function changeMqtt()
{
  const isEnableMqttChecked=document.getElementById("enable_mqtt").checked;
  const isEnableAutoDiscoveryChecked=document.getElementById("enable_auto_discovery").checked;
  if(isEnableMqttChecked)
  {
    document.getElementById("mqttSetup").style.display = "initial";
    if(isEnableAutoDiscoveryChecked){
      document.getElementById("autoDiscoverySetup").style.display = "initial";
    }
  }
  else
  {
    document.getElementById("mqttSetup").style.display = "none";
  }
}
</script>)");

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
    nodeID=String(ESP.getChipId());
    ("IFH-"+nodeID).toCharArray(mqtt_client_name, 16);
    if(SPIFFS.exists("/config.txt"))
    {
      SPIFFS.rename("/config.txt", "/config.json"); 
    }
    if(SPIFFS.exists("/config.json"))
    {
      Serial.println(F("Config exist"));
      reloadConfigSettings();
    }
    custom_device_user_name.setValue(device_user_name, 64);
    setAdditionalParameters();
    custom_mqtt_server.setValue(mqtt_server, 48);
    custom_mqtt_user.setValue(mqtt_user, 64);
    //custom_mqtt_password.setValue(mqtt_password, 64);
    custom_mqtt_client_name.setValue(mqtt_client_name, 16);
    custom_mqtt_port.setValue(mqtt_port, 5);
    custom_mqtt_prefix.setValue(mqtt_prefix, 16);
    custom_server_status_topic.setValue(serverStatusTopic, 64);
    custom_server_online_payload.setValue(serverOnlineStatusPayload, 64);
    custom_server_offline_payload.setValue(serverOfflineStatusPayload, 64);
    custom_discovery_prefix.setValue(discoveryPrefix, 64);
    custom_enable_auto_discovery.setValue(isDiscoveryOn?"t":"f",2);
    custom_enable_mqtt.setValue(isMqttEnabled?"t":"f",2);
    custom_enable_arduino_ota.setValue(isArduinoOtaEnabled?"t":"f",2);
    pinsSetup();
    wm.setDeviceName(host_name, device_user_name);
    wm.addParameter(&custom_device_user_name);
    addAdditionalParameters();
    wm.addParameter(&custom_enable_mqtt);
    wm.addParameter(&custom_mqtt_div);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_mqtt_server);
    wm.addParameter(&custom_mqtt_user);
    wm.addParameter(&custom_mqtt_password);
    wm.addParameter(&custom_mqtt_client_name);
    wm.addParameter(&custom_mqtt_port);
    wm.addParameter(&custom_mqtt_prefix);
    wm.addParameter(&custom_server_status_topic);
    wm.addParameter(&custom_server_online_payload);
    wm.addParameter(&custom_server_offline_payload);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_enable_auto_discovery);
    wm.addParameter(&custom_auto_discovery_div);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_br);
    wm.addParameter(&custom_discovery_prefix);
    wm.addParameter(&custom_end_div);
    wm.addParameter(&custom_end_div);
    wm.addParameter(&custom_hr);
    wm.addParameter(&custom_enable_arduino_ota);
    wm.addParameter(&custom_js);
    std::vector<const char *> menu = {"control","param","wifi","sep","firmware","restart","info"};
    wm.setMenu(menu);
    wm.setMinimumSignalQuality(10);
    wm.setRemoveDuplicateAPs(true);
    wm.setConfigPortalBlocking(false);
    wm.setClass("invert");
    wm.setWiFiAutoReconnect(true);
    wm.setHostname(host_name.c_str());
    wm.setSaveParamsCallback(saveParamsCallback);
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setAPCallback(apStarted);
    mqttTopicsSetup();
    if(mqtt_server[0]!='\0' && mqtt_port[0]!='\0' && mqtt_prefix[0]!='\0' && mqtt_client_name[0]!='\0' && isCharArrayDigit(mqtt_port) && isMqttEnabled)
    {
      if((isDiscoveryOn && discoveryPrefix[0]=='\0') || !isDiscoveryOn)
      {
        Serial.println(F("Disabling MQTT autodicovery"));
        isDiscoveryOn=false;
      }
      mqttClient.begin(mqtt_server, atol(mqtt_port), net);
      mqttClient.onMessage(mqttMessageReceived);
      mqttClient.setKeepAlive(45);
      wm.setMqttProps(true, mqttClient.connected(), mqtt_server, mqtt_port, mqtt_client_name);
      prevSetMqttProps=millis();
    }
    else
    {
      Serial.println(F("Disabling MQTT client"));
      disableMqtt=true;
      isServerOnline=false;
      wm.setMqttProps(disableMqtt, mqttClient.connected(), mqtt_server, mqtt_port, mqtt_client_name);
      prevSetMqttProps=millis();
    }
    if(!isArduinoOtaEnabled)Serial.println(F("Disabling Arduino IDE OTA"));

    apName = device_user_name[0]!='\0'?device_user_name:host_name.c_str();
    if(apName=='\0') apName = nodeID;
    if(wm.autoConnect(apName.c_str(), "admin123"))
    {
        Serial.println(F("connected...yeey :)"));
        wm.startWebPortal();
        wm.setup_web_ota();
    }
    else 
    {
        Serial.println(F("Configportal running"));
        wm.setup_web_ota();
    } 
}

void wifiConnected()
{
  Serial.println(F("WiFi connected"));
  wm.stopConfigPortal();
  isApStarted=false;
  delay(20);
  wm.startWebPortal();
  if(!disableMqtt)needMqttConnect=true;
  if(!isOtaStarted && isArduinoOtaEnabled)
  {
  Serial.println(F("Setting up OTA"));
  ArduinoOTA.setHostname(host_name.c_str());  
  ArduinoOTA.onStart([]() {Serial.println(F("OTA starting..."));isOtaUploading=true;});
  ArduinoOTA.onEnd([]() {Serial.println(F("OTA update finished!"));Serial.println(F("Rebooting..."));});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {Serial.printf("OTA in progress: %u%%\r\n", (progress / (total / 100)));}); 
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
  });  
  Serial.println(F("Done!"));
  ArduinoOTA.begin();
  isOtaStarted=true;
  }
}

void wifi_handle()
{
  if(isApStarted && millis()-prevMillisReconnect >= intervalReconnect)
  {
    Serial.println(F("AP callback reconnect"));
    wm.autoConnect(apName.c_str(), "admin123");
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
    Serial.println(F("WiFi disconncted"));
    isDisconnected=true;
    needMqttConnect=false;
    wm.stopWebPortal();
    delay(20);
    wm.startConfigPortal(apName.c_str(), "admin123");
  }
}

void mqtt_handle()
{
  if(!disableMqtt)
  {
    mqttClient.loop();
    delay(5);
    if(millis() - prevSetMqttProps>=intervalSetMqttProps && !isDisconnected)
    {
      wm.setMqttProps(disableMqtt, mqttClient.connected(), mqtt_server, mqtt_port, mqtt_client_name);
      prevSetMqttProps=millis();
    }
    if(needMqttConnect)
    {
      if(connectMqtt())
      {
        needMqttConnect = false;
        wm.setMqttProps(disableMqtt, mqttClient.connected(), mqtt_server, mqtt_port, mqtt_client_name);
      }
    }
    else if (WiFi.status()==WL_CONNECTED && !mqttClient.connected())
    {
      Serial.println(F("MQTT reconnect"));
      needMqttConnect=true;
      wm.setMqttProps(disableMqtt, mqttClient.connected(), mqtt_server, mqtt_port, mqtt_client_name);
    }
  }
}

bool connectMqtt() 
{
  if (millis() - lastMqttConnectionAttempt<intervalMqttReconnect)
  {
    return false;
  }
  Serial.println(F("Connecting to MQTT server..."));
  if (!connectMqttOptions()) 
  {
    Serial.println(F("Not connected"));
    lastMqttConnectionAttempt = millis();
    if(disableMqtt)Serial.println(F("MQTT client disbled"));
    return false;
  }
  else
  {
    Serial.println(F("Connected!"));

    mqttClient.subscribe(command_topic);
    mqttClient.subscribe(serverStatusTopic);
    mqttStatusResend();
    return true;
  }
  
}

bool connectMqttOptions()
{
  mqttClient.disconnect();
  bool result=false;
  if (mqtt_client_name[0] != '\0' && mqtt_password[0] != '\0' && mqtt_user[0] != '\0' && mqtt_server[0] != '\0')
  {
    Serial.println(F("Connect option: with password"));
    result = mqttClient.connect(mqtt_client_name, mqtt_user, mqtt_password);
  }
  else if (mqtt_client_name[0] != '\0' && mqtt_user[0] != '\0' && mqtt_server[0] != '\0')
  {
    Serial.println(F("Connect option: without password"));
    result = mqttClient.connect(mqtt_client_name, custom_mqtt_user.getValue());
  }
  else if (mqtt_client_name[0] != '\0' && mqtt_server[0] != '\0')
  {
    Serial.println(F("Connect option: without password and user name"));
    result = mqttClient.connect(mqtt_client_name);
  }
  else
  {
    Serial.println(F("Connect option: ERROR, invalid data"));
    Serial.println(F("Disabling MQTT client"));
    disableMqtt=true;
    result=false;
  }
  Serial.print(F("Mqtt connection result: "));
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
  Serial.print(F("Sending attributes: ")); Serial.println(obj);
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
      String page = getHTTPHead("Firmware"); 
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
        if(Update.end(true)){ 
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          _setUpdaterError();
        }
        Serial.setDebugOutput(false);
      } else if(upload.status == UPLOAD_FILE_ABORTED){
        Update.end();
        Serial.println(F("Update was aborted"));
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
  Serial.println(F("Getting config settings"));
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
  reloadAdditionalParameters();
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
  if(conf.containsKey("mqtt_client_name"))
  {
    stringBuffer = conf["mqtt_client_name"].as<String>();
    stringBuffer.toCharArray(mqtt_client_name, 16);
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
  Serial.println(F("JSON:"));
  conf["deviceUserName"] = custom_device_user_name.getValue();
  Serial.println(conf["deviceUserName"].as<String>());
  saveAdditionalParameters();
  if((strncmp(custom_enable_mqtt.getValue(), "f", 1) == 0)==true || (strncmp(custom_enable_mqtt.getValue(), "t", 1) == 0)==true){
    conf["isMqttEnabled"]=true;}
  else{
    conf["isMqttEnabled"]=false;}
  conf["mqtt_server"] = custom_mqtt_server.getValue(); 
  Serial.println(conf["mqtt_server"].as<String>());
  conf["mqtt_user"] = custom_mqtt_user.getValue();
  Serial.println(conf["mqtt_user"].as<String>());
  conf["mqtt_password"] = custom_mqtt_password.getValue();
  Serial.println(conf["mqtt_password"].as<String>());
  conf["mqtt_client_name"] = custom_mqtt_client_name.getValue();
  Serial.println(conf["mqtt_client_name"].as<String>());
  conf["mqtt_port"] = custom_mqtt_port.getValue();
  Serial.println(conf["mqtt_port"].as<String>());
  conf["serverStatusTopic"] = custom_server_status_topic.getValue();
  Serial.println(conf["serverStatusTopic"].as<String>());
  conf["serverOnlineStatusPayload"] = custom_server_online_payload.getValue();
  Serial.println(conf["serverOnlineStatusPayload"].as<String>());
  conf["serverOfflineStatusPayload"] = custom_server_offline_payload.getValue();
  Serial.println(conf["serverOfflineStatusPayload"].as<String>());
  if((strncmp(custom_enable_auto_discovery.getValue(), "f", 1) == 0)==true || (strncmp(custom_enable_auto_discovery.getValue(), "t", 1) == 0)==true){
    conf["isAutoDiscoveryOn"]=true;}
  else{
    conf["isAutoDiscoveryOn"]=false;}
  Serial.println(conf["isAutoDiscoveryOn"].as<String>());
  conf["discoveryPrefix"] = custom_discovery_prefix.getValue();
  Serial.println(conf["discoveryPrefix"].as<String>());
  if((strncmp(custom_enable_arduino_ota.getValue(), "f", 1) == 0)==true || (strncmp(custom_enable_arduino_ota.getValue(), "t", 1) == 0)==true){
    conf["isArduinoOtaEnabled"]=true;}
  else{
    conf["isArduinoOtaEnabled"]=false;}
  Serial.println(conf["isArduinoOtaEnabled"].as<String>());

  serializeJson(conf, serializedConf);
  Serial.println(F("Serialized conf:"));
  confFile = SPIFFS.open("/config.json", "w");
  confFile.print(serializedConf);
  Serial.println(serializedConf);
  confFile.close();
  reloadConfigSettings();
  needRestart=true;
  prevMillisTimeToReset=millis();
  intervalTimeToReset=5000;
  Serial.print(F("Restart in "));
  Serial.print(intervalTimeToReset/1000);
  Serial.println(F("s"));
}

void saveConfigCallback() 
{
  needRestart=true;
  prevMillisTimeToReset=millis();
  intervalTimeToReset=10000;
  Serial.print(F("Restart in "));
  Serial.print(intervalTimeToReset/1000);
  Serial.println(F("s"));
}

void apStarted(WiFiManager *myWiFiManager)
{
  Serial.println(F("AP callback"));
  isApStarted=true;
  prevMillisReconnect=millis();
}