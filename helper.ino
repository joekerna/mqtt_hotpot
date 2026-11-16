#include "helper.h"
// Function that gets current epoch time
unsigned long getTime()
{
  struct tm timeinfo;

  // Try the cached time first. If it is not synced yet, wait up to 1 s
  // for NTP to deliver the time before giving up.
  if (!getLocalTime(&timeinfo, 1000))
  {
    sprintf(mqtt_message, "Time failed");
    mqtt_client.publish(debug_topic, mqtt_message, false);
    return(0);
  }

  return (unsigned long)mktime(&timeinfo);
}

void setTimezone(String timezone)
{
  setenv("TZ",timezone.c_str(),1);
  tzset(); 
}

