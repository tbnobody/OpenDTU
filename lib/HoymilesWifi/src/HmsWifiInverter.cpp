// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * InverterAbstract subclass for WiFi-connected Hoymiles HMS inverters.
 */
#include "HmsWifiInverter.h"
#include <parser/AlarmLogParser.h>
#include <esp_log.h>

#undef TAG
static const char* TAG = "HmsWifiInverter";

// ---------------------------------------------------------------------------
// Byte assignment variants — declare the fields each model exposes.
// Byte positions are dedicated storage slots in _payloadStatistic (112 bytes).
// They are never written by RF fragment parsing; values are set directly via
// setChannelFieldValue() from the TCP callback.
//
// Three variants for 1-string, 2-string, and 4-string models, matching the
// same serial-prefix detection as RF HMS_1CH / HMS_2CH / HMS_4CH.
// ---------------------------------------------------------------------------

// 1-string model (HMS-300/350/400/450/500-1T)
// Layout: DC CH0 0–11, AC CH0 12–23, INV CH0 24–35
static const byteAssign_t byteAssignment_1CH[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V,   0, 2,   10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A,   2, 2,  100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W,   4, 2,   10, false, 1 },
    { TYPE_DC, CH0, FLD_YD,  UNIT_WH,  6, 2,    1, false, 0 },
    { TYPE_DC, CH0, FLD_YT,  UNIT_KWH, 8, 4, 1000, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V,   12, 2,   10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A,   14, 2,  100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W,   16, 2,   10, false, 1 },
    { TYPE_AC, CH0, FLD_Q,   UNIT_VAR, 18, 2,   10, true,  1 },
    { TYPE_AC, CH0, FLD_F,   UNIT_HZ,  20, 2,  100, false, 2 },
    { TYPE_AC, CH0, FLD_PF,  UNIT_NONE,22, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T,       UNIT_C,   24, 2,   10, true,  1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE,26, 2,    1, false, 0 },
    { TYPE_INV, CH0, FLD_YD,      UNIT_WH,  28, 2,    1, false, 0 },
    { TYPE_INV, CH0, FLD_YT,      UNIT_KWH, 30, 4, 1000, false, 3 },
    { TYPE_INV, CH0, FLD_PDC,     UNIT_W,   34, 2,   10, false, 1 },
};

// 2-string model (HMS-600/700/800/900/1000-2T) — default
// Layout: DC CH0 0–11, DC CH1 12–23, AC CH0 24–35, INV CH0 36–47
static const byteAssign_t byteAssignment_2CH[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V,   0, 2,   10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A,   2, 2,  100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W,   4, 2,   10, false, 1 },
    { TYPE_DC, CH0, FLD_YD,  UNIT_WH,  6, 2,    1, false, 0 },
    { TYPE_DC, CH0, FLD_YT,  UNIT_KWH, 8, 4, 1000, false, 3 },

    { TYPE_DC, CH1, FLD_UDC, UNIT_V,  12, 2,   10, false, 1 },
    { TYPE_DC, CH1, FLD_IDC, UNIT_A,  14, 2,  100, false, 2 },
    { TYPE_DC, CH1, FLD_PDC, UNIT_W,  16, 2,   10, false, 1 },
    { TYPE_DC, CH1, FLD_YD,  UNIT_WH, 18, 2,    1, false, 0 },
    { TYPE_DC, CH1, FLD_YT,  UNIT_KWH,20, 4, 1000, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V,   24, 2,   10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A,   26, 2,  100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W,   28, 2,   10, false, 1 },
    { TYPE_AC, CH0, FLD_Q,   UNIT_VAR, 30, 2,   10, true,  1 },
    { TYPE_AC, CH0, FLD_F,   UNIT_HZ,  32, 2,  100, false, 2 },
    { TYPE_AC, CH0, FLD_PF,  UNIT_NONE,34, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T,       UNIT_C,   36, 2,   10, true,  1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE,38, 2,    1, false, 0 },
    { TYPE_INV, CH0, FLD_YD,      UNIT_WH,  40, 2,    1, false, 0 },
    { TYPE_INV, CH0, FLD_YT,      UNIT_KWH, 42, 4, 1000, false, 3 },
    { TYPE_INV, CH0, FLD_PDC,     UNIT_W,   46, 2,   10, false, 1 },
};

// 4-string model (HMS-1600/1800/2000-4T)
// Layout: DC CH0 0–11, CH1 12–23, CH2 24–35, CH3 36–47, AC CH0 48–59, INV CH0 60–71
static const byteAssign_t byteAssignment_4CH[] = {
    { TYPE_DC, CH0, FLD_UDC, UNIT_V,   0, 2,   10, false, 1 },
    { TYPE_DC, CH0, FLD_IDC, UNIT_A,   2, 2,  100, false, 2 },
    { TYPE_DC, CH0, FLD_PDC, UNIT_W,   4, 2,   10, false, 1 },
    { TYPE_DC, CH0, FLD_YD,  UNIT_WH,  6, 2,    1, false, 0 },
    { TYPE_DC, CH0, FLD_YT,  UNIT_KWH, 8, 4, 1000, false, 3 },

    { TYPE_DC, CH1, FLD_UDC, UNIT_V,  12, 2,   10, false, 1 },
    { TYPE_DC, CH1, FLD_IDC, UNIT_A,  14, 2,  100, false, 2 },
    { TYPE_DC, CH1, FLD_PDC, UNIT_W,  16, 2,   10, false, 1 },
    { TYPE_DC, CH1, FLD_YD,  UNIT_WH, 18, 2,    1, false, 0 },
    { TYPE_DC, CH1, FLD_YT,  UNIT_KWH,20, 4, 1000, false, 3 },

    { TYPE_DC, CH2, FLD_UDC, UNIT_V,  24, 2,   10, false, 1 },
    { TYPE_DC, CH2, FLD_IDC, UNIT_A,  26, 2,  100, false, 2 },
    { TYPE_DC, CH2, FLD_PDC, UNIT_W,  28, 2,   10, false, 1 },
    { TYPE_DC, CH2, FLD_YD,  UNIT_WH, 30, 2,    1, false, 0 },
    { TYPE_DC, CH2, FLD_YT,  UNIT_KWH,32, 4, 1000, false, 3 },

    { TYPE_DC, CH3, FLD_UDC, UNIT_V,  36, 2,   10, false, 1 },
    { TYPE_DC, CH3, FLD_IDC, UNIT_A,  38, 2,  100, false, 2 },
    { TYPE_DC, CH3, FLD_PDC, UNIT_W,  40, 2,   10, false, 1 },
    { TYPE_DC, CH3, FLD_YD,  UNIT_WH, 42, 2,    1, false, 0 },
    { TYPE_DC, CH3, FLD_YT,  UNIT_KWH,44, 4, 1000, false, 3 },

    { TYPE_AC, CH0, FLD_UAC, UNIT_V,   48, 2,   10, false, 1 },
    { TYPE_AC, CH0, FLD_IAC, UNIT_A,   50, 2,  100, false, 2 },
    { TYPE_AC, CH0, FLD_PAC, UNIT_W,   52, 2,   10, false, 1 },
    { TYPE_AC, CH0, FLD_Q,   UNIT_VAR, 54, 2,   10, true,  1 },
    { TYPE_AC, CH0, FLD_F,   UNIT_HZ,  56, 2,  100, false, 2 },
    { TYPE_AC, CH0, FLD_PF,  UNIT_NONE,58, 2, 1000, false, 3 },

    { TYPE_INV, CH0, FLD_T,       UNIT_C,   60, 2,   10, true,  1 },
    { TYPE_INV, CH0, FLD_EVT_LOG, UNIT_NONE,62, 2,    1, false, 0 },
    { TYPE_INV, CH0, FLD_YD,      UNIT_WH,  64, 2,    1, false, 0 },
    { TYPE_INV, CH0, FLD_YT,      UNIT_KWH, 66, 4, 1000, false, 3 },
    { TYPE_INV, CH0, FLD_PDC,     UNIT_W,   70, 2,   10, false, 1 },
};

// Channel number constants for array indexing
static const ChannelNum_t pvChannels[] = { CH0, CH1, CH2, CH3 };

// ---------------------------------------------------------------------------
// Detect number of PV strings from serial prefix.
// Uses range-based detection following dtuGateway convention:
//   b0 = high byte, b1 = low byte of ((serial >> 32) & 0xFFFF)
//   b0=0x11: b1 in 0x20-0x29 → 1T, 0x40-0x49 → 2T, 0x60-0x69 → 4T
//   b0=0x10: 2T
//   b0=0x12: 4T
//   b0=0x14: 2T
//   b0=0x28: 2T (SOL-H series)
// ---------------------------------------------------------------------------
uint8_t HmsWifiInverter::_detectPvCount(uint64_t serial)
{
    uint16_t prefix = static_cast<uint16_t>((serial >> 32) & 0xFFFF);
    uint8_t b0 = (prefix >> 8) & 0xFF;
    uint8_t b1 = prefix & 0xFF;

    switch (b0) {
        case 0x11:
            if (b1 >= 0x20 && b1 <= 0x29) return 1;  // HMS-xxx-1T
            if (b1 >= 0x40 && b1 <= 0x49) return 2;  // HMS-xxx-2T
            if (b1 >= 0x60 && b1 <= 0x69) return 4;  // HMS-xxx-4T
            break;
        case 0x10:
            return 2;  // HMS-600W-2T variant
        case 0x12:
            return 4;  // HMS-1500W-4T and similar
        case 0x14:
            if (b1 == 0x00) return 1;  // HMS-1CHv2 (0x1400)
            if (b1 >= 0x20 && b1 <= 0x29) return 4;  // HMS-xxx-4T (0x1420)
            return 2;  // HMS-800W-2T variants (0x1410, 0x1412)
        case 0x28:
            return 2;  // SOL-H series
    }

    // Default: 2 strings (most common WiFi model)
    return 2;
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
HmsWifiInverter::HmsWifiInverter(const uint64_t serial, const char* dtuIp)
    : InverterAbstract(nullptr, serial)   // nullptr = no RF radio
    , _tcpClient(std::make_unique<DtuTcpClient>())
    , _pvCount(_detectPvCount(serial))
{
    // Select the byteAssignment variant matching the detected channel count
    switch (_pvCount) {
        case 1:
            _byteAssignment     = byteAssignment_1CH;
            _byteAssignmentSize = sizeof(byteAssignment_1CH) / sizeof(byteAssignment_1CH[0]);
            break;
        case 4:
            _byteAssignment     = byteAssignment_4CH;
            _byteAssignmentSize = sizeof(byteAssignment_4CH) / sizeof(byteAssignment_4CH[0]);
            break;
        default: // 2
            _byteAssignment     = byteAssignment_2CH;
            _byteAssignmentSize = sizeof(byteAssignment_2CH) / sizeof(byteAssignment_2CH[0]);
            break;
    }

    ESP_LOGI(TAG, "WiFi inverter %s: detected %u PV string(s)", serialString().c_str(), _pvCount);

    _tcpClient->setDataCallback(
        [this](const DtuData_t& d) { _onData(d); });
    _tcpClient->setConnectCallback(
        [this](bool connected) { _onConnect(connected); });

    _tcpClient->setup(dtuIp);
}

// ---------------------------------------------------------------------------
// InverterAbstract overrides
// ---------------------------------------------------------------------------
String HmsWifiInverter::typeName() const
{
    return "HMS (WiFi)";
}

const byteAssign_t* HmsWifiInverter::getByteAssignment() const
{
    return _byteAssignment;
}

uint8_t HmsWifiInverter::getByteAssignmentSize() const
{
    return _byteAssignmentSize;
}

bool HmsWifiInverter::sendActivePowerControlRequest(float limit,
                                                    PowerLimitControlType /*type*/)
{
    _lastPowerLimit  = limit;
    _hasPendingLimit = true;
    _tcpClient->setPowerLimit(static_cast<uint8_t>(limit));
    SystemConfigPara()->setLastLimitCommandSuccess(CMD_PENDING);
    return true;
}

bool HmsWifiInverter::resendActivePowerControlRequest()
{
    if (_lastPowerLimit < 0) return false;
    _tcpClient->setPowerLimit(static_cast<uint8_t>(_lastPowerLimit));
    return true;
}

bool HmsWifiInverter::sendPowerControlRequest(bool turnOn)
{
    _lastPowerState  = turnOn;
    _hasPendingPower = true;
    _tcpClient->setPowerState(turnOn);
    PowerCommand()->setLastPowerCommandSuccess(CMD_PENDING);
    return true;
}

bool HmsWifiInverter::resendPowerControlRequest()
{
    _tcpClient->setPowerState(_lastPowerState);
    return true;
}

bool HmsWifiInverter::sendRestartControlRequest()
{
    _tcpClient->requestRestart();
    return true;
}

void HmsWifiInverter::tick()
{
    _tcpClient->tick();
}

// ---------------------------------------------------------------------------
// TCP callbacks
// ---------------------------------------------------------------------------
void HmsWifiInverter::_onConnect(bool connected)
{
    if (connected) {
        ESP_LOGI(TAG, "Inverter %s: DTU connected", serialString().c_str());
        Statistics()->resetRxFailureCount();
    } else {
        ESP_LOGI(TAG, "Inverter %s: DTU disconnected", serialString().c_str());
        Statistics()->incrementRxFailureCount();
    }
}

void HmsWifiInverter::_onData(const DtuData_t& d)
{
    auto* stats = Statistics();

    // DC strings — populate however many the model has (1, 2, or 4)
    const uint8_t count = std::min(_pvCount, d.pvCount);
    float totalDcPower = 0.0f;
    for (uint8_t i = 0; i < count; i++) {
        stats->setChannelFieldValue(TYPE_DC, pvChannels[i], FLD_UDC, d.pv[i].voltage);
        stats->setChannelFieldValue(TYPE_DC, pvChannels[i], FLD_IDC, d.pv[i].current);
        stats->setChannelFieldValue(TYPE_DC, pvChannels[i], FLD_PDC, d.pv[i].power);
        stats->setChannelFieldValue(TYPE_DC, pvChannels[i], FLD_YD,  d.pv[i].dailyEnergy * 1000.0f); // kWh → Wh
        stats->setChannelFieldValue(TYPE_DC, pvChannels[i], FLD_YT,  d.pv[i].totalEnergy);            // kWh
        totalDcPower += d.pv[i].power;
    }

    // AC grid output
    stats->setChannelFieldValue(TYPE_AC, CH0, FLD_UAC, d.gridVoltage);
    stats->setChannelFieldValue(TYPE_AC, CH0, FLD_IAC, d.gridCurrent);
    stats->setChannelFieldValue(TYPE_AC, CH0, FLD_PAC, d.gridPower);
    stats->setChannelFieldValue(TYPE_AC, CH0, FLD_Q,   d.gridReactivePower);
    stats->setChannelFieldValue(TYPE_AC, CH0, FLD_F,   d.gridFrequency);
    stats->setChannelFieldValue(TYPE_AC, CH0, FLD_PF,  d.gridPowerFactor);

    // Inverter totals / status
    stats->setChannelFieldValue(TYPE_INV, CH0, FLD_T,       d.temperature);
    stats->setChannelFieldValue(TYPE_INV, CH0, FLD_EVT_LOG, static_cast<float>(d.warningsCount));
    stats->setChannelFieldValue(TYPE_INV, CH0, FLD_YD,      d.gridDailyEnergy * 1000.0f); // kWh → Wh
    stats->setChannelFieldValue(TYPE_INV, CH0, FLD_YT,      d.gridTotalEnergy);            // kWh
    stats->setChannelFieldValue(TYPE_INV, CH0, FLD_PDC,     totalDcPower);

    // Mark data as fresh
    stats->setLastUpdate(millis());
    stats->resetRxFailureCount();

    // Populate DevInfo once when AppInfo data becomes available
    if (d.invFwVersion != 0 && DevInfo()->getLastUpdateAll() == 0) {
        _populateDevInfo(d);
    }

    // Update power limit from config response
    SystemConfigPara()->setLimitPercent(static_cast<float>(d.powerLimit));
    SystemConfigPara()->setLastUpdateRequest(millis());
    SystemConfigPara()->setLastLimitCommandSuccess(CMD_OK);

    // Confirm commands if pending
    if (_hasPendingLimit) {
        _hasPendingLimit = false;
        SystemConfigPara()->setLastLimitCommandSuccess(CMD_OK);
    }
    if (_hasPendingPower) {
        _hasPendingPower = false;
        PowerCommand()->setLastPowerCommandSuccess(CMD_OK);
    }

    // Populate event log from WiFi alarm data
    _populateAlarms(d);

    ESP_LOGI(TAG, "Inverter %s: data updated (AC %.1f W, DC %.1f W, %u strings)",
             serialString().c_str(), d.gridPower, totalDcPower, count);
}

// ---------------------------------------------------------------------------
// Populate DevInfoParser from WiFi AppInfo data
// ---------------------------------------------------------------------------
void HmsWifiInverter::_populateDevInfo(const DtuData_t& d)
{
    auto* di = DevInfo();

    // Cache the model name reported by the DTU
    _wifiModelName = d.inverterModel;

    // DevInfoAll: FW version + build date + bootloader version
    // Byte layout: [0-1] FW version, [2-3] year, [4-5] month*100+day,
    //              [6-7] hour*100+min, [8-9] bootloader version
    uint8_t allBuf[10] = {};
    allBuf[0] = static_cast<uint8_t>((d.invFwVersion >> 8) & 0xFF);
    allBuf[1] = static_cast<uint8_t>(d.invFwVersion & 0xFF);
    // Valid date so containsValidData() passes (year > 2016)
    allBuf[2] = 0x07; allBuf[3] = 0xE8; // 2024
    allBuf[4] = 0x00; allBuf[5] = 0x65; // month=1, day=1
    allBuf[6] = 0x00; allBuf[7] = 0x00; // 00:00
    allBuf[8] = static_cast<uint8_t>((d.dtuFwVersion >> 8) & 0xFF);
    allBuf[9] = static_cast<uint8_t>(d.dtuFwVersion & 0xFF);

    di->clearBufferAll();
    di->appendFragmentAll(0, allBuf, sizeof(allBuf));
    di->setLastUpdateAll(millis());

    // DevInfoSimple: FW version + HW part number + HW version
    // The HW part number from pv_hw_pn maps directly to DevInfoParser's lookup
    // table (devInfo[]) for model name and max power detection.
    uint8_t simpleBuf[8] = {};
    simpleBuf[0] = allBuf[0]; // FW version high
    simpleBuf[1] = allBuf[1]; // FW version low
    // HW part number (4 bytes, big-endian) — matches DevInfoParser byte layout
    simpleBuf[2] = static_cast<uint8_t>((d.invHwPartNum >> 24) & 0xFF);
    simpleBuf[3] = static_cast<uint8_t>((d.invHwPartNum >> 16) & 0xFF);
    simpleBuf[4] = static_cast<uint8_t>((d.invHwPartNum >>  8) & 0xFF);
    simpleBuf[5] = static_cast<uint8_t>(d.invHwPartNum & 0xFF);
    // HW version (2 bytes)
    simpleBuf[6] = static_cast<uint8_t>((d.invHwVersion >> 8) & 0xFF);
    simpleBuf[7] = static_cast<uint8_t>(d.invHwVersion & 0xFF);

    di->clearBufferSimple();
    di->appendFragmentSimple(0, simpleBuf, sizeof(simpleBuf));
    di->setLastUpdateSimple(millis());

    // Grid profile — pv_gpf_code and pv_gpf are available from the AppInfo
    // response (APPPvInfoMO fields 7 & 8). Byte layout mirrors the RF
    // GridOnProFilePara payload so GridProfileParser can identify the profile:
    //   [0] lIdx  = (gpf_code >> 8) & 0xFF
    //   [1] hIdx  = gpf_code & 0xFF
    //   [2] version byte  = (gpf >> 8) & 0xFF  (high nibble = major)
    //   [3] version sub   = gpf & 0xFF
    // 7 bytes total — 0xFF sentinel at [4] makes getProfile() return empty
    // (detailed section values are not available via WiFi protocol).
    if (d.invGpfCode != 0) {
        uint8_t gpBuf[7] = { 0, 0, 0, 0, 0xFF, 0, 0 };
        gpBuf[0] = static_cast<uint8_t>((d.invGpfCode >> 8) & 0xFF);
        gpBuf[1] = static_cast<uint8_t>(d.invGpfCode & 0xFF);
        gpBuf[2] = static_cast<uint8_t>((d.invGpf >> 8) & 0xFF);
        gpBuf[3] = static_cast<uint8_t>(d.invGpf & 0xFF);
        GridProfile()->clearBuffer();
        GridProfile()->appendFragment(0, gpBuf, sizeof(gpBuf));
        GridProfile()->setLastUpdate(millis());
    }

    ESP_LOGI(TAG, "DevInfo populated: model=%s pvCount=%u invFW=%u HW_PN=0x%08X dtuFW=%u gpf=0x%04X/0x%04X",
             d.inverterModel.c_str(), _pvCount, d.invFwVersion, d.invHwPartNum, d.dtuFwVersion,
             static_cast<uint32_t>(d.invGpfCode), static_cast<uint32_t>(d.invGpf));
}

// ---------------------------------------------------------------------------
// Populate AlarmLogParser from WiFi alarm data
// ---------------------------------------------------------------------------
void HmsWifiInverter::_populateAlarms(const DtuData_t& d)
{
    auto* el = EventLog();

    if (d.alarmCount == 0) {
        el->clearDirectEntries();
        el->setLastAlarmRequestSuccess(CMD_OK);
        return;
    }

    // Convert DtuAlarmEntry_t → AlarmLogEntry_t with timezone-adjusted timestamps.
    // The frontend displays times via new Date(seconds * 1000) with timeZone:'UTC',
    // so we pre-add the local timezone offset (same approach as the RF path).
    const int tzOffset = AlarmLogParser::getTimezoneOffset();

    AlarmLogEntry_t entries[DtuData_t::MAX_ALARM_ENTRIES];
    for (uint8_t i = 0; i < d.alarmCount; i++) {
        entries[i].MessageId = d.alarms[i].messageId;
        entries[i].StartTime = d.alarms[i].startTime + tzOffset;
        entries[i].EndTime   = (d.alarms[i].endTime != 0) ? (d.alarms[i].endTime + tzOffset) : 0;
    }

    el->setDirectEntries(entries, d.alarmCount);
    el->setLastAlarmRequestSuccess(CMD_OK);
    el->setLastUpdate(millis());
}
