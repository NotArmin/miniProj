#include "image_processing.h"
#include "device_map.h"
#include "vga.h"
#include "sdram.h"
#include <stdint.h>

static uint32_t img_width;
static uint32_t img_height;
static uint32_t *img_buffer;

void image_load(const uint32_t *data, uint32_t width, uint32_t height) {
    img_width = width;
    img_height = height;
    img_buffer = (uint32_t *)SDRAM_BASE_ADDR; // Store at SDRAM base for simplicity

    for (uint32_t i = 0; i < width * height; i++) {
        img_buffer[i] = data[i];
    }
}

void image_grayscale(void) {
    for (uint32_t i = 0; i < img_width * img_height; i++) {
        uint32_t pixel = img_buffer[i];

        uint8_t r = (pixel >> 16) & 0xFF;
        uint8_t g = (pixel >> 8) & 0xFF;
        uint8_t b = pixel & 0xFF;

        uint8_t gray = (r + g + b) / 3;
        img_buffer[i] = (gray << 16) | (gray << 8) | gray;
    }
}

void image_draw(void) {
    VGA_DMA_BUFFER = (uint32_t)img_buffer;
    VGA_DMA_RESOLUTION = (img_height << 16) | img_width; // Y | X

    // Enable DMA (bit 2 = EN in your spec)
    VGA_DMA_STATUS |= (1 << 2);
}


void image_swap_buffers(void) {
    vga_swap_buffers();
}
