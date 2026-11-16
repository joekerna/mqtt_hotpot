#include "temperatures.h"
#include "filter.h"

// Flash Preferences
Preferences preferences_temperatures;

// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass OneWire reference to Dallas Temperature DallasTemperature
DallasTemperature sensors(&oneWire);



const float fire_threshold    = 10.0;
const float freeze_threshold  =  1.5;
const float freeze_hysteresis =  0.7;

// Maximum conversion attempts per reading (1 + 1 retry)
#define TEMPERATURE_MAX_ATTEMPTS 2

void initTemperatureSensors()
{
  // Connect to sensors
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
  //pinMode(ONE_WIRE_BUS, INPUT);
  sensors.begin();

  // Run conversions non-blocking. The history stays empty until the first
  // valid reading arrives (see completeTemperatureReading()).
  // From here on run conversions non-blocking
  // (see startTemperatureReading() / completeTemperatureReading())
  sensors.setWaitForConversion(false);

}

void fireTendency()
{
      if (HISTORY_LENGTH <= 1) {
          return;  // Skip tendency calculation
      }
      // Tendency
      float temp_tendency = 0.0;
      temp_tendency = (temperatures.temp_rueck[0] - temperatures.temp_rueck[HISTORY_LENGTH-1]) / (HISTORY_LENGTH-1);

      // Publish value to MQTT
      snprintf(mqtt_message, sizeof(mqtt_message), "{\"tendency\": %.2f}", temp_tendency);
      // mqtt_client.publish(temperature_state_topic, mqtt_message, false);

      // Debug
      snprintf(mqtt_message, sizeof(mqtt_message), "Temp_0: %.1f, Temp_5: %.1f, Tendency: %.4f", temperatures.temp_rueck[0], temperatures.temp_rueck[HISTORY_LENGTH-1], temp_tendency);
      mqtt_client.publish(debug_topic, mqtt_message);
}

// Non-blocking conversion state
static bool          readingInProgress = false;
static unsigned long readingStartTime  = 0;
static unsigned int  readingAttempts   = 0;
static bool          readingReady      = false;
static bool          historyValid      = false;  // true after the first valid reading
static float         pendingTempVor;
static float         pendingTempRueck;

void startTemperatureReading()
{
   if (readingInProgress)
      return;  // Conversion already running, its result will be used

   readingAttempts   = 1;
   readingReady      = false;
   readingInProgress = true;
   readingStartTime  = millis();
   sensors.requestTemperatures();  // Non-blocking: returns after starting the conversion
}

bool completeTemperatureReading()
{
   if (readingReady)
   {
      readingReady = false;

      if (!historyValid)
      {
         // First valid reading after boot: fill the whole history with it,
         // so the low-pass average starts at the real temperature
         for (int i = 0; i < HISTORY_LENGTH; i++)
         {
            temperatures.temp_vor[i]   = pendingTempVor;
            temperatures.temp_rueck[i] = pendingTempRueck;
         }
         historyValid = true;
         return true;
      }

      for (int i = HISTORY_LENGTH-1; i > 0; i--)
      {
         temperatures.temp_vor[i]   = temperatures.temp_vor[i-1];
         temperatures.temp_rueck[i] = temperatures.temp_rueck[i-1];
      }
      temperatures.temp_vor[0]   = pendingTempVor;
      temperatures.temp_rueck[0] = pendingTempRueck;
      return true;
   }

   if (!readingInProgress)
      return false;

   if ((millis() - readingStartTime) < (unsigned long)sensors.millisToWaitForConversion())
      return false;  // Conversion still running

   readingInProgress = false;
   float newTempVor   = sensors.getTempCByIndex(0);
   float newTempRueck = sensors.getTempCByIndex(1);

   // Errors?
   if ( ((newTempVor != DALLAS_SENSOR_CRC_ERROR) && (newTempRueck != DALLAS_SENSOR_CRC_ERROR)) &&
        ((newTempVor != DALLAS_SENSOR_NOT_INITIALIZED) && (newTempRueck != DALLAS_SENSOR_NOT_INITIALIZED)) )
   {
      pendingTempVor   = newTempVor;
      pendingTempRueck = newTempRueck;
      readingReady     = true;
      return false;  // Consumed on the next call
   }

   // Retry once
   if (readingAttempts < TEMPERATURE_MAX_ATTEMPTS)
   {
      readingAttempts++;
      readingStartTime  = millis();
      readingInProgress = true;
      sensors.requestTemperatures();
      return false;
   }

   // All attempts failed: report once, keep last good values
   snprintf(mqtt_message, sizeof(mqtt_message), "Error in temperature");
   mqtt_client.publish(error_topic, mqtt_message);

   // No valid reading since boot: there are no "last good values" yet,
   // so publish nothing (and don't evaluate frost/fire) until one arrives
   if (!historyValid)
      return false;

   pendingTempVor   = temperatures.temp_vor[0];
   pendingTempRueck = temperatures.temp_rueck[0];
   readingReady     = true;
   return false;
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
        snprintf(mqtt_message, sizeof(mqtt_message), "%.1f", temp_vor);
        mqtt_client.publish(temperature_vorlauf_state_topic, mqtt_message, true);
        snprintf(mqtt_message, sizeof(mqtt_message), "%.1f", temp_rueck);
        mqtt_client.publish(temperature_ruecklauf_state_topic, mqtt_message, true);
        snprintf(mqtt_message, sizeof(mqtt_message), "%.1f", temperatures.temp_difference);
        mqtt_client.publish(temperature_difference_state_topic, mqtt_message, true);

        temperatures.temp_vor_last_transmitted   = temp_vor;
        temperatures.temp_rueck_last_transmitted = temp_rueck;

        // Publish Fire state
        temperatures.fire = temperatures.temp_difference > fire_threshold;
        updateBinarysensor(fire_state_topic, temperatures.fire);

   
        // Calculate tendency
        fireTendency();

        // Publish Frost state
        if (temperatures.frost)
        {
           if ((temp_rueck > (freeze_threshold + freeze_hysteresis)) && (temp_vor > (freeze_threshold + freeze_hysteresis)))
	   {
               temperatures.frost = false;
           }
           updateBinarysensor(freeze_state_topic, temperatures.frost);
        }
        else
        {
           if ((temp_rueck < freeze_threshold) || (temp_vor < freeze_threshold))
	   {
               temperatures.frost = true;
           }
           updateBinarysensor(freeze_state_topic, temperatures.frost);
        }

        // Publish hot/warm/cold status (keeps the retained value in sync
        // with the current water temperature, not only on filter edges)
        publishTemperatureStatus();

     }

}

void setTemperatureChangeThreshold(float newThreshold)
{
   if ((newThreshold <= 0.0) || (newThreshold > 10.0))
   {
      snprintf(mqtt_message, sizeof(mqtt_message), "Invalid temperature change threshold: %.2f (allowed: >0.00 - 10.00)", newThreshold);
      mqtt_client.publish(error_topic, mqtt_message);
      return;
   }
   temperatures.temperature_change_threshold = newThreshold;
}

void setUpdateRate(unsigned int newUpdateRate)
{
   if ((newUpdateRate < 1) || (newUpdateRate > 3600))
   {
      snprintf(mqtt_message, sizeof(mqtt_message), "Invalid update rate: %u (allowed: 1-3600 s)", newUpdateRate);
      mqtt_client.publish(error_topic, mqtt_message);
      return;
   }
   snprintf(mqtt_message, sizeof(mqtt_message), "New Update Rate: %u", newUpdateRate);
   mqtt_client.publish(debug_topic, mqtt_message);
   temperatures.update_rate = newUpdateRate;
}

float getTempCByIndex(int index) {
    return sensors.getTempCByIndex(index);
}

float getCurrentTempVorlauf() {
    return lowpass(temperatures.temp_vor);
}

bool temperatureHistoryValid() {
    return historyValid;
}
