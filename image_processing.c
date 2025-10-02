#include "image_processing.h"
#include "vga.h"
#include <stdint.h>


static volatile uint8_t *img_buffer;

void image_load(const uint32_t *data, const int RES_X, const int RES_Y) {

    for (int i = 0; i < RES_X * RES_Y; i++) {
        uint32_t pixel = data[i];

        uint8_t r = (pixel >> 16) & 0xFF >> 5; // Scale to 3-bit
        uint8_t g = (pixel >> 8) & 0xFF >> 6; // Scale to 2-bit
        uint8_t b = pixel & 0xFF >> 6; // Scale to 2-bit

        img_buffer[i] = (r << 5) | (g << 2) | (b << 0); // RGB332 format
    }
}

void image_grayscale(void) {
    for (int i = 0; i < RES_X * RES_Y; i++) {
        uint32_t pixel = img_buffer[i];

        uint8_t r = (pixel >> 5) & 0x07;
        uint8_t g = (pixel >> 2) & 0x07;
        uint8_t b = pixel & 0x03;

        uint8_t gray = (r*30 + g*59 + b*11) / 100; // Weighted average
        img_buffer[i] = (gray << 5) | (gray << 2) | (gray >> 1);
    }
}

void image_invert(void) {
    for (int i = 0; i < RES_X * RES_Y; i++) {
        uint32_t pixel = img_buffer[i];

        uint8_t r = (pixel >> 5) & 0x07;
        uint8_t g = (pixel >> 2) & 0x07;
        uint8_t b = pixel & 0x03;

        r = 7 - r; // Invert Red
        g = 7 - g; // Invert Green
        b = 3 - b; // Invert Blue

        img_buffer[i] = (r << 5) | (g << 2) | (b << 0);
    }
}

void image_mirror(void) {
    for (uint32_t y = 0; y < RES_Y; y++) {
        for (uint32_t x = 0; x < RES_X / 2; x++) {
            uint32_t left_index = y * RES_X + x;
            uint32_t right_index = y * RES_X + (RES_X - 1 - x);

            uint32_t temp = img_buffer[left_index];
            img_buffer[left_index] = img_buffer[right_index];
            img_buffer[right_index] = temp;
        }
    }
}
