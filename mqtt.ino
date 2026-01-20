#include <PubSubClient.h>
#include "mqtt.h"
#include "filter.h"
#include "temperatures.h"
#include <string.h>

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  if (strcmp(topic,temp_threshold_topic)==0)
  {
     setTemperatureChangeThreshold((float)atof((const char *)(payload)));
  }
  else if (strcmp(topic,temp_update_rate_topic)==0)
  {
     char *endptr;
     setUpdateRate((unsigned int)strtoul((const char *)(payload), &endptr, 10));
  }
  else if (strcmp(topic,filter_duration_topic)==0)
  {
     char *endptr;
     updateFilterDuration((unsigned int)strtoul((const char *)(payload), &endptr, 10));
  }
  else if (strcmp(topic,filter_interval_topic)==0)
  {
     char *endptr;
     updateFilterInterval((unsigned int)strtoul((const char *)(payload), &endptr, 10));
  }
  else if (strcmp(topic, outside_temperature_topic) == 0)
  {
    mqtt_client.publish(debug_topic, "Temperature update received");
    float outside_temperature = (float)atof((const char *)(payload));
  }
  else if (strcmp(topic,filter_switch_set_topic)==0)
  {
    const char *on_state  = "ON";
    const char *off_state = "OFF";

    if (memcmp(payload, on_state, length) == 0) 
    {
       switchFilter(true);
       filter.mode = manual;
    }
    else if (memcmp(payload, off_state, length) == 0) 
    {
       switchFilter(false);
       filter.mode = automatically;
    }
  }

}


void mqtt_set_availability (bool state) {
    if (state)
    {
	mqtt_client.publish(availability_topic, "online");
    }
    else
    {
	mqtt_client.publish(availability_topic, "offline");
    }
}

void createHomeAssistantSensor()
{
  // Increase MQTT Buffer size
  mqtt_client.setBufferSize(512);

  // Create Vorlauf Sensor
  createNewSensor("Vorlauf",         "temperature", temperature_state_topic, "ui_vor",             "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.vorlauf }}");

  // Create Ruecklauf Sensor
  createNewSensor("Rücklauf",        "temperature", temperature_state_topic, "ui_rueck",           "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.ruecklauf }}");

  // Create Difference Sensor
  createNewSensor("Differenz",       "temperature", temperature_state_topic, "ui_diff",            "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.difference }}");

  // Create Tendency Sensor
  createNewSensor("Tendenz",         "temperature", temperature_state_topic, "ui_tendency",        "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.tendency }}");

  // Create Fire state Sensor
  createNewSensor("Feuer",           "heat",        fire_state_topic,        "ui_fire_state",      "hotpot_temperature", device_name, manufacturer, model, model_id, "binary_sensor", "{{ value_json.fire }}");

  // Create Freeze state Sensor      
  createNewSensor("Frost",           "cold",        freeze_state_topic,      "ui_freeze_state",    "hotpot_temperature", device_name, manufacturer, model, model_id, "binary_sensor", "{{ value_json.freeze }}");

  // Create Filter state Sensor      
  createNewSensor("Filter",          "running",     filter_state_topic,      "ui_filter_state",    "hotpot_temperature", device_name, manufacturer, model, model_id, "binary_sensor", "{{ value_json.filter }}");

  // Create Filter interval sensor
  createNewSensor("Filter Interval", "duration",    temperature_state_topic, "ui_filter_interval", "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.filter_interval }}");

  // Create Filter duration sensor
  // createNewSensor("Filter Duration", "duration",    temperature_state_topic, "ui_filter_duration", "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.filter_duration }}");

  mqtt_client.setBufferSize(256);

}

void connectMQTT()
{
   String client_id = "hotpot_mqtt-client";
    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqtt_callback);
   if (WiFi.status() == WL_CONNECTED)
   {
    if (!mqtt_client.connected()) {
      Serial.println("Connecting to MQTT broker ...");
      if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        mqtt_client.subscribe(temp_threshold_topic);
        mqtt_client.subscribe("hotpot/filter");
        mqtt_client.subscribe(filter_duration_topic);
        mqtt_client.subscribe(filter_interval_topic);
        mqtt_client.subscribe("hotpot/filter/switch");
        mqtt_client.subscribe(filter_switch_set_topic);
        mqtt_client.subscribe(temp_update_rate_topic);
        mqtt_set_availability(true);
      } else {
        delay(2000);
      }
    }
   }
}

// void discoverFilterDuration()
// {
//   char payload[512];
// 
//   int n = snprintf(payload, sizeof(payload),
//     "{"
//       "\"name\":\"Hour\","
//       "\"unique_id\":\"%s_hour\","
//       "\"state_topic\":\"%s/hour/state\","
//       "\"command_topic\":\"%s/hour/set\","
//       "\"min\":1,\"max\":24,\"step\":1,\"mode\":\"slider\","
//       "\"device\":{"
//         "\"identifiers\":[\"%s\"],"
//         "\"name\":\"%s\","
//         "\"manufacturer\":\"%s\","
//         "\"model\":\"%s\","
//         "\"sw_version\":\"%s\""
//       "}"
//     "}",
//     HA_ID, HA_ID, HA_ID, HA_ID, DEVICE_NAME, MANUFACTURER, MODEL, SW_VERSION
//   );
// 
//   if (n <= 0 || n >= (int)sizeof(payload)) {
//     // payload got truncated; increase buffer or shorten strings
//     return;
//   }
// 
//   mqtt.publish(TOPIC_NUM_CONFIG, payload, true);
// 
// }
