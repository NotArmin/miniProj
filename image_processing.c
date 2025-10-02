#include "image_processing.h"
#include <stdint.h>

// Helper: clamp to 0..255
static inline unsigned char clamp255(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (unsigned char)v;
}

// Get pixel with border replicate
static inline unsigned char pixel_at(const unsigned char img[RES_Y][RES_X], int y, int x) {
    if (y < 0) y = 0;
    if (y >= RES_Y) y = RES_Y - 1;
    if (x < 0) x = 0;
    if (x >= RES_X) x = RES_X - 1;
    return img[y][x];
}

// Write to dst at (y,x)
static inline void dst_write(volatile unsigned char *dst, int y, int x, unsigned char v) {
    dst[y * RES_X + x] = v;
}

/* 1) Invert: */
void ip_invert(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            unsigned char s = src[y][x];
            dst_write(dst, y, x, (unsigned char)(255 - s));
        }
    }
}

/* 2) Black & White using threshold */
void ip_blackwhite(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst, unsigned char threshold) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            unsigned char s = src[y][x];
            dst_write(dst, y, x, (s >= threshold) ? 255 : 0);
        }
    }
}

/* 3) Mirror (horizontal flip) */
void ip_mirror(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            unsigned char s = src[y][RES_X - 1 - x];
            dst_write(dst, y, x, s);
        }
    }
}

/* 4) Blur 3x3 (box blur) */
void ip_blur3x3(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    sum += pixel_at(src, y + ky, x + kx);
                }
            }
            dst_write(dst, y, x, (unsigned char)(sum / 9));
        }
    }
}

/* 5) Sharpen 3x3 kernel:
   0  -1  0
  -1   5 -1
   0  -1  0
*/
void ip_sharpen3x3(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            int center = pixel_at(src, y, x) * 5;
            int s = center
                    - pixel_at(src, y - 1, x)   // up
                    - pixel_at(src, y + 1, x)   // down
                    - pixel_at(src, y, x - 1)   // left
                    - pixel_at(src, y, x + 1);  // right
            dst_write(dst, y, x, clamp255(s));
        }
    }
}

/* 6) Sobel (edge detection) using kernels:
   Gx =  [-1 0 1; -2 0 2; -1 0 1]
   Gy =  [-1 -2 -1; 0 0 0; 1 2 1]
   Magnitude approx = |Gx| + |Gy|
*/
void ip_sobel(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            int p00 = pixel_at(src, y - 1, x - 1);
            int p01 = pixel_at(src, y - 1, x    );
            int p02 = pixel_at(src, y - 1, x + 1);
            int p10 = pixel_at(src, y    , x - 1);
            int p12 = pixel_at(src, y    , x + 1);
            int p20 = pixel_at(src, y + 1, x - 1);
            int p21 = pixel_at(src, y + 1, x    );
            int p22 = pixel_at(src, y + 1, x + 1);

            int gx = -p00 + p02 - 2 * p10 + 2 * p12 - p20 + p22;
            int gy = -p00 - 2 * p01 - p02 + p20 + 2 * p21 + p22;

            int mag = ( (gx < 0) ? -gx : gx ) + ( (gy < 0) ? -gy : gy );
            // normalize: maximum possible mag for 8-bit input is 4*255 + 4*255 = 2040,
            // but using clamp to 255 works; we scale down by dividing by 8 to keep contrast reasonable
            mag = mag / 8;
            dst_write(dst, y, x, clamp255(mag));
        }
    }
}
