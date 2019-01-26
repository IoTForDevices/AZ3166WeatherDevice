#ifndef IOTHUBMESSAGEHANDLING_H
#define IOTHUBMESSAGEHANDLING_H

typedef enum {
    NORMAL_EVENT,
    WARNING_EVENT,
    ERROR_EVENT
} IOTC_EVENT_TYPE;

typedef struct {
    int measureInterval;                // Different Intervals
    int sendInterval;
    int sleepInterval;
    int warmingUpTime;
    int upTime;                         // Device uptime in seconds
    int mImsec;                         // For internal use only - conversion of all intervals into msec
    int sImsec;
    int wUTmsec;
    int dSmsec;                         // Device sleep time in msec
    int temperatureAlert;               // High temperature alert value
    float temperatureAccuracy;          // Different sensor reading accuracies
    float pressureAccuracy;
    float humidityAccuracy;
    int maxDeltaBetweenMeasurements;    // Maximum value that two sensor readings can change without considering it to be false readings
    float temperatureCorrection;        // Sensor reading correction factors
    float pressureCorrection;
    float humidityCorrection;
    bool enableHumidityReading;         // Flags to allow suppressing sending of Humidity / Pressure sensor readings
    bool enablePressureReading;
} DEVICE_SETTINGS;

bool CreateTelemetryMessage(char *payload, bool forceCreate, DEVICE_SETTINGS *pDeviceSettings);
void CreateEventMsg(char* payLoad, IOTC_EVENT_TYPE eventType);
bool SendDeviceInfo(DEVICE_SETTINGS *pDeviceSettings);
bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message, DEVICE_SETTINGS *pDeviceSettings);

#endif // IOTHUBMESSAGEHANDLING_H