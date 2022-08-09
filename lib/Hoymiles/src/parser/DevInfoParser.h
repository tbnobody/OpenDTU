#include "Parser.h"
#include <Arduino.h>

#define DEV_INFO_SIZE 20

class DevInfoParser : public Parser {
public:
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

    uint16_t getFwBuildVersion();
    time_t getFwBuildDateTime();
    uint16_t getFwBootloaderVersion();

private:
    time_t timegm(struct tm *tm);

    uint8_t _payloadDevInfo[DEV_INFO_SIZE];
    uint8_t _devInfoLength;
};