// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "HoymilesRadio_CMT.h"
#include "Hoymiles.h"
#include "crc.h"
#include <FunctionalInterrupt.h>
#include <cmt2300a.h>

#define HOY_BOOT_FREQ 868000000 // Hoymiles boot/init frequency after power up inverter or connection lost for 15 min

// offset from initalized CMT base frequency to Hoy base frequency in channels
#define CMT_BASE_CH_OFFSET860 ((860000000 - CMT_BASE_FREQ) / CMT2300A_ONE_STEP_SIZE / FH_OFFSET)

// frequency can not be lower than actual initailized base freq
#define CMT_MIN_FREQ_KHZ ((CMT_BASE_FREQ + (CMT_BASE_CH_OFFSET860 >= 1 ? CMT_BASE_CH_OFFSET860 : 1) * CMT2300A_ONE_STEP_SIZE * FH_OFFSET) / 1000)

// =923500, 0xFF does not work
#define CMT_MAX_FREQ_KHZ ((CMT_BASE_FREQ + 0xFE * CMT2300A_ONE_STEP_SIZE * FH_OFFSET) / 1000)

String HoymilesRadio_CMT::cmtChToFreq(const uint8_t channel)
{
    return String((CMT_BASE_FREQ + (CMT_BASE_CH_OFFSET860 + channel) * FH_OFFSET * CMT2300A_ONE_STEP_SIZE) / 1000000.0, 2) + " MHz";
}

void HoymilesRadio_CMT::cmtSwitchChannel(const uint8_t channel)
{
    yield();
    CMT2300A_SetFrequencyChannel(channel);
    yield();
    cmtCurrentCh = channel;
}

uint8_t HoymilesRadio_CMT::cmtFreqToChan(const String& func_name, const String& var_name, const uint32_t freq_kHz)
{
    if ((freq_kHz % 250) != 0) {
        Hoymiles.getMessageOutput()->printf("%s %s %.3f MHz is not divisible by 250 kHz!\r\n",
            func_name.c_str(), var_name.c_str(), freq_kHz / 1000.0);
        return 0xFF; // ERROR
    }
    if (freq_kHz < CMT_MIN_FREQ_KHZ || freq_kHz > CMT_MAX_FREQ_KHZ) {
        Hoymiles.getMessageOutput()->printf("%s %s %.2f MHz is out of Hoymiles/CMT range! (%.2f MHz - %.2f MHz)\r\n",
            func_name.c_str(), var_name.c_str(), freq_kHz / 1000.0, CMT_MIN_FREQ_KHZ / 1000.0, CMT_MAX_FREQ_KHZ / 1000.0);
        return 0xFF; // ERROR
    }
    if (freq_kHz < 863000 || freq_kHz > 870000) {
        Hoymiles.getMessageOutput()->printf("%s !!! caution: %s %.2f MHz is out of EU legal range! (863 - 870 MHz)\r\n",
            func_name.c_str(), var_name.c_str(), freq_kHz / 1000.0);
    }
    return (freq_kHz * 1000 - CMT_BASE_FREQ) / CMT2300A_ONE_STEP_SIZE / FH_OFFSET - CMT_BASE_CH_OFFSET860; // frequency to channel
}

bool HoymilesRadio_CMT::cmtSwitchDtuFreq(const uint32_t to_freq_kHz)
{
    const uint8_t toChannel = cmtFreqToChan("[cmtSwitchDtuFreq]", "to_freq_kHz", to_freq_kHz);
    if (toChannel == 0xFF) {
        return false;
    }

    cmtSwitchChannel(toChannel);

    return true;
}

bool HoymilesRadio_CMT::cmtSwitchInvAndDtuFreq(const uint64_t inv_serial, const uint32_t from_freq_kHz, const uint32_t to_freq_kHz)
{
    const uint8_t fromChannel = cmtFreqToChan("[cmtSwitchInvAndDtuFreq]", "from_freq_kHz", from_freq_kHz);
    const uint8_t toChannel = cmtFreqToChan("[cmtSwitchInvAndDtuFreq]", "to_freq_kHz", to_freq_kHz);
    if (fromChannel == 0xFF || toChannel == 0xFF) {
        return false;
    }

    cmtSwitchChannel(fromChannel);
    cmtTx56toCh = toChannel;

    // CMD56 for inverter frequency/channel switch
    cmtTxBuffer[0] = 0x56;
    // cmtTxBuffer[1-4] = last inverter serial
    // cmtTxBuffer[5-8] = dtu serial
    cmtTxBuffer[9] = 0x02;
    cmtTxBuffer[10] = 0x15;
    cmtTxBuffer[11] = 0x21;
    cmtTxBuffer[12] = (uint8_t)(CMT_BASE_CH_OFFSET860 + toChannel);
    cmtTxBuffer[13] = 0x14;
    cmtTxBuffer[14] = crc8(cmtTxBuffer, 14);

    Hoymiles.getMessageOutput()->printf("TX CMD56 %s --> ", cmtChToFreq(cmtCurrentCh).c_str());
    dumpBuf(cmtTxBuffer, 15);

    cmtTxLength = 15;
    _txTimeout.set(100);

    cmtNextState = CMT_STATE_TX_START;

    return true;
}

enumCMTresult HoymilesRadio_CMT::cmtProcess(void)
{
    enumCMTresult nRes = CMT_BUSY;

    switch (cmtNextState) {
    case CMT_STATE_IDLE:
        nRes = CMT_IDLE;
        break;

    case CMT_STATE_RX_START:
        CMT2300A_GoStby();
        CMT2300A_ClearInterruptFlags();

        /* Must clear FIFO after enable SPI to read or write the FIFO */
        CMT2300A_EnableReadFifo();
        CMT2300A_ClearRxFifo();

        if (!CMT2300A_GoRx()) {
            cmtNextState = CMT_STATE_ERROR;
        } else {
            cmtNextState = CMT_STATE_RX_WAIT;
        }

        cmtRxTimeCount = CMT2300A_GetTickCount();
        cmtRxTimeout = 200;

        break;

    case CMT_STATE_RX_WAIT:
        if (!_gpio3_configured) {
            if (CMT2300A_MASK_PKT_OK_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG)) { // read INT2, PKT_OK flag
                _packetReceived = true;
            }
        }

        if (_packetReceived)
        {
            Hoymiles.getMessageOutput()->println("Interrupt 2 received");
            _packetReceived = false; // reset interrupt 2
            cmtNextState = CMT_STATE_RX_DONE;
        }

        if ((CMT2300A_GetTickCount() - cmtRxTimeCount) > cmtRxTimeout) {
            cmtNextState = CMT_STATE_RX_TIMEOUT;
        }

        break;

    case CMT_STATE_RX_DONE: {
        CMT2300A_GoStby();

        bool isLastFrame = false;

        uint8_t state = CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG);
        if ((state & 0x1b) == 0x1b) {
            cmtRxTimeoutCnt = 0;

            if (!(_rxBuffer.size() > FRAGMENT_BUFFER_SIZE)) {
                fragment_t f;
                memset(f.fragment, 0xcc, MAX_RF_PAYLOAD_SIZE);
                CMT2300A_ReadFifo(&f.len, 1); // first byte in FiFo is length
                f.channel = cmtCurrentCh;
                f.rssi = CMT2300A_GetRssiDBm();
                if (f.len > MAX_RF_PAYLOAD_SIZE) {
                    f.len = MAX_RF_PAYLOAD_SIZE;
                }
                CMT2300A_ReadFifo(f.fragment, f.len);
                if (f.fragment[9] & 0x80) { // last frame detection for end Rx
                    isLastFrame = true;
                }
                _rxBuffer.push(f);
            } else {
                Hoymiles.getMessageOutput()->println("Buffer full");
            }
        } else if ((state & 0x19) == 0x19) {
            Hoymiles.getMessageOutput()->printf("[CMT_STATE_RX_DONE] state: %x (CRC_ERROR)\r\n", state);
        } else {
            Hoymiles.getMessageOutput()->printf("[CMT_STATE_RX_DONE] wrong state: %x\r\n", state);
        }

        CMT2300A_ClearInterruptFlags();

        CMT2300A_GoSleep();

        if (isLastFrame) { // last frame received
            cmtNextState = CMT_STATE_IDLE;
        } else {
            cmtNextState = CMT_STATE_RX_START; // receive next frame(s)
        }

        nRes = CMT_RX_DONE;
        break;
    }

    case CMT_STATE_RX_TIMEOUT:
        CMT2300A_GoSleep();

        Hoymiles.getMessageOutput()->println("RX timeout!");

        cmtNextState = CMT_STATE_IDLE;

        // send CMD56 after 3 Rx timeouts
        if (cmtRxTimeoutCnt < 2) {
            cmtRxTimeoutCnt++;
        } else {
            uint32_t invSerial = cmtTxBuffer[1] << 24 | cmtTxBuffer[2] << 16 | cmtTxBuffer[3] << 8 | cmtTxBuffer[4]; // read inverter serial from last Tx buffer
            cmtSwitchInvAndDtuFreq(invSerial, HOY_BOOT_FREQ / 1000, _inverterTargetFrequency);
        }

        nRes = CMT_RX_TIMEOUT;
        break;

    case CMT_STATE_TX_START:
        CMT2300A_GoStby();
        CMT2300A_ClearInterruptFlags();

        /* Must clear FIFO after enable SPI to read or write the FIFO */
        CMT2300A_EnableWriteFifo();
        CMT2300A_ClearTxFifo();

        CMT2300A_WriteReg(CMT2300A_CUS_PKT15, cmtTxLength); // set Tx length
        /* The length need be smaller than 32 */
        CMT2300A_WriteFifo(cmtTxBuffer, cmtTxLength);

        if (!(CMT2300A_ReadReg(CMT2300A_CUS_FIFO_FLAG) & CMT2300A_MASK_TX_FIFO_NMTY_FLG)) {
            cmtNextState = CMT_STATE_ERROR;
        }

        if (!CMT2300A_GoTx()) {
            cmtNextState = CMT_STATE_ERROR;
        } else {
            cmtNextState = CMT_STATE_TX_WAIT;
        }

        _txTimeout.reset();

        break;

    case CMT_STATE_TX_WAIT:
        if (!_gpio2_configured) {
            if (CMT2300A_MASK_TX_DONE_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_CLR1)) { // read INT1, TX_DONE flag
                _packetSent = true;
            }
        }
        if (_packetSent) {
            Hoymiles.getMessageOutput()->println(F("Interrupt 1 received"));
            _packetSent = false; // reset interrupt 1
            cmtNextState = CMT_STATE_TX_DONE;
        }

        if (_txTimeout.occured()) {
            cmtNextState = CMT_STATE_TX_TIMEOUT;
        }

        break;

    case CMT_STATE_TX_DONE:
        CMT2300A_ClearInterruptFlags();
        CMT2300A_GoSleep();

        if (cmtTx56toCh != 0xFF) {
            cmtSwitchChannel(cmtTx56toCh);
            cmtTx56toCh = 0xFF;
            cmtNextState = CMT_STATE_IDLE;
        } else {
            cmtNextState = CMT_STATE_RX_START; // receive answer
        }

        nRes = CMT_TX_DONE;
        break;

    case CMT_STATE_TX_TIMEOUT:
        CMT2300A_GoSleep();

        Hoymiles.getMessageOutput()->println("TX timeout!");

        if (cmtTx56toCh != 0xFF) {
            cmtTx56toCh = 0xFF;
            cmtNextState = CMT_STATE_IDLE;
        }

        cmtNextState = CMT_STATE_IDLE;

        nRes = CMT_TX_TIMEOUT;
        break;

    case CMT_STATE_ERROR:
        CMT2300A_SoftReset();
        CMT2300A_DelayMs(20);

        CMT2300A_GoStby();
        _radio->begin();

        cmtNextState = CMT_STATE_IDLE;

        nRes = CMT_ERROR;
        break;

    default:
        break;
    }

    return nRes;
}

void HoymilesRadio_CMT::init(int8_t pin_sdio, int8_t pin_clk, int8_t pin_cs, int8_t pin_fcs, int8_t pin_gpio2, int8_t pin_gpio3)
{
    _dtuSerial.u64 = 0;

    _radio.reset(new CMT2300A(pin_sdio, pin_clk, pin_cs, pin_fcs));

    _radio->begin();

    cmtSwitchDtuFreq(_inverterTargetFrequency); // start dtu at work freqency, for fast Rx if inverter is already on and frequency switched

    if (_radio->isChipConnected()) {
        Hoymiles.getMessageOutput()->println("Connection successful");
    } else {
        Hoymiles.getMessageOutput()->println("Connection error!!");
    }

    if (pin_gpio2 >= 0) {
        attachInterrupt(digitalPinToInterrupt(pin_gpio2), std::bind(&HoymilesRadio_CMT::handleInt1, this), RISING);
        _gpio2_configured = true;
    }

    if (pin_gpio3 >= 0) {
        attachInterrupt(digitalPinToInterrupt(pin_gpio3), std::bind(&HoymilesRadio_CMT::handleInt2, this), RISING);
        _gpio3_configured = true;
    }

    _isInitialized = true;
}

void HoymilesRadio_CMT::loop()
{
    if (!_isInitialized) {
        return;
    }
    enumCMTresult mCMTstate = cmtProcess();

    if (mCMTstate != CMT_RX_DONE) { // Perform package parsing only if no packages are received
        if (!_rxBuffer.empty()) {
            fragment_t f = _rxBuffer.back();
            if (checkFragmentCrc(&f)) {
                std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterByFragment(&f);

                if (nullptr != inv) {
                    // Save packet in inverter rx buffer
                    Hoymiles.getMessageOutput()->printf("RX %s --> ", cmtChToFreq(f.channel).c_str());
                    dumpBuf(f.fragment, f.len, false);
                    Hoymiles.getMessageOutput()->printf("| %d dBm\r\n", f.rssi);

                    inv->addRxFragment(f.fragment, f.len);
                } else {
                    Hoymiles.getMessageOutput()->println("Inverter Not found!");
                }

            } else {
                Hoymiles.getMessageOutput()->println("Frame kaputt"); // ;-)
            }

            // Remove paket from buffer even it was corrupted
            _rxBuffer.pop();
        }
    }

    if (_busyFlag && _rxTimeout.occured()) {
        Hoymiles.getMessageOutput()->println("RX Period End");
        std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterBySerial(_commandQueue.front().get()->getTargetAddress());

        if (nullptr != inv) {
            CommandAbstract* cmd = _commandQueue.front().get();
            uint8_t verifyResult = inv->verifyAllFragments(cmd);
            if (verifyResult == FRAGMENT_ALL_MISSING_RESEND) {
                Hoymiles.getMessageOutput()->println("Nothing received, resend whole request");
                sendLastPacketAgain();

            } else if (verifyResult == FRAGMENT_ALL_MISSING_TIMEOUT) {
                Hoymiles.getMessageOutput()->println("Nothing received, resend count exeeded");
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_RETRANSMIT_TIMEOUT) {
                Hoymiles.getMessageOutput()->println("Retransmit timeout");
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult == FRAGMENT_HANDLE_ERROR) {
                Hoymiles.getMessageOutput()->println("Packet handling error");
                _commandQueue.pop();
                _busyFlag = false;

            } else if (verifyResult > 0) {
                // Perform Retransmit
                Hoymiles.getMessageOutput()->print("Request retransmit: ");
                Hoymiles.getMessageOutput()->println(verifyResult);
                sendRetransmitPacket(verifyResult);

            } else {
                // Successful received all packages
                Hoymiles.getMessageOutput()->println("Success");
                _commandQueue.pop();
                _busyFlag = false;
            }
        } else {
            // If inverter was not found, assume the command is invalid
            Hoymiles.getMessageOutput()->println("RX: Invalid inverter found");
            _commandQueue.pop();
            _busyFlag = false;
        }
    } else if (!_busyFlag) {
        // Currently in idle mode --> send packet if one is in the queue
        if (!_commandQueue.empty()) {
            CommandAbstract* cmd = _commandQueue.front().get();

            auto inv = Hoymiles.getInverterBySerial(cmd->getTargetAddress());
            if (nullptr != inv) {
                inv->clearRxFragmentBuffer();
                sendEsbPacket(cmd);
            } else {
                Hoymiles.getMessageOutput()->println("TX: Invalid inverter found");
                _commandQueue.pop();
            }
        }
    }
}

void HoymilesRadio_CMT::setPALevel(int8_t paLevel)
{
    if (!_isInitialized) {
        return;
    }

    if (_radio->setPALevel(paLevel)) {
        Hoymiles.getMessageOutput()->printf("CMT TX power set to %d dBm\r\n", paLevel);
    } else {
        Hoymiles.getMessageOutput()->printf("CMT TX power %d dBm is not defined! (min: -10 dBm, max: 20 dBm)\r\n", paLevel);
    }
}

void HoymilesRadio_CMT::setInverterTargetFrequency(uint32_t frequency)
{
    _inverterTargetFrequency = frequency;
    if (!_isInitialized) {
        return;
    }
    cmtSwitchDtuFreq(_inverterTargetFrequency);
}

bool HoymilesRadio_CMT::isConnected()
{
    if (!_isInitialized) {
        return false;
    }
    return _radio->isChipConnected();
}

uint32_t HoymilesRadio_CMT::getMinFrequency()
{
    return CMT_MIN_FREQ_KHZ;
}

uint32_t HoymilesRadio_CMT::getMaxFrequency()
{
    return CMT_MAX_FREQ_KHZ;
}

void ARDUINO_ISR_ATTR HoymilesRadio_CMT::handleInt1()
{
    _packetSent = true;
}

void ARDUINO_ISR_ATTR HoymilesRadio_CMT::handleInt2()
{
    _packetReceived = true;
}

void HoymilesRadio_CMT::sendEsbPacket(CommandAbstract* cmd)
{
    cmd->incrementSendCount();

    cmd->setRouterAddress(DtuSerial().u64);

    Hoymiles.getMessageOutput()->printf("TX %s %s --> ",
        cmd->getCommandName().c_str(), cmtChToFreq(cmtCurrentCh).c_str());
    cmd->dumpDataPayload(Hoymiles.getMessageOutput());

    // Still here for to handle CMD56 correctly (inverter serial etc.)
    memcpy(cmtTxBuffer, cmd->getDataPayload(), cmd->getDataSize());

    if (_radio->write(cmd->getDataPayload(), cmd->getDataSize())) {
        _packetSent = false; // still bad hack, to be removed
        cmtNextState = CMT_STATE_RX_START;
    } else {
        Hoymiles.getMessageOutput()->println("TX SPI Timeout");
    }

    _busyFlag = true;
    _rxTimeout.set(cmd->getTimeout());
}
