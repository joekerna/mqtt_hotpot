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


unsigned long readTimestamp = millis();

void setup(void) {
  // Connect to serial interface
  Serial.begin(9600);
  Serial.println("Starting...");

// -------------------------------------------------------------------------
  // Connecting to a Wi-Fi network
  WiFi.setHostname(device_name);
  connectWifi();
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
    Serial.println("Start updating " + type);
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
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqtt_callback);
  while (!mqtt_client.connected()) {
     connectMQTT();
  }
// -------------------------------------------------------------------------

  createHomeAssistantSensor();

  initTemperatureSensors();

  initFilter();
}

void loop(void) {
  ArduinoOTA.handle();
  mqtt_client.loop();

  // Check WIFI connection
  if (WiFi.status() != WL_CONNECTED)
  {
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
  }

// -------------------------------------------------------------
// TEMPERATURES
// -------------------------------------------------------------
  if ((millis() - readTimestamp) > (UPDATE_RATE_SECONDS*1000))
  {
    // Check MQTT connection
     if (!mqtt_client.connected())
     {
        //reconnect();
        connectMQTT();
     }
     // Say you're still there
     mqtt_set_availability(true);


     // Filter control
    //  filterControl();

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
  Serial.print("Connecting to WiFi...");
  WiFi.begin((char *)ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  } 
  Serial.println("... connected");
  // Update time
  configTime(0, 0, ntpServer);
}

void connectMQTT()
{
   String client_id = "hotpot_mqtt-client";
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

  // Serial.print("Creating sensor: ");
  // Serial.println(topic);
  // Serial.println(mqtt_message);

  // Publish
  mqtt_client.publish(topic, mqtt_message, true);
}

void updateBinarysensor(const char* topic, bool state)
{
        if (state) {
          mqtt_client.publish(topic, "ON");
        } else {
          mqtt_client.publish(topic, "OFF");
        }
}


