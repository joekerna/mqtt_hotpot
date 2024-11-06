#include "filter.h"

void switchFilter(bool state)
{
  if(state)
  {
    //Turn filter on
    Serial.println("Turning filter on");
    filterOnSince = millis();
    //updateBinarysensor(filter_state_topic, true);
  }
  else
  {
    // Turn filter off
    Serial.println("Turning filter off");
    filterOffSince = millis();
    //updateBinarysensor(filter_state_topic, false);
  }
}
