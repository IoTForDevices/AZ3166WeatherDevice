#ifndef AZ3166LEDS_H
#define AZ3166LEDS_H

#include "IoTHubMessageHandling.h"

void ShowTelemetryData(float temperature, float humidity, float pressure, DEVICE_SETTINGS *pDeviceSettings);
void BlinkLED();
void BlinkSendConfirmation();

#endif  // AZ3166LEDS_H