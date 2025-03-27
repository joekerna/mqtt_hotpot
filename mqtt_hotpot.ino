//#include <ESP8266WiFi.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "mqtt_hotpot.h"
#include "temperatures.h"
#include "filter.h"


// Wifi Client
WiFiClient espClient;

// MQTT Client
PubSubClient mqtt_client(espClient);

// MQTT Topics
char mqtt_message[512];
char topic[60];

// Bluetooth
// #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
// #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
// #endif

// BluetoothSerial SerialBT;

unsigned long readTimestamp = millis();

void onWifiEvent(WiFiEvent_t event) {
	switch (event) {
		case WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED:
			Serial.println("Connected or reconnected to WiFi");
			break;
		case WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
			Serial.println("WiFi Disconnected. Enabling WiFi autoconnect");
			WiFi.setAutoReconnect(true);
			break;
		default: break;
  }
}

void setup(void) {
  // Connect to serial interface
  Serial.begin(9600);
  Serial.println("Starting...");
  // SerialBT.begin("Hotpot");

// -------------------------------------------------------------------------
  // Connecting to a Wi-Fi network
  WiFi.setHostname(device_name);
  connectWifi();
  WiFi.onEvent(onWifiEvent);
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
// OTA Updates
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else  // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    // Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
         if (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)     Serial.println("End Failed");
  });
  ArduinoOTA.setHostname("Hotpot_OTA");
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.begin();
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
  // Connect to MQTT broker
  connectMQTT();
// -------------------------------------------------------------------------

  createHomeAssistantSensor();

  initTemperatureSensors();

  initFilter();
  
  Serial.println("Setup finished");
}

void loop(void) {
  ArduinoOTA.handle();
  mqtt_client.loop();

  // Check WIFI connection
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wifi connection lost ...");
    connectWifi();
  }
  // Check MQTT connection
  connectMQTT();


// -------------------------------------------------------------
// TEMPERATURES
// -------------------------------------------------------------
  if ((millis() - readTimestamp) > (temperatures.update_rate*1000))
  {
    // Serial.println("Loop ...");
     // Say you're still there
     mqtt_set_availability(true);


     // Filter control
     filterControl();

     // Store timestamp
     readTimestamp = millis();


     // Get temperature updates
     updateTemperaturesFromSensor();


     // Update Temperatures on MQTT
     updateTemperaturesToMQTT();
  }
} 

void connectWifi()
{
  // Serial.print("WiFi...");
  int retryCounter = 0;
  WiFi.begin((char *)ssid, password);
  while ((WiFi.status() != WL_CONNECTED) & (retryCounter <= 20))
  {
    // Serial.print(".");
    Serial.println(retryCounter);
    delay(500);
    retryCounter++;
  } 
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wifi Connect successful...");

    // Serial.println("...connected");
    // Update time
    configTime(0, 0, ntpServer);
    setTimezone("CET-1CEST,M3.5.0,M10.5.0/3");
  }
  else
  {
    Serial.println("Wifi Connect failed...");
  }
}


void createNewSensor(const char *name,
                     const char *device_class, 
                     const char *state_topic, 
                     const char *unique_id, 
                     const char *identifiers, 
                     const char *device_name, 
                     const char *manufacturer, 
                     const char *model, 
                     const char *model_id,
                     const char *sensor_type,
                     const char *value_template)
{
  // Create MQTT payload
  if (sensor_type == "sensor")
  {
     sprintf(mqtt_message, 
             "{\"name\": \"%s\", \"device_class\": \"%s\", \"state_topic\": \"%s\", \"unit_of_measurement\": \"°C\", \"value_template\": \"%s\", \"unique_id\": \"%s\", \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\", \"manufacturer\": \"%s\", \"model\": \"%s\", \"model_id\": \"%s\"} }",\
             name, device_class, state_topic, value_template, unique_id, identifiers, device_name, manufacturer, model, model_id);
  } else {
     sprintf(mqtt_message, 
             "{\"name\": \"%s\", \"device_class\": \"%s\", \"state_topic\": \"%s\", \"unique_id\": \"%s\", \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\", \"manufacturer\": \"%s\", \"model\": \"%s\", \"model_id\": \"%s\"} }",\
             name, device_class, state_topic, unique_id, identifiers, device_name, manufacturer, model, model_id);
  }
  // Create MQTT topic
  sprintf(topic, "%s/%s/%s/config", topic_prefix, sensor_type, unique_id);

  // Publish
  mqtt_client.publish(topic, mqtt_message, true);
}

void updateBinarysensor(const char* topic, bool state)
{
        if (state) {
          mqtt_client.publish(topic, "ON", true);
        } else {
          mqtt_client.publish(topic, "OFF", true);
        }
}


