#ifndef DEBUG_ZONES_H
#define DEBUG_ZONES_H

#define DEBUGMSG (debugmask, format, ...) \
{ \
    if (debugmask) { \
        LogInfo(format, __VA_ARGS__);
        delay(200);
    } \
}

#endif