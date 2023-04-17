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
 * @file    cmt2300a.h
 * @brief   CMT2300A transceiver RF chip driver
 *
 * @version 1.3
 * @date    Jul 17 2017
 * @author  CMOSTEK R@D
 */

#ifndef __CMT2300A_H
#define __CMT2300A_H

#include "cmt2300a_defs.h"
#include "cmt2300a_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_AUTO_SWITCH_CHIP_STATUS /* Enable the auto switch chip status */

/* ************************************************************************
   The following are for chip status controls.
*  ************************************************************************ */
void CMT2300A_SoftReset(void);
uint8_t CMT2300A_GetChipStatus(void);
bool CMT2300A_AutoSwitchStatus(uint8_t nGoCmd);
bool CMT2300A_GoSleep(void);
bool CMT2300A_GoStby(void);
bool CMT2300A_GoTFS(void);
bool CMT2300A_GoRFS(void);
bool CMT2300A_GoTx(void);
bool CMT2300A_GoRx(void);

/* ************************************************************************
 *  The following are for chip interrupts, GPIO, FIFO operations.
 *  ************************************************************************ */
void CMT2300A_ConfigGpio(uint8_t nGpioSel);
void CMT2300A_ConfigInterrupt(uint8_t nInt1Sel, uint8_t nInt2Sel);
void CMT2300A_SetInterruptPolar(bool bActiveHigh);
void CMT2300A_SetFifoThreshold(uint8_t nFifoThreshold);
void CMT2300A_EnableAntennaSwitch(uint8_t nMode);
void CMT2300A_EnableInterrupt(uint8_t nEnable);
void CMT2300A_EnableRxFifoAutoClear(bool bEnable);
void CMT2300A_EnableFifoMerge(bool bEnable);
void CMT2300A_EnableReadFifo(void);
void CMT2300A_EnableWriteFifo(void);
void CMT2300A_RestoreFifo(void);
uint8_t CMT2300A_ClearTxFifo(void);
uint8_t CMT2300A_ClearRxFifo(void);
uint8_t CMT2300A_ClearInterruptFlags(void);

/* ************************************************************************
 *  The following are for Tx DIN operations in direct mode.
 *  ************************************************************************ */
void CMT2300A_ConfigTxDin(uint8_t nDinSel);
void CMT2300A_EnableTxDin(bool bEnable);
void CMT2300A_EnableTxDinInvert(bool bEnable);

/* ************************************************************************
 *  The following are general operations.
 *  ************************************************************************ */
bool CMT2300A_IsExist(void);
uint8_t CMT2300A_GetRssiCode(void);
int CMT2300A_GetRssiDBm(void);
void CMT2300A_SetFrequencyChannel(uint8_t nChann);
void CMT2300A_SetFrequencyStep(uint8_t nOffset);
void CMT2300A_SetPayloadLength(uint16_t nLength);
void CMT2300A_EnableLfosc(bool bEnable);
void CMT2300A_EnableLfoscOutput(bool bEnable);
void CMT2300A_EnableAfc(bool bEnable);
void CMT2300A_SetAfcOvfTh(uint8_t afcOvfTh);

/* ************************************************************************
 *  The following are for chip initializes.
 *  ************************************************************************ */
bool CMT2300A_Init(void);
bool CMT2300A_ConfigRegBank(uint8_t base_addr, const uint8_t bank[], uint8_t len);

#ifdef __cplusplus
}
#endif

#endif
