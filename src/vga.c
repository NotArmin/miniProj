#include "vga.h"
#include "device_map.h"

#define VGA_DMA_CONTROL (*(volatile uint32_t *)VGA_DMA_STATUS_ADDR)
#define VGA_DMA_BUFFER (*(volatile uint32_t *)VGA_DMA_BUFFER_ADDR)
#define VGA_DMA_BACKBUFFER (*(volatile uint32_t *)VGA_DMA_BACKBUFFER_ADDR)
#define VGA_DMA_RESOLUTION (*(volatile uint32_t *)VGA_DMA_RESOLUTION_ADDR)

#define VGA_BUFFER ((volatile uint32_t *)VGA_SCREEN_BUFFER_ADDR)

void vga_init(void) {
    VGA_DMA_CONTROL = 0x1; // Enable DMA
}

void vga_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    uint32_t resolution = VGA_DMA_RESOLUTION;
    uint32_t width = resolution & 0xFFFF;
    VGA_BUFFER[y * width + x] = color;
}

void vga_swap_buffers(void) {
    VGA_DMA_CONTROL |= 0x1; // Trigger swap
}
