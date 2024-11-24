#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncUDP.h>
#include "FirebaseESP8266.h"

#include <map>
#include <vector>
#include <string>  // For std::string
#include <cstdlib> // For std::stoi
using namespace std;

#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// WiFi credentials
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// Node-specific configuration
#define NODE_ID 3  // Change this for each node (1, 2, 3)
#define START_DELAY NODE_ID * 10000  // Node-specific start delay (10s, 20s, 30s)

#define ELECTION_TIME 45000  // 120 seconds
#define PARENT_TIMEOUT 15000 
const int FIREBASE_UPDATE_INTERVAL = 10000;

const int DISCOVERY_PORT = 8266;

String currentState;
String prevState;
String nodeId;
int myRandomNumber;
unsigned long startTime;
AsyncUDP udp;
unsigned long lastFirebaseUpdate = 0;
unsigned long lastDataSent = 0;
unsigned long lastSeenParent;

struct SensorData {
  int value;
  unsigned long timestamp;
};

std::map<std::string, int> nodeNumMapping;
std::map<String, SensorData> nodeData;

const unsigned long PACKET_THROTTLE_TIME = 100; // Minimum time between packet processing in ms
unsigned long lastPacketTime = 0;
const int MAX_PACKETS_PER_SECOND = 20; // Limit packet processing rate
int packetCount = 0;
unsigned long lastPacketCountReset = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  randomSeed(analogRead(0) + millis()); // Better random seed
  
  // Generate unique node ID using MAC address
  nodeId = WiFi.macAddress();
  Serial.println("Node ID: " + nodeId);
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println("IP Address: " + WiFi.localIP().toString());
  
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  
  Serial.println("Start state: " + currentState);
  
  int udpRetries = 0;
  while (!udp.listen(DISCOVERY_PORT) && udpRetries < 3) {
    Serial.println("Failed to start UDP listener, retrying...");
    delay(1000);
    udpRetries++;
  }
  
  if (udp.listen(DISCOVERY_PORT)) {
    Serial.println("UDP Listening on port " + String(DISCOVERY_PORT));
    udp.onPacket([](AsyncUDPPacket packet) {
      handleUDPPacket(packet);
    });
  } else {
    Serial.println("Failed to start UDP listener - restarting");
    delay(1000);
    ESP.restart();
  }

  restart();
}

void restart() {
  nodeNumMapping.clear();
  currentState = "DISCOVERING";
  prevState = "DISCOVERING";
  delay(START_DELAY);
  lastSeenParent = millis();
  if(currentState == "DISCOVERING") {
    currentState = "PARENT";
    startTime = millis();
  } else {
    currentState = "CHILD";
  }
  if(prevState != currentState) {
    myRandomNumber = random(1, 1000);
  }
}

void loop() {
  // put your main code here, to run repeatedly
  Serial.println("Current state: " + currentState);
  prevState = currentState;
  if (currentState == "PARENT") {
    if ((millis() - startTime) >= ELECTION_TIME) {
      Serial.println("Starting new election");
      electNext();
    } else {
      updateFirebase();
      declareParent();
    }
  }
  if (currentState == "CHILD") {
    if((millis() - lastSeenParent) >= PARENT_TIMEOUT) {
      restart();
    }
    participateNext();
    sendDataToParent();
  }
  if(prevState != currentState) {
    myRandomNumber = random(1, 1000);
  }
  delay(1000);
}

void declareParent() {
  StaticJsonDocument<200> doc;
  doc["type"] = "declaration";
  doc["nodeId"] = nodeId;
  doc["num"] = myRandomNumber;
  doc["role"] = "parent";
  doc["parent"] = nodeId;
  
  String message;
  serializeJson(doc, message);
  udp.broadcast(message.c_str());
}

void participateNext() {
  StaticJsonDocument<200> doc;
  doc["type"] = "declaration";
  doc["nodeId"] = nodeId;
  doc["num"] = myRandomNumber;
  doc["role"] = "child";
  doc["parent"] = "";

  String message;
  serializeJson(doc, message);
  udp.broadcast(message.c_str());
}

void handleUDPPacket(AsyncUDPPacket packet) {
  unsigned long currentTime = millis();
  
  // Reset packet counter every second
  if (currentTime - lastPacketCountReset >= 1000) {
    packetCount = 0;
    lastPacketCountReset = currentTime;
  }
  
  // Check if we're processing packets too quickly
  if (currentTime - lastPacketTime < PACKET_THROTTLE_TIME) {
    return; // Skip this packet
  }
  
  // Check if we've processed too many packets this second
  if (packetCount >= MAX_PACKETS_PER_SECOND) {
    return; // Skip this packet
  }
  
  // Update packet tracking
  lastPacketTime = currentTime;
  packetCount++;
  
  // Create a local copy of the packet data
  String message;
  if (packet.length() > 200) { // Sanity check on packet size
    Serial.println("Packet too large");
    return;
  }
  
  message.reserve(packet.length() + 1);
  message = String((char*)packet.data());
  
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.println("Failed to parse UDP message");
    return;
  }
  
  String type = doc["type"];
  String senderId = doc["nodeId"];

  if (type == "data" && currentState == "PARENT") {
    int value = doc["value"];
    SensorData sensorData = {
      value,
      millis()
    };
    nodeData[senderId] = sensorData;
    Serial.println("Received data from node " + senderId + ": " + String(value));
    return;
  }

  String senderNum = doc["num"];
  String senderRole = doc["role"];
  String parentId = doc["parent"];
  
  if (senderId != nodeId) {
    std::string senderIdStd(senderId.c_str());
    nodeNumMapping[senderIdStd] = std::stoi(senderNum.c_str());
  }  

  if (type == "declaration") {
    if (senderRole == "parent") {
      lastSeenParent = millis();
      currentState = "CHILD";
    } else if(parentId == nodeId) {
      currentState = "PARENT";
      startTime = millis();
      Serial.print("Becoming new parent");
    }
  }
}

void electNext() {
  std::string maxSenderId;
  int maxSenderNum = -1;

  for (const auto& entry : nodeNumMapping) {
    if (entry.second > maxSenderNum) {
        maxSenderNum = entry.second;
        maxSenderId = entry.first;
    }
  }

  nodeNumMapping.clear();

  Serial.print("Informing new parent: ");
  Serial.print(maxSenderId.c_str());
  Serial.println();

  StaticJsonDocument<200> doc;
  doc["type"] = "declaration";
  doc["nodeId"] = nodeId;
  doc["num"] = myRandomNumber;
  doc["role"] = "child";
  doc["parent"] = maxSenderId;
  
  String message;
  serializeJson(doc, message);
  udp.broadcast(message.c_str());
}

void updateFirebase() {
  if ((millis() - lastFirebaseUpdate) >= FIREBASE_UPDATE_INTERVAL) {
    String networkPath = "network_status/";
    Firebase.setString(firebaseData, networkPath + "current_parent", nodeId);
    Firebase.setInt(firebaseData, networkPath + "active_nodes", nodeNumMapping.size() + 1);
    Firebase.setInt(firebaseData, networkPath + "last_update", millis());

    for (const auto& pair : nodeData) {
      String nodePath = "sensor_data/" + pair.first + "/" + pair.second.timestamp + "/"; 
      Firebase.setInt(firebaseData, nodePath + "value", pair.second.value);
    }
    
    // Also store our own sensor data
    String ownNodePath = "sensor_data/" + nodeId + "/" + millis() + "/";
    int ownValue = random(100); // Replace with actual sensor reading
    Firebase.setInt(firebaseData, ownNodePath + "value", ownValue);
    
    lastFirebaseUpdate = millis();
    nodeData.clear();
    Serial.println("Firebase update complete");
  }
}

void sendDataToParent() {
  if (millis() - lastDataSent >= 5000) { // Send data every 5 seconds
    StaticJsonDocument<200> data;
    data["type"] = "data";
    data["nodeId"] = nodeId;
    data["value"] = random(100); // Replace with actual sensor reading
    
    String message;
    serializeJson(data, message);
    udp.broadcast(message.c_str());
    lastDataSent = millis();
  }
}