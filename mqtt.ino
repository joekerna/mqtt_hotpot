#include <PubSubClient.h>
#include "mqtt.h"
#include "filter.h"
#include "temperatures.h"
#include <string.h>

void initTopics()
{
  snprintf(filter_duration_set_topic,          sizeof(filter_duration_set_topic),          "%s/filter_duration/set", unique_id);
  snprintf(filter_duration_state_topic,        sizeof(filter_duration_state_topic),        "%s/filter_duration/state", unique_id);
  snprintf(filter_interval_set_topic,          sizeof(filter_interval_set_topic),          "%s/filter_interval/set", unique_id);
  snprintf(filter_interval_state_topic,        sizeof(filter_interval_state_topic),        "%s/filter_interval/state", unique_id);
  snprintf(filter_set_topic,                   sizeof(filter_set_topic),                   "%s/filter/set", unique_id);
  snprintf(filter_state_topic,                 sizeof(filter_state_topic),                 "%s/filter/state", unique_id);
  snprintf(filter_mode_state_topic,            sizeof(filter_mode_state_topic),            "%s/filter_mode/state", unique_id);

  snprintf(fire_state_topic,                   sizeof(fire_state_topic),                   "%s/fire/state", unique_id);
  snprintf(freeze_state_topic,                 sizeof(freeze_state_topic),                 "%s/freeze/state", unique_id);

  snprintf(temperature_vorlauf_state_topic,    sizeof(temperature_vorlauf_state_topic),    "%s/temperature_vorlauf/state", unique_id);
  snprintf(temperature_ruecklauf_state_topic,  sizeof(temperature_ruecklauf_state_topic),  "%s/temperature_ruecklauf/state", unique_id);
  snprintf(temperature_difference_state_topic, sizeof(temperature_difference_state_topic), "%s/temperature_difference/state", unique_id);
}

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
  else if (strcmp(topic,filter_duration_set_topic)==0)
  {
     char *endptr;
     updateFilterDuration((unsigned int)strtoul((const char *)(payload), &endptr, 10));
  }
  else if (strcmp(topic,filter_interval_set_topic)==0)
  {
     char *endptr;
     updateFilterInterval((unsigned int)strtoul((const char *)(payload), &endptr, 10));
  }
  else if (strcmp(topic, outside_temperature_topic) == 0)
  {
    mqtt_client.publish(debug_topic, "Temperature update received");
    float outside_temperature = (float)atof((const char *)(payload));
  }
  else if (strcmp(topic,filter_set_topic)==0)
  {
    const char *on_state  = "ON";
    const char *off_state = "OFF";

    if (memcmp(payload, on_state, length) == 0) 
    {
       switchFilter(true);
       changeFilterMode(manual);
    }
    else if (memcmp(payload, off_state, length) == 0) 
    {
       switchFilter(false);
       changeFilterMode(automatically);
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
  discoverTemperatureVorlauf();

  // Create Ruecklauf Sensor
  discoverTemperatureRuecklauf();

  // Create Difference Sensor
  discoverTemperatureDifference();

  // Create Tendency Sensor

  // Create Fire state Sensor
  discoverFireState();

  // Create Freeze state Sensor      
  discoverFreezeState();

  // Create Filter state Sensor      
  discoverFilterSwitch();
  discoverFilterMode();

  // Create Filter interval sensor
  discoverFilterInterval();

  // Create Filter duration sensor
  discoverFilterDuration();

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
        mqtt_client.subscribe("hotpot/filter/switch");
        mqtt_client.subscribe(temp_update_rate_topic);

        // All set topics
        snprintf(topic, sizeof(topic), "%s/+/set", base_topic, unique_id);
        mqtt_client.subscribe(topic);

        mqtt_set_availability(true);
      } else {
        delay(2000);
      }
    }
   }
}

void discoverFilterDuration()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Filter Duration\","
      "\"unique_id\":\"%s_filter_duration\","
      "\"state_topic\":\"%s\","
      "\"command_topic\":\"%s\","
      "\"min\":1,\"max\":60,\"step\":1,\"mode\":\"slider\","
      "\"unit_of_measurement\":\"min\","
      "\"icon\":\"mdi:timer\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, filter_duration_state_topic, filter_duration_set_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/number/%s/filter_duration/%s",
           topic_prefix, unique_id, config_suffix);

  mqtt_client.publish(topic, payload, true);

}

void discoverFilterInterval()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Filter Interval\","
      "\"unique_id\":\"%s_filter_interval\","
      "\"state_topic\":\"%s\","
      "\"command_topic\":\"%s\","
      "\"min\":1,\"max\":48,\"step\":1,\"mode\":\"slider\","
      "\"unit_of_measurement\":\"h\","
      "\"icon\":\"mdi:timer\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, filter_interval_state_topic, filter_interval_set_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/number/%s/filter_interval/$s",
           topic_prefix, unique_id, config_suffix);

  mqtt_client.publish(topic, payload, true);

}

void discoverTemperatureVorlauf()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Vorlauftemperatur\","
      "\"unique_id\":\"%s_temperature_vorlauf\","
      "\"state_topic\":\"%s\","
      "\"device_class\":\"temperature\","
      "\"unit_of_measurement\":\"°C\","
      "\"state_class\":\"measurement\","
      "\"value_template\":\"{{ value | float }}\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, temperature_vorlauf_state_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/sensor/%s/temperature_vorlauf/%s",
           topic_prefix, unique_id, config_suffix);

  mqtt_client.publish(topic, payload, true);

}

void discoverTemperatureRuecklauf()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Rücklauftemperatur\","
      "\"unique_id\":\"%s_temperature_ruecklauf\","
      "\"state_topic\":\"%s\","
      "\"device_class\":\"temperature\","
      "\"unit_of_measurement\":\"°C\","
      "\"state_class\":\"measurement\","
      "\"value_template\":\"{{ value | float }}\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, temperature_ruecklauf_state_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/sensor/%s/temperature_ruecklauf/%s",
           topic_prefix, unique_id, config_suffix);

  mqtt_client.publish(topic, payload, true);

}

void discoverTemperatureDifference()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Temperaturdifferenz\","
      "\"unique_id\":\"%s_temperature_difference\","
      "\"state_topic\":\"%s\","
      "\"device_class\":\"temperature\","
      "\"unit_of_measurement\":\"°C\","
      "\"state_class\":\"measurement\","
      "\"value_template\":\"{{ value | float }}\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, temperature_difference_state_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/sensor/%s/temperature_difference/%s",
           topic_prefix, unique_id, config_suffix);

  mqtt_client.publish(topic, payload, true);

}

void discoverFilterSwitch()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Filter\","
      "\"unique_id\":\"%s_filter\","
      "\"state_topic\":\"%s\","
      "\"command_topic\":\"%s\","
      "\"payload_on\":\"ON\","
      "\"payload_off\":\"OFF\","
      "\"icon\":\"mdi:air-filter\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, filter_state_topic, filter_set_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/switch/%s/filter/%s",
           topic_prefix, unique_id, config_suffix);

  mqtt_client.publish(topic, payload, true);

}

void discoverFireState()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Feuer\","
      "\"unique_id\":\"%s_fire\","
      "\"state_topic\":\"%s\","
      "\"payload_on\":\"ON\","
      "\"payload_off\":\"OFF\","
      "\"device_class\":\"heat\","
      "\"icon\":\"mdi:fire\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, fire_state_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/binary_sensor/%s/fire/%s",
           topic_prefix, unique_id);

  mqtt_client.publish(topic, payload, true);

}

void discoverFreezeState()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Frost\","
      "\"unique_id\":\"%s_freeze\","
      "\"state_topic\":\"%s\","
      "\"payload_on\":\"ON\","
      "\"payload_off\":\"OFF\","
      "\"device_class\":\"heat\","
      "\"icon\":\"mdi:snowflake-thermometer\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, freeze_state_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/binary_sensor/%s/freeze/%s",
           topic_prefix, unique_id, config_suffix);

  mqtt_client.publish(topic, payload, true);

}

void discoverFilterMode()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Filter Mode\","
      "\"unique_id\":\"%s_filter_mode\","
      "\"state_topic\":\"%s\","
      "\"icon\":\"mdi:air-filter\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, filter_mode_state_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/sensor/%s/filter_mode/%s",
           topic_prefix, unique_id, config_suffix);

  mqtt_client.publish(topic, payload, true);

}

