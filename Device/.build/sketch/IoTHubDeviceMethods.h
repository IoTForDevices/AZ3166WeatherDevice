#ifndef IOTHUBDEVICEMETHODS_H
#define IOTHUBDEVICEMETHODS_H

bool HandleReset(unsigned char **response, int *responseLength);
bool HandleMeasureNow(unsigned char **response, int *responseLength);
bool HandleFirmwareUpdate(const char *payload, int payloadLength, unsigned char **response, int *responseLength);
void HandleUnknownMethod(unsigned char **response, int *responseLength);

#endif  // IOTHUBDEVICEMETHODS_H