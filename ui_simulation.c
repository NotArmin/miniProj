// ui_simulation.c
// Hard-coded simulation of menu navigation and applying filters.
// Assumes Image1/Image2/Image3 are mutable arrays, and backgrounds + arrowSprite exist.

#include "dtekv-lib.h"
#include "vga.h"
#include "image_processing.h"
#include "background.h"   // contains the background declarations (mainMenu, etc.)
#include <string.h>       // for memcpy

// external arrays (adjust if names/qualifiers differ)
extern const unsigned char mainMenu[RES_Y][RES_X];
extern const unsigned char UploadMenu[RES_Y][RES_X];
extern const unsigned char ProcessMenu[RES_Y][RES_X];

#define ARW_W 20
#define ARW_H 20
extern const unsigned char arrowSprite[ARW_H][ARW_W];

// VGA framebuffers (from your vga.c)
extern volatile unsigned char * const BUF0;
extern volatile unsigned char * const BUF1;
extern volatile unsigned int  * const VGA_CTRL_PTR;

// local working image buffer (holds currently selected image; will be processed)
static unsigned char current_image[RES_Y][RES_X];

// helper: copy image arrays
static void copy_image_to_current(const unsigned char src[RES_Y][RES_X]) {
    for (int y=0;y<RES_Y;y++)
      for (int x=0;x<RES_X;x++)
        current_image[y][x] = src[y][x];
}

/*
static void copy_current_to_image(unsigned char dst[RES_Y][RES_X]) {
    for (int y=0;y<RES_Y;y++)
      for (int x=0;x<RES_X;x++)
        dst[y][x] = current_image[y][x];
}*/

// draw a background array into a VGA buffer
static void draw_bg_to_vram(const unsigned char bg[RES_Y][RES_X], volatile unsigned char *vram) {
    for (int y=0;y<RES_Y;y++)
      for (int x=0;x<RES_X;x++)
        vram[y*RES_X + x] = bg[y][x];
}

// draw full background to both buffers and show
/*static void show_background(const unsigned char bg[RES_Y][RES_X]) {
    draw_bg_to_vram(bg, BUF0);
    draw_bg_to_vram(bg, BUF1);
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    for (volatile int i=0;i<200000;i++) asm volatile ("nop");
}*/

// draw arrow sprite at pixel top-left (sx,sy) over current background in vram
// transparent rule: draw sprite pixel only if value != 0 (assumes 0 = transparent)
static void draw_sprite(volatile unsigned char *vram, int sx, int sy) {
    for (int ry=0; ry<ARW_H; ry++) {
        int y = sy + ry;
        if (y < 0 || y >= RES_Y) continue;
        for (int rx=0; rx<ARW_W; rx++) {
            int x = sx + rx;
            if (x < 0 || x >= RES_X) continue;
            unsigned char s = arrowSprite[ry][rx];
            if (s != 0) vram[y*RES_X + x] = s;
        }
    }
}

// restore the background area under the arrow from the corresponding background array.
static void restore_bg_region(volatile unsigned char *vram, const unsigned char bg[RES_Y][RES_X], int sx, int sy) {
    for (int ry=0; ry<ARW_H; ry++) {
        int y = sy + ry;
        if (y < 0 || y >= RES_Y) continue;
        for (int rx=0; rx<ARW_W; rx++) {
            int x = sx + rx;
            if (x < 0 || x >= RES_X) continue;
            vram[y*RES_X + x] = bg[y][x];
        }
    }
}

// convenience: show background and draw arrow at (sx,sy)
static void show_bg_with_arrow(const unsigned char bg[RES_Y][RES_X], int sx, int sy) {
    draw_bg_to_vram(bg, BUF0);
    draw_bg_to_vram(bg, BUF1);
    draw_sprite(BUF0, sx, sy);
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    for (volatile int i=0;i<200000;i++) asm volatile ("nop");
}

// small delay helper
static void delay_simple(int loops) {
    for (volatile int i=0;i<loops;i++) asm volatile ("nop");
}

// simulate pressing KEY1 to continue.
static void wait_for_key1_sim(const char *msg) {
    print(msg);
    print("  (simulating KEY1 press after a short delay)\n");
    delay_simple(5000000);
}

// Map menu option index to a y coordinate where arrow should be drawn.
static int main_option_y(int idx) {
    const int base = 80;
    const int spacing = 40;
    return base + idx * spacing;
}
static int upload_option_y(int idx) {
    const int base = 70;
    const int spacing = 38;
    return base + idx * spacing;
}
static int process_option_y(int idx) {
    const int base = 60;
    const int spacing = 28;
    return base + idx * spacing;
}

// top-left x for arrow (left of menu texts)
static const int arrow_x_left = 30;

// state machine simulation following your updated flow:
// select Upload -> choose Image2 -> back Main -> Process -> choose Invert then Mirror -> Return -> Download (overwrite Image2)
void simulate_run_through(void) {
    print("=== UI simulation start (Invert then Mirror) ===\n");

    // 1) start at mainMenu, arrow at Upload (idx 0)
    int main_idx = 0; // 0: Upload, 1: Process, 2: Download
    show_bg_with_arrow(mainMenu, arrow_x_left, main_option_y(main_idx));

    // user presses KEY1 on Upload -> go to UploadMenu
    wait_for_key1_sim("User chooses Upload on Main...");
    print("Switching to UploadMenu...\n");
    int upload_idx = 0; // arrow starts on Image 1
    show_bg_with_arrow(UploadMenu, arrow_x_left, upload_option_y(upload_idx));

    // user moves to Image 2 (simulate using switches: move down once)
    delay_simple(2000000);
    restore_bg_region(BUF0, UploadMenu, arrow_x_left, upload_option_y(upload_idx));
    upload_idx = 1; // Image 2
    draw_sprite(BUF0, arrow_x_left, upload_option_y(upload_idx));
    draw_bg_to_vram(UploadMenu, BUF1);
    draw_sprite(BUF1, arrow_x_left, upload_option_y(upload_idx));
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    delay_simple(2000000);

    // user presses KEY1 to select Image 2 -> copy Image2 to current_image and return to mainMenu
    wait_for_key1_sim("User selects Image 2 (press KEY1)...");
    print("Loading Image2 into current image buffer and returning to mainMenu...\n");
    copy_image_to_current((const unsigned char (*)[RES_X])Bliss);
    show_bg_with_arrow(mainMenu, arrow_x_left, main_option_y(main_idx));

    // Now user moves arrow to Process and presses KEY1
    delay_simple(2000000);
    restore_bg_region(BUF0, mainMenu, arrow_x_left, main_option_y(main_idx));
    main_idx = 1; // Process
    draw_sprite(BUF0, arrow_x_left, main_option_y(main_idx));
    draw_bg_to_vram(mainMenu, BUF1);
    draw_sprite(BUF1, arrow_x_left, main_option_y(main_idx));
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    delay_simple(2000000);

    wait_for_key1_sim("User chooses Process on Main (press KEY1)...");
    print("Switching to ProcessMenu...\n");
    int proc_idx = 0; // Start at Black & White
    show_bg_with_arrow(ProcessMenu, arrow_x_left, process_option_y(proc_idx));

    // --- NEW FLOW: select Invert (proc_idx = 1) first ---
    // move down once to Invert
    delay_simple(1500000);
    restore_bg_region(BUF0, ProcessMenu, arrow_x_left, process_option_y(proc_idx));
    proc_idx = 1; // Invert
    draw_sprite(BUF0, arrow_x_left, process_option_y(proc_idx));
    draw_bg_to_vram(ProcessMenu, BUF1);
    draw_sprite(BUF1, arrow_x_left, process_option_y(proc_idx));
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    delay_simple(1500000);

    // press KEY1 to apply Invert
    wait_for_key1_sim("User selects 'Invert' (press KEY1)...");
    print("Applying Invert to current image and showing result full screen...\n");
    ip_invert(current_image, BUF0);
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    delay_simple(2000000);

    // wait for KEY1 to return to ProcessMenu; copy back into current_image so effects stack
    wait_for_key1_sim("Viewing inverted result. Press KEY1 to return to Process menu...");
    print("Copying inverted result back into current image so further filters stack.\n");
    for (int y=0;y<RES_Y;y++)
      for (int x=0;x<RES_X;x++)
        current_image[y][x] = BUF0[y*RES_X + x];

    // redraw ProcessMenu with arrow at proc_idx (still 1)
    show_bg_with_arrow(ProcessMenu, arrow_x_left, process_option_y(proc_idx));

    // Now move to Mirror (proc_idx = 2)
    delay_simple(1500000);
    restore_bg_region(BUF0, ProcessMenu, arrow_x_left, process_option_y(proc_idx));
    proc_idx = 2; // Mirror
    draw_sprite(BUF0, arrow_x_left, process_option_y(proc_idx));
    draw_bg_to_vram(ProcessMenu, BUF1);
    draw_sprite(BUF1, arrow_x_left, process_option_y(proc_idx));
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    delay_simple(1500000);

    // press KEY1 to apply Mirror
    wait_for_key1_sim("User selects 'Mirror' (press KEY1)...");
    print("Applying Mirror to current image and showing result full screen...\n");
    ip_mirror(current_image, BUF0);
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    delay_simple(2000000);

    // wait for KEY1 to return to ProcessMenu; copy back into current_image so effects stack
    wait_for_key1_sim("Viewing mirrored result. Press KEY1 to return to Process menu...");
    print("Copying mirrored result back into current image so Invert + Mirror are combined.\n");
    for (int y=0;y<RES_Y;y++)
      for (int x=0;x<RES_X;x++)
        current_image[y][x] = BUF0[y*RES_X + x];

    // return to ProcessMenu
    show_bg_with_arrow(ProcessMenu, arrow_x_left, process_option_y(proc_idx));

    // user presses Return (simulate moving arrow down to Return (idx 6) and pressing KEY1)
    for (int i = proc_idx; i < 6; ++i) {
        restore_bg_region(BUF0, ProcessMenu, arrow_x_left, process_option_y(i));
        draw_bg_to_vram(ProcessMenu, BUF1);
        draw_sprite(BUF1, arrow_x_left, process_option_y(i+1));
        draw_sprite(BUF0, arrow_x_left, process_option_y(i+1));
        *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
        *(VGA_CTRL_PTR + 0) = 0;
        delay_simple(250000);
    }
    proc_idx = 6;
    wait_for_key1_sim("User selects 'Return' (press KEY1)...");
    print("Returning to mainMenu...\n");
    main_idx = 0; // keep arrow on Upload by default
    show_bg_with_arrow(mainMenu, arrow_x_left, main_option_y(main_idx));

    // Now user wants to Download (overwrite Image2). Move arrow to Download (idx 2)
    for (int i=0;i<2;i++) {
        restore_bg_region(BUF0, mainMenu, arrow_x_left, main_option_y(main_idx));
        main_idx++;
        draw_bg_to_vram(mainMenu, BUF1);
        draw_sprite(BUF1, arrow_x_left, main_option_y(main_idx));
        draw_sprite(BUF0, arrow_x_left, main_option_y(main_idx));
        *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
        *(VGA_CTRL_PTR + 0) = 0;
        delay_simple(2000000);
    }

    wait_for_key1_sim("User selects 'Download' (press KEY1)...");
    print("Overwriting Image2 with current image (Download)...\n");
    copy_image_to_current(Bliss);

    // show confirmation: draw current_image to screen momentarily
    print("Showing current image (downloaded to Image2) for confirmation...\n");
    for (int y=0;y<RES_Y;y++)
      for (int x=0;x<RES_X;x++)
        BUF0[y*RES_X + x] = current_image[y][x];
    *(VGA_CTRL_PTR + 1) = (unsigned int) BUF0;
    *(VGA_CTRL_PTR + 0) = 0;
    delay_simple(3000000);

    print("=== UI simulation complete ===\n");
    while (1) {
        delay_simple(10000000);
    }
}
