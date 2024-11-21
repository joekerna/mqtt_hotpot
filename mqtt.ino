#include <PubSubClient.h>
#include "mqtt.h"
#include "filter.h"

void mqtt_callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Received MQTT message: ");
  Serial.print(topic);
  Serial.print(": ");
  for (int i = 0; i < length; i++)
  {
      Serial.print((char)payload[i] );
  }
  Serial.println("");


  if (strcmp(topic,"hotpot/temp_threshold")==0)
  {
     Serial.print("Updating temperature threshold to ");
     temperature_change_threshold = (float)atof((const char *)(payload));
     Serial.println(temperature_change_threshold);
  }
  else if (strcmp(topic,"hotpot/filter_duration")==0)
  {
     Serial.print("Updating filter duration to ");
    //  temperature_change_threshold = (float)atof((const char *)(payload));
     Serial.println((const char*)payload);
  }
  else if (strcmp(topic,"hotpot/filter")==0)
  {
    if (payload[0] == '1')
    {
       switchFilter(true);
    }
    else
    {
       switchFilter(false);
    }
  }

}
