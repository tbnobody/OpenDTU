// SPDX-License-Identifier: GPL-2.0-or-later

/* Runtime Data Management
 *
 * Read and write runtime data persistent on LittleFS
 * - The data is stored in JSON format
 * - The data is written during WebApp 'OTA firmware upgrade' and during Webapp 'Reboot'
 * - For security reasons such as 'unexpected power cycles' or 'physical resets', data is also written once a day at 00:05
 * - The data will not be written if the last write operation was less than 10 minutes ago.
 * - Threadsafe access to data and interface is provided by two mutexes.
 * - Reading is done on startup and if requested on demand.
 *
 * How to use:
 *  - Derive your own class from the interface class InterfaceProviderRT.
 *  - Implement the serializeRT() and deserializeRT() methods to define how your subsystem's runtime data is stored and read.
 *  - Register the provider to the RuntimeProvider singleton instance in the setup() method of your subsystem
 *  - Use requestWriteOnNextLoop() and requestReadOnNextLoop() if you want to handle runtime data on demand.
 *
 * Note:
 * - The LittleFS filesystem must be initialized before using Runtime, otherwise the write and read operations will fail.
 *
 * 2025.09.11 - 1.0 - first version
 * 2025.12.01 - 1.1 - added read mode ON_DEMAND and START_UP
 * 2026.05.06 - 1.2 - added InterfaceProviderRT interface and provider registration and unregistration methods
 *                    improved read() method to read data of all or of specific providers
 *                    improved writeAll() method to keep data of providers that are currently not active or not registered
 */

#include <Utils.h>
#include <LittleFS.h>
#include <esp_log.h>
#include <ArduinoJson.h>
#include <algorithm>
#include "RuntimeData.h"

#undef TAG
static const char* TAG = "runtime";

static constexpr const char* RUNTIME_FILENAME = "/runtime.json";    // filename of the runtime data file
static constexpr uint16_t RUNTIME_VERSION = 1;                      // version prepared for future migration support

// runtime data keys
static constexpr const char* INFO = "info";
static constexpr const char* VERSION = "version";
static constexpr const char* SAVE_COUNT = "save_count";
static constexpr const char* SAVE_EPOCH = "save_epoch";


RuntimeProvider Runtime; // singleton instance


/*
 * Init the runtime data loop task
 */
void RuntimeProvider::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&RuntimeProvider::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.setInterval(15 * 1000); // every 15 seconds
    _loopTask.enable();
}


/*
 * The runtime data loop is called every 15 seconds and checks if a write or read operation is requested
 */
void RuntimeProvider::loop(void)
{
    // check if we need to write the runtime data, either it is 00:05 or on request
    bool dailyWriteTriggered = getWriteTrigger();
    if (_writeNow.exchange(false) || dailyWriteTriggered) {
        writeAll(0); // no freeze time.
    }

    // check if we need to read runtime data on request
    if (_readNow.exchange(false)) {
        read(false); // read on demand, not on startup
    }
}


/*
 * Register a provider to be managed by the RuntimeProvider,
 * if readOnStartup is true, the data of this provider will be read on startup
 */
void RuntimeProvider::registerProvider(InterfaceProviderRT* provider, bool readOnStartup) {

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexInterface);

        provider->setReadOnStartupRT(readOnStartup);
        _providers[provider->getIdRT()] = provider;
    } // mutex is automatically released when lock goes out of this scope

    ESP_LOGI(TAG, "Provider '%s' registered", provider->getIdRT().c_str());
}


/*
 * Unregister a provider, the provider will no longer be managed by the RuntimeProvider,
 * and its data will not be read or written anymore
 */
void RuntimeProvider::unregisterProvider(const String& id) {

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexInterface);

        _providers.erase(id);
    } // mutex is automatically released when lock goes out of this scope

    ESP_LOGI(TAG, "Provider '%s' unregistered", id.c_str());
}

void RuntimeProvider::unregisterProvider(InterfaceProviderRT* provider) {

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexInterface);

        _providers.erase(provider->getIdRT());
    } // mutex is automatically released when lock goes out of this scope

    ESP_LOGI(TAG, "Provider '%s' unregistered", provider->getIdRT().c_str());
}


/*
 * Write the runtime data from all registered providers into LittleFS file
 * freezeMinutes: Minimum necessary time [minutes] between now and last write operation
 */
bool RuntimeProvider::writeAll(uint16_t const freezeMinutes)
{
    auto cleanExit = [this](const bool writeOk, const char* text) -> bool {
        if (writeOk) {
            ESP_LOGI(TAG,"%s", text);
        } else {
            ESP_LOGE(TAG,"%s", text);
        }
        _writeOK.store(writeOk);
        return writeOk;
    };

    // we need a valid epoch time before we can write the runtime data
    time_t nextEpoch;
    if (!Utils::getEpoch(&nextEpoch, 1)) { return cleanExit(false, "Local time not available, skipping write"); }

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexInterface);

        // fast exit if no providers are registered, no need to write an empty file
        if (_providers.empty()) {
            return cleanExit(true, "No providers registered, skipping write");
        }
    } // mutex is automatically released when lock goes out of this scope

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexData);

        if (!LittleFS.exists(RUNTIME_FILENAME)) { _writeEpoch = 0; }

        // check minimum interval between writes (enforced only when freezeMinutes > 0)
        if ((freezeMinutes > 0) && (_writeEpoch != 0) && (difftime(nextEpoch, _writeEpoch) < 60 * freezeMinutes)) {
            return cleanExit(false, "Time interval too short, skipping write");
        }
    } // mutex is automatically released when lock goes out of this scope

    // prepare the JSON document and store the runtime data is done outside the
    // mutex protection to minimize the time the mutex is locked.
    JsonDocument doc;

    // read the existing runtime data to keep the data of providers that are currently not active or
    // not registered anymore, otherwise we would lose this data on write
    File fRuntime = LittleFS.open(RUNTIME_FILENAME, "r", false);
    if (fRuntime) {
        Utils::skipBom(fRuntime);
        DeserializationError error = deserializeJson(doc, fRuntime);
        fRuntime.close();
        if (error || !Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
            ESP_LOGW(TAG, "Read data error, rewriting runtime file from current provider state");
            doc.clear();
        }
    }

    JsonObject info = doc[INFO].as<JsonObject>();
    uint16_t nextCount = info[SAVE_COUNT] | 0U;
    nextCount++; // increase the count for the next write operation

    info = doc[INFO].to<JsonObject>();
    info[VERSION] = RUNTIME_VERSION;
    info[SAVE_COUNT] = nextCount;
    info[SAVE_EPOCH] = nextEpoch;

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexInterface);

        // serialize the runtime data of all registered providers
        for (auto& [id, provider] : _providers) {
            provider->serializeRT(doc[id].to<JsonObject>());
        }
    } // mutex is automatically released when lock goes out of this scope

    if (!Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
        return cleanExit(false, "JSON alloc fault, skipping write");
    }

    fRuntime = LittleFS.open(RUNTIME_FILENAME, "w");
    if (!fRuntime) { return cleanExit(false, "Failed to open file for writing"); }

    if (serializeJson(doc, fRuntime) == 0) {
        fRuntime.close();
        return cleanExit(false, "Failed to serialize to file");
    }
    fRuntime.close();

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexData);

        // commit the new state only after a successful write
        _fileVersion = RUNTIME_VERSION;
        _writeEpoch = nextEpoch;
        _writeCount = nextCount;
    } // mutex is automatically released when lock goes out of this scope

    return cleanExit(true, "Written to file");
}


/*
 * Read the runtime data from LittleFS file
 * readOnStartup = true: read data from providers that are marked to be read on startup
 * readOnStartup = false: read data from providers that are requested to be read on demand
 */
bool RuntimeProvider::read(bool const readOnStartup)
{
    auto cleanExit = [this](const bool readOk, const char* text) -> bool {
        if (readOk) {
            ESP_LOGI(TAG,"%s", text);
        } else {
            ESP_LOGE(TAG,"%s", text);
        }
        _readOK.store(readOk);
        return readOk;
    };

    JsonDocument doc;

    // Note: We do not exit on read or allocation errors.
    // Every provider can decide by itself whether to use default configuration values in that case
    bool readOk = LittleFS.exists(RUNTIME_FILENAME);
    File fRuntime = LittleFS.open(RUNTIME_FILENAME, "r", false);
    if (fRuntime) {
        Utils::skipBom(fRuntime);
        DeserializationError error = deserializeJson(doc, fRuntime);
        fRuntime.close();
        if (error || !Utils::checkJsonAlloc(doc, __FUNCTION__, __LINE__)) {
            readOk = false;
        }
    }

    JsonObject info = doc[INFO].as<JsonObject>();

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexData);

        // 0 means no file available and runtime data is not valid
        _fileVersion = info[VERSION] | 0U;
        _writeCount = info[SAVE_COUNT] | 0U;
        _writeEpoch = info[SAVE_EPOCH].as<time_t>();
        if (_writeEpoch == 0) { readOk = false; } // no valid data available
    } // mutex is automatically released when lock goes out of this scope

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexInterface);

        for (auto& [id, provider] : _providers) {
            if (readOnStartup) {
                if (!provider->getReadOnStartupRT()) { continue; } // skip providers that are not read on startup
            } else {
                auto providerIt = std::find(_readIDList.begin(), _readIDList.end(), id);
                if (providerIt == _readIDList.end()) { continue; } // skip providers that are not requested to be read on demand
            }

            // JsonObject can be empty, but as mentioned before, we do not exit on read or allocation errors,
            // so the provider can decide by itself whether to use default configuration values in that case
            provider->deserializeRT(doc[id].as<JsonObject>());
        }

        // Clear the on-demand read list after processing
        if (!readOnStartup) { _readIDList.clear(); }

    } // mutex is automatically released when lock goes out of this scope

    if (!readOk) {
        return cleanExit(false, "File not found or error, using default values");
    }

    return cleanExit(true, "Read from file");
}


/*
 * Get the write counter
 */
uint16_t RuntimeProvider::getWriteCount(void) const
{
    std::lock_guard<std::mutex> lock(_mutexData);
    return _writeCount;
}


/*
 * Get the write epoch time
 */
time_t RuntimeProvider::getWriteEpochTime(void) const
{
    std::lock_guard<std::mutex> lock(_mutexData);
    return _writeEpoch;
}


/*
 * Get the write count and time as string
 * Format: "<count> / <dd>-<mon> <hh>:<mm>"
 * If epoch time and local time is not available the time is replaced by "no time"
 */
String RuntimeProvider::getWriteCountAndTimeString(void) const
{
    time_t epoch;
    uint16_t count;

    { // mutex is automatically released when lock goes out of this scope
        std::lock_guard<std::mutex> lock(_mutexData);
        epoch = _writeEpoch;
        count = _writeCount;
    } // mutex is automatically released when lock goes out of this scope

    char buf[32] = "";
    struct tm time;

    // Before we can convert the epoch to local time, we need to ensure we've received the correct time
    // from the time server. This may take some time after the system startup.
    if ((epoch != 0) && (getLocalTime(&time, 1))) {
        localtime_r(&epoch, &time);
        strftime(buf, sizeof(buf), " / %d-%h %R", &time);
    } else {
        snprintf(buf, sizeof(buf), " / no time");
    }
    String ctString = String(count) + String(buf);
    return ctString;
}


/*
 * Get the daily write trigger
 * Returns true once a day between 00:05 - 00:10
 */
bool RuntimeProvider::getWriteTrigger(void) {

    struct tm nowTime;
    if (!getLocalTime(&nowTime, 1)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutexData);
    if ((nowTime.tm_hour == 0) && (nowTime.tm_min >= 5) && (nowTime.tm_min <= 10)) {
        if (_lastTrigger == false) {
            _lastTrigger = true;
            return true;
        }
    } else {
        _lastTrigger = false;
    }
    return false;
}


/*
 * Add an provider ID to the read list
 */
void RuntimeProvider::requestReadOnNextLoop(String const& addID) {

    std::lock_guard<std::mutex> lock(_mutexInterface);

    // if the ID is not already on the list, add it
    auto id = std::find(_readIDList.begin(), _readIDList.end(), addID);
    if (id == _readIDList.end()) { _readIDList.push_back(addID); }
    _readNow.store(true);
}
