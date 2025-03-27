#include "helper.h"
// Function that gets current epoch time
unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    sprintf(mqtt_message, "Time failed");
    mqtt_client.publish(debug_topic, mqtt_message, true);
    return(0);
  }
  time(&now);

  char buffer[80];
  strftime(buffer, 80, "%H:%M:%S", (const tm*)(&timeinfo));
  sprintf(mqtt_message, buffer);
  mqtt_client.publish(debug_topic, mqtt_message);
  return now;
}

void setTimezone(String timezone)
{
  setenv("TZ",timezone.c_str(),1);
  tzset(); 
}

