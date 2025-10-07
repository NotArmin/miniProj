// ui_state.c

#include "ui_state.h"
#include "dtekv-lib.h"
#include "vga.h"
#include "image_processing.h"
#include "background.h"
#include "performance_analysis.h"

#include <stdint.h>

/* Hardware register addresses */
#define BTN1_INT_EN_ADDR   ((volatile unsigned int *) 0x040000D8)
#define BTN1_INT_STAT_ADDR ((volatile unsigned int *) 0x040000DC)
#define SW_INT_EN_ADDR     ((volatile unsigned int *) 0x04000018)
#define SW_INT_STAT_ADDR   ((volatile unsigned int *) 0x0400001C)
#define SW_BASE            ((volatile unsigned int *) 0x04000010)  // switch values (bit 0)

/* Externs from other modules */
extern volatile unsigned char * const BUF0;
extern volatile unsigned char * const BUF1;
extern volatile unsigned int  * const VGA_CTRL_PTR;

/* Some external functions provided elsewhere; declare them so compiler knows about them. */
extern void enable_interrupt(void); // if this is defined elsewhere (your platform init)
extern void delay(int ms);         // from timetemplate.S

/* Local state */
static unsigned char current_image[RES_Y][RES_X]; // working buffer for current image
static bg_id_t current_bg = BG_MAIN; // current background/menu
static int arrow_idx = 0; // current arrow index in menu
static int selected_image_index; // 1,2,3 for Bliss,KTH,Icecream
static ui_state_t current_state = STATE_MENU_NAVIGATION; // Explicit state

/* Interrupt flags - volatile for ISR */
volatile static unsigned int flag_move_down = 0; // set by KEY1 ISR
volatile static unsigned int flag_enter = 0;     // set by switch ISR

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
            unsigned char s = handSprite[ry][rx];
            if (s != 0) vram[y * RES_X + x] = s;
        }
    }
}

static void restore_region_from_bg(volatile unsigned char *vram, const unsigned char bg[RES_Y][RES_X], int sx, int sy) {
    for (int ry = 0; ry < 20; ++ry) {
        int y = sy + ry;
        if (y < 0 || y >= RES_Y) continue;
        for (int rx = 0; rx < 20; ++rx) {
            int x = sx + rx;
            if (x < 0 || x >= RES_X) continue;
            vram[y * RES_X + x] = bg[y][x];
        }
    }
}

/* pixel positions */
static int main_option_y(int idx)    { return 80 + idx * 40; }
static int upload_option_y(int idx)  { 
    if (idx == 3) return 200;  // Special position for Return button
    return 70 + idx * 45;      // Normal menu items
}
static int process_option_y(int idx) { 
    if (idx == 7) return 200;  // Special position for Return button  
    return 40 + idx * 25;      // Normal menu items
}

/* Arrow X positions - Return button needs different X too */
static int upload_option_x(int idx) {
    if (idx == 3) return 170;  // Special X for Return button
    return 20;                  // Normal left-aligned for menu items
}

static int process_option_x(int idx) {
    if (idx == 7) return 170;  // Special X for Return button
    return 20;                  // Normal left-aligned for menu items
}

static int main_option_x(int idx)    { return 40; } // Always left-aligned


/* load selected image into working buffer */
static void load_selected_image(void) {
    unsigned char (*src)[RES_X] = ( (void*)0 ); // ( (void*)0 ) is NULL pointer
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
    unsigned char (*dst)[RES_X] = ( (void*)0 ); // ( (void*)0 ) is NULL pointer
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
            show_background_now(mainMenuStyle);
            draw_arrow_on_vram(BUF0, main_option_x(arrow_idx), main_option_y(arrow_idx));
            break;
        case BG_UPLOAD:
            show_background_now(uploadMenuStyle);
            draw_arrow_on_vram(BUF0, upload_option_x(arrow_idx), upload_option_y(arrow_idx));
            break;
        case BG_PROCESS:
            show_background_now(processMenuStyle);
            draw_arrow_on_vram(BUF0, process_option_x(arrow_idx), process_option_y(arrow_idx));
            break;
    }
}

static void apply_process_and_show(int option_idx) {
    const char* filter_names[] = {
        "Grayscale", "Black & White", "Invert", "Mirror", 
        "Blur 3x3", "Sharpen 3x3", "Sobel"
    };
    
    // Only run performance analysis if we have a valid filter
    if (option_idx >= 0 && option_idx <= 6) {
        before_perf();
    }
    
    // Apply the filter
    switch (option_idx) {
        case 0: ip_blackwhite(current_image, BUF0); break;
        case 1: ip_invert(current_image, BUF0); break;
        case 2: ip_mirror(current_image, BUF0); break;
        case 3: ip_blur3x3(current_image, BUF0); break;
        case 4: ip_sharpen3x3(current_image, BUF0); break;
        case 5: ip_sobel(current_image, BUF0); break;
        default: return;
    }
    
    // Display the result
    *(VGA_CTRL_PTR + 1) = (unsigned int)BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    
    // Copy back into current_image for filter stacking
    for (int y=0;y<RES_Y;y++) {
        for (int x=0;x<RES_X;x++) {
            current_image[y][x] = BUF0[y*RES_X + x];
        }
    }
    
    // Run performance analysis after filter application
    if (option_idx >= 0 && option_idx <= 6) {
        present_data(filter_names[option_idx]);
    }
}

/*
static void apply_process_and_show(int option_idx) {
    switch (option_idx) {
        case 0: ip_grayscale(current_image, BUF0); break;
        case 1: ip_blackwhite(current_image, BUF0); break;
        case 2: ip_invert(current_image, BUF0); break;
        case 3: ip_mirror(current_image, BUF0); break;
        case 4: ip_blur3x3(current_image, BUF0); break;
        case 5: ip_sharpen3x3(current_image, BUF0); break;
        case 6: ip_sobel(current_image, BUF0); break;
        default: return;
    }
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
}
*/

/* Only updates arrow, not entire screen */
static void update_arrow_position(int new_idx) {
    int old_idx = arrow_idx;
    arrow_idx = new_idx;
    
    // Only redraw the pixels that changed
    switch (current_bg) {
        case BG_MAIN:
            restore_region_from_bg(BUF0, mainMenuStyle, main_option_x(old_idx), main_option_y(old_idx));
            draw_arrow_on_vram(BUF0, main_option_x(arrow_idx), main_option_y(arrow_idx));
            break;
        case BG_UPLOAD:
            restore_region_from_bg(BUF0, uploadMenuStyle, upload_option_x(old_idx), upload_option_y(old_idx));
            draw_arrow_on_vram(BUF0, upload_option_x(arrow_idx), upload_option_y(arrow_idx));
            break;
        case BG_PROCESS:
            restore_region_from_bg(BUF0, processMenuStyle, process_option_x(old_idx), process_option_y(old_idx));
            draw_arrow_on_vram(BUF0, process_option_x(arrow_idx), process_option_y(arrow_idx));
            break;
    }
    
    // Single buffer swap
    *(VGA_CTRL_PTR + 1) = (unsigned int)BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
}

/* Get maximum index for current background */
static int get_max_index(void) {
    switch (current_bg) {
        case BG_MAIN: return 2;
        case BG_UPLOAD: return 3; 
        case BG_PROCESS: return 7;
        default: return 0;
    }
}

/* ---------- INTERRUPT INIT ---------- */
void interrupt_init_ui(void) {
    *BTN1_INT_EN_ADDR = 0x1;
    *SW_INT_EN_ADDR = 0x1;
    sw_prev = (*SW_BASE) & 0x1;
    enable_interrupt();
}

/* UI Flag Setters */
void ui_flag_move_down(void) {
    flag_move_down = 1;
}

void ui_flag_enter(void) {
    flag_enter = 1; 
}

/* ---------- INTERRUPT HANDLER  ----------
   KEY1 moves arrow down, SW0 UP acts as ENTER.
*/
void handle_interrupt_ui(unsigned cause) {
    // Button interrupt
    if ((*BTN1_INT_STAT_ADDR & 0x1) != 0) {
        *BTN1_INT_STAT_ADDR = 0x0;  // clear flag
        flag_move_down = 1;
        // print("KEY1 Interrupt\n");
    }

    // Switch interrupt  
    if ((*SW_INT_STAT_ADDR & 0x1) != 0) {
        *SW_INT_STAT_ADDR = 0x0; // clear flag
        unsigned int cur = (*SW_BASE) & 0x1;
        if (cur != sw_prev && cur == 1) { // Only on switch ON transition
            flag_enter = 1;
            // print("SW0 Interrupt\n");
        }
        sw_prev = cur;
    }
}

/* ---------- PROCESS EVENTS (call from main loop) ---------- */
void process_ui_events(void) {
    // Handle move down (KEY1)
    if (flag_move_down) {
        flag_move_down = 0;
        
        if (current_state == STATE_VIEWING_IMAGE) {
            // Return from image view
            current_state = STATE_MENU_NAVIGATION;
            render_current_menu();
        } else {
            // Normal menu navigation
            int max_idx = get_max_index();
            update_arrow_position((arrow_idx + 1) % (max_idx + 1));
        }
    }

    // Handle enter (SW0)  
    if (flag_enter) {
        flag_enter = 0;
        
        if (current_state == STATE_VIEWING_IMAGE) {
            // Return from image view
            current_state = STATE_MENU_NAVIGATION;
            render_current_menu();
            return;
        }
        
        // Menu-specific enter handling
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
                    *(VGA_CTRL_PTR + 1) = (unsigned int)BUF0;
                    *(VGA_CTRL_PTR + 0) = 0;
                    current_state = STATE_VIEWING_IMAGE;
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
            if (arrow_idx >= 0 && arrow_idx <= 6) {
                apply_process_and_show(arrow_idx);
                // Copy result back for filter stacking
                for (int y=0;y<RES_Y;y++) {
                    for (int x=0;x<RES_X;x++) {
                        current_image[y][x] = BUF0[y*RES_X + x];
                    }
                }
                current_state = STATE_VIEWING_IMAGE;
            } else if (arrow_idx == 7) { // Return
                current_bg = BG_MAIN;
                arrow_idx = 0;
                render_current_menu();
            }
        }
    }
}

/* public helper */
void ui_draw_initial(void) {
    render_current_menu();
}
