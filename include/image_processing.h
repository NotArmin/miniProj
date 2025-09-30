#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include <stdint.h>

void image_load(const uint32_t *data, uint32_t width, uint32_t height);
void image_grayscale(void);
void image_draw(void);
void image_swap_buffers(void);

#endif // IMAGE_PROCESSING_H
