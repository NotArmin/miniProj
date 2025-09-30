#include "led.h"
#include "device_map.h"

#define LED_REG (*(volatile uint32_t *)LED_BASE_ADDR)

void led_set(uint32_t value) {
    LED_REG = value;
}

uint32_t led_get(void) {
    return LED_REG;
}
