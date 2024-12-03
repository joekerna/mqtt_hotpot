#include "temperatures.h"


// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature DallasTemperature
DallasTemperature sensors(&oneWire);

temperatures_t temperatures;

      float temperature_change_threshold = 0.2;
const float fire_threshold               = 5.0;
const float freeze_threshold = 2.0;



// Tendency calculation
float temp_vor_avg = 0.0, temp_rueck_avg = 0.0;

void initTemperatureSensors()
{
  // Connect to sensors
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  //pinMode(ONE_WIRE_BUS, INPUT);
  sensors.begin();
}
void updateTemperaturesFromSensor()
{
     // Tendency
     temp_vor_avg   = 0.0;
     temp_rueck_avg = 0.0;
     for (int i = 0; i < HISTORY_LENGTH; i++)
     {
        temp_vor_avg   += temperatures.temp_vor[i];
        temp_rueck_avg += temperatures.temp_rueck[i];
     }
     temp_vor_avg   /= HISTORY_LENGTH;
     temp_rueck_avg /= HISTORY_LENGTH;

     // Get temperature updates
     sensors.requestTemperatures();
     for (int i = HISTORY_LENGTH-1; i > 0; i--)
     {
        temperatures.temp_vor[i]   = temperatures.temp_vor[i-1];
        temperatures.temp_rueck[i] = temperatures.temp_rueck[i-1];
     }
     temperatures.temp_vor[0]   = sensors.getTempCByIndex(0);
     temperatures.temp_rueck[0] = sensors.getTempCByIndex(1)-0.5;
     

}

void updateTemperaturesToMQTT()
{
     float vor_change = 0.0, rueck_change = 0.0;
     vor_change    = temperatures.temp_vor[0]   - temperatures.temp_vor_last_transmitted;
     rueck_change  = temperatures.temp_rueck[0] - temperatures.temp_rueck_last_transmitted;
   mqtt_client.publish("hotpot/debug", "temperature_update to mqtt");

     // Update Temperatures if significant change or after fixed time interval
     if (( (fabs(vor_change) >= temperature_change_threshold) || (fabs(rueck_change) >= temperature_change_threshold) ) && ( temperatures.temp_rueck[0] != 85.0 ) && ( temperatures.temp_vor[0] != 85.0))
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
           if ((temperatures.temp_rueck[0] > (freeze_threshold + 1.0)) & (temperatures.temp_vor[0] > (freeze_threshold + 1.0)))
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
      temperature_change_threshold = newThreshold;
}
