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
  snprintf(temperature_status_state_topic, sizeof(temperature_status_state_topic), "%s/temperature_status/state", unique_id);
}

// Publish an error for a malformed MQTT payload
void publishParseError(const char *topic)
{
  snprintf(mqtt_message, sizeof(mqtt_message), "Invalid payload on topic: %s", topic);
  mqtt_client.publish(error_topic, mqtt_message);
}

// Parse a numeric payload (NUL-terminated local buffer)
bool parseNumericPayload(const char *buf, const char *topic, double *out)
{
  char *endptr;
  *out = strtod(buf, &endptr);
  if ((endptr == buf) || (*endptr != '\0'))
  {
    publishParseError(topic);
    return false;
  }
  return true;
}

// Parse an unsigned integer payload (NUL-terminated local buffer)
bool parseUnsignedPayload(const char *buf, const char *topic, unsigned int *out)
{
  double value;
  if (!parseNumericPayload(buf, topic, &value))
    return false;
  if ((value < 0.0) || (value >= 4294967296.0))
  {
    publishParseError(topic);
    return false;
  }
  *out = (unsigned int)value;
  return true;
}

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  // The MQTT payload is not NUL-terminated, so copy it into a local,
  // length-capped, NUL-terminated buffer before parsing it
  char buf[32];
  uint16_t len = (length < sizeof(buf) - 1) ? (uint16_t)length : (uint16_t)(sizeof(buf) - 1);
  memcpy(buf, payload, len);
  buf[len] = '\0';

  if (strcmp(topic,temp_threshold_topic)==0)
  {
     double value;
     if (parseNumericPayload(buf, topic, &value))
        setTemperatureChangeThreshold((float)value);
  }
  else if (strcmp(topic,temp_update_rate_topic)==0)
  {
     unsigned int value;
     if (parseUnsignedPayload(buf, topic, &value))
        setUpdateRate(value);
  }
  else if (strcmp(topic,filter_duration_set_topic)==0)
  {
     unsigned int value;
     if (parseUnsignedPayload(buf, topic, &value))
        updateFilterDuration(value);
  }
  else if (strcmp(topic,filter_interval_set_topic)==0)
  {
     unsigned int value;
     if (parseUnsignedPayload(buf, topic, &value))
        updateFilterInterval(value);
  }
  else if (strcmp(topic, outside_temperature_topic) == 0)
  {
     double value;
     if (parseNumericPayload(buf, topic, &value))
     {
        snprintf(mqtt_message, sizeof(mqtt_message), "Temperature update received: %.2f", value);
        mqtt_client.publish(debug_topic, mqtt_message);
     }
  }
  else if (strcmp(topic,filter_set_topic)==0)
  {
    if (strcmp(buf, "ON") == 0)
    {
       switchFilter(true);
       changeFilterMode(manual);
    }
    else if (strcmp(buf, "OFF") == 0)
    {
       switchFilter(false);
       changeFilterMode(automatically);
    }
  }

}


void mqtt_set_availability (bool state) {
    if (state)
    {
	mqtt_client.publish(availability_topic, "online", true);
    }
    else
    {
	mqtt_client.publish(availability_topic, "offline", true);
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
  discoverTemperatureStatus();

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
    // Longer keepalive so brief WiFi hiccups / blocking waits don't drop the session
    mqtt_client.setKeepAlive(60);
   if (WiFi.status() == WL_CONNECTED)
   {
    if (!mqtt_client.connected()) {
      Serial.println("Connecting to MQTT broker ...");
      //if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password,
      //                        availability_topic, 1, true, "offline")) {
      if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        mqtt_client.subscribe(temp_threshold_topic);
        mqtt_client.subscribe(temp_update_rate_topic);
        mqtt_client.subscribe(outside_temperature_topic);

        // All set topics
        snprintf(topic, sizeof(topic), "%s/+/set", base_topic);
        mqtt_client.subscribe(topic);

        mqtt_set_availability(true);
        // Publish retained state after reconnect so broker/clients know current state
        publishFilterState(filter.state);
        changeFilterMode(filter.mode);
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
      "\"availability_topic\":\"%s\","
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
    unique_id, filter_duration_state_topic, availability_topic, filter_duration_set_topic,
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

void discoverTemperatureStatus()
{
  char payload[512];

  int n = snprintf(payload, sizeof(payload),
    "{"
      "\"name\":\"Temperature Status\","
      "\"unique_id\":\"%s_status\","
      "\"state_topic\":\"%s\","
      "\"availability_topic\":\"%s\","
      "\"device_class\":\"enum\","
      "\"options\":[\"hot\",\"warm\",\"cold\"],"
      "\"icon\":\"mdi:water-thermometer\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, temperature_status_state_topic, availability_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/sensor/%s/temperature_status/%s",
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
      "\"availability_topic\":\"%s\","
      "\"command_topic\":\"%s\","
      "\"min\":1,\"max\":168,\"step\":1,\"mode\":\"slider\","
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
    unique_id, filter_interval_state_topic, availability_topic, filter_interval_set_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/number/%s/filter_interval/%s",
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
      "\"availability_topic\":\"%s\","
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
    unique_id, temperature_vorlauf_state_topic, availability_topic,
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
      "\"availability_topic\":\"%s\","
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
    unique_id, temperature_ruecklauf_state_topic, availability_topic,
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
      "\"availability_topic\":\"%s\","
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
    unique_id, temperature_difference_state_topic, availability_topic,
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
      "\"availability_topic\":\"%s\","
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
    unique_id, filter_state_topic, availability_topic, filter_set_topic,
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
      "\"availability_topic\":\"%s\","
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
    unique_id, fire_state_topic, availability_topic,
    unique_id, device_name, manufacturer, model, model_id
  );

  if (n <= 0 || n >= (int)sizeof(payload)) {
    // payload got truncated; increase buffer or shorten strings
    return;
  }

  snprintf(topic, sizeof(topic),
           "%s/binary_sensor/%s/fire/%s",
           topic_prefix, unique_id, config_suffix);

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
      "\"availability_topic\":\"%s\","
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
    unique_id, freeze_state_topic, availability_topic,
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
      "\"availability_topic\":\"%s\","
      "\"icon\":\"mdi:air-filter\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"name\":\"%s\","
        "\"manufacturer\":\"%s\","
        "\"model\":\"%s\","
        "\"sw_version\":\"%s\""
      "}"
    "}",
    unique_id, filter_mode_state_topic, availability_topic,
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

