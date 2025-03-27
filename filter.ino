#include "filter.h"

// Flash Preferences
Preferences preferences_filter;



void initFilter()
{
  preferences_filter.begin("filter", false);
     filter.state           = preferences_filter.getBool("state", false); //false;
     filter.onTime          = preferences_filter.getULong64("onTime", 1740676217); //getTime(); //
     filter.offTime         = preferences_filter.getULong64("offTime", 1740676217); //getTime(); 
     filter.intervalHours   = preferences_filter.getUInt("filterInterval",  6);
     filter.durationMinutes = preferences_filter.getUInt("filterDuration", 30);
  preferences_filter.end();
  
  filter.mode = automatically;

  sprintf(mqtt_message, "Init: state: %d, onTime: %lu offTime: %lu", filter.state, filter.onTime, filter.offTime);
  mqtt_client.publish(debug_topic, mqtt_message);

  pinMode(FILTER_RELAY, OUTPUT);

}

void publishFilterState(bool state)
{
    if (filter.state)
    {
	mqtt_client.publish(filter_switch_state_topic, "ON", true);
    }
    else
    {
        mqtt_client.publish(filter_switch_state_topic, "OFF", true);
    }
}

void switchFilter(bool state)
{
  preferences_filter.begin("filter", false);
  if(state)
  {
    if (!(filter.state))
    {
      //Turn filter on
      filter.onTime = getTime();
      preferences_filter.putULong64("onTime", filter.onTime);
      preferences_filter.putBool("state", true);
      digitalWrite(FILTER_RELAY, HIGH);
      filter.state = true;
      publishFilterState(filter.state);
    }
  }
  else
  {
    if (filter.state)
    {
      // Turn filter off
      filter.offTime = getTime();
      preferences_filter.putULong64("offTime", filter.offTime);
      preferences_filter.putBool("state", false);
      digitalWrite(FILTER_RELAY, LOW);
      filter.state = false;
      publishFilterState(filter.state);
    }
  }
  preferences_filter.end();
}

void filterControl()
{
    sprintf(mqtt_message, "Filter control: state: %d, onTime: %lu offTime: %lu interval: %u duration %u", filter.state, filter.onTime, filter.offTime, filter.intervalHours, filter.durationMinutes);
    mqtt_client.publish(debug_topic, mqtt_message);

    publishFilterState(filter.state);
    if (filter.state)
    {
      // Turn off after fixed time and if no longer freezing
      if (((unsigned int)((getTime() - filter.onTime)/60) >= filter.durationMinutes) &&
          (!(temperatures.frost))                                                    &&
          (filter.mode == automatically))
      {
        switchFilter(false);
      }
   }
   else
   {
      // Turn on after fixed time or if thread of frost exists
      if (((unsigned int)((getTime() - filter.offTime)/60/60) >= filter.intervalHours) ||
          (temperatures.frost))
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

  // Store new value to flash
  preferences_filter.begin("filter", false);
      preferences_filter.putUInt("filterInterval", filter.intervalHours);
  preferences_filter.end();
}

void updateFilterDuration(unsigned int newDuration)
{
  sprintf(mqtt_message, "{\"filter_duration\": %d}", newDuration);
  mqtt_client.publish(temperature_state_topic, mqtt_message);
  filter.durationMinutes = newDuration;

  // Store new value to flash
  preferences_filter.begin("filter", false);
      preferences_filter.putUInt("filterDuration", filter.durationMinutes);
  preferences_filter.end();
}

