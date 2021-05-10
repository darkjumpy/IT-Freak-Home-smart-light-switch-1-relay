//I know the code is a little mess but it's waiting for cleanup ;)
#include <modified-WiFiManager.h>

#define software_version "0.2.7"
#define compatible_hardware "ESP-12E"
#define compatible_device "Single light switch"
#define github_project_link "https://github.com/IT-freak-Jake/IT-Freak-Home-smart-light-switch-1-relay"
#define confSize 700
#define mqttBufferSize 1024

String host_name="LightSwitch-"+String(ESP.getChipId());

#define light_on_state "ON"
#define light_off_state "OFF"

#define light_on_command "ON"
#define light_off_command "OFF"

#define RELAY 14
#define SWITCH 13

String command_topic, state_topic, attributes_topic, config_topic, icon;
boolean isLightOn=false, prevSwitchState;

boolean isDynamicIconsEnabled=false;
char icon_off[32]="mdi:lightbulb";
char icon_on[32]="mdi:lightbulb-on";

WiFiManagerParameter custom_enable_dynamic_icons("enable_dynamic_icons", "Enable dynamic icons", "f", 2, "type=\"checkbox\" style=\"width: 20px\" onchange=\"changeDynamicIcons()\"", WFM_LABEL_AFTER);
WiFiManagerParameter custom_icon_off("icon_off", "Icon", icon_off, 31, "placeholder=\"mdi:lightbulb\"");
WiFiManagerParameter custom_icon_on("icon_on", "Icon on ON", icon_on, 31, "placeholder=\"mdi:lightbulb-on\"");
WiFiManagerParameter custom_icon_div("<div id=\"icon\" style=\"padding: 0px\">");
WiFiManagerParameter custom_additional_js(R"(<script>
document.getElementById("enable_dynamic_icons").checked = document.getElementById("enable_dynamic_icons").value=="t";
var labels = document.getElementsByTagName('LABEL');
for (var i = 0; i < labels.length; i++) {
  if (labels[i].htmlFor != '') {
     var elem = document.getElementById(labels[i].htmlFor);
     if (elem)
        elem.label = labels[i];         
  }
}
changeDynamicIcons();
function changeDynamicIcons()
{
  const isChecked=document.getElementById("enable_dynamic_icons").checked;
  if(isChecked)
  {
    document.getElementById("icon_off").label.innerHTML = "Icon on OFF";
    document.getElementById("icon").style.display = "initial";
  }
  else
  {
    document.getElementById("icon_off").label.innerHTML = "Icon";
    document.getElementById("icon").style.display = "none";
  }
}
</script>)");

#include "IT_Freak_Home_essentials.h"

void WiFiManager::handleControl() {
  DEBUG_WM(DEBUG_VERBOSE,F("<- HTTP Control"));
  if (captivePortal()) return;
  handleRequest();

  if (server->hasArg("action"))
  {
    String action = server->arg("action");
    if (action=="on")
    {
      isLightOn=true;
      if(mqttClient.connected())
      {
        msg = light_on_state;
        mqttClient.publish(state_topic, msg);
        if(isDynamicIconsEnabled && isDiscoveryOn)resendConfig();
      }
      digitalWrite(RELAY, HIGH);
    }
    else if (action=="off")
    {
      isLightOn=false;
      if(mqttClient.connected())
      {
        msg = light_off_state;
        mqttClient.publish(state_topic, msg);
        if(isDynamicIconsEnabled && isDiscoveryOn)resendConfig();
      }
      digitalWrite(RELAY, LOW);
    }
  }
  
  String page = getHTTPHead("Control");
  page += "<h1>";
  page += (device_user_name[0]!='\0'?device_user_name:host_name.c_str());
  page += "</h1>";
  page += "Switch is ";
  page += (isLightOn?"ON":"OFF");
  page += "<br/><br/>";
  page += "<button ";
  page += isLightOn?"style=\"height: 70px;line-height: 1rem;\" class=\"D\" onclick=\"location.href='?action=off';\"":"style=\"background-color: #44bb44;height: 70px;line-height: 1rem;\" onclick=\"location.href='?action=on';\"";
  page += "><h3>Turn ";
  page += (isLightOn?"OFF":"ON");
  page += "</h3></button><br/><br/>";
  page += "<hr><br/>";
  page += "<button style=\"width:47%;\" onclick=\"location.href='/';\" >Back</button>";
  page += "<button style=\"width:47%; margin-left: 6%\" onclick=\"location.href='?';\" >Refresh</button>";
  reportStatus(page);
  reportMqttStatus(page);
  page += FPSTR(HTTP_END);

  server->sendHeader(FPSTR(HTTP_HEAD_CL), String(page.length()));
  server->send(200, FPSTR(HTTP_HEAD_CT), page);
  if(_preloadwifiscan) WiFi_scanNetworks(_scancachetime,true); 
}

void addAdditionalParameters()
{
  wm.addParameter(&custom_br);
  wm.addParameter(&custom_enable_dynamic_icons);
  wm.addParameter(&custom_br);
  wm.addParameter(&custom_br);
  wm.addParameter(&custom_icon_off);
  wm.addParameter(&custom_icon_div);
  wm.addParameter(&custom_icon_on);
  wm.addParameter(&custom_end_div);
  wm.addParameter(&custom_additional_js);
  wm.addParameter(&custom_hr);
}

void setAdditionalParameters()
{
  custom_enable_dynamic_icons.setValue(isDynamicIconsEnabled?"t":"f",2);
  custom_icon_off.setValue(icon_off, 32);
  custom_icon_on.setValue(icon_on, 32);
  String stringBuffer;
  if(icon_off[0]=='\0'){
    stringBuffer="mdi:lightbulb";
    stringBuffer.toCharArray(icon_off, 32);
  }
  if(icon_on[0]=='\0'){
    stringBuffer="mdi:lightbulb-on";
    stringBuffer.toCharArray(icon_on, 32);
  }
  icon=icon_off;
}

void reloadAdditionalParameters()
{
  String stringBuffer;
  if(conf.containsKey("isDynamicIconsEnabled"))isDynamicIconsEnabled = conf["isDynamicIconsEnabled"];
  if(conf.containsKey("icon_off"))
  {
    stringBuffer = conf["icon_off"].as<String>();
    stringBuffer.toCharArray(icon_off, 32);
  }
  if(conf.containsKey("icon_on"))
  {
    stringBuffer = conf["icon_on"].as<String>();
    stringBuffer.toCharArray(icon_on, 32);
  }
}

void saveAdditionalParameters()
{
  if((strncmp(custom_enable_dynamic_icons.getValue(), "f", 1) == 0)==true || (strncmp(custom_enable_dynamic_icons.getValue(), "t", 1) == 0)==true){
    conf["isDynamicIconsEnabled"]=true;}
  else{
    conf["isDynamicIconsEnabled"]=false;}
  conf["icon_off"] = custom_icon_off.getValue();
  Serial.println(conf["icon_off"].as<String>());
  conf["icon_on"] = custom_icon_on.getValue();
  Serial.println(conf["icon_on"].as<String>());
}

void pinsSetup()
{
  Serial.println("Setting up pins");
  pinMode(RELAY, OUTPUT); 
  pinMode(SWITCH, INPUT_PULLUP);
  digitalWrite(RELAY, LOW);
  prevSwitchState=digitalRead(SWITCH);
  Serial.println("Done!");
}

void mqttTopicsSetup()
{
  Serial.println("Setting up MQTT topics");
  command_topic=String(mqtt_prefix)+"/"+nodeID;
  state_topic=String(mqtt_prefix)+"/"+nodeID+"/state";
  attributes_topic=String(mqtt_prefix)+"/"+nodeID+"/attributes";
  config_topic=String(discoveryPrefix)+"/switch/"+host_name+"/switch1/config";
  Serial.println("Done!");
}

void mqttStatusResend()
{
  Serial.println("Resending all MQTT messages");
  if(isDiscoveryOn)
  {
    resendConfig();
    delay(20);
  }
  else
  {
    Serial.println("Auto discovery disabled");
  }
  msg = isLightOn ? light_on_state:light_off_state;
  mqttClient.publish(state_topic, msg);
  resendAttributes();
}

void switch_handle()
{
  if(digitalRead(SWITCH) != prevSwitchState)
  {
    isLightOn = !isLightOn;
    digitalWrite(RELAY, isLightOn);
    msg = isLightOn ? light_on_state:light_off_state;
    Serial.print("Light is ");  
    Serial.println(msg);  
    if(mqttClient.connected())mqttClient.publish(state_topic, msg);
    if(isDynamicIconsEnabled && isDiscoveryOn && mqttClient.connected())resendConfig();
    delay(20);
    prevSwitchState=digitalRead(SWITCH);
  }  
}

void resendConfig()
{
  Serial.println("Sending auto discovery config");
  if(isDynamicIconsEnabled)icon=isLightOn?icon_on:icon_off;
  StaticJsonDocument<570> doc;
  StaticJsonDocument<128> doc2;
  String obj;
  obj="";
  doc2["identifiers"] = host_name;
  doc2["name"] = (device_user_name[0]!='\0'?device_user_name:host_name.c_str());
  doc2["manufacturer"] = mark;
  doc2["model"] = compatible_device;
  doc2["sw_version"] = software_version;

  doc["device"] = doc2;
  doc["name"] = (device_user_name[0]!='\0'?device_user_name:host_name.c_str());
  doc["unique_id"] = host_name;
  doc["command_topic"] = command_topic;
  doc["state_topic"] = state_topic;
  doc["json_attributes_topic"] = attributes_topic;
  doc["retain"] = false;
  doc["payload_on"] = light_on_command;
  doc["payload_off"] = light_off_command;
  doc["state_on"] = light_on_state;
  doc["state_off"] = light_off_state;
  doc["icon"] = icon;
  serializeJson(doc, obj);
  Serial.println(config_topic);
  Serial.print("Sending auto discovery conf: "); Serial.println(obj);
  mqttClient.publish(config_topic, obj);
}

void setup() 
{
  setupProcess();
}

void loop() 
{
    wifi_handle();
    mqtt_handle();
    ota_handle();
    switch_handle();
}

void mqtt_message_handle(String topic, String payload)
{
  if(payload==light_on_command)
  {
    isLightOn=true;
    msg = light_on_state;
    mqttClient.publish(state_topic, msg);
    digitalWrite(RELAY, HIGH);
    if(isDynamicIconsEnabled && isDiscoveryOn)resendConfig();
  }
  else if(payload==light_off_command)
  {
    isLightOn=false;
    msg = light_off_state;
    mqttClient.publish(state_topic, msg);
    digitalWrite(RELAY, LOW);
    if(isDynamicIconsEnabled && isDiscoveryOn)resendConfig();
  }
}
