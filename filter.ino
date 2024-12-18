// #include <Preferences.h>

#include "filter.h"

// Flash
// Preferences preferences;

#define FILTER_SWITCH_STATE_TOPIC "hotpot/filter/switch"

filter_t filter;

void initFilter()
{
  // preferences.begin("filter", false);
     filter.state           = false; //preferences.getBool("state", false);
     filter.onTime          = getTime(); //preferences.getULong64("onTime", 111);
     filter.offTime         = getTime(); //preferences.getULong64("offTime", 111);
     filter.intervalHours   = 6;
     filter.durationMinutes = 30;
  // preferences.end();

  char* times = (char*)malloc(50);  // Max 10 digits for 32-bit int, plus null terminator
  sprintf(times, "Init: state: %d, onTime: %u offTime: %u", filter.state, filter.onTime, filter.offTime);
  mqtt_client.publish("hotpot/debug", times);

  pinMode(FILTER_RELAY, OUTPUT);

}


void switchFilter(bool state)
{
  // preferences.begin("filter", false);
  if(state)
  {
    if (!(filter.state))
    {
      //Turn filter on
      filter.onTime = getTime();
      // preferences.putULong64("onTime", filter.onTime);
      // preferences.putBool("state", true);
      digitalWrite(FILTER_RELAY, HIGH);
      filter.state = true;
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "ON");
    }
  }
  else
  {
    if (filter.state)
    {
      // Turn filter off
      filter.offTime = getTime();
      // preferences.putULong64("offTime", filter.offTime);
      // preferences.putBool("state", false);
      digitalWrite(FILTER_RELAY, LOW);
      filter.state = false;
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "OFF");
    }
  }
  // preferences.end();
}

void filterControl()
{
    char* times = (char*)malloc(50);
    sprintf(times, "Filter control: state: %d, onTime: %u offTime: %u", filter.state, filter.onTime, filter.offTime);
    mqtt_client.publish("hotpot/debug", times);

    if (filter.state)
    {
      // Turn off after fixed time
      unsigned int onForMinutes = ((getTime() - filter.onTime)/60);
      sprintf(times, "onForMinutes: %u", onForMinutes);
      mqtt_client.publish("hotpot/debug", times);
      if (onForMinutes >= filter.durationMinutes)
      {
        switchFilter(false);
      }
   }
   else
   {
      // Turn on after fixed time
      unsigned int offForHours = ((getTime() - filter.offTime)/60/60);
      sprintf(times, "offForHours: %u", offForHours);
      mqtt_client.publish("hotpot/debug", times);

      if (offForHours >= filter.intervalHours)
      {
        switchFilter(true);
      }
  }

}

void updateFilterInterval(unsigned int newInterval)
{
  char mqtt_message[512];
  sprintf(mqtt_message, "{\"filter_interval\": %d}", newInterval);
  mqtt_client.publish(temperature_state_topic, mqtt_message);
}

