#include "temperatures.h"


// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature DallasTemperature
DallasTemperature sensors(&oneWire);



const float fire_threshold    = 10.0;
const float freeze_threshold  =  2.0;
const float freeze_hysteresis =  1.0;

void initTemperatureSensors()
{
  // Connect to sensors
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  //pinMode(ONE_WIRE_BUS, INPUT);
  sensors.begin();

  sensors.requestTemperatures();
  for (int i = HISTORY_LENGTH-1; i > 0; i--)
  {
     temperatures.temp_vor[i]   = sensors.getTempCByIndex(0);
     temperatures.temp_rueck[i] = sensors.getTempCByIndex(1);
  }

}

void fireTendency()
{
     // Tendency
     float temp_tendency = 0.0;
     temp_tendency = (temperatures.temp_rueck[0] - temperatures.temp_rueck[HISTORY_LENGTH-1]) / (HISTORY_LENGTH-1);

     // Publish value to MQTT
     sprintf(mqtt_message, "{\"tendency\": %.1f}", temp_tendency);
     mqtt_client.publish(temperature_state_topic, mqtt_message, true);

     // Debug
     sprintf(mqtt_message, "Temp_0: %.1f, Temp_5: %.1f, Tendency: %.4f", temperatures.temp_rueck[0], temperatures.temp_rueck[HISTORY_LENGTH-1], temp_tendency);
     mqtt_client.publish("hotpot/debug", mqtt_message);
}

void updateTemperaturesFromSensor()
{
     // Get temperature updates
     sensors.requestTemperatures();
     for (int i = HISTORY_LENGTH-1; i > 0; i--)
     {
        temperatures.temp_vor[i]   = temperatures.temp_vor[i-1];
        temperatures.temp_rueck[i] = temperatures.temp_rueck[i-1];
     }
     temperatures.temp_vor[0]   = sensors.getTempCByIndex(0);
     temperatures.temp_rueck[0] = sensors.getTempCByIndex(1);

     fireTendency();
}

void updateTemperaturesToMQTT()
{
     float vor_change = 0.0, rueck_change = 0.0;
     vor_change    = temperatures.temp_vor[0]   - temperatures.temp_vor_last_transmitted;
     rueck_change  = temperatures.temp_rueck[0] - temperatures.temp_rueck_last_transmitted;

     // Update Temperatures if significant change or after fixed time interval
     if (( (fabs(vor_change) >= temperatures.temperature_change_threshold) || (fabs(rueck_change) >= temperatures.temperature_change_threshold) ) && ( temperatures.temp_rueck[0] != 85.0 ) && ( temperatures.temp_vor[0] != 85.0) && ( temperatures.temp_rueck[0] != -127.0 ) && ( temperatures.temp_vor[0] != -127.0))
     {
        // Calculate difference
        temperatures.temp_difference = temperatures.temp_rueck[0] - temperatures.temp_vor[0];

        // Publish temperatures
        sprintf(mqtt_message, "{\"vorlauf\": %.1f, \"ruecklauf\": %.1f , \"difference\": %.1f}", temperatures.temp_vor[0], temperatures.temp_rueck[0], temperatures.temp_difference);
        mqtt_client.publish(temperature_state_topic, mqtt_message);
        temperatures.temp_vor_last_transmitted   = temperatures.temp_vor[0];
        temperatures.temp_rueck_last_transmitted = temperatures.temp_rueck[0];

        // Publish Fire state
        temperatures.fire = temperatures.temp_difference > fire_threshold;
        updateBinarysensor(fire_state_topic, temperatures.fire);

        // Publish Frost state
        if (temperatures.frost)
        {
           if ((temperatures.temp_rueck[0] > (freeze_threshold + freeze_hysteresis)) & (temperatures.temp_vor[0] > (freeze_threshold + freeze_hysteresis)))
	   {
               temperatures.frost = false;
           }
           updateBinarysensor(freeze_state_topic, temperatures.frost);
        }
        else
        {
           if ((temperatures.temp_rueck[0] < freeze_threshold) | (temperatures.temp_vor[0] < freeze_threshold))
	   {
               temperatures.frost = true;
           }
           updateBinarysensor(freeze_state_topic, temperatures.frost);
        }

     }

}

void setTemperatureChangeThreshold(float newThreshold)
{
      temperatures.temperature_change_threshold = newThreshold;
}
