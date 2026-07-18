// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * TCP client for WiFi-based Hoymiles HMS inverters.
 *
 * Protocol reverse-engineered by tobiasw91 (dtuGateway, Apache-2.0).
 * Ported and adapted for OpenDTU by OpenDTU contributors.
 */
#include "DtuTcpClient.h"
#include <esp_log.h>
#include <cstring>
#include <ctime>

// nanopb generated headers (built by PlatformIO nanopb plugin)
#include "proto/RealtimeDataNew.pb.h"
#include "proto/GetConfig.pb.h"
#include "proto/CommandPB.pb.h"
#include "proto/AlarmData.pb.h"
#include "proto/APPInformationData.pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

#undef TAG
static const char* TAG = "DtuTcpClient";

// ---------------------------------------------------------------------------
// CRC16 Modbus (inline, no external library needed)
// ---------------------------------------------------------------------------
uint16_t DtuTcpClient::_crc16Modbus(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

float DtuTcpClient::_calcValue(int32_t raw, int32_t divisor)
{
    return static_cast<float>(raw) / static_cast<float>(divisor);
}

uint32_t DtuTcpClient::_currentTimestamp() const
{
    return static_cast<uint32_t>(time(nullptr));
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------
DtuTcpClient::DtuTcpClient() = default;

DtuTcpClient::~DtuTcpClient()
{
    teardown();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void DtuTcpClient::setup(const char* host)
{
    if (_client) {
        return; // already set up
    }

    _host = host;
    ESP_LOGI(TAG, "Setup DTU client for host: %s", host);

    _client = new AsyncClient();
    _client->onConnect   (_onConnect,    this);
    _client->onDisconnect(_onDisconnect, this);
    _client->onError     (_onError,      this);
    _client->onData      (_onData,       this);

    _loopTimer.attach(DTU_LOOP_SEC, _loopCb, this);
}

void DtuTcpClient::teardown()
{
    _loopTimer.detach();

    if (_client) {
        if (_client->connected()) {
            _client->close();
        }
        delete _client;
        _client = nullptr;
    }

    _connState = DTU_CONN_OFFLINE;
    _txrxState = TXRX_IDLE;
    _appInfoReceived = false;
}

bool DtuTcpClient::isConnected() const
{
    return _client && _client->connected() && _connState == DTU_CONN_CONNECTED;
}

void DtuTcpClient::tick()
{
    // Deliver data before disconnecting so stats are already fresh when the
    // connection-closed event fires (avoids a spurious failure-count increment).
    if (_dataReady.load(std::memory_order_acquire) && _dataCallback) {
        _dataReady.store(false, std::memory_order_relaxed);
        ESP_LOGI(TAG, "tick: delivering data to inverter (AC %.1f W)", _data.gridPower);
        _dataCallback(_data);
    }

    // Planned disconnect after a completed poll cycle.
    if (_pendingDisconnect.exchange(false, std::memory_order_acq_rel)) {
        _lastPollCompletedAt = millis();
        _connRetries = 0;
        _connState   = DTU_CONN_OFFLINE;
        _txrxState   = TXRX_IDLE;
        // Tell _onDisconnect not to fire the failure callback.
        _plannedDisconnect.store(true, std::memory_order_release);
        if (_client && _client->connected()) {
            _client->close();
        }
    }
}

void DtuTcpClient::setPowerLimit(uint8_t percent)
{
    _pendingLimit    = true;
    _pendingLimitVal = percent;
}

void DtuTcpClient::setPowerState(bool on)
{
    _pendingPower   = true;
    _pendingPowerOn = on;
}

void DtuTcpClient::requestRestart()
{
    _pendingRestart = true;
}

// ---------------------------------------------------------------------------
// Timer callbacks (static → instance)
// ---------------------------------------------------------------------------
void DtuTcpClient::_loopCb(DtuTcpClient* self)
{
    if (self) self->_loop();
}

// ---------------------------------------------------------------------------
// Internal loop (called every DTU_LOOP_SEC seconds by Ticker)
// ---------------------------------------------------------------------------
void DtuTcpClient::_loop()
{
    _txrxObserver();

    if (_connState == DTU_CONN_OFFLINE || _connState == DTU_CONN_ERROR) {
        // Respect pause after a retry burst caused by connection errors.
        if (_pauseUntil > 0 && millis() < _pauseUntil) {
            return;
        }
        _pauseUntil = 0;

        // Connect only when there is actual work pending.
        bool hasPendingCmds = _pendingLimit || _pendingPower || _pendingRestart;
        bool pollDue = (millis() - _lastPollCompletedAt) >=
                       static_cast<unsigned long>(DTU_WIFI_POLL_SEC) * 1000UL;

        if (!hasPendingCmds && !pollDue) {
            return; // idle between polls — stay disconnected
        }

        if (_connRetries < DTU_RECONNECT_MAX) {
            _connRetries++;
            _connState = DTU_CONN_CONNECTING;
            _connect();
        } else {
            ESP_LOGW(TAG, "Max retries reached, pausing %u ms", DTU_RECONNECT_PAUSE_MS);
            _connRetries = 0;
            _pauseUntil  = millis() + DTU_RECONNECT_PAUSE_MS;
            _connState   = DTU_CONN_OFFLINE;
        }
        return;
    }

    if (_connState != DTU_CONN_CONNECTED) {
        return; // still connecting
    }

    // Only dispatch new requests when idle
    if (_txrxState != TXRX_IDLE) {
        return;
    }

    // --- Pending commands take priority ---
    if (_pendingLimit) {
        _pendingLimit = false;
        _writeReqSetPowerLimit(_pendingLimitVal);
        return;
    }
    if (_pendingPower) {
        _pendingPower = false;
        if (_pendingPowerOn) {
            _writeReqPowerOn();
        } else {
            _writeReqPowerOff();
        }
        return;
    }
    if (_pendingRestart) {
        _pendingRestart = false;
        _writeReqRestart();
        return;
    }

    // --- Periodic device info request ---
    // Fire immediately if we've never received AppInfo this session (first connect
    // after boot or after an unexpected disconnect), or after the refresh interval.
    unsigned long now = millis();
    if (!_appInfoReceived || (now - _lastAppInfoAt > DTU_APPINFO_INTERVAL_MS)) {
        _writeReqAppInfo();
        _lastAppInfoAt = now;
        return;
    }

    // --- Default: poll real data ---
    _writeReqRealData();
}

void DtuTcpClient::_txrxObserver()
{
    if (_txrxState == TXRX_IDLE) {
        return;
    }
    if (millis() - _txrxStateAt > DTU_TXRX_TIMEOUT_MS) {
        ESP_LOGW(TAG, "TX/RX timeout in state %u — resetting", static_cast<unsigned>(_txrxState));
        _txrxState = TXRX_IDLE;
        _disconnect();
    }
}

// ---------------------------------------------------------------------------
// AsyncTCP event handlers
// ---------------------------------------------------------------------------
void DtuTcpClient::_onConnect(void* arg, AsyncClient* /*c*/)
{
    auto* self = static_cast<DtuTcpClient*>(arg);
    ESP_LOGI(TAG, "Connected to DTU");
    self->_connState   = DTU_CONN_CONNECTED;
    self->_connRetries = 0;
    if (self->_connectCallback) self->_connectCallback(true);
    // Start the first request immediately — don't wait up to DTU_LOOP_SEC for
    // the next ticker fire, or the effective poll cycle becomes 3× longer.
    self->_loop();
}

void DtuTcpClient::_onDisconnect(void* arg, AsyncClient* /*c*/)
{
    auto* self = static_cast<DtuTcpClient*>(arg);
    self->_connState = DTU_CONN_OFFLINE;
    self->_txrxState = TXRX_IDLE;

    if (self->_plannedDisconnect.exchange(false, std::memory_order_acq_rel)) {
        // Intentional disconnect after a completed poll cycle — don't report a failure
        // and keep _appInfoReceived so the next session skips the AppInfo request.
        ESP_LOGI(TAG, "Disconnected from DTU (planned)");
    } else {
        // Unexpected disconnect — reset AppInfo so the next session re-fetches it.
        self->_appInfoReceived = false;
        ESP_LOGI(TAG, "Disconnected from DTU");
        if (self->_connectCallback) self->_connectCallback(false);
    }
}

void DtuTcpClient::_onError(void* arg, AsyncClient* c, int8_t error)
{
    auto* self = static_cast<DtuTcpClient*>(arg);
    ESP_LOGW(TAG, "TCP error: %s (%d)", c->errorToString(error), error);
    self->_connState = DTU_CONN_ERROR;
    self->_txrxState = TXRX_IDLE;
}

void DtuTcpClient::_onData(void* arg, AsyncClient* /*c*/, void* data, size_t len)
{
    auto* self = static_cast<DtuTcpClient*>(arg);
    if (len < 10) {
        ESP_LOGW(TAG, "onData: packet too short (%u bytes)", len);
        return;
    }

    const auto* hdr = static_cast<const uint8_t*>(data);
    ESP_LOGD(TAG, "onData: state=%u cmd=%02X/%02X len=%u",
             static_cast<unsigned>(self->_txrxState), hdr[2], hdr[3], (unsigned)len);

    // Payload starts at byte 10 (after the 10-byte header)
    const auto* payload  = static_cast<const uint8_t*>(data) + 10;
    const size_t pbLen   = len - 10;

    switch (self->_txrxState) {
        case TXRX_WAIT_APP_INFO:
            self->_readRespAppInfo(payload, pbLen);
            // Immediately start the data poll — don't wait for the next loop tick.
            self->_writeReqRealData();
            break;

        case TXRX_WAIT_REALDATA:
            self->_readRespRealData(payload, pbLen);
            // Chain: immediately request config after real data
            self->_writeReqGetConfig();
            break;

        case TXRX_WAIT_CONFIG:
            self->_readRespGetConfig(payload, pbLen);
            // Chain: request alarm list after config
            self->_writeReqRequestAlarms();
            break;

        case TXRX_WAIT_REQ_ALARMS:
            // Ack from DTU — now fetch the actual alarm data
            self->_readRespCommand(payload, pbLen);
            self->_writeReqGetAlarms();
            break;

        case TXRX_WAIT_GET_ALARMS:
            self->_readRespGetAlarms(payload, pbLen);
            // _readRespGetAlarms sets _dataReady and _pendingDisconnect.
            break;

        case TXRX_WAIT_CMD_SET_LIMIT:
        case TXRX_WAIT_CMD_POWER_ON:
        case TXRX_WAIT_CMD_POWER_OFF:
        case TXRX_WAIT_CMD_RESTART:
            self->_readRespCommand(payload, pbLen);
            // Re-fetch config to confirm new limit
            self->_writeReqGetConfig();
            break;

        default:
            // Silently ignore data in IDLE state — expected when the DTU sends the
            // alarm list as multiple TCP segments after state already transitioned.
            if (self->_txrxState != TXRX_IDLE) {
                ESP_LOGW(TAG, "onData: unexpected state %u for cmd %02X/%02X",
                         static_cast<unsigned>(self->_txrxState), hdr[2], hdr[3]);
                self->_txrxState = TXRX_IDLE;
            }
            break;
    }
}

// ---------------------------------------------------------------------------
// Connection helpers
// ---------------------------------------------------------------------------
void DtuTcpClient::_connect()
{
    if (!_client || !_host) return;
    if (_client->connected()) {
        _connState = DTU_CONN_CONNECTED;
        return;
    }
    ESP_LOGI(TAG, "Connecting to %s:%u (attempt %u)", _host, DTU_TCP_PORT, _connRetries);
    if (!_client->connect(_host, DTU_TCP_PORT)) {
        ESP_LOGW(TAG, "connect() failed immediately");
        _connState = DTU_CONN_ERROR;
    }
}

void DtuTcpClient::_disconnect()
{
    if (_client && _client->connected()) {
        _client->close();
    }
    _connState = DTU_CONN_OFFLINE;
    _appInfoReceived = false;
    if (_connectCallback) _connectCallback(false);
}

// ---------------------------------------------------------------------------
// Low-level message builder
// Header: 0x48 0x4D cmd0 cmd1 0x00 0x01 crcH crcL lenH lenL
// ---------------------------------------------------------------------------
void DtuTcpClient::_sendMessage(uint8_t cmd0, uint8_t cmd1,
                                const uint8_t* pbPayload, size_t pbLen)
{
    if (!_client || !_client->connected()) {
        ESP_LOGW(TAG, "_sendMessage: not connected");
        return;
    }

    uint16_t crc = _crc16Modbus(pbPayload, pbLen);
    const size_t frameLen = 10 + pbLen;

    // Build header+payload in one contiguous buffer so they are sent in a
    // single TCP segment. The DTU (embedded firmware) processes data per
    // segment and will not respond if the header arrives alone.
    uint8_t frame[256];
    if (frameLen > sizeof(frame)) {
        ESP_LOGE(TAG, "_sendMessage: payload too large (%u bytes)", (unsigned)pbLen);
        return;
    }
    frame[0] = 0x48;
    frame[1] = 0x4D;
    frame[2] = cmd0;
    frame[3] = cmd1;
    frame[4] = 0x00;
    frame[5] = 0x01;
    frame[6] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    frame[7] = static_cast<uint8_t>(crc & 0xFF);
    frame[8] = static_cast<uint8_t>((frameLen >> 8) & 0xFF);
    frame[9] = static_cast<uint8_t>(frameLen & 0xFF);
    if (pbLen > 0) {
        memcpy(frame + 10, pbPayload, pbLen);
    }

    ESP_LOGD(TAG, "_sendMessage: cmd=%02X/%02X frameLen=%u",
             cmd0, cmd1, (unsigned)frameLen);

    size_t written = _client->write(reinterpret_cast<const char*>(frame), frameLen);
    if (written != frameLen) {
        ESP_LOGW(TAG, "_sendMessage: only wrote %u of %u bytes", (unsigned)written, (unsigned)frameLen);
    }
}

// ---------------------------------------------------------------------------
// Request builders
// ---------------------------------------------------------------------------
void DtuTcpClient::_writeReqAppInfo()
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    APPInfoDataResDTO req = APPInfoDataResDTO_init_zero;
    req.offset      = DTU_TIME_OFFSET;
    req.time        = _currentTimestamp();
    req.package_now = 0;
    req.err_code    = 0;

    if (!pb_encode(&stream, APPInfoDataResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqAppInfo: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_APP_INFO;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x01, buf, stream.bytes_written);
    ESP_LOGD(TAG, "Sent app info request");
}

void DtuTcpClient::_writeReqRealData()
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    RealDataNewResDTO req = RealDataNewResDTO_init_default;
    req.offset = DTU_TIME_OFFSET;
    req.time   = static_cast<int32_t>(_currentTimestamp());

    if (!pb_encode(&stream, RealDataNewResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqRealData: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_REALDATA;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x11, buf, stream.bytes_written);
    ESP_LOGD(TAG, "Sent real-data request");
}

void DtuTcpClient::_writeReqGetConfig()
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    GetConfigResDTO req = GetConfigResDTO_init_default;
    req.offset = DTU_TIME_OFFSET;
    req.time   = _currentTimestamp();

    if (!pb_encode(&stream, GetConfigResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqGetConfig: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_CONFIG;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x09, buf, stream.bytes_written);
    ESP_LOGD(TAG, "Sent get-config request");
}

void DtuTcpClient::_writeReqSetPowerLimit(uint8_t percent)
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    CommandResDTO req = CommandResDTO_init_zero;
    req.time   = static_cast<int32_t>(_currentTimestamp());
    req.action = 8; // CMD_ACTION_LIMIT_POWER
    req.dev_kind = 3; // DEV_MICRO

    // Data format: "A:{limit*10},B:0,C:0\r"
    uint16_t limitLevel = static_cast<uint16_t>(percent) * 10;
    snprintf(req.data, sizeof(req.data), "A:%u,B:0,C:0\r", limitLevel);

    if (!pb_encode(&stream, CommandResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqSetPowerLimit: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_CMD_SET_LIMIT;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x05, buf, stream.bytes_written);
    ESP_LOGI(TAG, "Sent set-power-limit %u%%", percent);
}

void DtuTcpClient::_writeReqPowerOn()
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    CommandResDTO req = CommandResDTO_init_zero;
    req.time     = static_cast<int32_t>(_currentTimestamp());
    req.action   = 6; // CMD_ACTION_MI_START
    req.dev_kind = 3;

    if (!pb_encode(&stream, CommandResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqPowerOn: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_CMD_POWER_ON;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x05, buf, stream.bytes_written);
    ESP_LOGI(TAG, "Sent power-on command");
}

void DtuTcpClient::_writeReqPowerOff()
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    CommandResDTO req = CommandResDTO_init_zero;
    req.time     = static_cast<int32_t>(_currentTimestamp());
    req.action   = 7; // CMD_ACTION_MI_SHUTDOWN
    req.dev_kind = 3;

    if (!pb_encode(&stream, CommandResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqPowerOff: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_CMD_POWER_OFF;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x05, buf, stream.bytes_written);
    ESP_LOGI(TAG, "Sent power-off command");
}

void DtuTcpClient::_writeReqRestart()
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    CommandResDTO req = CommandResDTO_init_zero;
    req.time     = static_cast<int32_t>(_currentTimestamp());
    req.action   = 3; // CMD_ACTION_MI_REBOOT
    req.dev_kind = 3;

    if (!pb_encode(&stream, CommandResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqRestart: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_CMD_RESTART;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x05, buf, stream.bytes_written);
    ESP_LOGI(TAG, "Sent restart command");
}

void DtuTcpClient::_writeReqRequestAlarms()
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    CommandResDTO req = CommandResDTO_init_zero;
    req.time     = static_cast<int32_t>(_currentTimestamp());
    req.action   = 50; // CMD_ACTION_ALARM_LIST (request)
    req.dev_kind = 3;

    if (!pb_encode(&stream, CommandResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqRequestAlarms: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_REQ_ALARMS;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x05, buf, stream.bytes_written);
}

void DtuTcpClient::_writeReqGetAlarms()
{
    uint8_t buf[200];
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(buf));

    WInfoResDTO req = WInfoResDTO_init_default;
    req.offset = DTU_TIME_OFFSET;
    req.time   = static_cast<int32_t>(_currentTimestamp());

    if (!pb_encode(&stream, WInfoResDTO_fields, &req)) {
        ESP_LOGE(TAG, "writeReqGetAlarms: encode failed");
        return;
    }

    _txrxState   = TXRX_WAIT_GET_ALARMS;
    _txrxStateAt = millis();
    _sendMessage(0xA3, 0x04, buf, stream.bytes_written);
    ESP_LOGD(TAG, "Sent get-alarms request");
}

// ---------------------------------------------------------------------------
// Response parsers
// ---------------------------------------------------------------------------
void DtuTcpClient::_readRespAppInfo(const uint8_t* payload, size_t len)
{
    _txrxState = TXRX_IDLE;

    APPInfoDataReqDTO resp = APPInfoDataReqDTO_init_default;
    pb_istream_t stream = pb_istream_from_buffer(payload, len);

    if (!pb_decode(&stream, &APPInfoDataReqDTO_msg, &resp)) {
        ESP_LOGW(TAG, "readRespAppInfo: decode failed");
        return;
    }

    _appInfoReceived = true;

    strlcpy(_data.dtuSerial, resp.dtu_sn, sizeof(_data.dtuSerial));
    ESP_LOGI(TAG, "DTU serial: %s", _data.dtuSerial);

    if (resp.has_mAPPDtuInfo) {
        _data.dtuFwVersion = static_cast<uint32_t>(resp.mAPPDtuInfo.dtu_sw_version);
        ESP_LOGI(TAG, "DTU FW version: %u", _data.dtuFwVersion);
    }

    if (resp.mAPPpvInfo_count > 0) {
        const auto& pvInfo = resp.mAPPpvInfo[0];
        _data.invFwVersion  = static_cast<uint32_t>(pvInfo.pv_sw_version);
        _data.invHwPartNum  = static_cast<uint32_t>(pvInfo.pv_hw_pn);
        _data.invHwVersion  = static_cast<uint16_t>(pvInfo.pv_hw_version);
        _data.invGpfCode    = pvInfo.pv_gpf_code;
        _data.invGpf        = pvInfo.pv_gpf;

        // Derive model name from the inverter serial (following dtuGateway convention).
        // Uses range-based detection: b0=high byte, b1=low byte of prefix.
        int64_t invSerial = pvInfo.pv_sn;
        if (invSerial != 0) {
            uint16_t prefix = static_cast<uint16_t>((invSerial >> 32) & 0xFFFF);
            uint8_t b0 = (prefix >> 8) & 0xFF;
            uint8_t b1 = prefix & 0xFF;

            // Try exact match first (known specific models)
            switch (prefix) {
                case 0x1121: _data.inverterModel = "HMS-350W-1T";  break;
                case 0x1122: _data.inverterModel = "HMS-400W-1T";  break;
                case 0x1124: _data.inverterModel = "HMS-500-1T";   break;
                case 0x1125: _data.inverterModel = "HMS-500-1T (v2)"; break;
                case 0x1141: _data.inverterModel = "HMS-600W-2T";  break;
                case 0x1142: _data.inverterModel = "HMS-700W-2T";  break;
                case 0x1143: _data.inverterModel = "HMS-800-2T";   break;
                case 0x1144: _data.inverterModel = "HMS-800W-2T";  break;
                case 0x114a: _data.inverterModel = "HMS-1000-2T";  break;
                case 0x1014: _data.inverterModel = "HMS-600W-2T";  break;
                case 0x1410: _data.inverterModel = "HMS-800W-2T";  break;
                case 0x1412: _data.inverterModel = "HMS-800W-2T";  break;
                case 0x1161: _data.inverterModel = "HMS-1000W-4T"; break;
                case 0x1162: _data.inverterModel = "HMS-1200W-4T"; break;
                case 0x1164: _data.inverterModel = "HMS-1600-4T";  break;
                case 0x1166: _data.inverterModel = "HMS-1800-4T";  break;
                case 0x1222: _data.inverterModel = "HMS-1500W-4T"; break;
                case 0x1420: _data.inverterModel = "HMS-2000-4T";  break;
                case 0x2821: _data.inverterModel = "SOL-H800W-2T"; break;
                default:
                    // Fallback: range-based detection for b0=0x11
                    if (b0 == 0x11) {
                        if (b1 >= 0x20 && b1 <= 0x29)      _data.inverterModel = "HMS-1T (WiFi)";
                        else if (b1 >= 0x40 && b1 <= 0x49)  _data.inverterModel = "HMS-2T (WiFi)";
                        else if (b1 >= 0x60 && b1 <= 0x69)  _data.inverterModel = "HMS-4T (WiFi)";
                        else                                 _data.inverterModel = "HMS (WiFi)";
                    } else {
                        _data.inverterModel = "HMS (WiFi)";
                    }
                    break;
            }
        }
        ESP_LOGI(TAG, "Inverter: model=%s FW=%u HW_PN=0x%08X HW_VER=%u",
                 _data.inverterModel.c_str(), _data.invFwVersion,
                 _data.invHwPartNum, _data.invHwVersion);
    }
}

void DtuTcpClient::_readRespRealData(const uint8_t* payload, size_t len)
{
    _txrxState = TXRX_IDLE;

    pb_istream_t stream = pb_istream_from_buffer(payload, len);
    RealDataNewReqDTO resp = RealDataNewReqDTO_init_default;

    if (!pb_decode(&stream, &RealDataNewReqDTO_msg, &resp)) {
        ESP_LOGW(TAG, "readRespRealData: decode failed");
        return;
    }

    if (resp.timestamp == 0) {
        ESP_LOGW(TAG, "readRespRealData: zero timestamp — inverter may be offline");
        return;
    }

    ESP_LOGI(TAG, "RealData OK: ts=%u sgs=%u pv=%u",
             resp.timestamp, resp.sgs_data_count, resp.pv_data_count);

    // Grid (AC output) — first SGS entry
    if (resp.sgs_data_count > 0) {
        const SGSMO& g = resp.sgs_data[0];
        _data.gridVoltage       = _calcValue(g.voltage,       10);
        _data.gridCurrent       = _calcValue(g.current,      100);
        _data.gridPower         = _calcValue(g.active_power,  10);
        _data.gridReactivePower = _calcValue(g.reactive_power,10);
        _data.gridFrequency     = _calcValue(g.frequency,    100);
        _data.gridPowerFactor   = _calcValue(g.power_factor, 1000);
        _data.temperature       = _calcValue(g.temperature,   10);
        _data.warningsCount     = static_cast<uint8_t>(g.warning_number);
    }

    // PV strings (DC inputs) — up to 4 for HMS-xxxx-4T models
    _data.pvCount = std::min(static_cast<int>(resp.pv_data_count),
                             static_cast<int>(DtuData_t::MAX_PV_COUNT));
    for (int i = 0; i < _data.pvCount; i++) {
        const PvMO& pv = resp.pv_data[i];
        _data.pv[i].voltage     = _calcValue(pv.voltage,        10);
        _data.pv[i].current     = _calcValue(pv.current,       100);
        _data.pv[i].power       = _calcValue(pv.power,          10);
        _data.pv[i].dailyEnergy = _calcValue(pv.energy_daily,  1000); // kWh
        if (pv.energy_total != 0) {
            _data.pv[i].totalEnergy = _calcValue(pv.energy_total, 1000); // kWh
        }
    }

    // Aggregate AC energy (sum of all PV strings)
    _data.gridDailyEnergy = 0.0f;
    _data.gridTotalEnergy = 0.0f;
    for (int i = 0; i < _data.pvCount; i++) {
        _data.gridDailyEnergy += _data.pv[i].dailyEnergy;
        _data.gridTotalEnergy += _data.pv[i].totalEnergy;
    }
}

void DtuTcpClient::_readRespGetConfig(const uint8_t* payload, size_t len)
{
    _txrxState = TXRX_IDLE;

    pb_istream_t stream = pb_istream_from_buffer(payload, len);
    GetConfigReqDTO resp = GetConfigReqDTO_init_default;

    if (!pb_decode(&stream, GetConfigReqDTO_fields, &resp)) {
        ESP_LOGW(TAG, "readRespGetConfig: decode failed");
        return;
    }
    // limit_power_mypower is in percent×10 units (0–1000); convert to 0–100.
    _data.powerLimit = static_cast<uint8_t>(resp.limit_power_mypower / 10);
    ESP_LOGD(TAG, "Power limit: %u%%", _data.powerLimit);
}

void DtuTcpClient::_readRespCommand(const uint8_t* payload, size_t len)
{
    _txrxState = TXRX_IDLE;

    pb_istream_t stream = pb_istream_from_buffer(payload, len);
    CommandReqDTO resp = CommandReqDTO_init_default;

    if (!pb_decode(&stream, CommandReqDTO_fields, &resp)) {
        ESP_LOGD(TAG, "readRespCommand: decode failed (may be normal)");
        return;
    }
    ESP_LOGD(TAG, "Command response: action=%d err=%d", resp.action, resp.err_code);
}

void DtuTcpClient::_readRespGetAlarms(const uint8_t* payload, size_t len)
{
    _txrxState = TXRX_IDLE;

    // Heap-allocate WInfoReqDTO (~1 KB) to avoid async_tcp stack overflow
    auto* resp = static_cast<WInfoReqDTO*>(malloc(sizeof(WInfoReqDTO)));
    if (!resp) {
        ESP_LOGE(TAG, "readRespGetAlarms: allocation failed");
        return;
    }
    WInfoReqDTO init = WInfoReqDTO_init_default;
    memcpy(resp, &init, sizeof(WInfoReqDTO));

    pb_istream_t stream = pb_istream_from_buffer(payload, len);
    if (!pb_decode(&stream, WInfoReqDTO_fields, resp)) {
        ESP_LOGD(TAG, "readRespGetAlarms: decode failed (may be empty)");
        free(resp);
        return;
    }

    // Copy alarm entries to DtuData_t
    _data.alarmCount = 0;
    for (pb_size_t i = 0; i < resp->mWInfo_count && _data.alarmCount < DtuData_t::MAX_ALARM_ENTRIES; i++) {
        const auto& w = resp->mWInfo[i];
        if (w.pv_sn == 0) continue;

        auto& entry = _data.alarms[_data.alarmCount++];
        entry.messageId = static_cast<uint16_t>(w.WCode & 0xFF);
        entry.startTime = w.WTime1;
        entry.endTime   = w.WTime2;
    }

    ESP_LOGI(TAG, "Alarms: %u entries", _data.alarmCount);
    free(resp);

    // Signal the main task to deliver data and then close the connection.
    // Staying connected between polls caused the DTU to reset the connection.
    _dataReady.store(true, std::memory_order_release);
    _pendingDisconnect.store(true, std::memory_order_release);
}
