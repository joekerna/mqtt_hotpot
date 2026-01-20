#include "helper.h"
// Function that gets current epoch time
unsigned long getTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
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

