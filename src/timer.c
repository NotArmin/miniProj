#include "timer.h"
#include "device_map.h"

#define TIMER_STATUS   (*(volatile uint32_t *)TIMER_STATUS_ADDR)
#define TIMER_CONTROL  (*(volatile uint32_t *)TIMER_CONTROL_ADDR)
#define TIMER_PERIODL  (*(volatile uint32_t *)TIMER_PERIODL_ADDR)
#define TIMER_PERIODH  (*(volatile uint32_t *)TIMER_PERIODH_ADDR)

void timer_set_period(uint32_t period) {
    TIMER_PERIODL = period & 0xFFFF;
    TIMER_PERIODH = (period >> 16) & 0xFFFF;
}

void timer_start(void) {
    TIMER_CONTROL |= 0x2; // Set START bit
}

void timer_stop(void) {
    TIMER_CONTROL |= 0x4; // Set STOP bit
}

uint32_t timer_get_status(void) {
    return TIMER_STATUS;
}
