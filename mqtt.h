#ifndef MQTT_H
#define MQTT_H

const char *unique_id     = "hotpot";
const char *topic_prefix  = "homeassistant";
const char *base_topic    = "hotpot";
const char *config_suffix = "config";

// -----  Topics -----
// Home Assistant Discovery
char filter_duration_set_topic[128];
char filter_duration_state_topic[128];
char filter_interval_set_topic[128];
char filter_interval_state_topic[128];
char filter_set_topic[128];
char filter_state_topic[128];
char filter_mode_state_topic[128];
char temperature_vorlauf_state_topic[128];
char temperature_ruecklauf_state_topic[128];
char temperature_difference_state_topic[128];
char fire_state_topic[128];
char freeze_state_topic[128];


const char *availability_topic        = "hotpot/filter/available";
const char *debug_topic               = "hotpot/debug";
const char *error_topic               = "hotpot/error";
const char *outside_temperature_topic = "hotpot/outside_temperature";
const char *temp_threshold_topic      = "hotpot/temp_threshold";
const char *temp_update_rate_topic    = "hotpot/temperature/update_rate";


void initTopics();
void connectMQTT();
void mqtt_callback(char *topic, byte *payload, unsigned int length);
void mqtt_set_availability (bool state);

void createHomeAssistantSensor();

#endif
