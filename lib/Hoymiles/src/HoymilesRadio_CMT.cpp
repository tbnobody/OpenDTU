// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "HoymilesRadio_CMT.h"
#include "Hoymiles.h"
#include "crc.h"
#include <FunctionalInterrupt.h>
#include <cmt2300a.h>
#include <cmt2300a_params.h>

#define CMT2300A_ONE_STEP_SIZE 2500 // frequency channel step size for fast frequency hopping operation: One step size is 2.5 kHz.
#define FH_OFFSET 100 // value * CMT2300A_ONE_STEP_SIZE = channel frequency offset
#define HOY_BASE_FREQ 860000000 // Hoymiles base frequency for CMD56 channels is 860.00 MHz
#define HOY_BOOT_FREQ 868000000 // Hoymiles boot/init frequency after power up inverter

String HoymilesRadio_CMT::cmtChToFreq(const uint8_t channel)
{
    return String((HOY_BASE_FREQ + (cmtBaseChOff860 + channel) * FH_OFFSET * CMT2300A_ONE_STEP_SIZE) / 1000000.0, 2) + " MHz";
}

void HoymilesRadio_CMT::cmtSwitchChannel(const uint8_t channel)
{
    yield();
    CMT2300A_SetFrequencyChannel(channel);
    yield();
    cmtActualCh = channel;
    // Hoymiles.getMessageOutput()->println("[cmtSwitchChannel] switched channel to " + cmtGetActFreq());
}

uint8_t HoymilesRadio_CMT::cmtFreqToChan(const String func_name, const String var_name, const uint32_t freq_kHz)
{
    if ((freq_kHz % 250) != 0) {
        Hoymiles.getMessageOutput()->println(func_name + " " + var_name + " " + String(freq_kHz / 1000.0, 3) + " MHz is not divisible by 250 kHz!");
        return 0xFF; // ERROR
    }
    const uint32_t min_Freq_kHz = (HOY_BASE_FREQ + (cmtBaseChOff860 >= 1 ? cmtBaseChOff860 : 1) * CMT2300A_ONE_STEP_SIZE * FH_OFFSET) / 1000; // frequency can not be lower than actual initailized base freq
    const uint32_t max_Freq_kHz = (HOY_BASE_FREQ + 0xFE * CMT2300A_ONE_STEP_SIZE * FH_OFFSET) / 1000; // =923500, 0xFF does not work
    if (freq_kHz < min_Freq_kHz || freq_kHz > max_Freq_kHz) {
        Hoymiles.getMessageOutput()->println(func_name + " " + var_name + " " + String(freq_kHz / 1000.0, 2) + " MHz is out of Hoymiles/CMT range! (" + String(min_Freq_kHz / 1000.0, 2) + " MHz - " + String(max_Freq_kHz / 1000.0, 2) + " MHz)");
        return 0xFF; // ERROR
    }
    if (freq_kHz < 863000 || freq_kHz > 870000)
        Hoymiles.getMessageOutput()->println(func_name + " !!! caution: " + var_name + " " + String(freq_kHz / 1000.0, 2) + " MHz is out of EU legal range! (863 - 870 MHz)");
    return (freq_kHz * 1000 - HOY_BASE_FREQ) / CMT2300A_ONE_STEP_SIZE / FH_OFFSET - cmtBaseChOff860; // frequency to channel
}

bool HoymilesRadio_CMT::cmtSwitchDtuFreq(const uint32_t to_freq_kHz)
{
    const uint8_t toChannel = cmtFreqToChan("[cmtSwitchDtuFreq]", "to_freq_kHz", to_freq_kHz);
    if (toChannel == 0xFF)
        return false;

    cmtSwitchChannel(toChannel);

    return true;
}

bool HoymilesRadio_CMT::cmtConfig(void)
{
#ifdef ENABLE_ANTENNA_SWITCH
    /* If you enable antenna switch, GPIO1/GPIO2 will output RX_ACTIVE/TX_ACTIVE,
       and it can't output INT1/INT2 via GPIO1/GPIO2 */
    CMT2300A_EnableAntennaSwitch(0);

#else
    /* Config GPIOs */
    CMT2300A_ConfigGpio(
        CMT2300A_GPIO3_SEL_INT2);

    /* Config interrupt */
    CMT2300A_ConfigInterrupt(
        CMT2300A_INT_SEL_TX_DONE, /* Config INT1 */
        CMT2300A_INT_SEL_PKT_OK /* Config INT2 */
    );
#endif

    /* Enable interrupt */
    CMT2300A_EnableInterrupt(
        CMT2300A_MASK_TX_DONE_EN | CMT2300A_MASK_PREAM_OK_EN | CMT2300A_MASK_SYNC_OK_EN | CMT2300A_MASK_CRC_OK_EN | CMT2300A_MASK_PKT_DONE_EN);

    /* Disable low frequency OSC calibration */
    // CMT2300A_EnableLfosc(FALSE);

    CMT2300A_SetFrequencyStep(100); // set FH_OFFSET to 100 (frequency = base freq + 2.5kHz*FH_OFFSET*FH_CHANNEL)

    /* Use a single 64-byte FIFO for either Tx or Rx */
    CMT2300A_EnableFifoMerge(true);

    /* Go to sleep for configuration to take effect */
    if (!CMT2300A_GoSleep())
        return false; // CMT2300A not switched to sleep mode!

    return true;
}

bool HoymilesRadio_CMT::cmtSwitchInvAndDtuFreq(const uint64_t inv_serial, const uint32_t from_freq_kHz, const uint32_t to_freq_kHz)
{
    const uint8_t fromChannel = cmtFreqToChan("[cmtSwitchInvAndDtuFreq]", "from_freq_kHz", from_freq_kHz);
    const uint8_t toChannel = cmtFreqToChan("[cmtSwitchInvAndDtuFreq]", "to_freq_kHz", to_freq_kHz);
    if (fromChannel == 0xFF || toChannel == 0xFF)
        return false;

    cmtSwitchChannel(fromChannel);
    cmtTx56toCh = toChannel;

    // CMD56 for inverter frequency/channel switch
    cmtTxBuffer[0] = 0x56;
    // cmtTxBuffer[1-4] = last inverter serial
    // cmtTxBuffer[5-8] = dtu serial
    cmtTxBuffer[9] = 0x02;
    cmtTxBuffer[10] = 0x15;
    cmtTxBuffer[11] = 0x21;
    cmtTxBuffer[12] = (uint8_t)(cmtBaseChOff860 + toChannel);
    cmtTxBuffer[13] = 0x14;
    cmtTxBuffer[14] = crc8(cmtTxBuffer, 14);

    Hoymiles.getMessageOutput()->print("TX CMD56 ");
    Hoymiles.getMessageOutput()->print(cmtChToFreq(cmtActualCh));
    Hoymiles.getMessageOutput()->print(" --> ");
    dumpBuf("", cmtTxBuffer, 15);

    cmtTxLength = 15;
    cmtTxTimeout = 100;

    cmtNextState = CMT_STATE_TX_START;

    //_busyFlag = true;

    return true;
}

enumCMTresult HoymilesRadio_CMT::cmtProcess(void)
{
    enumCMTresult nRes = CMT_BUSY;

    switch (cmtNextState) {
    case CMT_STATE_IDLE: {
        nRes = CMT_IDLE;
        break;
    }
    case CMT_STATE_RX_START: {
        CMT2300A_GoStby();
        CMT2300A_ClearInterruptFlags();

        /* Must clear FIFO after enable SPI to read or write the FIFO */
        CMT2300A_EnableReadFifo();
        CMT2300A_ClearRxFifo();

        if (!CMT2300A_GoRx())
            cmtNextState = CMT_STATE_ERROR;
        else
            cmtNextState = CMT_STATE_RX_WAIT;

        cmtRxTimeCount = CMT2300A_GetTickCount();
        cmtRxTimeout = 200;

        break;
    }
    case CMT_STATE_RX_WAIT: {
#ifdef ENABLE_ANTENNA_SWITCH
        if (CMT2300A_MASK_PKT_OK_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG)) /* Read PKT_OK flag */
#else
        if (_packetReceived) /* Read INT2, PKT_OK */
#endif
        {
            Hoymiles.getMessageOutput()->println("Interrupt received");
            _packetReceived = false; // reset interrupt
            cmtNextState = CMT_STATE_RX_DONE;
        }

        if ((CMT2300A_GetTickCount() - cmtRxTimeCount) > cmtRxTimeout)
            cmtNextState = CMT_STATE_RX_TIMEOUT;

        break;
    }
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
                f.channel = cmtActualCh;
                f.rssi = CMT2300A_GetRssiDBm();
                if (f.len > MAX_RF_PAYLOAD_SIZE)
                    f.len = MAX_RF_PAYLOAD_SIZE;
                CMT2300A_ReadFifo(f.fragment, f.len);
                if (f.fragment[9] & 0x80) // last frame detection for end Rx
                    isLastFrame = true;
                _rxBuffer.push(f);
            } else {
                Hoymiles.getMessageOutput()->println("Buffer full");
            }
        } else if ((state & 0x19) == 0x19)
            Hoymiles.getMessageOutput()->println("[CMT_STATE_RX_DONE] state: " + String(state, HEX) + " (CRC_ERROR)");
        else
            Hoymiles.getMessageOutput()->println("[CMT_STATE_RX_DONE] wrong state: " + String(state, HEX));

        CMT2300A_ClearInterruptFlags();

        CMT2300A_GoSleep();

        if (isLastFrame) // last frame received
            cmtNextState = CMT_STATE_IDLE;
        else
            cmtNextState = CMT_STATE_RX_START; // receive next frame(s)

        nRes = CMT_RX_DONE;
        break;
    }
    case CMT_STATE_RX_TIMEOUT: {
        CMT2300A_GoSleep();

        Hoymiles.getMessageOutput()->println("RX timeout!");

        cmtNextState = CMT_STATE_IDLE;

        // send CMD56 after 3 Rx timeouts
        if (cmtRxTimeoutCnt < 2)
            cmtRxTimeoutCnt++;
        else {
            uint32_t invSerial = cmtTxBuffer[1] << 24 | cmtTxBuffer[2] << 16 | cmtTxBuffer[3] << 8 | cmtTxBuffer[4]; // read inverter serial from last Tx buffer
            cmtSwitchInvAndDtuFreq(invSerial, HOY_BOOT_FREQ / 1000, CMT_WORK_FREQ);
        }

        nRes = CMT_RX_TIMEOUT;
        break;
    }
    case CMT_STATE_TX_START: {
        CMT2300A_GoStby();
        CMT2300A_ClearInterruptFlags();

        /* Must clear FIFO after enable SPI to read or write the FIFO */
        CMT2300A_EnableWriteFifo();
        CMT2300A_ClearTxFifo();

        CMT2300A_WriteReg(CMT2300A_CUS_PKT15, cmtTxLength); // set Tx length
        /* The length need be smaller than 32 */
        CMT2300A_WriteFifo(cmtTxBuffer, cmtTxLength);

        if (!(CMT2300A_ReadReg(CMT2300A_CUS_FIFO_FLAG) & CMT2300A_MASK_TX_FIFO_NMTY_FLG))
            cmtNextState = CMT_STATE_ERROR;

        if (!CMT2300A_GoTx())
            cmtNextState = CMT_STATE_ERROR;
        else
            cmtNextState = CMT_STATE_TX_WAIT;

        cmtTxTimeCount = CMT2300A_GetTickCount();

        break;
    }
    case CMT_STATE_TX_WAIT: {
        // #ifdef ENABLE_ANTENNA_SWITCH
        if (CMT2300A_MASK_TX_DONE_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_CLR1)) /* Read TX_DONE flag */
        // #else
        //             if(CMT2300A_ReadGpio1())  /* Read INT1, TX_DONE */
        // #endif
        {
            cmtNextState = CMT_STATE_TX_DONE;
        }

        if ((CMT2300A_GetTickCount() - cmtTxTimeCount) > cmtTxTimeout)
            cmtNextState = CMT_STATE_TX_TIMEOUT;

        break;
    }
    case CMT_STATE_TX_DONE: {
        CMT2300A_ClearInterruptFlags();
        CMT2300A_GoSleep();

        if (cmtTx56toCh != 0xFF) {
            cmtSwitchChannel(cmtTx56toCh);
            cmtTx56toCh = 0xFF;
            cmtNextState = CMT_STATE_IDLE;
        } else
            cmtNextState = CMT_STATE_RX_START; // receive answer

        nRes = CMT_TX_DONE;
        break;
    }
    case CMT_STATE_TX_TIMEOUT: {
        CMT2300A_GoSleep();

        Hoymiles.getMessageOutput()->println("TC timeout!");

        if (cmtTx56toCh != 0xFF) {
            cmtTx56toCh = 0xFF;
            cmtNextState = CMT_STATE_IDLE;
        }

        cmtNextState = CMT_STATE_IDLE;

        nRes = CMT_TX_TIMEOUT;
        break;
    }
    case CMT_STATE_ERROR: {
        CMT2300A_SoftReset();
        CMT2300A_DelayMs(20);

        CMT2300A_GoStby();
        cmtConfig();

        cmtNextState = CMT_STATE_IDLE;

        nRes = CMT_ERROR;
        break;
    }
    default:
        break;
    }

    return nRes;
}

void HoymilesRadio_CMT::init()
{
    _dtuSerial.u64 = 0;
    uint8_t tmp;

    CMT2300A_InitSpi();
    if (!CMT2300A_Init()) {
        Hoymiles.getMessageOutput()->println("CMT2300A_Init() failed!");
        return;
    }

    /* config registers */
    CMT2300A_ConfigRegBank(CMT2300A_CMT_BANK_ADDR, g_cmt2300aCmtBank, CMT2300A_CMT_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_SYSTEM_BANK_ADDR, g_cmt2300aSystemBank, CMT2300A_SYSTEM_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_FREQUENCY_BANK_ADDR, g_cmt2300aFrequencyBank, CMT2300A_FREQUENCY_BANK_SIZE); // cmtBaseChOff860 need to be changed to the same frequency for channel calculation
    CMT2300A_ConfigRegBank(CMT2300A_DATA_RATE_BANK_ADDR, g_cmt2300aDataRateBank, CMT2300A_DATA_RATE_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_BASEBAND_BANK_ADDR, g_cmt2300aBasebandBank, CMT2300A_BASEBAND_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_TX_BANK_ADDR, g_cmt2300aTxBank, CMT2300A_TX_BANK_SIZE);

    cmtBaseChOff860 = (860000000 - HOY_BASE_FREQ) / CMT2300A_ONE_STEP_SIZE / FH_OFFSET;

    // xosc_aac_code[2:0] = 2
    tmp = (~0x07) & CMT2300A_ReadReg(CMT2300A_CUS_CMT10);
    CMT2300A_WriteReg(CMT2300A_CUS_CMT10, tmp | 0x02);

    if (!cmtConfig()) {
        Hoymiles.getMessageOutput()->println("cmtConfig() failed!");
        return;
    }

    attachInterrupt(digitalPinToInterrupt(CMT_PIN_GPIO3), std::bind(&HoymilesRadio_CMT::handleIntr, this), RISING);

    cmtSwitchDtuFreq(CMT_WORK_FREQ); // start dtu at work freqency, for fast Rx if inverter is already on and frequency switched

    _ChipConnected = true;

    Hoymiles.getMessageOutput()->println("CMT init successful");
}

void HoymilesRadio_CMT::loop()
{
    enumCMTresult mCMTstate = cmtProcess();

    if (mCMTstate != CMT_RX_DONE) { // Perform package parsing only if no packages are received
        if (!_rxBuffer.empty()) {
            fragment_t f = _rxBuffer.back();
            if (checkFragmentCrc(&f)) {
                std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterByFragment(&f);

                if (nullptr != inv) {
                    // Save packet in inverter rx buffer
                    Hoymiles.getMessageOutput()->print("RX ");
                    Hoymiles.getMessageOutput()->print(cmtChToFreq(f.channel));
                    Hoymiles.getMessageOutput()->print(" --> ");
                    dumpBuf("", f.fragment, f.len);
                    Hoymiles.getMessageOutput()->print("| ");
                    Hoymiles.getMessageOutput()->print(f.rssi);
                    Hoymiles.getMessageOutput()->println(" dBm");

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
                Hoymiles.getMessageOutput()->println("Nothing received");
                // sendLastPacketAgain();
                _commandQueue.pop();
                _busyFlag = false;

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

bool HoymilesRadio_CMT::isIdle()
{
    return !_busyFlag;
}

bool HoymilesRadio_CMT::isConnected()
{
    return _ChipConnected;
}

void ARDUINO_ISR_ATTR HoymilesRadio_CMT::handleIntr()
{
    _packetReceived = true;
}

void HoymilesRadio_CMT::sendEsbPacket(CommandAbstract* cmd)
{
    cmd->incrementSendCount();

    cmd->setRouterAddress(DtuSerial().u64);

    Hoymiles.getMessageOutput()->print("TX ");
    Hoymiles.getMessageOutput()->print(cmd->getCommandName());
    Hoymiles.getMessageOutput()->print(" ");
    Hoymiles.getMessageOutput()->print(cmtChToFreq(cmtActualCh));
    Hoymiles.getMessageOutput()->print(" --> ");
    cmd->dumpDataPayload(Hoymiles.getMessageOutput());

    memcpy(cmtTxBuffer, cmd->getDataPayload(), cmd->getDataSize());
    cmtTxLength = cmd->getDataSize();
    cmtTxTimeout = 100;

    cmtNextState = CMT_STATE_TX_START;

    _busyFlag = true;
    _rxTimeout.set(cmd->getTimeout());
}
