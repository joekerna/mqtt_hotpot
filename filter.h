#ifndef FILTER_H
#define FILTER_H

#include "helper.h"

// Filter pump relay
#define FILTER_RELAY 16


// // MQTT topics
// const char *filter_interval_topic   = "homeassistant/binary_sensor/sensor_filter_interval_hotpot/state";

enum mode_e {
  manual,
  automatically
};

typedef struct {
   bool          state;
   enum mode_e   mode;
   unsigned long onTime;
   unsigned long offTime;
   unsigned int  intervalHours;
   unsigned int  durationMinutes;
} filter_t;

filter_t filter;

void switchFilter(bool state);
void updateFilterInterval(unsigned int newInterval);
void updateFilterDuration(unsigned int newDuration);
void publishFilterState(bool state);


#endif
