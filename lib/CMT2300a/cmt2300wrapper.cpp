// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "cmt2300wrapper.h"
#include "cmt2300a.h"
#include "cmt2300a_params.h"

CMT2300A::CMT2300A(uint8_t pin_sdio, uint8_t pin_clk, uint8_t pin_cs, uint8_t pin_fcs, uint32_t spi_speed)
{
    _pin_sdio = pin_sdio;
    _pin_clk = pin_clk;
    _pin_cs = pin_cs;
    _pin_fcs = pin_fcs;
    _spi_speed = spi_speed;
}

bool CMT2300A::begin(void)
{
    return _init_pins() && _init_radio();
}

bool CMT2300A::isChipConnected()
{
    return CMT2300A_IsExist();
}

bool CMT2300A::startListening(void)
{
    CMT2300A_GoStby();
    CMT2300A_ClearInterruptFlags();

    /* Must clear FIFO after enable SPI to read or write the FIFO */
    CMT2300A_EnableReadFifo();
    CMT2300A_ClearRxFifo();

    if (!CMT2300A_GoRx()) {
        return false;
    } else {
        return true;
    }
}

bool CMT2300A::stopListening(void)
{
    CMT2300A_ClearInterruptFlags();
    return CMT2300A_GoSleep();
}

bool CMT2300A::available(void)
{
    return (
        CMT2300A_MASK_PREAM_OK_FLG |
        CMT2300A_MASK_SYNC_OK_FLG |
        CMT2300A_MASK_CRC_OK_FLG |
        CMT2300A_MASK_PKT_OK_FLG
        ) & CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG);
}

void CMT2300A::read(void* buf, uint8_t len)
{
    // Fetch the payload
    CMT2300A_ReadFifo(static_cast<uint8_t*>(buf), len);

    CMT2300A_ClearInterruptFlags();
}

bool CMT2300A::write(const uint8_t* buf, uint8_t len)
{
    CMT2300A_GoStby();
    CMT2300A_ClearInterruptFlags();

    /* Must clear FIFO after enable SPI to read or write the FIFO */
    CMT2300A_EnableWriteFifo();
    CMT2300A_ClearTxFifo();

    CMT2300A_WriteReg(CMT2300A_CUS_PKT15, len); // set Tx length
    /* The length need be smaller than 32 */
    CMT2300A_WriteFifo(buf, len);

    if (!(CMT2300A_ReadReg(CMT2300A_CUS_FIFO_FLAG) & CMT2300A_MASK_TX_FIFO_NMTY_FLG)) {
        return false;
    }

    if (!CMT2300A_GoTx()) {
        return false;
    }

    uint32_t timer = millis();

    while (!(CMT2300A_MASK_TX_DONE_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_CLR1))) {
        if (millis() - timer > 95) {
            return false;
        }
    }

    CMT2300A_ClearInterruptFlags();
    CMT2300A_GoSleep();

    return true;
}

void CMT2300A::setChannel(uint8_t channel)
{
    CMT2300A_SetFrequencyChannel(channel);
}

uint8_t CMT2300A::getChannel(void)
{
    return CMT2300A_ReadReg(CMT2300A_CUS_FREQ_CHNL);
}

uint8_t CMT2300A::getDynamicPayloadSize(void)
{
    uint8_t result;
    CMT2300A_ReadFifo(&result, 1); // first byte in FiFo is length
    return result;
}

int CMT2300A::getRssiDBm()
{
    return CMT2300A_GetRssiDBm();
}

bool CMT2300A::setPALevel(int8_t level)
{
    uint16_t Tx_dBm_word;
    switch (level) {
    // for TRx Matching Network Type: 20 dBm
    case -10:
        Tx_dBm_word = 0x0501;
        break;
    case -9:
        Tx_dBm_word = 0x0601;
        break;
    case -8:
        Tx_dBm_word = 0x0701;
        break;
    case -7:
        Tx_dBm_word = 0x0801;
        break;
    case -6:
        Tx_dBm_word = 0x0901;
        break;
    case -5:
        Tx_dBm_word = 0x0A01;
        break;
    case -4:
        Tx_dBm_word = 0x0B01;
        break;
    case -3:
        Tx_dBm_word = 0x0C01;
        break;
    case -2:
        Tx_dBm_word = 0x0D01;
        break;
    case -1:
        Tx_dBm_word = 0x0E01;
        break;
    case 0:
        Tx_dBm_word = 0x1002;
        break;
    case 1:
        Tx_dBm_word = 0x1302;
        break;
    case 2:
        Tx_dBm_word = 0x1602;
        break;
    case 3:
        Tx_dBm_word = 0x1902;
        break;
    case 4:
        Tx_dBm_word = 0x1C02;
        break;
    case 5:
        Tx_dBm_word = 0x1F03;
        break;
    case 6:
        Tx_dBm_word = 0x2403;
        break;
    case 7:
        Tx_dBm_word = 0x2804;
        break;
    case 8:
        Tx_dBm_word = 0x2D04;
        break;
    case 9:
        Tx_dBm_word = 0x3305;
        break;
    case 10:
        Tx_dBm_word = 0x3906;
        break;
    case 11:
        Tx_dBm_word = 0x4107;
        break;
    case 12:
        Tx_dBm_word = 0x4908;
        break;
    case 13:
        Tx_dBm_word = 0x5309;
        break;
    case 14:
        Tx_dBm_word = 0x5E0B;
        break;
    case 15:
        Tx_dBm_word = 0x6C0C;
        break;
    case 16:
        Tx_dBm_word = 0x7D0C;
        break;
    // the following values require the double bit:
    case 17:
        Tx_dBm_word = 0x4A0C;
        break;
    case 18:
        Tx_dBm_word = 0x580F;
        break;
    case 19:
        Tx_dBm_word = 0x6B12;
        break;
    case 20:
        Tx_dBm_word = 0x8A18;
        break;
    default:
        return false;
    }
    if (level > 16) { // set bit for double Tx value
        CMT2300A_WriteReg(CMT2300A_CUS_CMT4, CMT2300A_ReadReg(CMT2300A_CUS_CMT4) | 0x01); // set bit0
    } else {
        CMT2300A_WriteReg(CMT2300A_CUS_CMT4, CMT2300A_ReadReg(CMT2300A_CUS_CMT4) & 0xFE); // reset bit0
    }
    CMT2300A_WriteReg(CMT2300A_CUS_TX8, Tx_dBm_word >> 8);
    CMT2300A_WriteReg(CMT2300A_CUS_TX9, Tx_dBm_word & 0xFF);

    return true;
}

bool CMT2300A::rxFifoAvailable()
{
    return (
        CMT2300A_MASK_PKT_OK_FLG
        ) & CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG);
}

void CMT2300A::flush_rx(void)
{
    CMT2300A_ClearRxFifo();
}

bool CMT2300A::_init_pins()
{
    CMT2300A_InitSpi(_pin_sdio, _pin_clk, _pin_cs, _pin_fcs, _spi_speed);

    return true; // assuming pins are connected properly
}

bool CMT2300A::_init_radio()
{
    if (!CMT2300A_Init()) {
        return false;
    }

    /* config registers */
    CMT2300A_ConfigRegBank(CMT2300A_CMT_BANK_ADDR, g_cmt2300aCmtBank, CMT2300A_CMT_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_SYSTEM_BANK_ADDR, g_cmt2300aSystemBank, CMT2300A_SYSTEM_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_FREQUENCY_BANK_ADDR, g_cmt2300aFrequencyBank, CMT2300A_FREQUENCY_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_DATA_RATE_BANK_ADDR, g_cmt2300aDataRateBank, CMT2300A_DATA_RATE_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_BASEBAND_BANK_ADDR, g_cmt2300aBasebandBank, CMT2300A_BASEBAND_BANK_SIZE);
    CMT2300A_ConfigRegBank(CMT2300A_TX_BANK_ADDR, g_cmt2300aTxBank, CMT2300A_TX_BANK_SIZE);

    // xosc_aac_code[2:0] = 2
    uint8_t tmp;
    tmp = (~0x07) & CMT2300A_ReadReg(CMT2300A_CUS_CMT10);
    CMT2300A_WriteReg(CMT2300A_CUS_CMT10, tmp | 0x02);

    /* Config GPIOs */
    CMT2300A_ConfigGpio(
        CMT2300A_GPIO2_SEL_INT1 | CMT2300A_GPIO3_SEL_INT2);

    /* Config interrupt */
    CMT2300A_ConfigInterrupt(
        CMT2300A_INT_SEL_TX_DONE, /* Config INT1 */
        CMT2300A_INT_SEL_PKT_OK /* Config INT2 */
    );

    /* Enable interrupt */
    CMT2300A_EnableInterrupt(
        CMT2300A_MASK_TX_DONE_EN | CMT2300A_MASK_PREAM_OK_EN | CMT2300A_MASK_SYNC_OK_EN | CMT2300A_MASK_CRC_OK_EN | CMT2300A_MASK_PKT_DONE_EN);

    CMT2300A_SetFrequencyStep(FH_OFFSET); // set FH_OFFSET (frequency = base freq + 2.5kHz*FH_OFFSET*FH_CHANNEL)

    /* Use a single 64-byte FIFO for either Tx or Rx */
    CMT2300A_EnableFifoMerge(true);

    /* Go to sleep for configuration to take effect */
    if (!CMT2300A_GoSleep()) {
        return false; // CMT2300A not switched to sleep mode!
    }

    return true;
}
