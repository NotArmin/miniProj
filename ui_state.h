#ifndef UI_STATE_H
#define UI_STATE_H

#include "vga.h"
#include "background.h"

// number of images in UploadMenu
#define NUM_IMAGES 3

typedef enum {
    BG_MAIN,
    BG_UPLOAD,
    BG_PROCESS
} bg_id_t;

typedef struct {
    const unsigned char (*bg)[RES_X];
    int num_options;
} background_t;

/* UI API: call these from main.c */
void interrupt_init_ui(void);              // initialize IRQs for UI
void handle_interrupt_ui(unsigned cause);  // call from your global IRQ handler
void process_ui_events(void);              // run in main loop to process flags
void ui_draw_initial(void);                // draw initial menu to screen

#endif // UI_STATE_H
