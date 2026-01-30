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

  // Update MQTT sensor
  snprintf(mqtt_message, sizeof(mqtt_message), "%d", filter.intervalHours);
  mqtt_client.publish(filter_interval_state_topic, mqtt_message, true);
  snprintf(mqtt_message, sizeof(mqtt_message), "%d", filter.durationMinutes);
  mqtt_client.publish(filter_duration_state_topic, mqtt_message, true);
  
  changeFilterMode(automatically);

  sprintf(mqtt_message, "Init: state: %d, onTime: %lu offTime: %lu", filter.state, filter.onTime, filter.offTime);
  mqtt_client.publish(debug_topic, mqtt_message);

  pinMode(FILTER_RELAY, OUTPUT);

}

void publishFilterState(bool state)
{
    if (state)
    {
	mqtt_client.publish(filter_state_topic, "ON", false);
    }
    else
    {
        mqtt_client.publish(filter_state_topic, "OFF", false);
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
    unsigned long now = getTime();
    if (now == 0)
    {
       return;
    }


    publishFilterState(filter.state);
    if (filter.state)
    {
      // Turn off after fixed time and if no longer freezing
      if ( (((unsigned long)((now - filter.onTime)/60) >= (unsigned long)filter.durationMinutes) &&
           (!(temperatures.frost))                                                    &&
           (filter.mode == automatically))                                                           ||
           (filter.mode == frost) && (!(temperatures.frost))
         )
      {
	// sprintf(mqtt_message, "Turning off: 
        sprintf(mqtt_message, "Filter control: state: %d, onTime: %lu offTime: %lu interval: %u duration %u current duration: %u", filter.state, filter.onTime, filter.offTime, filter.intervalHours, filter.durationMinutes,
                    (unsigned int)((now - filter.onTime)/60));
        mqtt_client.publish(debug_topic, mqtt_message);
        changeFilterMode(automatically);
        switchFilter(false);
      }
   }
   else
   {
      // Turn on after fixed time or if thread of frost exists
      if (((unsigned long)((now - filter.offTime)/60/60) >= (unsigned long)filter.intervalHours) ||
          (temperatures.frost))
      {
        if (temperatures.frost)
        {
           changeFilterMode(frost);
        }
        switchFilter(true);
      }
  }

}

void updateFilterInterval(unsigned int newInterval)
{
  snprintf(mqtt_message, sizeof(mqtt_message), "%d", newInterval);
  mqtt_client.publish(filter_interval_state_topic, mqtt_message, true);

  filter.intervalHours = newInterval;

  // Store new value to flash
  preferences_filter.begin("filter", false);
      preferences_filter.putUInt("filterInterval", filter.intervalHours);
  preferences_filter.end();
}

void updateFilterDuration(unsigned int newDuration)
{
  snprintf(mqtt_message, sizeof(mqtt_message), "%d", newDuration);
  mqtt_client.publish(filter_duration_state_topic, mqtt_message, true);

  filter.durationMinutes = newDuration;

  // Store new value to flash
  preferences_filter.begin("filter", false);
      preferences_filter.putUInt("filterDuration", filter.durationMinutes);
  preferences_filter.end();
}

void changeFilterMode(mode_e newMode)
{
    filter.mode = newMode;
    const char *payload = "unknown";
    switch (newMode) {
      case manual: payload = "manual"; break;
      case automatically: payload = "automatically"; break;
      case frost: payload = "frost"; break;
    }
    mqtt_client.publish(filter_mode_state_topic, payload, true);
}
