// ui_state.c
#include "ui_state.h"
#include "dtekv-lib.h"
#include "vga.h"
#include "image_processing.h"
#include "background.h"
#include <string.h>

/* Hardware register addresses */
#define BTN1_INT_EN_ADDR   ((volatile unsigned int *) 0x040000D8)
#define BTN1_INT_STAT_ADDR ((volatile unsigned int *) 0x040000DC)

#define SW_INT_EN_ADDR     ((volatile unsigned int *) 0x04000018)
#define SW_INT_STAT_ADDR   ((volatile unsigned int *) 0x0400001C)
#define SW_BASE            ((volatile unsigned int *) 0x04000010)  // switch register (bit 0)

/* Externs from other modules */
extern volatile unsigned char * const BUF0;
extern volatile unsigned char * const BUF1;
extern volatile unsigned int  * const VGA_CTRL_PTR;

/* Some external functions provided elsewhere; declare them so compiler knows about them. */
extern void enable_interrupt(void); // if this is defined elsewhere (your platform init)
extern void delay(int ms);         // from timetemplate.S

/* Local state */
static unsigned char current_image[RES_Y][RES_X];

static bg_id_t current_bg = BG_MAIN;
static int arrow_idx = 0;
static int selected_image_index = 2; // default to image 2 as in your example

volatile static unsigned int flag_move_down = 0; // set by KEY1 ISR
volatile static unsigned int flag_enter = 0;     // set by switch ISR
volatile static unsigned int flag_redraw = 0;

static unsigned int sw_prev = 0; // previous SW0 state

/* ---- helpers ---- */
static inline unsigned char clamp255(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (unsigned char)v;
}

static void draw_bg_to_vram(const unsigned char bg[RES_Y][RES_X], volatile unsigned char *vram) {
    for (int y = 0; y < RES_Y; ++y)
        for (int x = 0; x < RES_X; ++x)
            vram[y * RES_X + x] = bg[y][x];
}

static void show_background_now(const unsigned char bg[RES_Y][RES_X]) {
    draw_bg_to_vram(bg, BUF0);
    draw_bg_to_vram(bg, BUF1);
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
}

static void draw_arrow_on_vram(volatile unsigned char *vram, int sx, int sy) {
    for (int ry = 0; ry < 20; ++ry) {
        int y = sy + ry;
        if (y < 0 || y >= RES_Y) continue;
        for (int rx = 0; rx < 20; ++rx) {
            int x = sx + rx;
            if (x < 0 || x >= RES_X) continue;
            unsigned char s = arrowSprite[ry][rx];
            if (s != 0) vram[y * RES_X + x] = s;
        }
    }
}

static void restore_region_from_bg(volatile unsigned char *vram, const unsigned char bg[RES_Y][RES_X], int sx, int sy) {
    for (int ry = 0; ry < 50; ++ry) {
        int y = sy + ry;
        if (y < 0 || y >= RES_Y) continue;
        for (int rx = 0; rx < 50; ++rx) {
            int x = sx + rx;
            if (x < 0 || x >= RES_X) continue;
            vram[y * RES_X + x] = bg[y][x];
        }
    }
}

/* pixel positions (tweak to match your bitmaps) */
static int main_option_y(int idx)    { return 100 + idx * 20; }
static int upload_option_y(int idx)  { return 80 + idx * 20; }
static int process_option_y(int idx) { return 60 + idx * 10; }

static const int arrow_x_left = 20;

/* load selected image into working buffer */
static void load_selected_image(void) {
    unsigned char (*src)[RES_X] = NULL;
    switch (selected_image_index) {
        case 1: src = (unsigned char (*)[RES_X])Bliss; break;
        case 2: src = (unsigned char (*)[RES_X])KTH; break;
        case 3: src = (unsigned char (*)[RES_X])Icecream; break;
    }
    if (src) {
        for (int y=0;y<RES_Y;y++)
            for (int x=0;x<RES_X;x++)
                current_image[y][x] = src[y][x];
    }
}

static void draw_current_image_to_vram(volatile unsigned char *vram) {
    for (int y=0;y<RES_Y;y++)
        for (int x=0;x<RES_X;x++)
            vram[y*RES_X + x] = current_image[y][x];
}

static void copy_current_to_imageN(void) {
    unsigned char (*dst)[RES_X] = NULL;
    switch (selected_image_index) {
        case 1: dst = (unsigned char (*)[RES_X])Bliss; break;
        case 2: dst = (unsigned char (*)[RES_X])KTH; break;
        case 3: dst = (unsigned char (*)[RES_X])Icecream; break;
    }
    if (dst) {
        for (int y=0;y<RES_Y;y++)
            for (int x=0;x<RES_X;x++)
                dst[y][x] = current_image[y][x];
    }
}

static void render_current_menu(void) {
    switch (current_bg) {
        case BG_MAIN:
            show_background_now(mainMenu);
            draw_arrow_on_vram(BUF0, arrow_x_left, main_option_y(arrow_idx));
            break;
        case BG_UPLOAD:
            show_background_now(UploadMenu);
            draw_arrow_on_vram(BUF0, arrow_x_left, upload_option_y(arrow_idx));
            break;
        case BG_PROCESS:
            show_background_now(ProcessMenu);
            draw_arrow_on_vram(BUF0, arrow_x_left, process_option_y(arrow_idx));
            break;
    }
}

static void apply_process_and_show(int option_idx) {
    switch (option_idx) {
        case 0: ip_blackwhite(current_image, BUF0, 128); break;
        case 1: ip_invert(current_image, BUF0); break;
        case 2: ip_mirror(current_image, BUF0); break;
        case 3: ip_blur3x3(current_image, BUF0); break;
        case 4: ip_sharpen3x3(current_image, BUF0); break;
        case 5: ip_sobel(current_image, BUF0); break;
        default: return;
    }
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
}

/* ---------- INTERRUPT INIT ---------- */
void interrupt_init_ui(void)
{
    *BTN1_INT_EN_ADDR = 0x1;
    *SW_INT_EN_ADDR = 0x1;

    /* init sw_prev from hardware */
    sw_prev = (*SW_BASE) & 0x1;

    /* if enable_interrupt is provided elsewhere, this will make sure IRQs are globally enabled */
    enable_interrupt();
}

/* ---------- INTERRUPT HANDLER (minimal) ----------
   KEY1 moves arrow down, SW0 change acts as ENTER.
*/
void handle_interrupt_ui(unsigned cause)
{
    if ((*BTN1_INT_STAT_ADDR & 0x1) != 0) {
        *BTN1_INT_STAT_ADDR = 0x1;  // clear
        flag_move_down = 1;
    }

    if ((*SW_INT_STAT_ADDR & 0x1) != 0) {
        *SW_INT_STAT_ADDR = 0x1; // clear
        unsigned int cur = (*SW_BASE) & 0x1;
        if (cur != sw_prev) {
            sw_prev = cur;
            flag_enter = 1;
        }
    }
}

/* ---------- PROCESS EVENTS (call from main loop) ---------- */
void process_ui_events(void)
{
    if (flag_move_down) {
        flag_move_down = 0;

        int old_idx = arrow_idx;
        int max_idx = 0;
        if (current_bg == BG_MAIN) max_idx = 2;
        else if (current_bg == BG_UPLOAD) max_idx = 3;
        else if (current_bg == BG_PROCESS) max_idx = 6;

        arrow_idx++;
        if (arrow_idx > max_idx) arrow_idx = 0;

        if (current_bg == BG_MAIN) restore_region_from_bg(BUF0, mainMenu, arrow_x_left, main_option_y(old_idx));
        else if (current_bg == BG_UPLOAD) restore_region_from_bg(BUF0, UploadMenu, arrow_x_left, upload_option_y(old_idx));
        else if (current_bg == BG_PROCESS) restore_region_from_bg(BUF0, ProcessMenu, arrow_x_left, process_option_y(old_idx));

        if (current_bg == BG_MAIN) draw_arrow_on_vram(BUF0, arrow_x_left, main_option_y(arrow_idx));
        else if (current_bg == BG_UPLOAD) draw_arrow_on_vram(BUF0, arrow_x_left, upload_option_y(arrow_idx));
        else if (current_bg == BG_PROCESS) draw_arrow_on_vram(BUF0, arrow_x_left, process_option_y(arrow_idx));

        *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
        *(VGA_CTRL_PTR + 0) = 0;
    }

    if (flag_enter) {
        flag_enter = 0;

        if (current_bg == BG_MAIN) {
            switch (arrow_idx) {
                case 0: // Upload
                    current_bg = BG_UPLOAD;
                    arrow_idx = 0;
                    render_current_menu();
                    break;
                case 1: // Process
                    current_bg = BG_PROCESS;
                    arrow_idx = 0;
                    render_current_menu();
                    break;
                case 2: // Download
                    copy_current_to_imageN();
                    draw_current_image_to_vram(BUF0);
                    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
                    *(VGA_CTRL_PTR + 0) = 0;
                    break;
            }
        } else if (current_bg == BG_UPLOAD) {
            if (arrow_idx >= 0 && arrow_idx <= 2) {
                selected_image_index = arrow_idx + 1;
                load_selected_image();
                current_bg = BG_MAIN;
                arrow_idx = 0;
                render_current_menu();
            } else if (arrow_idx == 3) { // Return
                current_bg = BG_MAIN;
                arrow_idx = 0;
                render_current_menu();
            }
        } else if (current_bg == BG_PROCESS) {
            if (arrow_idx >= 0 && arrow_idx <= 5) {
                apply_process_and_show(arrow_idx);
                // copy back into current_image (2D indexing properly)
                for (int y=0;y<RES_Y;y++)
                    for (int x=0;x<RES_X;x++)
                        current_image[y][x] = BUF0[y*RES_X + x];
                render_current_menu();
            } else if (arrow_idx == 6) { // Return
                current_bg = BG_MAIN;
                arrow_idx = 0;
                render_current_menu();
            }
        }
    }

    if (flag_redraw) {
        flag_redraw = 0;
        render_current_menu();
    }
}

/* public helper */
void ui_draw_initial(void) {
    render_current_menu();
}
