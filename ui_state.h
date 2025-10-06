// ui_state.h

#ifndef UI_STATE_H
#define UI_STATE_H

#include "vga.h"
#include "background.h"

typedef enum {
    BG_MAIN,
    BG_UPLOAD, 
    BG_PROCESS
} bg_id_t;

// Add explicit UI states for clearer state machine
typedef enum {
    STATE_MENU_NAVIGATION,  // User navigating menus
    STATE_VIEWING_IMAGE     // User viewing processed image
} ui_state_t;

/* UI API */
void interrupt_init_ui(void);
void handle_interrupt_ui(unsigned cause);
void process_ui_events(void);
void ui_draw_initial(void);

// Flag setters for ISR
void ui_flag_move_down(void);
void ui_flag_enter(void);

#endif // UI_STATE_H