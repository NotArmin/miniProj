#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include "device_map.h"

// DMA registers
#define VGA_DMA_BUFFER      (*(volatile uint32_t *)VGA_DMA_BUFFER_ADDR)
#define VGA_DMA_BACKBUFFER  (*(volatile uint32_t *)VGA_DMA_BACKBUFFER_ADDR)
#define VGA_DMA_RESOLUTION  (*(volatile uint32_t *)VGA_DMA_RESOLUTION_ADDR)
#define VGA_DMA_STATUS      (*(volatile uint32_t *)VGA_DMA_STATUS_ADDR)

// VGA functions
void vga_init(void);
void vga_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void vga_swap_buffers(void);

#endif // VGA_H
