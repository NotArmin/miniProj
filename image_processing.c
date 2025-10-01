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
