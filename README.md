# ESP8266 Wireless Sensor Network

This project implements a dynamic wireless sensor network using ESP8266 microcontrollers. The network features automatic parent election, data collection, and Firebase integration for remote monitoring.

## Features

- Dynamic network structure with automatic parent election
- Wireless communication using UDP
- Sensor data collection from child nodes
- Firebase integration for remote monitoring
- NTP time synchronization for accurate timestamps

## Hardware Requirements

- ESP8266 microcontrollers
- Sensors (specific to your application)

## Software Dependencies

- ESP8266WiFi
- ESP8266HTTPClient
- ArduinoJson
- ESPAsyncUDP
- FirebaseESP8266
- NTPClient

## Configuration

Before uploading the code, configure the following parameters:

```cpp
#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define NODE_ID 3  // Change this for each node (1, 2, 3)
```

## Network Operation

1. **Initialization**: Nodes connect to WiFi and start in the DISCOVERING state
2. **Parent Election**: Nodes elect a parent based on a random number algorithm
3. **Data Collection**: Child nodes send sensor data to the parent node
4. **Firebase Updates**: The parent node periodically uploads network status and sensor data
5. **Re-election**: The network periodically re-elects a parent to balance energy consumption

## Key Functions

- `setup()`: Initializes WiFi, Firebase, and UDP listener
- `loop()`: Main program loop handling node behavior
- `declareParent()`: Broadcasts parent status
- `participateNext()`: Joins next parent election
- `handleUDPPacket()`: Processes incoming UDP messages
- `electNext()`: Elects the next parent node
- `updateFirebase()`: Uploads data to Firebase
- `sendDataToParent()`: Sends sensor data from child to parent

## Customization

- Adjust `ELECTION_TIME` and `PARENT_TIMEOUT` for network dynamics
- Modify `sendDataToParent()` to include actual sensor readings
- Customize Firebase data structure in `updateFirebase()`

## Troubleshooting

- Verify WiFi credentials and Firebase configuration
- Use Serial Monitor for debugging
- Ensure nodes are within WiFi range of each other
