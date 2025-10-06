// image_processing.h

#ifndef IMAGEPROC_H
#define IMAGEPROC_H

#include "vga.h"

// Apply filters. 'src' is a pointer to the top-left of a RES_Y x RES_X image
// laid out row-major (we'll pass Bliss or a pointer to the Bliss array).
// 'dst' is a pointer to the output framebuffer (volatile so it can be VGA).
// If dst==NULL the function does nothing.

void ip_invert(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst);
void ip_blackwhite(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst, unsigned char threshold);
void ip_mirror(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst);
void ip_blur3x3(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst);
void ip_sharpen3x3(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst);
void ip_sobel(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst);

#endif // IMAGEPROC_H
