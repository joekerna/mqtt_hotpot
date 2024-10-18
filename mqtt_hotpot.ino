#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
//#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include "mqtt_hotpot.h"
#include "mqtt.h"
#include "filter.h"

void lowpass(float *filter, float temp);

      float temperature_change_threshold = 0.2;
const float freeze_threshold             = 2.0;
const float fire_threshold               = 5.0;
#define UPDATE_RATE_SECONDS 1


// MQTT Broker
const char *topic_prefix  = "homeassistant";
const char *config_suffix = "/config";


WiFiClient espClient;
PubSubClient client(espClient);
// MQTT Topics
char mqtt_message[512];
char topic[60];
const char *temperature_state_topic = "homeassistant/sensor/sensor_hotpot/state";
const char *fire_state_topic        = "homeassistant/binary_sensor/sensor_fire_hotpot/state";
const char *freeze_state_topic      = "homeassistant/binary_sensor/sensor_freeze_hotpot/state";
const char *filter_state_topic      = "homeassistant/binary_sensor/sensor_filter_hotpot/state";


// Temperature sensors
#define ONE_WIRE_BUS D4
//#define ONE_WIRE_BUS 26

// Filter pump
#define FILTER_RELAY 16
unsigned long filterOnSince = 0, filterOffSince = 0;
unsigned int filterIntervalHours = 6;
unsigned int filterDurationMinutes = 30;

// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature DallasTemperature
DallasTemperature sensors(&oneWire);

#define HISTORY_LENGTH 6
float temp_vor[HISTORY_LENGTH] = { }, temp_rueck[HISTORY_LENGTH] = { }, temp_difference = -5.0;
float temp_vor_last_transmitted = 0.0, temp_rueck_last_transmitted = 0.0;
// Tendency calculation
float temp_vor_avg = 0.0, temp_rueck_avg = 0.0;
float vor_change = 0.0, rueck_change = 0.0;

float vorlauf_filter[HISTORY_LENGTH];
unsigned long previousMillis;
bool fire = false;

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
     connectMQTT();
  }
// -------------------------------------------------------------------------


  // Increase MQTT Buffer size
  client.setBufferSize(512);

  // Create Vorlauf Sensor
  createNewSensor("Vorlauf",   "temperature", temperature_state_topic, "ui_vor",   "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.vorlauf }}");

  // Create Ruecklauf Sensor
  createNewSensor("Rücklauf",  "temperature", temperature_state_topic, "ui_rueck", "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.ruecklauf }}");

  // Create Difference Sensor
  createNewSensor("Differenz", "temperature", temperature_state_topic, "ui_diff",  "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.difference }}");

  // Create Fire state Sensor
  createNewSensor("Feuer",     "heat",        fire_state_topic,        "ui_fire_state", "hotpot_temperature", device_name, manufacturer, model, model_id, "binary_sensor", "{{ value_json.fire }}");

  // Create Freeze state Sensor
  createNewSensor("Frost",     "cold",        freeze_state_topic,      "ui_freeze_state", "hotpot_temperature", device_name, manufacturer, model, model_id, "binary_sensor", "{{ value_json.freeze }}");

  // Create Filter state Sensor
  createNewSensor("Filter",    "running",     filter_state_topic,      "ui_filter_state", "hotpot_temperature", device_name, manufacturer, model, model_id, "binary_sensor", "{{ value_json.filter }}");

  client.setBufferSize(256);

  // Connect to sensors
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  sensors.begin();

  pinMode(FILTER_RELAY, OUTPUT);
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

// -------------------------------------------------------------
// TEMPERATURES
// -------------------------------------------------------------
  if ((millis() - previousMillis) > (UPDATE_RATE_SECONDS*1000))
  {
    // digitalWrite(FILTER_RELAY, LOW);
    // delay(1000);
    // digitalWrite(FILTER_RELAY, HIGH);
    // delay(1000);
    // digitalWrite(FILTER_RELAY, LOW);
    // delay(1000);
    // digitalWrite(FILTER_RELAY, HIGH);
    // Check MQTT connection
     if (!client.connected())
     {
        //reconnect();
        connectMQTT();
     }
     // Store timestamp
     previousMillis = millis();

     // Tendency
     temp_vor_avg   = 0.0;
     temp_rueck_avg = 0.0;
     for (int i = 0; i < HISTORY_LENGTH; i++)
     {
        temp_vor_avg   += temp_vor[i];
        temp_rueck_avg += temp_rueck[i];
     }
     temp_vor_avg   /= HISTORY_LENGTH;
     temp_rueck_avg /= HISTORY_LENGTH;

     // Get temperature updates
     sensors.requestTemperatures();
     for (int i = HISTORY_LENGTH-1; i > 0; i--)
     {
        temp_vor[i]   = temp_vor[i-1];
        temp_rueck[i] = temp_rueck[i-1];
     }
     temp_vor[0]   = sensors.getTempCByIndex(0);
     temp_rueck[0] = sensors.getTempCByIndex(1);
     vor_change    = temp_vor[0]   - temp_vor_last_transmitted;
     rueck_change  = temp_rueck[0] - temp_rueck_last_transmitted;

//DEBUG
     // for (int i = 0; i < HISTORY_LENGTH; i++)
     // {
     //    Serial.print(temp_vor[i]);
     //    Serial.print(" ");
     // }
     //Serial.println("");
     // Serial.println(temp_vor[0]);
     // Serial.println(temp_rueck[0]);
     // Serial.println(vor_change);
     // Serial.println(rueck_change);
     // if (vor_change > 0) {
     //    Serial.println("Vorlauf temperature rising");
     // } else {
     //    Serial.println("Vorlauf temperature falling");
     // }
     // 
     // if (rueck_change > 0) {
     //    Serial.println("Ruecklauf temperature rising");
     // } else {
     //    Serial.println("Ruecklauf temperature falling");
     // }
     

     // Update Temperatures if significant change
     if ( (fabs(vor_change) >= temperature_change_threshold) || (fabs(rueck_change) >= temperature_change_threshold) )
     {
        // Calculate difference
        temp_difference = temp_rueck[0] - temp_vor[0];

        // Publish temperatures
        sprintf(mqtt_message, "{\"vorlauf\": %.1f, \"ruecklauf\": %.1f , \"difference\": %.1f}", temp_vor[0], temp_rueck[0], temp_difference);
        client.publish(temperature_state_topic, mqtt_message);
        temp_vor_last_transmitted   = temp_vor[0];
        temp_rueck_last_transmitted = temp_rueck[0];
        Serial.println(mqtt_message);

        // Publish Fire state
        fire = temp_difference > fire_threshold;
        updateBinarysensor(fire_state_topic, fire);

        // Publish Freeze state
        updateBinarysensor(freeze_state_topic, ((temp_rueck[0] < freeze_threshold) | (temp_vor[0] < freeze_threshold)));
     }
  }
  // lowpass(vorlauf_filter, temp_vor);

// -------------------------------------------------------------
// FILTER
// -------------------------------------------------------------
 //  if (fire)
 //  {
 //  // Turn off filter
 //    switchFilter(false);
 //  }
 //  if ((millis() - filterOffSince) > (filterIntervalHours*60*60*1000))
 //  {
 //    // Turn on filter
 //    switchFilter(true);
 //  }
 //  if ((millis() - filterOnSince) > (filterDurationMinutes*60*1000))
 //  {
 //    // Turn off filter
 //    switchFilter(false);
 //  }
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
}

void connectMQTT()
{
   String client_id = "hotpot_mqtt-client";
   Serial.print("Connecting to MQTT broker...");
   while (!client.connected()) {
     Serial.print(".");
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
       client.subscribe("hotpot/temp_threshold");
       client.subscribe("hotpot/filter_duration");
       client.subscribe("hotpot/filter");
       Serial.println("connected");
     } else {
       Serial.print("failed with state ");
       Serial.print(client.state());
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
  client.publish(topic, mqtt_message, true);
}

void updateBinarysensor(const char* topic, bool state)
{
        if (state) {
          client.publish(topic, "ON");
        } else {
          client.publish(topic, "OFF");
        }
}

void lowpass(float *filter, float temp)
{
   unsigned int length = sizeof(filter);
   for (int i = length; i > 0; i--)
   {
      filter[i] = filter[i-1];
   }
   filter[0] = temp;

   float output = 0.0;
   for (int i = 0; i < length; i++)
   {
      output = output + filter[i];
   }
   output = output / length;
  
}

