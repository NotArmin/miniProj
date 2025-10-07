// image_processing.c

#include "image_processing.h"
#include <stdint.h>

// Helper: clamp to 0..255. Prevents arithmetic overflow/underflow in pixel operations.
// Essential for filters that can produce values outside the valid color range.
static inline unsigned char clamp255(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (unsigned char)v;
}

// Get pixel with border replication (clamping). Handles edge cases gracefully.
// For pixels at image boundaries, uses the nearest valid pixel instead of crashing.
// This creates a "reflected" border effect for convolution filters.
static inline unsigned char pixel_at(const unsigned char img[RES_Y][RES_X], int y, int x) {
    if (y < 0) y = 0;
    if (y >= RES_Y) y = RES_Y - 1;
    if (x < 0) x = 0;
    if (x >= RES_X) x = RES_X - 1;
    return img[y][x];
}

// Extract color components from RRRGGBB pixel (3-2-2 format)
static inline unsigned char get_red(unsigned char pixel) {
    return (pixel >> 5) & 0x7;    // 3 bits: 0-7
}

static inline unsigned char get_green(unsigned char pixel) {
    return (pixel >> 3) & 0x3;    // 2 bits: 0-3 
}

static inline unsigned char get_blue(unsigned char pixel) {
    return (pixel >> 1) & 0x3;    // 2 bits: 0-3 
}

// Combine components back to RRRGGBB format (3-2-2)
static inline unsigned char make_rgb(unsigned char r, unsigned char g, unsigned char b) {
    return ((r & 0x7) << 5) | ((g & 0x3) << 3) | ((b & 0x3) << 1);
}

// Write to destination framebuffer at (y,x)
// Uses row-major indexing: y * width + x
static inline void dst_write(volatile unsigned char *dst, int y, int x, unsigned char v) {
    dst[y * RES_X + x] = v;
}

/* 1) Grayscale Conversion - Simpler version
 * Uses weighted average of R,G,B channels 
 */
void ip_grayscale(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {

            unsigned char r = get_red(pixel_at(src, y, x));
            unsigned char g = get_green(pixel_at(src, y, x));
            unsigned char b = get_blue(pixel_at(src, y, x));
            
            // Scale green and blue to match red's range (0-7)
            // Green: 0-3 → 0-6 (multiply by 2)
            // Blue: 0-3 → 0-6 (multiply by 2)
            int g_scaled = g * 2;
            int b_scaled = b * 2;
            
            // Weighted average (you can adjust weights)
            int gray = (r + g_scaled + b_scaled) / 3;
            
            // Ensure in 0-7 range
            if (gray > 7) gray = 7;
            
            // Convert back to output channels
            unsigned char out_g = gray >> 1;  // 0-7 → 0-3
            unsigned char out_b = gray >> 1;  // 0-7 → 0-3
            
            dst_write(dst, y, x, make_rgb(gray, out_g, out_b));
        }
    }
}

/* 2) Black & White using threshold
 * Converts image to binary black/white based on brightness threshold
 * Pixel >= threshold becomes white (255), otherwise black (0)
 * Uses the overall pixel brightness, not individual color channels
 */
void ip_blackwhite(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst, unsigned char threshold) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            unsigned char s = src[y][x];
            dst_write(dst, y, x, (s >= threshold) ? 255 : 0);
        }
    }
}

/* 3) Invert: Simple color inversion
 * Mathematical inverse: output = 255 - input
 * Works on RRRGGBB format because it preserves channel relationships
 * Bright becomes dark, colors become their complements
 */
void ip_invert(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            unsigned char s = src[y][x];
            dst_write(dst, y, x, (unsigned char)(255 - s));
        }
    }
}

/* 4) Mirror (horizontal flip)
 * Reflects image across vertical center axis
 * Pixel at (x,y) moves to (width-1-x, y)
 * Pure spatial transformation - no pixel value changes
 */
void ip_mirror(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            unsigned char s = src[y][RES_X - 1 - x];
            dst_write(dst, y, x, s);
        }
    }
}

/* 5) Blur 3x3 (box blur) - Low-pass filter
 * Kernel: 3x3 uniform averaging filter
 * [ 1 1 1 ]
 * [ 1 1 1 ] × (1/9)
 * [ 1 1 1 ]
 * 
 * Each output pixel is the average of its 3x3 neighborhood:
 * [ (x-1,y-1)  (x,y-1)  (x+1,y-1) ]
 * [ (x-1,y)    (x,y)    (x+1,y)   ]
 * [ (x-1,y+1)  (x,y+1)  (x+1,y+1) ]
 * 
 * Effect: Reduces noise and detail, creates smooth appearance
 * Processes R,G,B channels separately to maintain color integrity
 */
void ip_blur3x3(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            int sum_r = 0, sum_g = 0, sum_b = 0;
            
            // Sum 3x3 neighborhood for each color channel
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    unsigned char pixel = pixel_at(src, y + ky, x + kx);
                    sum_r += get_red(pixel);
                    sum_g += get_green(pixel);
                    sum_b += get_blue(pixel);
                }
            }
            
            // Average and output (integer division for speed)
            unsigned char out_r = sum_r / 9;
            unsigned char out_g = sum_g / 9;
            unsigned char out_b = sum_b / 9;
            
            dst_write(dst, y, x, make_rgb(out_r, out_g, out_b));
        }
    }
}

/* 6) Sharpen 3x3 - High-pass filter for edge enhancement
 * Kernel: Emphasizes center, suppresses neighbors
 * [  0  -1   0 ]
 * [ -1   5  -1 ]
 * [  0  -1   0 ]
 * 
 * Formula: output = 5×center - up - down - left - right
 * Amplifies high-frequency details (edges) while reducing flat areas
 * The negative weights create a differential effect that enhances contrasts
 */
void ip_sharpen3x3(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            // Extract color components from center pixel
            unsigned char center_r = get_red(pixel_at(src, y, x));
            unsigned char center_g = get_green(pixel_at(src, y, x));
            unsigned char center_b = get_blue(pixel_at(src, y, x));
            
            // Apply sharpen kernel to each channel separately
            // 5×center - neighbors creates edge enhancement
            int r = center_r * 5 
                    - get_red(pixel_at(src, y - 1, x))   // up
                    - get_red(pixel_at(src, y + 1, x))   // down
                    - get_red(pixel_at(src, y, x - 1))   // left
                    - get_red(pixel_at(src, y, x + 1));  // right
            
            int g = center_g * 5 
                    - get_green(pixel_at(src, y - 1, x))
                    - get_green(pixel_at(src, y + 1, x))
                    - get_green(pixel_at(src, y, x - 1))
                    - get_green(pixel_at(src, y, x + 1));
            
            int b = center_b * 5 
                    - get_blue(pixel_at(src, y - 1, x))
                    - get_blue(pixel_at(src, y + 1, x))
                    - get_blue(pixel_at(src, y, x - 1))
                    - get_blue(pixel_at(src, y, x + 1));
            
            // Clamp each channel to its valid range (R:0-7, G:0-7, B:0-3)
            unsigned char out_r = (r < 0) ? 0 : (r > 7) ? 7 : r;
            unsigned char out_g = (g < 0) ? 0 : (g > 7) ? 7 : g;
            unsigned char out_b = (b < 0) ? 0 : (b > 3) ? 3 : b;
            
            dst_write(dst, y, x, make_rgb(out_r, out_g, out_b));
        }
    }
}

/* 7) Sobel Edge Detection - Gradient-based edge finder
 * Uses two 3x3 kernels to approximate image gradients:
 * 
 * Gx (horizontal gradient):   Gy (vertical gradient):
 * [ -1  0  +1 ]              [ -1  -2  -1 ]
 * [ -2  0  +2 ]              [  0   0   0 ]  
 * [ -1  0  +1 ]              [ +1  +2  +1 ]
 * 
 * For each pixel:
 * 1. Compute Gx (sensitivity to horizontal edges)
 * 2. Compute Gy (sensitivity to vertical edges)  
 * 3. Edge magnitude ≈ |Gx| + |Gy| (simplified, faster than sqrt(Gx²+Gy²))
 * 4. Normalize by dividing by 8 to keep in display range
 * 
 * Effect: Highlights regions of rapid intensity change (edges)
 * Bright pixels indicate strong edges, dark pixels indicate flat regions
 */
void ip_sobel(const unsigned char src[RES_Y][RES_X], volatile unsigned char *dst) {
    if (!dst) return;
    for (int y = 0; y < RES_Y; ++y) {
        for (int x = 0; x < RES_X; ++x) {
            // Extract color components for the 3x3 neighborhood
            // Naming: p[row][column]_[channel], e.g., p00_r = top-left red
            int p00_r = get_red(pixel_at(src, y - 1, x - 1));
            int p00_g = get_green(pixel_at(src, y - 1, x - 1));
            int p00_b = get_blue(pixel_at(src, y - 1, x - 1));
            
            int p01_r = get_red(pixel_at(src, y - 1, x));      // top
            int p01_g = get_green(pixel_at(src, y - 1, x));
            int p01_b = get_blue(pixel_at(src, y - 1, x));
            
            int p02_r = get_red(pixel_at(src, y - 1, x + 1));  // top-right
            int p02_g = get_green(pixel_at(src, y - 1, x + 1));
            int p02_b = get_blue(pixel_at(src, y - 1, x + 1));
            
            int p10_r = get_red(pixel_at(src, y, x - 1));      // left
            int p10_g = get_green(pixel_at(src, y, x - 1));
            int p10_b = get_blue(pixel_at(src, y, x - 1));
            
            int p12_r = get_red(pixel_at(src, y, x + 1));      // right
            int p12_g = get_green(pixel_at(src, y, x + 1));
            int p12_b = get_blue(pixel_at(src, y, x + 1));
            
            int p20_r = get_red(pixel_at(src, y + 1, x - 1));  // bottom-left
            int p20_g = get_green(pixel_at(src, y + 1, x - 1));
            int p20_b = get_blue(pixel_at(src, y + 1, x - 1));
            
            int p21_r = get_red(pixel_at(src, y + 1, x));      // bottom
            int p21_g = get_green(pixel_at(src, y + 1, x));
            int p21_b = get_blue(pixel_at(src, y + 1, x));
            
            int p22_r = get_red(pixel_at(src, y + 1, x + 1));  // bottom-right
            int p22_g = get_green(pixel_at(src, y + 1, x + 1));
            int p22_b = get_blue(pixel_at(src, y + 1, x + 1));
            
            // Apply Sobel kernels to each color channel
            // Gx detects horizontal edges, Gy detects vertical edges
            int gx_r = -p00_r + p02_r - 2*p10_r + 2*p12_r - p20_r + p22_r;
            int gy_r = -p00_r - 2*p01_r - p02_r + p20_r + 2*p21_r + p22_r;
            
            int gx_g = -p00_g + p02_g - 2*p10_g + 2*p12_g - p20_g + p22_g;
            int gy_g = -p00_g - 2*p01_g - p02_g + p20_g + 2*p21_g + p22_g;
            
            int gx_b = -p00_b + p02_b - 2*p10_b + 2*p12_b - p20_b + p22_b;
            int gy_b = -p00_b - 2*p01_b - p02_b + p20_b + 2*p21_b + p22_b;
            
            // Calculate approximate magnitude: |Gx| + |Gy|
            // Faster than true magnitude sqrt(Gx² + Gy²), good for real-time
            int mag_r = (gx_r < 0 ? -gx_r : gx_r) + (gy_r < 0 ? -gy_r : gy_r);
            int mag_g = (gx_g < 0 ? -gx_g : gx_g) + (gy_g < 0 ? -gy_g : gy_g);
            int mag_b = (gx_b < 0 ? -gx_b : gx_b) + (gy_b < 0 ? -gy_b : gy_b);
            
            // Normalize: divide by 8 to scale from theoretical max 2040 to 0-255 range
            mag_r = mag_r / 8;
            mag_g = mag_g / 8;
            mag_b = mag_b / 8;
            
            // Clamp to each channel's valid range
            unsigned char out_r = (mag_r > 7) ? 7 : mag_r;
            unsigned char out_g = (mag_g > 7) ? 7 : mag_g;
            unsigned char out_b = (mag_b > 3) ? 3 : mag_b;
            
            // Reconstruct final edge-detected pixel
            dst_write(dst, y, x, make_rgb(out_r, out_g, out_b));
        }
    }
}