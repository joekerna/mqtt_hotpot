#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "mqtt_hotpot.h"

const float temperature_change_threshold = 0.1;
const float freeze_threshold             = 2.0;
const float fire_threshold               = 5.0;
#define UPDATE_RATE_SECONDS 1


// MQTT Broker
const char *topic_prefix  = "homeassistant/";
const char *config_suffix = "/config";


WiFiClient espClient;
PubSubClient client(espClient);
// MQTT Topics
char mqtt_message[800];
char topic[60];
const char *state_topic = "homeassistant/sensor/sensor_hotpot/state";


// Temperature sensors
// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS D4

// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature DallasTemperature
DallasTemperature sensors(&oneWire);

float temp_vor = 0.0, temp_rueck = 0.0, temp_vor_prev = 0.0, temp_rueck_prev = 0.0, temp_difference = -5.0;
float vor_change = 0.0, rueck_change = 0.0;
unsigned long previousMillis;

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  Serial.println("Received MQTT message");
}

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
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.setHostname("Hotpot_OTA");
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.begin();
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
  // Connect to MQTT broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(mqtt_callback);
  while (!client.connected()) {
    String client_id = "hotpot_mqtt-client";
    //client_id += String(WiFi.macAddress(mac));
    // Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public EMQX MQTT broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
// -------------------------------------------------------------------------


  // Increase MQTT Buffer size
  if (client.setBufferSize(512)) {
    Serial.println("Resized buffer successfully");
  }

  // Create Vorlauf Sensor
  createNewSensor("Vorlauf",   "temperature", "homeassistant/sensor/sensor_hotpot/state", "unique_id_vor",   "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "°C", "{{ value_json.vorlauf }}");

  // Create Ruecklauf Sensor
  createNewSensor("Rücklauf",  "temperature", "homeassistant/sensor/sensor_hotpot/state", "unique_id_rueck", "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "°C", "{{ value_json.ruecklauf }}");

  // Create Difference Sensor
  createNewSensor("Differenz", "temperature", "homeassistant/sensor/sensor_hotpot/state", "unique_id_diff",  "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "°C", "{{ value_json.difference }}");

  // Create Fire state Sensor
  createNewSensor("Feuer",     "heat",        "homeassistant/binary_sensor/sensor_fire_hotpot/state", "unique_id_fire_state", "hotpot_temperature", device_name, manufacturer, model, model_id, "binary_sensor");

  // Create Freeze state Sensor
  createNewSensor("Frost",     "cold",        "homeassistant/binary_sensor/sensor_freeze_hotpot/state", "unique_id_freeze_state", "hotpot_temperature", device_name, manufacturer, model, model_id, "binary_sensor");

  if (client.setBufferSize(256)) {
    Serial.println("Resized buffer successfully");
  }

  // Connect to sensors
  sensors.begin();
}

void loop(void) {
  ArduinoOTA.handle();
  client.loop();

  // Check WIFI connection
  if (WiFi.status() != WL_CONNECTED)
  {
    // if disconnected, reconnect
    connectWifi();
  }

  if ((millis() - previousMillis) > (UPDATE_RATE_SECONDS*1000))
  {
    // Check MQTT connection
     if (!client.connected())
     {
        //reconnect();
        String client_id = "hotpot_mqtt-client";
        while (!client.connected()) {
          if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Public EMQX MQTT broker connected");
          } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
          }
        }
     }
     // Store timestamp
     previousMillis = millis();

     // Get temperature updates
     sensors.requestTemperatures();
     temp_vor     = sensors.getTempCByIndex(0);
     temp_rueck   = sensors.getTempCByIndex(1);
     vor_change   = temp_vor_prev   - temp_vor;
     rueck_change = temp_rueck_prev - temp_rueck;
     Serial.print(temp_vor_prev);
     Serial.print(" ");
     Serial.print(temp_vor);
     Serial.print(" ");
     Serial.println(fabs(vor_change));

     // Update Temperatures if significant change
     if ( (fabs(vor_change) >= temperature_change_threshold) || (fabs(rueck_change) >= temperature_change_threshold) )
     {
        // Store temperatures
        temp_vor_prev   = temp_vor;
        temp_rueck_prev = temp_rueck;
        // Calculate difference
        temp_difference = temp_rueck - temp_vor;
        // Publish temperatures
        sprintf(mqtt_message, "{\"vorlauf\": %.2f, \"ruecklauf\": %.2f , \"difference\": %.2f}", temp_vor, temp_rueck, temp_difference);
        client.publish(state_topic, mqtt_message);
        Serial.println(mqtt_message);

        // Publish Fire state
        if (temp_difference > fire_threshold) {
          client.publish("homeassistant/binary_sensor/sensor_fire_hotpot/state", "ON");
        } else {
          client.publish("homeassistant/binary_sensor/sensor_fire_hotpot/state", "OFF");
        }

        // Publish Freeze state
        if ((temp_rueck < freeze_threshold) | (temp_vor < freeze_threshold)) {
          client.publish("homeassistant/binary_sensor/sensor_freeze_hotpot/state", "ON");
        } else {
          client.publish("homeassistant/binary_sensor/sensor_freeze_hotpot/state", "OFF");
        }
     }
  }

}

void connectWifi()
{
  Serial.print("Connecting to WiFi... ");
  WiFi.begin((char *)ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  } 
  Serial.println("... connected");
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
                     const char *unit,
                     const char *value_template)
{
  // Create MQTT payload
  if (sensor_type == "sensor")
  {
     sprintf(mqtt_message, 
             "{\"name\": \"%s\", \"device_class\": \"%s\", \"state_topic\": \"%s\", \"unit_of_measurement\":\"%s\", \"value_template\":\"%s\", \"unique_id\": \"%s\", \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\", \"manufacturer\": \"%s\", \"model\": \"%s\", \"model_id\": \"%s\"} }",\
             name, device_class, state_topic, unit, value_template, unique_id, identifiers, device_name, manufacturer, model, model_id);
  } else {
     sprintf(mqtt_message, 
             "{\"name\": \"%s\", \"device_class\": \"%s\", \"state_topic\": \"%s\", \"unique_id\": \"%s\", \"device\": {\"identifiers\": [\"%s\"], \"name\": \"%s\", \"manufacturer\": \"%s\", \"model\": \"%s\", \"model_id\": \"%s\"} }",\
             name, device_class, state_topic, unique_id, identifiers, device_name, manufacturer, model, model_id);
  }
  // Create MQTT topic
  sprintf(topic, "%s/%s/%s/config", topic_prefix, sensor_type, unique_id);

  Serial.print("Creating sensor: ");
  Serial.println(mqtt_message);

  // Publish
  client.publish(topic, mqtt_message,true);
}
