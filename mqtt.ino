#include <PubSubClient.h>
#include "mqtt.h"
#include "filter.h"
#include "temperatures.h"
#include <string.h>

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Received MQTT message: ");
  Serial.print(topic);
  Serial.print(": ");
  for (int i = 0; i < length; i++)
  {
      Serial.print((char)payload[i] );
  }
  Serial.println("");

  if (strcmp(topic,"hotpot/temp_threshold")==0)
  {
     setTemperatureChangeThreshold((float)atof((const char *)(payload)));
  }
  else if (strcmp(topic,"hotpot/filter/duration")==0)
  {
     Serial.print("Updating filter duration to ");
    //  temperature_change_threshold = (float)atof((const char *)(payload));
     Serial.println((const char*)payload);
  }
  else if (strcmp(topic,"hotpot/filter/interval")==0)
  {
    updateFilterInterval((unsigned int)*payload);
  }
  else if (strcmp(topic, outside_temperature_topic) == 0)
  {
    mqtt_client.publish("hotpot/debug", "Temperature update received");
    float outside_temperature = (float)atof((const char *)(payload));
  }
  else if (strcmp(topic,"hotpot/filter/switch/set")==0)
  {
    const char *on_state  = "ON";
    const char *off_state = "OFF";

    if (memcmp(payload, on_state, length) == 0) 
    {
       switchFilter(true);
    }
    else
    {
       switchFilter(false);
    }
  }

}


void mqtt_set_availability (bool state) {
    if (state)
    {
	mqtt_client.publish(AVAILABILITY_TOPIC, "online");
    }
    else
    {
	mqtt_client.publish(AVAILABILITY_TOPIC, "offline");
    }
}

void createHomeAssistantSensor()
{
  // Increase MQTT Buffer size
  mqtt_client.setBufferSize(512);

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

  // Create Filter interval sensor
  createNewSensor("Filter Interval", "temperature", temperature_state_topic, "ui_filter_interval", "hotpot_temperature", device_name, manufacturer, model, model_id, "sensor", "{{ value_json.filter_interval }}");

  mqtt_client.setBufferSize(256);

}
