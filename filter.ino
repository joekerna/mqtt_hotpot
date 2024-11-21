#include "filter.h"
unsigned long filterOnSince  = 0; 
unsigned long filterOffSince = 0;
unsigned int  filterIntervalHours   =  6;
unsigned int  filterDurationMinutes = 30;

void switchFilter(bool state)
{
  if(state)
  {
    //Turn filter on
    Serial.println("Turning filter on");
    filterOnSince = millis();
    digitalWrite(FILTER_RELAY, HIGH);
    //updateBinarysensor(filter_state_topic, true);
  }
  else
  {
    // Turn filter off
    Serial.println("Turning filter off");
    filterOffSince = millis();
    digitalWrite(FILTER_RELAY, LOW);
    //updateBinarysensor(filter_state_topic, false);
  }
}
