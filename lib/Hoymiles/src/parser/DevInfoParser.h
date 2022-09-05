#include "Parser.h"
#include <Arduino.h>

#define DEV_INFO_SIZE 20

class DevInfoParser : public Parser {
public:
    void clearBufferAll();
    void appendFragmentAll(uint8_t offset, uint8_t* payload, uint8_t len);

    void clearBufferSample();
    void appendFragmentSample(uint8_t offset, uint8_t* payload, uint8_t len);

    uint32_t getLastUpdateAll();
    void setLastUpdateAll(uint32_t lastUpdate);

    uint32_t getLastUpdateSample();
    void setLastUpdateSample(uint32_t lastUpdate);

    uint16_t getFwBuildVersion();
    time_t getFwBuildDateTime();
    uint16_t getFwBootloaderVersion();

    uint32_t getHwPartNumber();
    uint16_t getHwVersion();

private:
    time_t timegm(struct tm* tm);

    uint32_t _lastUpdateAll = 0;
    uint32_t _lastUpdateSample = 0;

    uint8_t _payloadDevInfoAll[DEV_INFO_SIZE] = {};
    uint8_t _devInfoAllLength = 0;

    uint8_t _payloadDevInfoSample[DEV_INFO_SIZE] = {};
    uint8_t _devInfoSampleLength = 0;
};