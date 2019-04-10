#ifndef IOTHUBMESSAGEHANDLING_H
#define IOTHUBMESSAGEHANDLING_H

typedef enum {
    MOTION_EVENT
} IOTC_EVENT_TYPE;

typedef enum {
    VALUE_INT,
    VALUE_BOOL,
    VALUE_FLOAT,
    VALUE_STRING
} DEVICE_DATA_TYPE;

typedef struct {
    int measureInterval;                // Different Intervals
    int sendInterval;
    int sleepInterval;
    int warmingUpTime;
    int motionDetectionInterval;
    int upTime;                         // Device uptime in seconds
    int mImsec;                         // For internal use only - conversion of all intervals into msec
    int sImsec;
    int wUTmsec;
    int dSmsec;                         // Device sleep time in msec
    int motionInMsec;
    int temperatureAlert;               // High temperature alert value
    float temperatureAccuracy;          // Different sensor reading accuracies
    float pressureAccuracy;
    float humidityAccuracy;
    int maxDeltaBetweenMeasurements;    // Maximum value that two sensor readings can change without considering it to be false readings
    float temperatureCorrection;        // Sensor reading correction factors
    float pressureCorrection;
    float humidityCorrection;
    int motionSensitivity;              // Number to indicate when motion detected between two accelerator readings
    bool enableHumidityReading;         // Flags to allow suppressing sending of Humidity / Pressure sensor readings
    bool enablePressureReading;
    bool enableMotionDetection;
} DEVICE_SETTINGS;

typedef struct {
    char *pszCurrentFwVersion;
    char *pszDeviceModel;
    char *pszLocation;
} DEVICE_PROPERTIES;

typedef struct {
    DEVICE_DATA_TYPE twinDataType;
    char *pszName;
} DEVICE_ENTRIES;

typedef union { 
    float fValue;
    int   iValue;
    bool  bValue;
    char *pszValue;
} DEVICE_DATA;

// Indices into DEVICE_TWIN_ENTRIES and DEVICE_TWIN_DATA
#define IDX_MEASUREINTERVAL             0
#define IDX_SENDINTERVAL                1
#define IDX_WARMINGUPTIME               2
#define IDX_MOTIONDETECTIONINTERVAL     3
#define IDX_SLEEPTIME                   4
#define IDX_TEMPERATUREALERT            5
#define IDX_TEMPERATUREACCURACY         6
#define IDX_HUMIDITYACCURACY            7
#define IDX_PRESSUREACCURACY            8
#define IDX_MAXDELTABETWEENMEASUREMENTS 9
#define IDX_TEMPERATURECORRECTION       10
#define IDX_HUMIDITYCORRECTION          11
#define IDX_PRESSURECORRECTION          12
#define IDX_READHUMIDITY                13
#define IDX_READPRESSURE                14
#define IDX_ENABLEMOTIONDETECTION       15
#define IDX_MOTIONSENSITIVITY           16
#define IDX_CURRENTFWVERSION            17
#define IDX_DEVICEMODEL                 18
#define IDX_DEVICELOCATION              19
#define IDX_DEBUGMASK                   20

#define IDX_TEMPERATURE_TELEMETRY       0
#define IDX_HUMIDITY_TELEMETRY          1
#define IDX_PRESSURE_TELEMETRY          2
#define IDX_UPTIME_TELEMETRY            3

bool CreateTelemetryMessage(char *payload, bool forceCreate);
void CreateEventMsg(char* payLoad, IOTC_EVENT_TYPE eventType);
bool SendDeviceInfo();
bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message);
bool InitialDeviceTwinDesiredReceived();
int  MeasurementInterval();
int  SleepInterval();
int  MotionInterval();
int  WarmingUpTime();
int  MotionSensitivity();
bool MotionDetectionEnabled();
bool UpdateReportedValues();

#endif // IOTHUBMESSAGEHANDLING_H