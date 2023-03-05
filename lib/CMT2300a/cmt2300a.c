/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, CMOSTEK SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * Copyright (C) CMOSTEK SZ.
 */

/*!
 * @file    cmt2300a.c
 * @brief   CMT2300A transceiver RF chip driver
 *
 * @version 1.3
 * @date    Jul 17 2017
 * @author  CMOSTEK R@D
 */

#include "cmt2300a.h"

/*! ********************************************************
 * @name    CMT2300A_SoftReset
 * @desc    Soft reset.
 * *********************************************************/
void CMT2300A_SoftReset(void)
{
    CMT2300A_WriteReg(0x7F, 0xFF);
}

/*! ********************************************************
 * @name    CMT2300A_GetChipStatus
 * @desc    Get the chip status.
 * @return
 *          CMT2300A_STA_PUP
 *          CMT2300A_STA_SLEEP
 *          CMT2300A_STA_STBY
 *          CMT2300A_STA_RFS
 *          CMT2300A_STA_TFS
 *          CMT2300A_STA_RX
 *          CMT2300A_STA_TX
 *          CMT2300A_STA_EEPROM
 *          CMT2300A_STA_ERROR
 *          CMT2300A_STA_CAL
 * *********************************************************/
uint8_t CMT2300A_GetChipStatus(void)
{
    return CMT2300A_ReadReg(CMT2300A_CUS_MODE_STA) & CMT2300A_MASK_CHIP_MODE_STA;
}

/*! ********************************************************
 * @name    CMT2300A_AutoSwitchStatus
 * @desc    Auto switch the chip status, and 10 ms as timeout.
 * @param   nGoCmd: the chip next status
 * @return  TRUE or FALSE
 * *********************************************************/
bool CMT2300A_AutoSwitchStatus(uint8_t nGoCmd)
{
#ifdef ENABLE_AUTO_SWITCH_CHIP_STATUS
    uint32_t nBegTick = CMT2300A_GetTickCount();
    uint8_t nWaitStatus = 0;

    switch (nGoCmd) {
    case CMT2300A_GO_SLEEP:
        nWaitStatus = CMT2300A_STA_SLEEP;
        break;
    case CMT2300A_GO_STBY:
        nWaitStatus = CMT2300A_STA_STBY;
        break;
    case CMT2300A_GO_TFS:
        nWaitStatus = CMT2300A_STA_TFS;
        break;
    case CMT2300A_GO_TX:
        nWaitStatus = CMT2300A_STA_TX;
        break;
    case CMT2300A_GO_RFS:
        nWaitStatus = CMT2300A_STA_RFS;
        break;
    case CMT2300A_GO_RX:
        nWaitStatus = CMT2300A_STA_RX;
        break;
    }

    CMT2300A_WriteReg(CMT2300A_CUS_MODE_CTL, nGoCmd);

    while (CMT2300A_GetTickCount() - nBegTick < 10) {
        CMT2300A_DelayUs(100);

        if (nWaitStatus == CMT2300A_GetChipStatus())
            return true;

        if (CMT2300A_GO_TX == nGoCmd) {
            CMT2300A_DelayUs(100);

            if (CMT2300A_MASK_TX_DONE_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_CLR1))
                return true;
        }

        if (CMT2300A_GO_RX == nGoCmd) {
            CMT2300A_DelayUs(100);

            if (CMT2300A_MASK_PKT_OK_FLG & CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG))
                return true;
        }
    }

    return false;

#else
    CMT2300A_WriteReg(CMT2300A_CUS_MODE_CTL, nGoCmd);
    return true;
#endif
}

/*! ********************************************************
 * @name    CMT2300A_GoSleep
 * @desc    Entry SLEEP mode.
 * @return  TRUE or FALSE
 * *********************************************************/
bool CMT2300A_GoSleep(void)
{
    return CMT2300A_AutoSwitchStatus(CMT2300A_GO_SLEEP);
}

/*! ********************************************************
 * @name    CMT2300A_GoStby
 * @desc    Entry Sleep mode.
 * @return  TRUE or FALSE
 * *********************************************************/
bool CMT2300A_GoStby(void)
{
    return CMT2300A_AutoSwitchStatus(CMT2300A_GO_STBY);
}

/*! ********************************************************
 * @name    CMT2300A_GoTFS
 * @desc    Entry TFS mode.
 * @return  TRUE or FALSE
 * *********************************************************/
bool CMT2300A_GoTFS(void)
{
    return CMT2300A_AutoSwitchStatus(CMT2300A_GO_TFS);
}

/*! ********************************************************
 * @name    CMT2300A_GoRFS
 * @desc    Entry RFS mode.
 * @return  TRUE or FALSE
 * *********************************************************/
bool CMT2300A_GoRFS(void)
{
    return CMT2300A_AutoSwitchStatus(CMT2300A_GO_RFS);
}

/*! ********************************************************
 * @name    CMT2300A_GoTx
 * @desc    Entry Tx mode.
 * @return  TRUE or FALSE
 * *********************************************************/
bool CMT2300A_GoTx(void)
{
    return CMT2300A_AutoSwitchStatus(CMT2300A_GO_TX);
}

/*! ********************************************************
 * @name    CMT2300A_GoRx
 * @desc    Entry Rx mode.
 * @return  TRUE or FALSE
 * *********************************************************/
bool CMT2300A_GoRx(void)
{
    return CMT2300A_AutoSwitchStatus(CMT2300A_GO_RX);
}

/*! ********************************************************
 * @name    CMT2300A_ConfigGpio
 * @desc    Config GPIO pins mode.
 * @param   nGpioSel: GPIO1_SEL | GPIO2_SEL | GPIO3_SEL | GPIO4_SEL
 *          GPIO1_SEL:
 *            CMT2300A_GPIO1_SEL_DOUT/DIN
 *            CMT2300A_GPIO1_SEL_INT1
 *            CMT2300A_GPIO1_SEL_INT2
 *            CMT2300A_GPIO1_SEL_DCLK
 *
 *          GPIO2_SEL:
 *            CMT2300A_GPIO2_SEL_INT1
 *            CMT2300A_GPIO2_SEL_INT2
 *            CMT2300A_GPIO2_SEL_DOUT/DIN
 *            CMT2300A_GPIO2_SEL_DCLK
 *
 *          GPIO3_SEL:
 *            CMT2300A_GPIO3_SEL_CLKO
 *            CMT2300A_GPIO3_SEL_DOUT/DIN
 *            CMT2300A_GPIO3_SEL_INT2
 *            CMT2300A_GPIO3_SEL_DCLK
 *
 *          GPIO4_SEL:
 *            CMT2300A_GPIO4_SEL_RSTIN
 *            CMT2300A_GPIO4_SEL_INT1
 *            CMT2300A_GPIO4_SEL_DOUT
 *            CMT2300A_GPIO4_SEL_DCLK
 * *********************************************************/
void CMT2300A_ConfigGpio(uint8_t nGpioSel)
{
    CMT2300A_WriteReg(CMT2300A_CUS_IO_SEL, nGpioSel);
}

/*! ********************************************************
 * @name    CMT2300A_ConfigInterrupt
 * @desc    Config interrupt on INT1 and INT2.
 * @param   nInt1Sel, nInt2Sel
 *            CMT2300A_INT_SEL_RX_ACTIVE
 *            CMT2300A_INT_SEL_TX_ACTIVE
 *            CMT2300A_INT_SEL_RSSI_VLD
 *            CMT2300A_INT_SEL_PREAM_OK
 *            CMT2300A_INT_SEL_SYNC_OK
 *            CMT2300A_INT_SEL_NODE_OK
 *            CMT2300A_INT_SEL_CRC_OK
 *            CMT2300A_INT_SEL_PKT_OK
 *            CMT2300A_INT_SEL_SL_TMO
 *            CMT2300A_INT_SEL_RX_TMO
 *            CMT2300A_INT_SEL_TX_DONE
 *            CMT2300A_INT_SEL_RX_FIFO_NMTY
 *            CMT2300A_INT_SEL_RX_FIFO_TH
 *            CMT2300A_INT_SEL_RX_FIFO_FULL
 *            CMT2300A_INT_SEL_RX_FIFO_WBYTE
 *            CMT2300A_INT_SEL_RX_FIFO_OVF
 *            CMT2300A_INT_SEL_TX_FIFO_NMTY
 *            CMT2300A_INT_SEL_TX_FIFO_TH
 *            CMT2300A_INT_SEL_TX_FIFO_FULL
 *            CMT2300A_INT_SEL_STATE_IS_STBY
 *            CMT2300A_INT_SEL_STATE_IS_FS
 *            CMT2300A_INT_SEL_STATE_IS_RX
 *            CMT2300A_INT_SEL_STATE_IS_TX
 *            CMT2300A_INT_SEL_LED
 *            CMT2300A_INT_SEL_TRX_ACTIVE
 *            CMT2300A_INT_SEL_PKT_DONE
 * *********************************************************/
void CMT2300A_ConfigInterrupt(uint8_t nInt1Sel, uint8_t nInt2Sel)
{
    nInt1Sel &= CMT2300A_MASK_INT1_SEL;
    nInt1Sel |= (~CMT2300A_MASK_INT1_SEL) & CMT2300A_ReadReg(CMT2300A_CUS_INT1_CTL);
    CMT2300A_WriteReg(CMT2300A_CUS_INT1_CTL, nInt1Sel);

    nInt2Sel &= CMT2300A_MASK_INT2_SEL;
    nInt2Sel |= (~CMT2300A_MASK_INT2_SEL) & CMT2300A_ReadReg(CMT2300A_CUS_INT2_CTL);
    CMT2300A_WriteReg(CMT2300A_CUS_INT2_CTL, nInt2Sel);
}

/*! ********************************************************
 * @name    CMT2300A_SetInterruptPolar
 * @desc    Set the polarity of the interrupt.
 * @param   bEnable(TRUE): active-high (default)
 *          bEnable(FALSE): active-low
 * *********************************************************/
void CMT2300A_SetInterruptPolar(bool bActiveHigh)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_INT1_CTL);

    if (bActiveHigh)
        tmp &= ~CMT2300A_MASK_INT_POLAR;
    else
        tmp |= CMT2300A_MASK_INT_POLAR;

    CMT2300A_WriteReg(CMT2300A_CUS_INT1_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_SetFifoThreshold
 * @desc    Set FIFO threshold.
 * @param   nFifoThreshold
 * *********************************************************/
void CMT2300A_SetFifoThreshold(uint8_t nFifoThreshold)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_PKT29);

    tmp &= ~CMT2300A_MASK_FIFO_TH;
    tmp |= nFifoThreshold & CMT2300A_MASK_FIFO_TH;

    CMT2300A_WriteReg(CMT2300A_CUS_PKT29, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableAntennaSwitch
 * @desc    Enable antenna switch, output TX_ACTIVE/RX_ACTIVE
 *          via GPIO1/GPIO2.
 * @param   nMode
 *            0: RF_SWT1_EN=1, RF_SWT2_EN=0
 *               GPIO1: RX_ACTIVE, GPIO2: TX_ACTIVE
 *            1: RF_SWT1_EN=0, RF_SWT2_EN=1
 *               GPIO1: RX_ACTIVE, GPIO2: ~RX_ACTIVE
 * *********************************************************/
void CMT2300A_EnableAntennaSwitch(uint8_t nMode)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_INT1_CTL);

    if (0 == nMode) {
        tmp |= CMT2300A_MASK_RF_SWT1_EN;
        tmp &= ~CMT2300A_MASK_RF_SWT2_EN;
    } else if (1 == nMode) {
        tmp &= ~CMT2300A_MASK_RF_SWT1_EN;
        tmp |= CMT2300A_MASK_RF_SWT2_EN;
    }

    CMT2300A_WriteReg(CMT2300A_CUS_INT1_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableInterrupt
 * @desc    Enable interrupt.
 * @param   nEnable
 *            CMT2300A_MASK_SL_TMO_EN   |
 *            CMT2300A_MASK_RX_TMO_EN   |
 *            CMT2300A_MASK_TX_DONE_EN  |
 *            CMT2300A_MASK_PREAM_OK_EN |
 *            CMT2300A_MASK_SYNC_OK_EN  |
 *            CMT2300A_MASK_NODE_OK_EN  |
 *            CMT2300A_MASK_CRC_OK_EN   |
 *            CMT2300A_MASK_PKT_DONE_EN
 * *********************************************************/
void CMT2300A_EnableInterrupt(uint8_t nEnable)
{
    CMT2300A_WriteReg(CMT2300A_CUS_INT_EN, nEnable);
}

/*! ********************************************************
 * @name    CMT2300A_EnableRxFifoAutoClear
 * @desc    Auto clear Rx FIFO before entry Rx mode.
 * @param   bEnable(TRUE): Enable it(default)
 *          bEnable(FALSE): Disable it
 * *********************************************************/
void CMT2300A_EnableRxFifoAutoClear(bool bEnable)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);

    if (bEnable)
        tmp &= ~CMT2300A_MASK_FIFO_AUTO_CLR_DIS;
    else
        tmp |= CMT2300A_MASK_FIFO_AUTO_CLR_DIS;

    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableFifoMerge
 * @desc    Enable FIFO merge.
 * @param   bEnable(TRUE): use a single 64-byte FIFO for either Tx or Rx
 *          bEnable(FALSE): use a 32-byte FIFO for Tx and another 32-byte FIFO for Rx(default)
 * *********************************************************/
void CMT2300A_EnableFifoMerge(bool bEnable)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);

    if (bEnable)
        tmp |= CMT2300A_MASK_FIFO_MERGE_EN;
    else
        tmp &= ~CMT2300A_MASK_FIFO_MERGE_EN;

    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableReadFifo
 * @desc    Enable SPI to read the FIFO.
 * *********************************************************/
void CMT2300A_EnableReadFifo(void)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);
    tmp &= ~CMT2300A_MASK_SPI_FIFO_RD_WR_SEL;
    tmp &= ~CMT2300A_MASK_FIFO_RX_TX_SEL;
    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableWriteFifo
 * @desc    Enable SPI to write the FIFO.
 * *********************************************************/
void CMT2300A_EnableWriteFifo(void)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);
    tmp |= CMT2300A_MASK_SPI_FIFO_RD_WR_SEL;
    tmp |= CMT2300A_MASK_FIFO_RX_TX_SEL;
    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_RestoreFifo
 * @desc    Restore the FIFO.
 * *********************************************************/
void CMT2300A_RestoreFifo(void)
{
    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CLR, CMT2300A_MASK_FIFO_RESTORE);
}

/*! ********************************************************
 * @name    CMT2300A_ClearFifo
 * @desc    Clear the Tx FIFO.
 * @return  FIFO flags
 *            CMT2300A_MASK_RX_FIFO_FULL_FLG |
 *            CMT2300A_MASK_RX_FIFO_NMTY_FLG |
 *            CMT2300A_MASK_RX_FIFO_TH_FLG   |
 *            CMT2300A_MASK_RX_FIFO_OVF_FLG  |
 *            CMT2300A_MASK_TX_FIFO_FULL_FLG |
 *            CMT2300A_MASK_TX_FIFO_NMTY_FLG |
 *            CMT2300A_MASK_TX_FIFO_TH_FLG
 * *********************************************************/
uint8_t CMT2300A_ClearTxFifo(void)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_FLAG);
    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CLR, CMT2300A_MASK_FIFO_CLR_TX);
    return tmp;
}

/*! ********************************************************
 * @name    CMT2300A_ClearFifo
 * @desc    Clear the Rx FIFO.
 * @return  FIFO flags
 *            CMT2300A_MASK_RX_FIFO_FULL_FLG |
 *            CMT2300A_MASK_RX_FIFO_NMTY_FLG |
 *            CMT2300A_MASK_RX_FIFO_TH_FLG   |
 *            CMT2300A_MASK_RX_FIFO_OVF_FLG  |
 *            CMT2300A_MASK_TX_FIFO_FULL_FLG |
 *            CMT2300A_MASK_TX_FIFO_NMTY_FLG |
 *            CMT2300A_MASK_TX_FIFO_TH_FLG
 * *********************************************************/
uint8_t CMT2300A_ClearRxFifo(void)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_FLAG);
    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CLR, CMT2300A_MASK_FIFO_CLR_RX);
    return tmp;
}

/*! ********************************************************
 * @name    CMT2300A_ClearInterruptFlags
 * @desc    Clear all interrupt flags.
 * @return  Some interrupt flags
 *            CMT2300A_MASK_SL_TMO_EN    |
 *            CMT2300A_MASK_RX_TMO_EN    |
 *            CMT2300A_MASK_TX_DONE_EN   |
 *            CMT2300A_MASK_PREAM_OK_FLG |
 *            CMT2300A_MASK_SYNC_OK_FLG  |
 *            CMT2300A_MASK_NODE_OK_FLG  |
 *            CMT2300A_MASK_CRC_OK_FLG   |
 *            CMT2300A_MASK_PKT_OK_FLG
 * *********************************************************/
uint8_t CMT2300A_ClearInterruptFlags(void)
{
    uint8_t nFlag1, nFlag2;
    uint8_t nClr1 = 0;
    uint8_t nClr2 = 0;
    uint8_t nRet = 0;
    uint8_t nIntPolar;

    nIntPolar = CMT2300A_ReadReg(CMT2300A_CUS_INT1_CTL);
    nIntPolar = (nIntPolar & CMT2300A_MASK_INT_POLAR) ? 1 : 0;

    nFlag1 = CMT2300A_ReadReg(CMT2300A_CUS_INT_FLAG);
    nFlag2 = CMT2300A_ReadReg(CMT2300A_CUS_INT_CLR1);

    if (nIntPolar) {
        /* Interrupt flag active-low */
        nFlag1 = ~nFlag1;
        nFlag2 = ~nFlag2;
    }

    if (CMT2300A_MASK_LBD_FLG & nFlag1) {
        nClr2 |= CMT2300A_MASK_LBD_CLR; /* Clear LBD_FLG */
    }

    if (CMT2300A_MASK_COL_ERR_FLG & nFlag1) {
        nClr2 |= CMT2300A_MASK_PKT_DONE_CLR; /* Clear COL_ERR_FLG by PKT_DONE_CLR */
    }

    if (CMT2300A_MASK_PKT_ERR_FLG & nFlag1) {
        nClr2 |= CMT2300A_MASK_PKT_DONE_CLR; /* Clear PKT_ERR_FLG by PKT_DONE_CLR */
    }

    if (CMT2300A_MASK_PREAM_OK_FLG & nFlag1) {
        nClr2 |= CMT2300A_MASK_PREAM_OK_CLR; /* Clear PREAM_OK_FLG */
        nRet |= CMT2300A_MASK_PREAM_OK_FLG; /* Return PREAM_OK_FLG */
    }

    if (CMT2300A_MASK_SYNC_OK_FLG & nFlag1) {
        nClr2 |= CMT2300A_MASK_SYNC_OK_CLR; /* Clear SYNC_OK_FLG */
        nRet |= CMT2300A_MASK_SYNC_OK_FLG; /* Return SYNC_OK_FLG */
    }

    if (CMT2300A_MASK_NODE_OK_FLG & nFlag1) {
        nClr2 |= CMT2300A_MASK_NODE_OK_CLR; /* Clear NODE_OK_FLG */
        nRet |= CMT2300A_MASK_NODE_OK_FLG; /* Return NODE_OK_FLG */
    }

    if (CMT2300A_MASK_CRC_OK_FLG & nFlag1) {
        nClr2 |= CMT2300A_MASK_CRC_OK_CLR; /* Clear CRC_OK_FLG */
        nRet |= CMT2300A_MASK_CRC_OK_FLG; /* Return CRC_OK_FLG */
    }

    if (CMT2300A_MASK_PKT_OK_FLG & nFlag1) {
        nClr2 |= CMT2300A_MASK_PKT_DONE_CLR; /* Clear PKT_OK_FLG */
        nRet |= CMT2300A_MASK_PKT_OK_FLG; /* Return PKT_OK_FLG */
    }

    if (CMT2300A_MASK_SL_TMO_FLG & nFlag2) {
        nClr1 |= CMT2300A_MASK_SL_TMO_CLR; /* Clear SL_TMO_FLG */
        nRet |= CMT2300A_MASK_SL_TMO_EN; /* Return SL_TMO_FLG by SL_TMO_EN */
    }

    if (CMT2300A_MASK_RX_TMO_FLG & nFlag2) {
        nClr1 |= CMT2300A_MASK_RX_TMO_CLR; /* Clear RX_TMO_FLG */
        nRet |= CMT2300A_MASK_RX_TMO_EN; /* Return RX_TMO_FLG by RX_TMO_EN */
    }

    if (CMT2300A_MASK_TX_DONE_FLG & nFlag2) {
        nClr1 |= CMT2300A_MASK_TX_DONE_CLR; /* Clear TX_DONE_FLG */
        nRet |= CMT2300A_MASK_TX_DONE_EN; /* Return TX_DONE_FLG by TX_DONE_EN */
    }

    CMT2300A_WriteReg(CMT2300A_CUS_INT_CLR1, nClr1);
    CMT2300A_WriteReg(CMT2300A_CUS_INT_CLR2, nClr2);

    if (nIntPolar) {
        /* Interrupt flag active-low */
        nRet = ~nRet;
    }

    return nRet;
}

/*! ********************************************************
 * @name    CMT2300A_ConfigTxDin
 * @desc    Used to select whether to use GPIO1 or GPIO2 or GPIO3
 *          as DIN in the direct mode. It only takes effect when
 *          call CMT2300A_EnableTxDin(TRUE) in the direct mode.
 * @param   nDinSel
 *            CMT2300A_TX_DIN_SEL_GPIO1
 *            CMT2300A_TX_DIN_SEL_GPIO2
 *            CMT2300A_TX_DIN_SEL_GPIO3
 * *********************************************************/
void CMT2300A_ConfigTxDin(uint8_t nDinSel)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);
    tmp &= ~CMT2300A_MASK_TX_DIN_SEL;
    tmp |= nDinSel;
    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableTxDin
 * @desc    Used to change GPIO1/GPIO2/GPIO3 between DOUT and DIN.
 * @param   bEnable(TRUE): used as DIN
 *          bEnable(FALSE): used as DOUT(default)
 * *********************************************************/
void CMT2300A_EnableTxDin(bool bEnable)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FIFO_CTL);

    if (bEnable)
        tmp |= CMT2300A_MASK_TX_DIN_EN;
    else
        tmp &= ~CMT2300A_MASK_TX_DIN_EN;

    CMT2300A_WriteReg(CMT2300A_CUS_FIFO_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableTxDinInvert
 * @desc    Used to invert DIN data in direct mode.
 * @param   bEnable(TRUE): invert DIN
 *          bEnable(FALSE): not invert DIN(default)
 * *********************************************************/
void CMT2300A_EnableTxDinInvert(bool bEnable)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_INT2_CTL);

    if (bEnable)
        tmp |= CMT2300A_MASK_TX_DIN_INV;
    else
        tmp &= ~CMT2300A_MASK_TX_DIN_INV;

    CMT2300A_WriteReg(CMT2300A_CUS_INT2_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_IsExist
 * @desc    Chip indentify.
 * @return  TRUE: chip is exist, FALSE: chip not found
 * *********************************************************/
bool CMT2300A_IsExist(void)
{
    uint8_t back, dat;

    back = CMT2300A_ReadReg(CMT2300A_CUS_PKT17);
    CMT2300A_WriteReg(CMT2300A_CUS_PKT17, 0xAA);

    dat = CMT2300A_ReadReg(CMT2300A_CUS_PKT17);
    CMT2300A_WriteReg(CMT2300A_CUS_PKT17, back);

    if (0xAA == dat)
        return true;
    else
        return false;
}

/*! ********************************************************
 * @name    CMT2300A_GetRssiCode
 * @desc    Get RSSI code.
 * @return  RSSI code
 * *********************************************************/
uint8_t CMT2300A_GetRssiCode(void)
{
    return CMT2300A_ReadReg(CMT2300A_CUS_RSSI_CODE);
}

/*! ********************************************************
 * @name    CMT2300A_GetRssiDBm
 * @desc    Get RSSI dBm.
 * @return  dBm
 * *********************************************************/
int CMT2300A_GetRssiDBm(void)
{
    return (int)CMT2300A_ReadReg(CMT2300A_CUS_RSSI_DBM) - 128;
}

/*! ********************************************************
 * @name    CMT2300A_SetFrequencyChannel
 * @desc    This defines up to 255 frequency channel
 *          for fast frequency hopping operation.
 * @param   nChann: the frequency channel
 * *********************************************************/
void CMT2300A_SetFrequencyChannel(uint8_t nChann)
{
    CMT2300A_WriteReg(CMT2300A_CUS_FREQ_CHNL, nChann);
}

/*! ********************************************************
 * @name    CMT2300A_SetFrequencyStep
 * @desc    This defines the frequency channel step size
 *          for fast frequency hopping operation.
 *          One step size is 2.5 kHz.
 * @param   nOffset: the frequency step
 * *********************************************************/
void CMT2300A_SetFrequencyStep(uint8_t nOffset)
{
    CMT2300A_WriteReg(CMT2300A_CUS_FREQ_OFS, nOffset);
}

/*! ********************************************************
 * @name    CMT2300A_SetPayloadLength
 * @desc    Set payload length.
 * @param   nLength
 * *********************************************************/
void CMT2300A_SetPayloadLength(uint16_t nLength)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_PKT14);

    tmp &= ~CMT2300A_MASK_PAYLOAD_LENG_10_8;
    tmp |= (nLength >> 4) & CMT2300A_MASK_PAYLOAD_LENG_10_8;
    CMT2300A_WriteReg(CMT2300A_CUS_PKT14, tmp);

    tmp = nLength & CMT2300A_MASK_PAYLOAD_LENG_7_0;
    CMT2300A_WriteReg(CMT2300A_CUS_PKT15, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableLfosc
 * @desc    If you need use sleep timer, you should enable LFOSC.
 * @param   bEnable(TRUE): Enable it(default)
 *          bEnable(FALSE): Disable it
 * *********************************************************/
void CMT2300A_EnableLfosc(bool bEnable)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_SYS2);

    if (bEnable) {
        tmp |= CMT2300A_MASK_LFOSC_RECAL_EN;
        tmp |= CMT2300A_MASK_LFOSC_CAL1_EN;
        tmp |= CMT2300A_MASK_LFOSC_CAL2_EN;
    } else {
        tmp &= ~CMT2300A_MASK_LFOSC_RECAL_EN;
        tmp &= ~CMT2300A_MASK_LFOSC_CAL1_EN;
        tmp &= ~CMT2300A_MASK_LFOSC_CAL2_EN;
    }

    CMT2300A_WriteReg(CMT2300A_CUS_SYS2, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableLfoscOutput
 * @desc    LFOSC clock is output via GPIO3.
 * @param   bEnable(TRUE): Enable it
 *          bEnable(FALSE): Disable it(default)
 * *********************************************************/
void CMT2300A_EnableLfoscOutput(bool bEnable)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_INT2_CTL);

    if (bEnable)
        tmp |= CMT2300A_MASK_LFOSC_OUT_EN;
    else
        tmp &= ~CMT2300A_MASK_LFOSC_OUT_EN;

    CMT2300A_WriteReg(CMT2300A_CUS_INT2_CTL, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_EnableAfc
 * @desc    AFC enable or disanble.
 * @param   bEnable(TRUE): Enable it
 *          bEnable(FALSE): Disable it(default)
 * *********************************************************/
void CMT2300A_EnableAfc(bool bEnable)
{
    uint8_t tmp = CMT2300A_ReadReg(CMT2300A_CUS_FSK5);

    if (bEnable)
        tmp |= 0x10;
    else
        tmp &= ~0x10;

    CMT2300A_WriteReg(CMT2300A_CUS_FSK5, tmp);
}

/*! ********************************************************
 * @name    CMT2300A_SetAfcOvfTh
 * @desc    This is optional, only needed when using Rx fast frequency hopping.
 * @param   afcOvfTh: AFC_OVF_TH see AN142 and AN197 for details.
 * *********************************************************/
void CMT2300A_SetAfcOvfTh(uint8_t afcOvfTh)
{
    CMT2300A_WriteReg(CMT2300A_CUS_FSK4, afcOvfTh);
}

/*! ********************************************************
 * @name    CMT2300A_Init
 * @desc    Initialize chip status.
 * *********************************************************/
bool CMT2300A_Init(void)
{
    uint8_t tmp;

    CMT2300A_SoftReset();
    CMT2300A_DelayMs(20);

    if (!CMT2300A_GoStby())
        return false; // CMT2300A not switched to standby mode!

    if (!CMT2300A_IsExist())
        return false; // CMT2300A not found!

    tmp = CMT2300A_ReadReg(CMT2300A_CUS_MODE_STA);
    tmp |= CMT2300A_MASK_CFG_RETAIN; /* Enable CFG_RETAIN */
    tmp &= ~CMT2300A_MASK_RSTN_IN_EN; /* Disable RSTN_IN */
    CMT2300A_WriteReg(CMT2300A_CUS_MODE_STA, tmp);

    tmp = CMT2300A_ReadReg(CMT2300A_CUS_EN_CTL);
    tmp |= CMT2300A_MASK_LOCKING_EN; /* Enable LOCKING_EN */
    CMT2300A_WriteReg(CMT2300A_CUS_EN_CTL, tmp);

    CMT2300A_EnableLfosc(false); /* Disable LFOSC */

    CMT2300A_ClearInterruptFlags();

    return true;
}

/*! ********************************************************
 * @name    CMT2300A_ConfigRegBank
 * @desc    Config one register bank.
 * *********************************************************/
bool CMT2300A_ConfigRegBank(uint8_t base_addr, const uint8_t bank[], uint8_t len)
{
    uint8_t i;
    for (i = 0; i < len; i++)
        CMT2300A_WriteReg(i + base_addr, bank[i]);

    return true;
}
