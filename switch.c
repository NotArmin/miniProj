#include "switch.h"
#include "device_map.h"

#define SWITCH_REG (*(volatile uint32_t *)SWITCH_BASE_ADDR)

uint32_t switch_get(void) {
    return SWITCH_REG;
}
