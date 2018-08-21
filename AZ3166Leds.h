#ifndef AZ3166LEDS_H
#define AZ3166LEDS_H

void ShowTelemetryData(float temperature, float humidity, float pressure);
void BlinkLED();
void BlinkSendConfirmation();

#endif  // AZ3166LEDS_H