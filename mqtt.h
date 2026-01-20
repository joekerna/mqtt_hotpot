#ifndef MQTT_H
#define MQTT_H

const char *topic_prefix  = "homeassistant";
const char *config_suffix = "/config";

// -----  Topics -----
// Home Assistant
const char *temperature_state_topic      = "homeassistant/sensor/sensor_hotpot/state";
const char *fire_state_topic             = "homeassistant/binary_sensor/sensor_fire_hotpot/state";
const char *freeze_state_topic           = "homeassistant/binary_sensor/sensor_freeze_hotpot/state";
const char *filter_state_topic           = "homeassistant/binary_sensor/sensor_filter_hotpot/state";
const char *filter_parameter_state_topic = "homeassistant/sensor/sensor_filter_parameter/state";

const char *availability_topic        = "hotpot/filter/available";
const char *filter_switch_state_topic = "hotpot/filter/switch";
const char *filter_switch_set_topic   = "hotpot/filter/switch/set";
const char *filter_duration_topic     = "hotpot/filter/duration";
const char *filter_interval_topic     = "hotpot/filter/interval";
const char *debug_topic               = "hotpot/debug";
const char *error_topic               = "hotpot/error";
const char *outside_temperature_topic = "hotpot/outside_temperature";
const char *temp_threshold_topic      = "hotpot/temp_threshold";
const char *temp_update_rate_topic    = "hotpot/temperature/update_rate";

void connectMQTT();
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void mqtt_set_availability (bool state);

void createHomeAssistantSensor();

#endif
