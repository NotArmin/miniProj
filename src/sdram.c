#include "sdram.h"
#include "device_map.h"

#define SDRAM ((volatile uint32_t *)SDRAM_BASE_ADDR)

void sdram_init(void) {
    // SDRAM initialization logic here, if needed
}

uint32_t sdram_read(uint32_t addr) {
    return SDRAM[addr / 4];
}

void sdram_write(uint32_t addr, uint32_t value) {
    SDRAM[addr / 4] = value;
}
