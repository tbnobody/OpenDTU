// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * TCP client for WiFi-based Hoymiles HMS inverters.
 *
 * Protocol reverse-engineered by tobiasw91 (dtuGateway, Apache-2.0).
 * Ported and adapted for OpenDTU by OpenDTU contributors.
 */
#pragma once

#include <Arduino.h>
#include <AsyncTCP.h>
#include <Ticker.h>
#include <atomic>
#include <functional>
#include <time.h>

// Protocol constants
#define DTU_TCP_PORT          10081
#define DTU_TIME_OFFSET       28800   // 8-hour offset used by DTU firmware
#define DTU_LOOP_SEC          5    // Ticker interval; how often _loop() runs to check for work
#define DTU_WIFI_POLL_SEC     30   // Minimum seconds between full data polls
#define DTU_TXRX_TIMEOUT_MS   30000
#define DTU_RECONNECT_MAX     5
#define DTU_RECONNECT_PAUSE_MS 60000
#define DTU_APPINFO_INTERVAL_MS 600000  // 10 min

// TX/RX state machine
enum DtuTxRxState_t : uint8_t {
    TXRX_IDLE = 0,
    TXRX_WAIT_APP_INFO,
    TXRX_WAIT_REALDATA,
    TXRX_WAIT_CONFIG,
    TXRX_WAIT_CMD_SET_LIMIT,
    TXRX_WAIT_CMD_POWER_ON,
    TXRX_WAIT_CMD_POWER_OFF,
    TXRX_WAIT_CMD_RESTART,
    TXRX_WAIT_REQ_ALARMS,
    TXRX_WAIT_GET_ALARMS,
};

// Connection state
enum DtuConnState_t : uint8_t {
    DTU_CONN_OFFLINE = 0,
    DTU_CONN_CONNECTING,
    DTU_CONN_CONNECTED,
    DTU_CONN_ERROR,
};

// Parsed alarm entry from WiFi DTU
struct DtuAlarmEntry_t {
    uint16_t messageId  = 0;  // WCode & 0xFF (alarm code)
    int32_t  startTime  = 0;  // WTime1 (Unix timestamp, 0 = unknown)
    int32_t  endTime    = 0;  // WTime2 (Unix timestamp, 0 = still active)
};

// Parsed PV string data
struct DtuPvData_t {
    float voltage    = 0.0f;   // V
    float current    = 0.0f;   // A
    float power      = 0.0f;   // W
    float dailyEnergy = 0.0f;  // kWh
    float totalEnergy = 0.0f;  // kWh
};

// All parsed inverter data, delivered to callback on each successful poll
struct DtuData_t {
    // AC grid output
    float gridVoltage      = 0.0f;  // V
    float gridCurrent      = 0.0f;  // A
    float gridPower        = 0.0f;  // W (active)
    float gridReactivePower = 0.0f; // VAR
    float gridFrequency    = 0.0f;  // Hz
    float gridPowerFactor  = 0.0f;  // 0.0–1.0
    float gridDailyEnergy  = 0.0f;  // kWh  (sum of PV strings)
    float gridTotalEnergy  = 0.0f;  // kWh  (sum of PV strings)

    // DC PV inputs (up to 4 strings for 4T models)
    static const uint8_t MAX_PV_COUNT = 4;
    DtuPvData_t pv[MAX_PV_COUNT];
    uint8_t     pvCount = 0;  // actual count from RealData response

    // Inverter status
    float    temperature   = 0.0f;  // °C
    uint8_t  powerLimit    = 0;     // % (0–100)
    uint8_t  warningsCount = 0;

    // Device info (populated once after connection)
    char   dtuSerial[17]   = "";
    String inverterModel;
    uint32_t dtuFwVersion  = 0;
    uint32_t invFwVersion  = 0;
    uint32_t invHwPartNum  = 0;     // HW part number (from pv_hw_pn)
    uint16_t invHwVersion  = 0;     // HW version (from pv_hw_version)
    int32_t  invGpfCode    = 0;     // grid profile type ID (pv_gpf_code, field 7)
    int32_t  invGpf        = 0;     // grid profile version (pv_gpf, field 8)

    // Alarm / event log entries (from WInfoReqDTO)
    static const uint8_t MAX_ALARM_ENTRIES = 15;
    uint8_t         alarmCount = 0;
    DtuAlarmEntry_t alarms[MAX_ALARM_ENTRIES];
};

class DtuTcpClient {
public:
    using DataCallback    = std::function<void(const DtuData_t&)>;
    using ConnectCallback = std::function<void(bool connected)>;

    DtuTcpClient();
    ~DtuTcpClient();

    // Call once with the DTU's IP address; starts the connection loop.
    void setup(const char* host);

    // Shut down cleanly.
    void teardown();

    bool isConnected() const;
    DtuConnState_t getConnState() const { return _connState; }

    // --- Commands (queued; sent when connection is idle) ---
    void setPowerLimit(uint8_t percent);
    void setPowerState(bool on);
    void requestRestart();

    // --- Callbacks ---
    void setDataCallback(DataCallback cb)    { _dataCallback = cb; }
    void setConnectCallback(ConnectCallback cb) { _connectCallback = cb; }

    // Call from the main task (not async_tcp context) to deliver deferred data.
    void tick();

private:
    // AsyncTCP event handlers (static → instance dispatch)
    static void _onConnect   (void* arg, AsyncClient* c);
    static void _onDisconnect(void* arg, AsyncClient* c);
    static void _onError     (void* arg, AsyncClient* c, int8_t error);
    static void _onData      (void* arg, AsyncClient* c, void* data, size_t len);

    // Timer callbacks
    static void _loopCb(DtuTcpClient* self);

    // Internal connection management
    void _connect();
    void _disconnect();
    void _loop();

    // TX/RX state observer (timeout detection)
    void _txrxObserver();

    // Low-level send
    void _sendMessage(uint8_t cmd0, uint8_t cmd1,
                      const uint8_t* pbPayload, size_t pbLen);

    // Request builders (write to DTU)
    void _writeReqAppInfo();
    void _writeReqRealData();
    void _writeReqGetConfig();
    void _writeReqSetPowerLimit(uint8_t percent);
    void _writeReqPowerOn();
    void _writeReqPowerOff();
    void _writeReqRestart();
    void _writeReqRequestAlarms();
    void _writeReqGetAlarms();

    // Response parsers (read from DTU)
    void _readRespAppInfo     (const uint8_t* payload, size_t len);
    void _readRespRealData    (const uint8_t* payload, size_t len);
    void _readRespGetConfig   (const uint8_t* payload, size_t len);
    void _readRespCommand     (const uint8_t* payload, size_t len);
    void _readRespGetAlarms   (const uint8_t* payload, size_t len);

    // Helpers
    static float  _calcValue(int32_t raw, int32_t divisor = 10);
    static uint16_t _crc16Modbus(const uint8_t* data, size_t len);
    uint32_t _currentTimestamp() const;

    // Network
    const char*   _host   = nullptr;
    AsyncClient*  _client = nullptr;

    // Timers
    Ticker _loopTimer;

    // State
    DtuConnState_t  _connState  = DTU_CONN_OFFLINE;
    DtuTxRxState_t  _txrxState  = TXRX_IDLE;
    unsigned long   _txrxStateAt   = 0;
    uint8_t         _connRetries   = 0;
    unsigned long   _pauseUntil    = 0;

    // App-info tracking
    bool          _appInfoReceived = false;
    unsigned long _lastAppInfoAt   = 0;

    // Pending commands (set from outside; consumed when idle)
    bool    _pendingLimit     = false;
    uint8_t _pendingLimitVal  = 0;
    bool    _pendingPower     = false;
    bool    _pendingPowerOn   = false;
    bool    _pendingRestart   = false;

    // Parsed data (assembled across polling cycle; delivered on alarm completion)
    DtuData_t _data;
    // Set by async_tcp task; cleared + callback fired by main task via tick().
    std::atomic<bool> _dataReady { false };
    // Set by async_tcp task when a full poll cycle completes; tick() closes the connection.
    std::atomic<bool> _pendingDisconnect { false };
    // Set by tick() before closing; suppresses the failure callback in _onDisconnect.
    std::atomic<bool> _plannedDisconnect { false };
    // Timestamp of last successful poll completion (set by tick()).
    unsigned long _lastPollCompletedAt = 0;

    DataCallback    _dataCallback;
    ConnectCallback _connectCallback;
};
