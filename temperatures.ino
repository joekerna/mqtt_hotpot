#include "temperatures.h"
#include "filter.h"

// Flash Preferences
Preferences preferences_temperatures;

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
  for (int i = 0; i < HISTORY_LENGTH; i++)
  {
     temperatures.temp_vor[i]   = sensors.getTempCByIndex(0);
     temperatures.temp_rueck[i] = sensors.getTempCByIndex(1);
  }

}

void fireTendency()
{
     // if (!(filter.state))
     {
	     // Tendency
	     float temp_tendency = 0.0;
	     temp_tendency = (temperatures.temp_rueck[0] - temperatures.temp_rueck[HISTORY_LENGTH-1]) / (HISTORY_LENGTH-1);

	     // Publish value to MQTT
	     sprintf(mqtt_message, "{\"tendency\": %.2f}", temp_tendency);
	     mqtt_client.publish(temperature_state_topic, mqtt_message, true);

	     // Debug
	     sprintf(mqtt_message, "Temp_0: %.1f, Temp_5: %.1f, Tendency: %.4f", temperatures.temp_rueck[0], temperatures.temp_rueck[HISTORY_LENGTH-1], temp_tendency);
	     mqtt_client.publish(debug_topic, mqtt_message);
     }
}

void updateTemperaturesFromSensor()
{
   float newTempVor;
   float newTempRueck;
   for (int i = 0; i < 10; i++)
   {
      // Get temperature updates
      sensors.requestTemperatures();
      newTempVor   = sensors.getTempCByIndex(0);
      newTempRueck = sensors.getTempCByIndex(1);
      sprintf(mqtt_message, "0: %.2f  1: %.2f", newTempVor, newTempRueck);
      mqtt_client.publish("hotpot/temperature", mqtt_message);

      // Errors?
      if ( ((newTempVor != 85.0) && (newTempRueck != 85.0)) && ((newTempVor != -127.0) && (newTempRueck != -127.0)))
      {
         break;
      }
      else
      {
         sprintf(mqtt_message, "Error in temperature");
         mqtt_client.publish(error_topic, mqtt_message);
         if ( i == 10-1)
         {
            newTempRueck = temperatures.temp_rueck[0];
            newTempVor   = temperatures.temp_vor[0];
         }
      }
   }

   for (int i = HISTORY_LENGTH-1; i > 0; i--)
   {
      temperatures.temp_vor[i]   = temperatures.temp_vor[i-1];
      temperatures.temp_rueck[i] = temperatures.temp_rueck[i-1];
   }
   temperatures.temp_vor[0]   = newTempVor;
   temperatures.temp_rueck[0] = newTempRueck;

}

float lowpass(float *temperatures)
{
   float filtered = 0.0;

   for (int i = 0; i < HISTORY_LENGTH; i++)
   {
       filtered += temperatures[i];
   }
   filtered /= HISTORY_LENGTH;

   return filtered;
}

void updateTemperaturesToMQTT()
{
     float temp_vor   = lowpass(temperatures.temp_vor);
     float temp_rueck = lowpass(temperatures.temp_rueck);

     float vor_change    = temp_vor   - temperatures.temp_vor_last_transmitted;
     float rueck_change  = temp_rueck - temperatures.temp_rueck_last_transmitted;

     // Update Temperatures if significant change or after fixed time interval
     if ( (fabs(vor_change)   >= temperatures.temperature_change_threshold) ||
          (fabs(rueck_change) >= temperatures.temperature_change_threshold)
        ) 
     {
        // Calculate difference
        temperatures.temp_difference = temp_rueck - temp_vor;
    

        // Publish temperatures
        sprintf(mqtt_message, "{\"vorlauf\": %.1f, \"ruecklauf\": %.1f , \"difference\": %.1f}", temp_vor, temp_rueck, temperatures.temp_difference);
        mqtt_client.publish(temperature_state_topic, mqtt_message);
        temperatures.temp_vor_last_transmitted   = temp_vor;
        temperatures.temp_rueck_last_transmitted = temp_rueck;

        // Publish Fire state
        temperatures.fire = temperatures.temp_difference > fire_threshold;
        updateBinarysensor(fire_state_topic, temperatures.fire);

        // Update configuration
        sprintf(mqtt_message, "{\"filter_interval\": %.1f, \"filter_duration\": %.1f}", filter.intervalHours, filter.durationMinutes);
        mqtt_client.publish(temperature_state_topic, mqtt_message);
        
   
        // Calculate tendency
        fireTendency();

        // Publish Frost state
        if (temperatures.frost)
        {
           if ((temp_rueck > (freeze_threshold + freeze_hysteresis)) & (temp_vor > (freeze_threshold + freeze_hysteresis)))
	   {
               temperatures.frost = false;
           }
           updateBinarysensor(freeze_state_topic, temperatures.frost);
        }
        else
        {
           if ((temp_rueck < freeze_threshold) | (temp_vor < freeze_threshold))
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

void setUpdateRate(unsigned int newUpdateRate)
{
   sprintf(mqtt_message, "New Update Rate: %d", newUpdateRate);
   mqtt_client.publish(debug_topic, mqtt_message);
   temperatures.update_rate = newUpdateRate;
}
