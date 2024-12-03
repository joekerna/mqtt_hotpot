#ifndef TEMPERATURES_H
#define TEMPERATURES_H
#include <OneWire.h>
#include <DallasTemperature.h>

// Onewire Temperature sensors
// #define ONE_WIRE_BUS D4  // ESP8266
#define ONE_WIRE_BUS 15 // ESP32

#define UPDATE_RATE_SECONDS 5
#define HISTORY_LENGTH 6


typedef struct {
   float temp_vor[HISTORY_LENGTH] = { };
   float temp_rueck[HISTORY_LENGTH] = { };
   float temp_difference = -5.0;
   float temp_vor_last_transmitted;
   float temp_rueck_last_transmitted;
   bool  fire  = false;
   bool  frost = false;
} temperatures_t;


void initTemperatureSensors();
void updateTemperaturesFromSensor();
void updateTemperaturesToMQTT();
void setTemperatureChangeThreshold(float newThreshold);

#endif
