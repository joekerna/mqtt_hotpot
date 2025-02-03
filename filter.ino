<<<<<<< HEAD
#include <Preferences.h>
=======
// #include <Preferences.h>
>>>>>>> refs/remotes/origin/main

#include "filter.h"

// Flash
<<<<<<< HEAD
Preferences preferences;
=======
// Preferences preferences;
>>>>>>> refs/remotes/origin/main

#define FILTER_SWITCH_STATE_TOPIC "hotpot/filter/switch"

filter_t filter;

void initFilter()
{
<<<<<<< HEAD
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
=======
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
>>>>>>> refs/remotes/origin/main

  pinMode(FILTER_RELAY, OUTPUT);

}


void switchFilter(bool state)
{
<<<<<<< HEAD
  preferences.begin("filter", false);
=======
  // preferences.begin("filter", false);
>>>>>>> refs/remotes/origin/main
  if(state)
  {
    if (!(filter.state))
    {
      //Turn filter on
      filter.onTime = getTime();
<<<<<<< HEAD
      preferences.putULong64("onTime", filter.onTime);
      preferences.putBool("state", true);
      digitalWrite(FILTER_RELAY, HIGH);
      filter.state = true;
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "ON", true);
=======
      // preferences.putULong64("onTime", filter.onTime);
      // preferences.putBool("state", true);
      digitalWrite(FILTER_RELAY, HIGH);
      filter.state = true;
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "ON");
>>>>>>> refs/remotes/origin/main
    }
  }
  else
  {
    if (filter.state)
    {
      // Turn filter off
      filter.offTime = getTime();
<<<<<<< HEAD
      preferences.putULong64("offTime", filter.offTime);
      preferences.putBool("state", false);
      digitalWrite(FILTER_RELAY, LOW);
      filter.state = false;
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "OFF", true);
    }
  }
  preferences.end();
=======
      // preferences.putULong64("offTime", filter.offTime);
      // preferences.putBool("state", false);
      digitalWrite(FILTER_RELAY, LOW);
      filter.state = false;
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "OFF");
    }
  }
  // preferences.end();
>>>>>>> refs/remotes/origin/main
}

void filterControl()
{
<<<<<<< HEAD
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
=======
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
>>>>>>> refs/remotes/origin/main
      {
        switchFilter(false);
      }
   }
   else
   {
<<<<<<< HEAD
      mqtt_client.publish(FILTER_SWITCH_STATE_TOPIC, "OFF");
      // Turn on after fixed time
      unsigned int offForHours = ((getTime() - filter.offTime)/60/60);
      //sprintf(mqtt_message, "offForHours: %u", offForHours);
      //mqtt_client.publish("hotpot/debug", mqtt_message);

      if ((offForHours >= filter.intervalHours) || (temperatures.frost))
=======
      // Turn on after fixed time
      unsigned int offForHours = ((getTime() - filter.offTime)/60/60);
      sprintf(times, "offForHours: %u", offForHours);
      mqtt_client.publish("hotpot/debug", times);

      if (offForHours >= filter.intervalHours)
>>>>>>> refs/remotes/origin/main
      {
        switchFilter(true);
      }
  }

}

void updateFilterInterval(unsigned int newInterval)
{
<<<<<<< HEAD
  sprintf(mqtt_message, "{\"filter_interval\": %d}", newInterval);
  mqtt_client.publish(temperature_state_topic, mqtt_message);
  filter.intervalHours = newInterval;
}

void updateFilterDuration(unsigned int newDuration)
{
  sprintf(mqtt_message, "{\"filter_duration\": %d}", newDuration);
  mqtt_client.publish(temperature_state_topic, mqtt_message);
  filter.durationMinutes = newDuration;
=======
  char mqtt_message[512];
  sprintf(mqtt_message, "{\"filter_interval\": %d}", newInterval);
  mqtt_client.publish(temperature_state_topic, mqtt_message);
>>>>>>> refs/remotes/origin/main
}

