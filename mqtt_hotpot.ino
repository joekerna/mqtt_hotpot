<<<<<<< HEAD
=======
//#include <ESP8266WiFi.h>
#include <WiFi.h>
>>>>>>> refs/remotes/origin/main
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "mqtt_hotpot.h"
#include "temperatures.h"
#include "filter.h"

<<<<<<< HEAD
=======

>>>>>>> refs/remotes/origin/main

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

<<<<<<< HEAD
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
=======
unsigned long readTimestamp = millis();
>>>>>>> refs/remotes/origin/main

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
<<<<<<< HEAD
  connectMQTT();
=======
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqtt_callback);
  while (!mqtt_client.connected()) {
     connectMQTT();
  }
>>>>>>> refs/remotes/origin/main
// -------------------------------------------------------------------------

  createHomeAssistantSensor();

  initTemperatureSensors();

  initFilter();
<<<<<<< HEAD
  
  Serial.println("Setup finished");
=======
>>>>>>> refs/remotes/origin/main
}

void loop(void) {
  ArduinoOTA.handle();
  mqtt_client.loop();

  // Check WIFI connection
  if (WiFi.status() != WL_CONNECTED)
  {
<<<<<<< HEAD
    Serial.println("Wifi connection lost ...");
    connectWifi();
=======
    Serial.println("WiFi connection lost");
    // if disconnected, reconnect
    WiFi.reconnect();
    delay(1000);
    Serial.println("WiFi reconnected");
    mqtt_client.publish("hotpot/debug", "WiFi reconnected");
  }
  // Check MQTT connection
  if (!mqtt_client.connected())
  {
      //reconnect();
      connectMQTT();
>>>>>>> refs/remotes/origin/main
  }
  // Check MQTT connection
  connectMQTT();


// -------------------------------------------------------------
// TEMPERATURES
// -------------------------------------------------------------
  if ((millis() - readTimestamp) > (UPDATE_RATE_SECONDS*1000))
  {
<<<<<<< HEAD
    // Serial.println("Loop ...");

=======
    // Check MQTT connection
     if (!mqtt_client.connected())
     {
        //reconnect();
        connectMQTT();
     }
>>>>>>> refs/remotes/origin/main
     // Say you're still there
     mqtt_set_availability(true);


     // Filter control
<<<<<<< HEAD
     filterControl();
=======
    //  filterControl();
>>>>>>> refs/remotes/origin/main

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
<<<<<<< HEAD
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wifi Connect successful...");

    // Serial.println("...connected");
    // Update time
    configTime(0, 0, ntpServer);
  }
  else
  {
    Serial.println("Wifi Connect failed...");
  }
=======
  Serial.println("... connected");
  // Update time
  configTime(0, 0, ntpServer);
>>>>>>> refs/remotes/origin/main
}

void connectMQTT()
{
   String client_id = "hotpot_mqtt-client";
<<<<<<< HEAD
    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqtt_callback);
   if (WiFi.status() == WL_CONNECTED)
   {
    if (!mqtt_client.connected()) {
      Serial.println("Connecting to MQTT broker ...");
      if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        mqtt_client.subscribe("hotpot/temp_threshold");
        mqtt_client.subscribe("hotpot/filter");
        mqtt_client.subscribe("hotpot/filter/duration");
        mqtt_client.subscribe("hotpot/filter/interval");
        mqtt_client.subscribe("hotpot/filter/switch");
        mqtt_client.subscribe("hotpot/filter/switch/set");
        mqtt_client.subscribe("hotpot/debug");
        // Serial.println("connected");
        mqtt_set_availability(true);
      } else {
        // Serial.print("failed with state ");
        // Serial.print(mqtt_client.state());
        delay(2000);
      }
    }
=======
   Serial.print("Connecting to MQTT broker...");
   if (!mqtt_client.connected()) {
     Serial.print(".");
     if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
       mqtt_client.subscribe("hotpot/temp_threshold");
       mqtt_client.subscribe("hotpot/filter");
       mqtt_client.subscribe("hotpot/filter/duration");
       mqtt_client.subscribe("hotpot/filter/interval");
       mqtt_client.subscribe("hotpot/filter/switch");
       mqtt_client.subscribe("hotpot/filter/switch/set");
       mqtt_client.subscribe("hotpot/debug");
       Serial.println("connected");
       mqtt_set_availability(true);
     } else {
       Serial.print("failed with state ");
       Serial.print(mqtt_client.state());
       delay(2000);
     }
>>>>>>> refs/remotes/origin/main
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
<<<<<<< HEAD
          mqtt_client.publish(topic, "ON", true);
        } else {
          mqtt_client.publish(topic, "OFF", true);
=======
          mqtt_client.publish(topic, "ON");
        } else {
          mqtt_client.publish(topic, "OFF");
>>>>>>> refs/remotes/origin/main
        }
}


