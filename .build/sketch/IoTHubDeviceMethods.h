#ifndef IOTHUBDEVICEMETHODS_H
#define IOTHUBDEVICEMETHODS_H

bool HandleReset(unsigned char **response, int *responseLength);
void HandleUnknownMethod(unsigned char **response, int *responseLength);

#endif  // IOTHUBDEVICEMETHODS_H