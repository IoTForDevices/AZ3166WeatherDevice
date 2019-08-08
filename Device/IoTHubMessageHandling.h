#ifndef IOTHUBMESSAGEHANDLING_H
#define IOTHUBMESSAGEHANDLING_H

typedef enum {
    MOTION_EVENT
} IOTC_EVENT_TYPE;

typedef enum {
    VALUE_INT,
    VALUE_ULONG,
    VALUE_BOOL,
    VALUE_FLOAT,
    VALUE_STRING
} DEVICE_DATA_TYPE;

typedef struct {
    DEVICE_DATA_TYPE twinDataType;
    char const *pszName;
} DEVICE_ENTRIES;

typedef union { 
    float         fValue;
    int           iValue;
    uint64_t      ulValue;
    bool          bValue;
    char         *pszValue;
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
#define IDX_TEMPERATURECORRECTION       9
#define IDX_HUMIDITYCORRECTION          10
#define IDX_PRESSURECORRECTION          11
#define IDX_READHUMIDITY                12
#define IDX_READPRESSURE                13
#define IDX_ENABLEMOTIONDETECTION       14
#define IDX_MOTIONSENSITIVITY           15
#define IDX_CURRENTFWVERSION            16
#define IDX_DEVICEMODEL                 17
#define IDX_DEVICELOCATION              18
#define IDX_DEBUGMASK                   19

#define IDX_TEMPERATURE_TELEMETRY       0
#define IDX_HUMIDITY_TELEMETRY          1
#define IDX_PRESSURE_TELEMETRY          2
#define IDX_UPTIME_TELEMETRY            3

//bool CreateTelemetryMessage(char *payload, bool forceCreate);
bool CreateTelemetryMessage(char *payload, uint64_t upTime, uint64_t sensorReads, uint64_t telemetrySend, bool forceCreate);
void CreateEventMsg(char* payLoad, IOTC_EVENT_TYPE eventType);
bool SendDeviceInfo();
bool ParseTwinMessage(DEVICE_TWIN_UPDATE_STATE updateState, const char *message, int msgLength);
bool InitialDeviceTwinDesiredReceived();
int  MeasurementInterval();
int  SendInterval();
int  SleepInterval();
int  MotionInterval();
int  WarmingUpTime();
int  MotionSensitivity();
bool MotionDetectionEnabled();
bool UpdateReportedValues();
char *CurrentFWVersion();
char *CurrentDevice();

#endif // IOTHUBMESSAGEHANDLING_H