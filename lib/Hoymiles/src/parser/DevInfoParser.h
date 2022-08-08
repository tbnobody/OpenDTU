#include "Parser.h"
#include <Arduino.h>

#define DEV_INFO_SIZE 30

class DevInfoParser : public Parser {
public:
    void clearBuffer();
    void appendFragment(uint8_t offset, uint8_t* payload, uint8_t len);

private:
    uint8_t _payloadDevInfo[DEV_INFO_SIZE];
    uint8_t _devInfoLength;
};