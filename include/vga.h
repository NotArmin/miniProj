#ifndef VGA_H
#define VGA_H

#include <stdint.h>

#define VGA_WIDTH 320
#define VGA_HEIGHT 240

void vga_init();
void vga_set_pixel(int x, int y, uint32_t color);
uint32_t vga_get_pixel(int x, int y);

#endif // VGA_H