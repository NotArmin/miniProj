// vga.c

#include "vga.h"
#include "background.h"

volatile unsigned char * const BUF0 = (volatile unsigned char *) VGA_BASE;
volatile unsigned char * const BUF1 = (volatile unsigned char *) (VGA_BASE + RES_X * RES_Y);
volatile unsigned int  * const VGA_CTRL_PTR = (volatile unsigned int *) VGA_CTRL;

void vga_init(void) {
    *VGA_CTRL_PTR = 0x1; // Enable DMA
}

void vga_swap_buffers(void) {
    *VGA_CTRL_PTR = 0x1; // Trigger swap
}

void draw_background(volatile unsigned char *vram) {
    for (int y = 0; y < RES_Y; y++) {
        for (int x = 0; x < RES_X; x++) {
            vram[y * RES_X + x] = test1[y][x];
        }
    }
}

void vga_show_background(void) {
    draw_background(BUF0);
    draw_background(BUF1);
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;

    // little wait so the hardware settles
    for (volatile int i = 0; i < 100000; i++) {
        asm volatile ("nop");
    }
}