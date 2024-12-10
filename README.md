# ESP8266 Wireless Sensor Network

This project implements a dynamic wireless sensor network using ESP8266 microcontrollers. The network features automatic parent election, data collection, and Firebase integration for remote monitoring.

---

## Features

- **Dynamic network structure** with automatic parent election
- **Wireless communication** using UDP
- Sensor data collection from child nodes
- **Firebase integration** for remote monitoring
- **NTP time synchronization** for accurate timestamps

---

## Hardware Requirements

- ESP8266 microcontrollers
- Sensors (specific to your application)

---

## Software Dependencies

The following libraries are required for this project:

- `ESP8266WiFi`
- `ESP8266HTTPClient`
- `ArduinoJson`
- `ESPAsyncUDP`
- `FirebaseESP8266`
- `NTPClient`

---

## Configuration

Before uploading the code, configure the following parameters in the code:

```cpp
#define FIREBASE_HOST "your-project.firebaseio.com"  // Your Firebase project URL
#define FIREBASE_AUTH "your-firebase-auth-token"    // Your Firebase authentication token
#define WIFI_SSID "your-wifi-ssid"                  // Your WiFi SSID
#define WIFI_PASSWORD "your-wifi-password"          // Your WiFi password
#define NODE_ID 1                                   // Unique ID for each node (1, 2, 3, etc.)
