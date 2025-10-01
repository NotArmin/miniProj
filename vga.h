#ifndef VGA_H
#define VGA_H

#define VGA_BASE   0x08000000
#define VGA_CTRL   0x04000100
#define RES_X      320
#define RES_Y      240

extern volatile unsigned char * const BUF0;
extern volatile unsigned char * const BUF1;
extern volatile unsigned int  * const VGA_CTRL_PTR;

void vga_init(void);
void vga_swap_buffers(void);
void vga_show_background(void);
void draw_background(volatile unsigned char *vram);

#endif // VGA_H
