
#include <time.h>
#include "Clock_Impl.h"
#include <stdexcept>
#include <Arduino.h>

int Clock_Impl::getTimezoneOffset()
{
    // see: https://stackoverflow.com/questions/13804095/get-the-time-zone-gmt-offset-in-c/44063597#44063597

    time_t gmt, rawtime = time(NULL);
    struct tm* ptm;

    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);

    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);

    return static_cast<int>(difftime(rawtime, gmt));
}

bool Clock_Impl::getNow(time_t* nowPtr)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return false;
    }

    time(nowPtr);
    return true;
}