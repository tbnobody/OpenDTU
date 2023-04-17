#ifndef __CMT_SPI3_H
#define __CMT_SPI3_H

#include <stdint.h>

void cmt_spi3_init(int8_t pin_sdio, int8_t pin_clk, int8_t pin_cs, int8_t pin_fcs, uint32_t spi_speed);

void cmt_spi3_write(uint8_t addr, uint8_t dat);
uint8_t cmt_spi3_read(uint8_t addr);

void cmt_spi3_write_fifo(const uint8_t* p_buf, uint16_t len);
void cmt_spi3_read_fifo(uint8_t* p_buf, uint16_t len);

#endif
