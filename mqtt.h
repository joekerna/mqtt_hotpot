#ifndef MQTT_H
#define MQTT_H

const char *topic_prefix  = "homeassistant";
const char *config_suffix = "/config";

// Topics
#define AVAILABILITY_TOPIC "hotpot/filter/available"
const char *temperature_state_topic = "homeassistant/sensor/sensor_hotpot/state";
const char *fire_state_topic        = "homeassistant/binary_sensor/sensor_fire_hotpot/state";
const char *freeze_state_topic      = "homeassistant/binary_sensor/sensor_freeze_hotpot/state";
const char *filter_state_topic      = "homeassistant/binary_sensor/sensor_filter_hotpot/state";
const char *outside_temperature_topic = "hotpot/outside_temperature";

void mqtt_callback(char *topic, byte *payload, unsigned int length);
void mqtt_set_availability (bool state);

void createHomeAssistantSensor();

#endif
