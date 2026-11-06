// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Locking policy:
 * - Public getters having the prefix 'get' take a lock internally. getWriteCount(), getWriteEpochTime(), etc.
 * - Public mutating methods take a lock internally.
 * - Private methods are called with the appropriate lock held by the public methods.
 * - Fetching external data is done before acquiring the mutex.
 * - Flags marked as atomic may be read lock-free.
 * - methods read() and writeAll() must be called from the task loop to avoid deadlocks,
 *   use requestWriteOnNextLoop() and requestReadOnNextLoop() to trigger them on demand.
 * - Do not call registerProvider(), unregisterProvider(), or requestReadOnNextLoop() from serializeRT() or deserializeRT()
 *   to avoid deadlocks, as they also take locks internally.
 */

#pragma once

#include <TaskSchedulerDeclarations.h>
#include <mutex>
#include <atomic>
#include <map>
#include <vector>
#include <string>
#include <ArduinoJson.h>


// The RuntimeClass Interface, each subsystem have to implement this interface
// to be able to store and read data in the runtime data file.
class InterfaceProviderRT {
public:
    virtual String getIdRT() const = 0;
    virtual void serializeRT(JsonObject obj) const = 0;
    virtual void deserializeRT(JsonObject obj) = 0;
    virtual ~InterfaceProviderRT() = default;

    void setReadOnStartupRT(bool readOnStartup) { _readOnStartup = readOnStartup; }
    bool getReadOnStartupRT() const { return _readOnStartup; }
private:
    bool _readOnStartup = true;   // if true, the data of this provider is readable on startup
};


// The RuntimeClass manages the runtime data of the system, which is stored in a file in the flash memory.
class RuntimeProvider {
public:
    RuntimeProvider() = default;
    ~RuntimeProvider() = default;
    RuntimeProvider(const RuntimeProvider&) = delete;
    RuntimeProvider& operator=(const RuntimeProvider&) = delete;
    RuntimeProvider(RuntimeProvider&&) = delete;
    RuntimeProvider& operator=(RuntimeProvider&&) = delete;

    // init the runtime data loop task
    void init(Scheduler& scheduler);

    // write all providers to file, do not write if last write operation was less than freezeMinutes ago
    // just call it from the task loop, do not call it from a locally locked mutex to avoid deadlocks
    bool writeAll(uint16_t const freezeMinutes = 10);

    // read from file, if readOnStartup is true, read data from providers that are marked to be read on startup,
    // otherwise read on demand data from providers from the _readIDList
    // just call it from the task loop, do not call it from a locally locked mutex to avoid deadlocks
    bool read(bool const readOnStartup = true);

    // use this method to request storing data on demand
    void requestWriteOnNextLoop(void) { _writeNow.store(true); }

    // use this method to request reading data on demand, addID is the ID of the provider to be read
    void requestReadOnNextLoop(const String& addID);

    // register a provider to be managed by the RuntimeProvider, if readOnStartup is true, the data of this provider will be read on startup
    // The caller retains ownership and must ensure the provider outlives RuntimeProvider.
    void registerProvider(InterfaceProviderRT* provider, bool readOnStartup = true);

    // unregister a provider, the provider will no longer be managed by the RuntimeProvider, and its data will not be read or written anymore
    void unregisterProvider(const String& id);
    void unregisterProvider(InterfaceProviderRT* provider);

    // getters for the runtime data, these methods are protected by mutex to ensure thread safety
    uint16_t getWriteCount(void) const;
    time_t getWriteEpochTime(void) const;
    bool getReadState(void) const { return _readOK.load(); }
    bool getWriteState(void) const { return _writeOK.load(); }
    String getWriteCountAndTimeString(void) const;

private:
    void loop(void);
    bool getWriteTrigger(void);

    Task _loopTask;
    std::atomic<bool> _readOK = false;      // true if the last read operation was successful
    std::atomic<bool> _writeOK = false;     // true if the last write operation was successful
    std::atomic<bool> _readNow = false;     // if true, the data is read in the next task loop()
    std::atomic<bool> _writeNow = false;    // if true, the data is stored in the next task loop()

    mutable std::mutex _mutexData;          // to protect the shared data below
    bool _lastTrigger = false;              // auxiliary value to prevent multiple triggering on the same day
    uint16_t _fileVersion = 0;              // shared data: version of the runtime data file, prepared for future migration support
    uint16_t _writeCount = 0;               // shared data: number of write operations
    time_t _writeEpoch = 0;                 // shared data: epoch time when the data was written

    mutable std::mutex _mutexInterface;     // to protect the interface data below
    std::map<String, InterfaceProviderRT*> _providers;
    std::vector<String> _readIDList;        // list of provider IDs to be read on demand
};

extern RuntimeProvider Runtime;
