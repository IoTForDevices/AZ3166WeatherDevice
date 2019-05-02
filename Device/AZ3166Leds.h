#ifndef AZ3166LEDS_H
#define AZ3166LEDS_H

#include "IoTHubMessageHandling.h"

void ShowTelemetryData(float temperature, float humidity, float pressure, bool showHumidity, bool showPressure);
void BlinkLED();
void BlinkSendConfirmation();

#endif  // AZ3166LEDS_H