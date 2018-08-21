#ifndef IOTHUBMESSAGEHANDLING_H
#define IOTHUBMESSAGEHANDLING_H

typedef struct {
    int measureInterval;        // Different Intervals
    int sendInterval;
    int warmingUpTime;
    int mImsec;                 // For internal use only
    int sImsec;
    int wUTmsec;
    int temperatureAlert;       // High temperature alert value
    float temperatureAccuracy;  // Different sensor reading accuracies
    float pressureAccuracy;
    float humidityAccuracy;
    int maxDeltaBetweenMeasurements;
    float temperatureCorrection;
    float pressureCorrection;
    float humidityCorrection;
} DEVICE_SETTINGS;

bool CreateTelemetryMessage(int messageId, char *payload, bool forceCreate, DEVICE_SETTINGS *pDeviceSettings);
bool SendDeviceInfo(DEVICE_SETTINGS *pDeviceSettings);
bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message, DEVICE_SETTINGS *pDeviceSettings);

#endif // IOTHUBMESSAGEHANDLING_H