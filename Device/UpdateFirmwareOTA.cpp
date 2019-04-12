#include "AZ3166WiFi.h"
#include "AzureIotHub.h"
#include "DevKitMQTTClient.h"
#include "DevKitOTAUtils.h"
#include "OTAFirmwareUpdate.h"
#include "SystemTime.h"
#include "Config.h"
#include "DebugZones.h"

static char *currentFirmwareVersion = DEVICE_FIRMWARE_VERSION;

static bool hasWifi = false;

static bool enableOTA = true;
static FW_INFO fwInfo{NULL, NULL, NULL, 0};
static MAP_HANDLE OTAStatusMap = NULL;
static char *pszFwUri;
static char *pszFwVersion;
static char *pszFwCheck;
static int dFwLength;

void FreeFwInfo()
{
    if (fwInfo.fwVersion != NULL)
    {
        free(fwInfo.fwVersion);
        fwInfo.fwVersion = NULL;
    }
    if (fwInfo.fwPackageURI != NULL)
    {
        free(fwInfo.fwPackageURI);
        fwInfo.fwPackageURI = NULL;
    }
    if (fwInfo.fwPackageCheckValue != NULL)
    {
        free(fwInfo.fwPackageCheckValue);
        fwInfo.fwPackageCheckValue = NULL;
    }
    fwInfo.fwSize = 0;
}

// Report the OTA update status to Azure
void ReportOTAStatus(const char *currentFwVersion, const char *pendingFwVersion, const char *fwUpdateStatus, const char *fwUpdateSubstatus, const char *lastFwUpdateStartTime, const char *lastFwUpdateEndTime)
{
    OTAStatusMap = Map_Create(NULL);
    Map_Add(OTAStatusMap, "type", "IoTDevKit"); // The type of the firmware
    if (currentFwVersion)
    {
        Map_Add(OTAStatusMap, OTA_CURRENT_FW_VERSION, currentFwVersion);
    }
    if (pendingFwVersion)
    {
        Map_Add(OTAStatusMap, OTA_PENDING_FW_VERSION, pendingFwVersion);
    }
    if (fwUpdateStatus)
    {
        Map_Add(OTAStatusMap, OTA_FW_UPDATE_STATUS, fwUpdateStatus);
    }
    if (fwUpdateSubstatus)
    {
        Map_Add(OTAStatusMap, OTA_FW_UPDATE_SUBSTATUS, fwUpdateSubstatus);
    }
    if (lastFwUpdateStartTime)
    {
        Map_Add(OTAStatusMap, OTA_LAST_FW_UPDATE_STARTTIME, lastFwUpdateStartTime);
    }
    if (lastFwUpdateEndTime)
    {
        Map_Add(OTAStatusMap, OTA_LAST_FW_UPDATE_ENDTIME, lastFwUpdateEndTime);
    }
    IoTHubClient_ReportOTAStatus(OTAStatusMap);
    Map_Destroy(OTAStatusMap);
    DevKitMQTTClient_Check();
}

// Enter a failed state, print failed message and report status
void OTAUpdateFailed(const char *failedMsg)
{
    ReportOTAStatus(currentFirmwareVersion, fwInfo.fwVersion != NULL ? fwInfo.fwVersion : "", OTA_STATUS_ERROR, failedMsg, "", "");
    enableOTA = false;
    Screen.print(1, "OTA failed:");
    Screen.print(2, failedMsg);
    Screen.print(3, " ");
    DEBUGMSG(ZONE_ERROR, "<-- %s() - Failed to update fwVersion = %s: failedMsg = %s.", FUNC_NAME, fwInfo.fwVersion != NULL ? fwInfo.fwVersion : "<unknown>", failedMsg);
}

void SetNewFirmwareInfo(const char *pszFWVersion, const char *pszFWLocation, const char *pszFWChecksum, int fileSize)
{
    fwInfo.fwVersion = (char *)malloc(strlen_P(pszFWVersion + 1));
    if (fwInfo.fwVersion != NULL)
    {
        strcpy(fwInfo.fwVersion, pszFWVersion);
    }
    fwInfo.fwPackageURI = (char *)malloc(strlen_P(pszFWLocation + 1));
    if (fwInfo.fwPackageURI != NULL)
    {
        strcpy(fwInfo.fwPackageURI, pszFWLocation);
    }
    fwInfo.fwPackageCheckValue = (char *)malloc(strlen_P(pszFWChecksum + 1));
    if (fwInfo.fwPackageCheckValue != NULL)
    {
        strcpy(fwInfo.fwPackageCheckValue, pszFWChecksum);
    }
    fwInfo.fwSize = fileSize;
}

// Check for new firmware
void CheckNewFirmware()
{
    DEBUGMSG(ZONE_INIT, "--> %s()", FUNC_NAME);

    if (!enableOTA)
    {
        DEBUGMSG(ZONE_ERROR, "    %s(%d) - enableOTA = false, not updating firmware!", FUNC_NAME, __LINE__);
        FreeFwInfo();
        return;
    }

    DEBUGMSG(ZONE_FWOTAUPD, "fwInfo initialized: %s", fwInfo.fwVersion);

    if (fwInfo.fwVersion == NULL || fwInfo.fwPackageURI == NULL)
    {
        // Disable
        DEBUGMSG(ZONE_ERROR, "    %s(%d) - fwVersion and/or fwPackageURI missing.", FUNC_NAME, __LINE__);
        enableOTA = false;
        FreeFwInfo();
        return;
    }

    // Check if the URL is https as we require it for safety purpose.
    if (strlen(fwInfo.fwPackageURI) < 6 || (strncmp("https:", fwInfo.fwPackageURI, 6) != 0))
    {
        DEBUGMSG(ZONE_ERROR, "    %s(%d) - No secure connection.", FUNC_NAME, __LINE__);
        // Report error status, URINotHTTPS
        OTAUpdateFailed("URINotHTTPS");
        FreeFwInfo();
        return;
    }

    // Check if this is a new version.
    if (IoTHubClient_FwVersionCompare(fwInfo.fwVersion, currentFirmwareVersion) <= 0)
    {
        DEBUGMSG(ZONE_ERROR, "    %s(%d) - fwVersion stored in cloud (%s) <= running fwVersion (%s)", FUNC_NAME, __LINE__, fwInfo.fwVersion, currentFirmwareVersion);
        FreeFwInfo();
        return;
    }

    // New firemware
    Screen.print(1, "New firmware:");
    Screen.print(2, fwInfo.fwVersion);
    DEBUGMSG(ZONE_FWOTAUPD, "New firmware is available: %s.", fwInfo.fwVersion);
    Screen.print(3, " downloading...");
    DEBUGMSG(ZONE_FWOTAUPD, "    %s(%d) - >> Downloading from %s...", FUNC_NAME, __LINE__, fwInfo.fwPackageURI);
    // Report downloading status.
    char startTimeStr[30];
    time_t t = time(NULL);
    strftime(startTimeStr, 30, "%Y-%m-%dT%H:%M:%S.0000000Z", gmtime(&t));
    ReportOTAStatus(currentFirmwareVersion, fwInfo.fwVersion, OTA_STATUS_DOWNLOADING, fwInfo.fwPackageURI, startTimeStr, "");

    // Close IoT Hub Client to release the TLS resource for firmware download.
    DevKitMQTTClient_Close();
    // Download the firmware. This can be customized according to the board type.
    uint16_t checksum = 0;
    int fwSize = OTADownloadFirmware(fwInfo.fwPackageURI, &checksum);

    // Reopen the IoT Hub Client for reporting status.
    DevKitMQTTClient_Init(true);

    // Check result
    if (fwSize == 0 || fwSize == -1)
    {
        DEBUGMSG(ZONE_ERROR, "    %s(%d) - Download Failed!", FUNC_NAME, __LINE__);
        // Report error status, DownloadFailed
        OTAUpdateFailed("DownloadFailed");
        FreeFwInfo();
        return;
    }
    else if (fwSize == -2)
    {
        DEBUGMSG(ZONE_ERROR, "    %s(%d) - Firmware update failed: device error!", FUNC_NAME, __LINE__, fwInfo.fwPackageURI);
        // Report error status, DeviceError
        OTAUpdateFailed("DeviceError");
        FreeFwInfo();
        return;
    }
    else if (fwSize != fwInfo.fwSize)
    {
        DEBUGMSG(ZONE_ERROR, "    %s(%d) - Firmware update File Size mismatch!", FUNC_NAME, __LINE__, fwInfo.fwPackageURI);
        // Report error status, FileSizeNotMatch
        OTAUpdateFailed("FileSizeNotMatch");
        FreeFwInfo();
        return;
    }

    Screen.print(3, " Finished.");
    DEBUGMSG(ZONE_FWOTAUPD, "    %s(%d) - >> Finished download.", FUNC_NAME, __LINE__);

    // CRC check
    if (fwInfo.fwPackageCheckValue != NULL)
    {
        Screen.print(3, " verifying...");

        if (checksum == strtoul(fwInfo.fwPackageCheckValue, NULL, 16))
        {
            Screen.print(3, " passed.");
            DEBUGMSG(ZONE_FWOTAUPD, "    %s(%d) - >> CRC check passed.", FUNC_NAME, __LINE__);
        }
        else
        {
            DEBUGMSG(ZONE_ERROR, "    %s(%d) - Firmware update failed: CRC error!", FUNC_NAME, __LINE__);
            // Report error status, VerifyFailed
            OTAUpdateFailed("VerifyFailed");
            Screen.print(3, " CRC failed.");
            FreeFwInfo();
            return;
        }
    }

    // Report status
    char endTimeStr[30];
    t = time(NULL);
    strftime(endTimeStr, 30, "%Y-%m-%dT%H:%M:%S.0000000Z", gmtime(&t));
    ReportOTAStatus(currentFirmwareVersion, fwInfo.fwVersion, OTA_STATUS_APPLYING, "", startTimeStr, endTimeStr);

    // Applying
    if (OTAApplyNewFirmware(fwSize, checksum) != 0)
    {
        DEBUGMSG(ZONE_ERROR, "    %s(%d) - Apply Firmware failed!", FUNC_NAME, __LINE__);
        // Report error status, ApplyFirmwareFailed
        OTAUpdateFailed("ApplyFirmwareFailed");
        Screen.print(3, " Apply failed.");
        FreeFwInfo();
        return;
    }
    // Report status
    t = time(NULL);
    strftime(endTimeStr, 30, "%Y-%m-%dT%H:%M:%S.0000000Z", gmtime(&t));
    ReportOTAStatus(fwInfo.fwVersion, "", OTA_STATUS_CURRENT, "", startTimeStr, endTimeStr);

    // Counting down and reboot
    Screen.clean();
    Screen.print(0, "Reboot system");
    DEBUGMSG(ZONE_FWOTAUPD, "    %s(%d) - >> Reboot system.", FUNC_NAME, __LINE__);
    char msg[2] = {0};
    for (int i = 0; i < 5; i++)
    {
        msg[0] = '0' + 5 - i;
        Screen.print(2, msg);
        LogInfo(msg);
        delay(1000);
    }

    // Reboot system to apply the firmware.
    FreeFwInfo();
    SystemReboot();
}
