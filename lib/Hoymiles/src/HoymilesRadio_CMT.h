// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HoymilesRadio.h"
#include "TimeoutHelper.h"
#include "commands/CommandAbstract.h"
#include "types.h"
#include <Arduino.h>
#include <memory>
#include <queue>
#include <cmt2300wrapper.h>

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30

/* CMT states */
typedef enum {
    CMT_STATE_IDLE = 0,
    CMT_STATE_RX_START,
    CMT_STATE_RX_WAIT,
    CMT_STATE_RX_DONE,
    CMT_STATE_RX_TIMEOUT,
    CMT_STATE_TX_START,
    CMT_STATE_TX_WAIT,
    CMT_STATE_TX_DONE,
    CMT_STATE_TX_TIMEOUT,
    CMT_STATE_ERROR,
} enumCMTstate;

/* CMT process function results */
typedef enum {
    CMT_IDLE = 0,
    CMT_BUSY,
    CMT_RX_DONE,
    CMT_RX_TIMEOUT,
    CMT_TX_DONE,
    CMT_TX_TIMEOUT,
    CMT_ERROR,
} enumCMTresult;

class HoymilesRadio_CMT : public HoymilesRadio {
public:
    void init(int8_t pin_sdio, int8_t pin_clk, int8_t pin_cs, int8_t pin_fcs, int8_t pin_gpio3);
    void loop();
    void setPALevel(int8_t paLevel);

    bool isConnected();

private:
    void ARDUINO_ISR_ATTR handleIntr();

    void sendEsbPacket(CommandAbstract* cmd);

    std::unique_ptr<CMT2300a> _radio;

    volatile bool _packetReceived = false;

    std::queue<fragment_t> _rxBuffer;
    TimeoutHelper _rxTimeout;
    TimeoutHelper _txTimeout;

    String cmtChToFreq(const uint8_t channel);
    void cmtSwitchChannel(const uint8_t channel);
    uint8_t cmtFreqToChan(const String& func_name, const String& var_name, const uint32_t freq_kHz);
    bool cmtSwitchDtuFreq(const uint32_t to_freq_kHz);
    bool cmtSwitchInvAndDtuFreq(const uint64_t inv_serial, const uint32_t from_freq_kHz, const uint32_t to_freq_kHz);
    enumCMTresult cmtProcess(void);

    enumCMTstate cmtNextState = CMT_STATE_IDLE;
    uint8_t cmtTxBuffer[32];
    uint8_t cmtTxLength = 0;

    uint32_t cmtRxTimeout = 200;
    uint32_t cmtRxTimeCount = 0;

    uint8_t cmtBaseChOff860; // offset from initalized CMT base frequency to Hoy base frequency in channels
    uint8_t cmtCurrentCh; // current used channel, should be stored per inverter und set before next Tx, if hopping is used

    uint8_t cmtTx56toCh = 0xFF; // send CMD56 active to Channel xx, inactive = 0xFF

    uint8_t cmtRxTimeoutCnt = 0; // Rx timeout counter !!! should be stored per inverter !!!
};