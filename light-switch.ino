#include "painlessMesh.h"
#include <EEPROM.h>

#define   MESH_PREFIX     "YourMeshNetworkSSID" 
#define   MESH_PASSWORD   "YourMeshNetworkPassword"
#define   MESH_PORT       5555
//every node have to have this same SSID, password and port

#define RELAY 14 //It's pin D5 for wemos D1 mini
#define SWITCH 13 //It's pin D7 for wemos D1 mini

painlessMesh  mesh;

int addr = 0;
bool isLightOn=false, isGrootConnected=false, prevSwitchState, isConnectionTesting=false, isTestPassed=true;
unsigned long prevMillisConnectionTest=0, intervalConnectionTest=300000, prevMillisConnectionTestWait=0, intervalConnectionTestWait=2000; 
String nodeId, msg, topic, RootAskingMessage="whoIsGroot", RootReciveMessage="imGroot";
uint32_t GrootId;

void sendMessage();

void receivedCallback( uint32_t from, String &msg ) 
{
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  if(isGrootConnected==true)
  {
    if(msg=="ON")
    {
      isLightOn=true;
      sendMessageAndTopic("ON", topic);
      digitalWrite(RELAY, HIGH);
    }
    else if(msg=="OFF")
    {
      isLightOn=false;
      sendMessageAndTopic("OFF", topic);
      digitalWrite(RELAY, LOW);
    }
  }
  if(msg=="imGroot")
  {
    GrootId=from;
    Serial.print("Groot ID: ");
    Serial.println(GrootId);
    sendStartupMessage();
  }
  else if(isGrootConnected==false)
  {
    askAboutGroot();
  }
  if(msg=="pong")
  {
    isTestPassed=true;
  }
}

void newConnectionCallback(uint32_t nodeId) 
{
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
    askAboutGroot();
}

void changedConnectionCallback() 
{
    Serial.printf("Changed connections\n");
    askAboutGroot();
}

void nodeTimeAdjustedCallback(int32_t offset) 
{
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() 
{  
  EEPROM.begin(512);
  isLightOn=EEPROM.read(addr);
  digitalWrite(RELAY, isLightOn);
  pinMode(RELAY, OUTPUT);
  pinMode(SWITCH, INPUT_PULLUP);
  EEPROM.write(addr, false);
  EEPROM.commit(); 
  
  Serial.begin(115200);

//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  mesh.initOTAReceive("LightSwitch");
  nodeId=String(mesh.getNodeId());
  topic="Home/"+nodeId+"/state";
  prevSwitchState=digitalRead(SWITCH);
  prevMillisConnectionTest=millis();
  
  Serial.print("My state topic: ");Serial.println(topic);
  Serial.print("My command topic: ");Serial.print("Home/");Serial.println(nodeId);
}

void loop() 
{
 mesh.update();
 connectionTest();
 
 if(digitalRead(SWITCH) != prevSwitchState)
 {
  isLightOn = !isLightOn;
  digitalWrite(RELAY, isLightOn);
  sendMessageAndTopic(isLightOn ? "ON" : "OFF", topic);
  prevSwitchState=digitalRead(SWITCH);
 }
}

void sendMessageAndTopic(String msgToSend, String topicToSend)
{
  if(mesh.isConnected(GrootId))
  {
    Serial.println("Sending: "+topicToSend+" "+msgToSend+" To: "+GrootId);
    String packedMessage = topicToSend+","+msgToSend;
    mesh.sendSingle(GrootId, packedMessage);
  }
  else
  {
    Serial.println("Message sending ERROR: Root not found");
    isGrootConnected=false;
    askAboutGroot();
  }
}

void sendStartupMessage()
{
  if(mesh.isConnected(GrootId))
  {
    mesh.sendSingle(GrootId, isLightOn ? topic+",ON" : topic+",OFF");
    isGrootConnected=true;
  }
  else
  {
    isGrootConnected=false;
    askAboutGroot();
  }
}

void askAboutGroot()
{
  if(isGrootConnected && mesh.isConnected(GrootId))
  {
    Serial.println("Groot already connected");
  }
  else
  {
    Serial.println("Groot asking");
    isGrootConnected=false;
    mesh.sendBroadcast(RootAskingMessage);
  }
}

void connectionTest()
{
  if(millis()-prevMillisConnectionTest >= intervalConnectionTest)
  {
    Serial.println("Connection test start");
    isTestPassed=false;
    isConnectionTesting=true;
    if(isGrootConnected && mesh.isConnected(GrootId))
    {
      mesh.sendSingle(GrootId, "ping");
    }
    else
    {
      askAboutGroot();
      delay(500);
      if(isGrootConnected && mesh.isConnected(GrootId))
      {
        mesh.sendSingle(GrootId, "ping");
      }
      else
      {
        Serial.println("Connection test failed. Restarting");
        EEPROM.write(addr, isLightOn);
        EEPROM.commit();
        delay(1000);
        ESP.restart();
      }
    }
    prevMillisConnectionTestWait=millis();
    prevMillisConnectionTest=millis();
  }
  if(millis()-prevMillisConnectionTestWait >= intervalConnectionTestWait && isTestPassed==false && isConnectionTesting==true)
  {
    Serial.println("Connection test failed. Restarting");
    EEPROM.write(addr, isLightOn);
    EEPROM.commit(); 
    delay(1000);
    ESP.restart();
  }
  else if(millis()-prevMillisConnectionTestWait >= intervalConnectionTestWait && isTestPassed==true && isConnectionTesting==true)
  {
    Serial.println("Ping was ponged by Groot. Connection test passed");
    isConnectionTesting=false;
  }
}
