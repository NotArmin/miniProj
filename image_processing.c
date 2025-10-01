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
        uint32_t pixel = data[i];

        uint8_t r = (pixel >> 16) & 0xFF >> 5; // Scale to 3-bit
        uint8_t g = (pixel >> 8) & 0xFF >> 5; // Scale to 3-bit
        uint8_t b = pixel & 0xFF >> 6; // Scale to 2-bit

        img_buffer[i] = (r << 5) | (g << 2) | (b << 0); // RGB332 format
    }
}

void image_grayscale(void) {
    for (uint32_t i = 0; i < img_width * img_height; i++) {
        uint32_t pixel = img_buffer[i];

        uint8_t r = (pixel >> 5) & 0x07;
        uint8_t g = (pixel >> 2) & 0x07;
        uint8_t b = pixel & 0x03;

        uint8_t gray = (r*30 + g*59 + b*11) / 100; // Weighted average
        img_buffer[i] = (gray << 5) | (gray << 2) | (gray >> 1);
    }
}

void image_invert(void) {
    for (uint32_t i = 0; i < img_width * img_height; i++) {
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
    for (uint32_t y = 0; y < img_height; y++) {
        for (uint32_t x = 0; x < img_width / 2; x++) {
            uint32_t left_index = y * img_width + x;
            uint32_t right_index = y * img_width + (img_width - 1 - x);

            uint32_t temp = img_buffer[left_index];
            img_buffer[left_index] = img_buffer[right_index];
            img_buffer[right_index] = temp;
        }
    }
}

void image_draw(void) {
    VGA_DMA_BUFFER = (uint32_t)img_buffer;
    VGA_DMA_RESOLUTION = (img_height << 16) | img_width; // Y | X

    /*from PIL import Image

    # Load sprite (original size, no resize here, but you can resize if needed)
    img = Image.open("background.bmp").convert("RGB")
    width, height = img.size  # e.g., 100x53

    with open("background.h", "w") as f:

        f.write("const unsigned char tool_open[RES_Y][RES_X] = {\n")

        for y in range(height):
            row = []
            for x in range(width):
                r, g, b = img.getpixel((x, y))
                # Skip transparent color check here (we'll handle in C)
                r3 = r >> 5
                g3 = g >> 5
                b2 = b >> 6
                pixel = (r3 << 5) | (g3 << 2) | b2
                row.append(str(pixel))
            f.write("    {" + ", ".join(row) + "},\n")

        f.write("};\n\n")
        f.write("#endif // smoke_H\n")

    print("Header file created!") */

    // Enable DMA (bit 2 = EN in your spec)
    VGA_DMA_STATUS |= (1 << 2);
}


void image_swap_buffers(void) {
    vga_swap_buffers();
}
