#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "credentials.h"

const float temperature_change_threshold = 0.1;
const float freeze_threshold             = 2.0;
const float fire_threshold               = 5.0;
#define UPDATE_RATE_SECONDS 1


// MQTT Broker
const char *topic_prefix = "homeassistant/sensor/hotpot_";
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
  WiFi.setHostname(hostname);
  Serial.print("Connecting to WiFi... ");
  WiFi.begin((char *)ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("... connected");
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
  sprintf(mqtt_message, "{\"name\": \"Vorlauf\",  \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/sensor_hotpot/state\", \"unit_of_measurement\":\"°C\", \"value_template\":\"{{ value_json.vorlauf}}\",    \"unique_id\": \"unique_id_vor\",   \"device\": {\"identifiers\": [\"hotpot_temperature\"], \"name\": \"Hotpot\", \"manufacturer\": \"Kirami\", \"model\": \"Comfort Steady M\", \"model_id\": \"Nightblack\"} }");
  sprintf(topic, "%svorlauf%s", topic_prefix, config_suffix);
  client.publish(topic, mqtt_message);
  // Create Ruecklauf Sensor
  sprintf(mqtt_message, "{\"name\": \"Rücklauf\",  \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/sensor_hotpot/state\", \"unit_of_measurement\":\"°C\", \"value_template\":\"{{ value_json.ruecklauf}}\", \"unique_id\": \"unique_id_rueck\", \"device\": {\"identifiers\": [\"hotpot_temperature\"], \"name\": \"Hotpot\", \"manufacturer\": \"Kirami\", \"model\": \"Comfort Steady M\", \"model_id\": \"Nightblack\"} }");
  sprintf(topic, "%sruecklauf%s", topic_prefix, config_suffix);
  client.publish(topic, mqtt_message);
  // Create Difference Sensor
  sprintf(mqtt_message, "{\"name\": \"Differenz\", \"device_class\": \"temperature\", \"state_topic\": \"homeassistant/sensor/sensor_hotpot/state\", \"unit_of_measurement\":\"°C\", \"value_template\":\"{{ value_json.difference}}\",\"unique_id\": \"unique_id_diff\",  \"device\": {\"identifiers\": [\"hotpot_temperature\"], \"name\": \"Hotpot\", \"manufacturer\": \"Kirami\", \"model\": \"Comfort Steady M\", \"model_id\": \"Nightblack\"} }");
  sprintf(topic, "%sdifference%s", topic_prefix, config_suffix);
  client.publish(topic, mqtt_message);
  // Create Fire state Sensor
  sprintf(mqtt_message, "{\"name\": \"Feuer\", \"device_class\": \"heat\", \"state_topic\": \"homeassistant/binary_sensor/sensor_fire_hotpot/state\", \"unique_id\": \"unique_id_fire_state\",  \"device\": {\"identifiers\": [\"hotpot_temperature\"], \"name\": \"Hotpot\", \"manufacturer\": \"Kirami\", \"model\": \"Comfort Steady M\", \"model_id\": \"Nightblack\"} }");
  sprintf(topic, "homeassistant/binary_sensor/hotpot_fire_state%s", config_suffix);
  client.publish(topic, mqtt_message);
  // Create Freeze state Sensor
  sprintf(mqtt_message, "{\"name\": \"Frost\", \"device_class\": \"cold\", \"state_topic\": \"homeassistant/binary_sensor/sensor_freeze_hotpot/state\", \"unique_id\": \"unique_id_freeze_state\",  \"device\": {\"identifiers\": [\"hotpot_temperature\"], \"name\": \"Hotpot\", \"manufacturer\": \"Kirami\", \"model\": \"Comfort Steady M\", \"model_id\": \"Nightblack\"} }");
  sprintf(topic, "homeassistant/binary_sensor/hotpot_freeze_state%s", config_suffix);
  client.publish(topic, mqtt_message);

  if (client.setBufferSize(256)) {
    Serial.println("Resized buffer successfully");
  }

  // Connect to sensors
  sensors.begin();
}

void loop(void) {
  ArduinoOTA.handle();
  client.loop();

  if ((millis() - previousMillis) > (UPDATE_RATE_SECONDS*1000))
  {
     if (!client.connected())
     {
        //connect();
        Serial.println("MQTT disconnected");
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
