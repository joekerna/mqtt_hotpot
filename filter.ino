#include <Preferences.h>

#include "filter.h"

// Flash
Preferences preferences;

#define FILTER_SWITCH_STATE_TOPIC "hotpot/filter/switch"

filter_t filter;

void initFilter()
{
  preferences.begin("filter", false);
     filter.state           = preferences.getBool("state", false); //false;
     filter.onTime          = preferences.getULong64("onTime", 111); //getTime(); //
     filter.offTime         = preferences.getULong64("offTime", 111); //getTime(); 
     filter.intervalHours   = 6;
     filter.durationMinutes = 30;
  preferences.end();
  
  filter.mode = automatically;

  // char* times = (char*)malloc(50);  // Max 10 digits for 32-bit int, plus null terminator
  sprintf(mqtt_message, "Init: state: %d, onTime: %u offTime: %u", filter.state, filter.onTime, filter.offTime);
  mqtt_client.publish("hotpot/debug", mqtt_message);

  pinMode(FILTER_RELAY, OUTPUT);

}


void switchFilter(bool state)
{
  preferences.begin("filter", false);
  if(state)
  {
    if (!(filter.state))
    {
      //Turn filter on
      filter.onTime = getTime();
      preferences.putULong64("onTime", filter.onTime);
      preferences.putBool("state", true);
      digitalWrite(FILTER_RELAY, HIGH);
      filter.state = true;
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "ON", true);
    }
  }
  else
  {
    if (filter.state)
    {
      // Turn filter off
      filter.offTime = getTime();
      preferences.putULong64("offTime", filter.offTime);
      preferences.putBool("state", false);
      digitalWrite(FILTER_RELAY, LOW);
      filter.state = false;
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "OFF", true);
    }
  }
  preferences.end();
}

void filterControl()
{
    // char* times = (char*)malloc(200);
    sprintf(mqtt_message, "Filter control: state: %d, onTime: %u offTime: %u interval: %u duration %u", filter.state, filter.onTime, filter.offTime, filter.intervalHours, filter.durationMinutes);
    mqtt_client.publish("hotpot/debug", mqtt_message);

    if (filter.state)
    {
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "ON");
      // Turn off after fixed time
      unsigned int onForMinutes = ((getTime() - filter.onTime)/60);
      sprintf(mqtt_message, "onForMinutes: %u", onForMinutes);
      mqtt_client.publish("hotpot/debug", mqtt_message);
      if ((onForMinutes >= filter.durationMinutes) && (!(temperatures.frost)) && (filter.mode == automatically))
      {
        switchFilter(false);
      }
   }
   else
   {
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "OFF");
      // Turn on after fixed time
      unsigned int offForHours = ((getTime() - filter.offTime)/60/60);
      //sprintf(mqtt_message, "offForHours: %u", offForHours);
      //mqtt_client.publish("hotpot/debug", mqtt_message);

      if ((offForHours >= filter.intervalHours) || (temperatures.frost))
      {
        switchFilter(true);
      }
  }

}

void updateFilterInterval(unsigned int newInterval)
{
  sprintf(mqtt_message, "{\"filter_interval\": %d}", newInterval);
  mqtt_client.publish(temperature_state_topic, mqtt_message);
  filter.intervalHours = newInterval;
}

void updateFilterDuration(unsigned int newDuration)
{
  sprintf(mqtt_message, "{\"filter_duration\": %d}", newDuration);
  mqtt_client.publish(temperature_state_topic, mqtt_message);
  filter.durationMinutes = newDuration;
}

