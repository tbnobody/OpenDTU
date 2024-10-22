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
 * @file    cmt2300a_hal.c
 * @brief   CMT2300A hardware abstraction layer
 *
 * @version 1.2
 * @date    Jul 17 2017
 * @author  CMOSTEK R@D
 */

#include "cmt2300a_hal.h"
#include "cmt_spi3.h"
#include <Arduino.h>

/*! ********************************************************
 * @name    CMT2300A_InitSpi
 * @desc    Initializes the CMT2300A SPI interface.
 * *********************************************************/
void CMT2300A_InitSpi(const int8_t pin_sdio, const int8_t pin_clk, const int8_t pin_cs, const int8_t pin_fcs, const uint32_t spi_speed)
{
    cmt_spi3_init(pin_sdio, pin_clk, pin_cs, pin_fcs, spi_speed);
}

/*! ********************************************************
 * @name    CMT2300A_ReadReg
 * @desc    Read the CMT2300A register at the specified address.
 * @param   addr: register address
 * @return  Register value
 * *********************************************************/
uint8_t CMT2300A_ReadReg(const uint8_t addr)
{
    return cmt_spi3_read(addr);
}

/*! ********************************************************
 * @name    CMT2300A_WriteReg
 * @desc    Write the CMT2300A register at the specified address.
 * @param   addr: register address
 *          dat: register value
 * *********************************************************/
void CMT2300A_WriteReg(const uint8_t addr, const uint8_t dat)
{
    cmt_spi3_write(addr, dat);
}

/*! ********************************************************
 * @name    CMT2300A_ReadFifo
 * @desc    Reads the contents of the CMT2300A FIFO.
 * @param   buf: buffer where to copy the FIFO read data
 *          len: number of bytes to be read from the FIFO
 * *********************************************************/
void CMT2300A_ReadFifo(uint8_t buf[], const uint16_t len)
{
    cmt_spi3_read_fifo(buf, len);
}

/*! ********************************************************
 * @name    CMT2300A_WriteFifo
 * @desc    Writes the buffer contents to the CMT2300A FIFO.
 * @param   buf: buffer containing data to be put on the FIFO
 *          len: number of bytes to be written to the FIFO
 * *********************************************************/
void CMT2300A_WriteFifo(const uint8_t buf[], const uint16_t len)
{
    cmt_spi3_write_fifo(buf, len);
}
