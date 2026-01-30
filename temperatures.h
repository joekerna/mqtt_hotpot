#ifndef TEMPERATURES_H
#define TEMPERATURES_H

#include "helper.h"

#include <OneWire.h>
#include <DallasTemperature.h>

// Dallas Temperature sensor error codes
const float DALLAS_SENSOR_CRC_ERROR = 85.0;        // CRC error / invalid reading
const float DALLAS_SENSOR_NOT_INITIALIZED = -127.0; // Sensor not read or no device found

// Onewire Temperature sensors
// #define ONE_WIRE_BUS D4  // ESP8266
#define ONE_WIRE_BUS 15 // ESP32

#define HISTORY_LENGTH 15


typedef struct {
   unsigned int update_rate = 10;
   float temp_vor[HISTORY_LENGTH] = { };
   float temp_rueck[HISTORY_LENGTH] = { };
   float temp_difference = -5.0;
   float temp_vor_last_transmitted;
   float temp_rueck_last_transmitted;
   float temperature_change_threshold = 0.2;
   bool  fire  = false;
   bool  frost = false;
} temperatures_t;

temperatures_t temperatures;

void initTemperatureSensors();
void updateTemperaturesFromSensor();
void updateTemperaturesToMQTT();
void setTemperatureChangeThreshold(float newThreshold);

void setUpdateRate(unsigned int newUpdateRate);
float lowpass(float *temperatures);

void fireTendency();

#endif
