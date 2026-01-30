// #include <ESP8266WiFi.h>
#include <WiFi.h>
// #include "BluetoothSerial.h"
#include "mqtt.h"

// WiFi
const char *ssid     = "Lammbock";          // Enter your WiFi name
const char *password = "jingundmatthias1";  // Enter WiFi password

// MQTT 
const char *mqtt_broker   = "192.168.178.27";
const int   mqtt_port     = 1883;
const char *mqtt_username = "hotpot";
const char *mqtt_password = "hotpot";

// OTA
const char *ota_password = "maxiaolong";

// Hotpot Info
const char *manufacturer = "Kirami";
const char *model        = "Comfort Steady M";
const char *model_id     = "Nightblack";

// Homeassistant
const char *device_name = "Hotpot";

// NTP
const char* ntpServer = "pool.ntp.org";
