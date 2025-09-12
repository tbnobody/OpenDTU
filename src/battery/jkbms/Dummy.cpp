#include <Arduino.h>
#include <vector>
#include <battery/jkbms/Dummy.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "battery";
static const char* SUBTAG = "JK BMS";

namespace Batteries::JkBms {

void DummySerial::begin(uint32_t, uint32_t, int8_t, int8_t)
{
    DTU_LOGI("Dummy Serial: begin()");
}

void DummySerial::end()
{
    DTU_LOGI("Dummy Serial: end()");
}

size_t DummySerial::write(const uint8_t *buffer, size_t size)
{
    DTU_LOGD("Dummy Serial: write(%d Bytes)", size);
    _byte_idx = 0;
    _msg_idx = (_msg_idx + 1) % _data.size();
    return size;
}

} // namespace Batteries::JkBms
