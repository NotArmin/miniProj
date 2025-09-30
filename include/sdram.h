#ifndef SDRAM_H
#define SDRAM_H

#include <stdint.h>

void sdram_init(void);
uint32_t sdram_read(uint32_t addr);
void sdram_write(uint32_t addr, uint32_t value);

#endif // SDRAM_H
