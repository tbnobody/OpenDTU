// SPDX-License-Identifier: GPL-2.0-or-later
#include <powermeter/sml/http/Provider.h>
#include <WiFiClientSecure.h>
#include <base64.h>
#include <ESPmDNS.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "powerMeter";
static const char* SUBTAG = "HTTP/SML";

namespace PowerMeters::Sml::Http {

Provider::~Provider()
{
    _taskDone = false;

    std::unique_lock<std::mutex> lock(_pollingMutex);
    _stopPolling = true;
    lock.unlock();

    _cv.notify_all();

    if (_taskHandle != nullptr) {
        while (!_taskDone) { delay(10); }
        _taskHandle = nullptr;
    }
}

bool Provider::init()
{
    _upHttpGetter = std::make_unique<HttpGetter>(_cfg.HttpRequest);

    if (_upHttpGetter->init()) { return true; }

    DTU_LOGE("Initializing HTTP getter failed: %s", _upHttpGetter->getErrorText());

    _upHttpGetter = nullptr;

    return false;
}

void Provider::loop()
{
    if (_taskHandle != nullptr) { return; }

    std::unique_lock<std::mutex> lock(_pollingMutex);
    _stopPolling = false;
    lock.unlock();

    uint32_t constexpr stackSize = 6144;
    xTaskCreate(Provider::pollingLoopHelper, "PM:HTTP+SML",
            stackSize, this, 1/*prio*/, &_taskHandle);
}

void Provider::pollingLoopHelper(void* context)
{
    auto pInstance = static_cast<Provider*>(context);
    pInstance->pollingLoop();
    pInstance->_taskDone = true;
    vTaskDelete(nullptr);
}

void Provider::pollingLoop()
{
    std::unique_lock<std::mutex> lock(_pollingMutex);

    while (!_stopPolling) {
        auto elapsedMillis = millis() - _lastPoll;
        auto intervalMillis = _cfg.PollingInterval * 1000;
        if (_lastPoll > 0 && elapsedMillis < intervalMillis) {
            auto sleepMs = intervalMillis - elapsedMillis;
            _cv.wait_for(lock, std::chrono::milliseconds(sleepMs),
                    [this] { return _stopPolling; }); // releases the mutex
            continue;
        }

        _lastPoll = millis();

        lock.unlock(); // polling can take quite some time
        auto res = poll();
        lock.lock();

        if (!res.isEmpty()) {
            DTU_LOGE("%s", res.c_str());
            continue;
        }
    }
}

bool Provider::isDataValid() const
{
    uint32_t age = millis() - getLastUpdate();
    return getLastUpdate() > 0 && (age < (3 * _cfg.PollingInterval * 1000));
}

String Provider::poll()
{
    if (!_upHttpGetter) {
        return "Initialization of HTTP request failed";
    }

    auto res = _upHttpGetter->performGetRequest();
    if (!res) {
        return _upHttpGetter->getErrorText();
    }

    auto pStream = res.getStream();
    if (!pStream) {
        return "Programmer error: HTTP request yields no stream";
    }

    while (pStream->available()) {
        processSmlByte(pStream->read());
    }

    ::PowerMeters::Sml::Provider::reset();

    return "";
}

} // namespace PowerMeters::Sml::Http
