#ifndef __CMT_SPI3_H
#define __CMT_SPI3_H

#include <stdint.h>

void cmt_spi3_init(void);

void cmt_spi3_write(uint8_t addr, uint8_t dat);
uint8_t cmt_spi3_read(uint8_t addr);

void cmt_spi3_write_fifo(const uint8_t* p_buf, uint16_t len);
void cmt_spi3_read_fifo(uint8_t* p_buf, uint16_t len);

#endif
