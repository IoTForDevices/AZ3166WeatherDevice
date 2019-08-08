#ifndef AZ3166LEDS_H
#define AZ3166LEDS_H

void ShowTelemetryData(uint64_t upTime, uint64_t sensorReads, uint64_t telemetrySend, float temperature, float humidity, float pressure, bool showHumidity, bool showPressure);
void BlinkLED();
void BlinkSendConfirmation();

#endif  // AZ3166LEDS_H