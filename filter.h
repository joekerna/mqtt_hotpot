#ifndef FILTER_H
#define FILTER_H

#include "helper.h"

// Filter pump relay
#define FILTER_RELAY 16

// // MQTT topics
// const char *filter_interval_topic   = "homeassistant/binary_sensor/sensor_filter_interval_hotpot/state";

typedef struct {
   bool          state;
   unsigned long onTime;
   unsigned long offTime;
   unsigned int  intervalHours;
   unsigned int  durationMinutes;
} filter_t;

void switchFilter(bool state);
void updateFilterInterval(unsigned int newInterval);



#endif
