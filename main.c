// main.c

#include "dtekv-lib.h"
#include "vga.h"
#include "image_processing.h"
#include "ui_state.h"
#include "performance_analysis.h"

extern void print(const char*);
extern void printc(char);
extern void print_dec(unsigned int);
extern void print_hex32(unsigned int x);
extern void display_string(char*);
extern void time2string(char*,int);
extern void enable_interrupt();
extern void delay(int);

/* Below is the function that will be called when an interrupt is triggered. */
void handle_interrupt(unsigned cause) 
{  
  handle_interrupt_ui(cause);
}



extern volatile unsigned char * const BUF0;
extern volatile unsigned char * const BUF1;
extern volatile unsigned int  * const VGA_CTRL_PTR;
/*
// Normal launch
int main(void) {
  vga_init();

  ui_draw_initial(); // show MainBackground with arrow at Upload
  interrupt_init_ui();
  while (1) {
    process_ui_events();
    delay(1000);
  }
}
*/

// Launch with performance tests
int main(void) {
  vga_init();
  ui_draw_initial();
  interrupt_init_ui();
  
#ifdef RUN_PERFORMANCE_TESTS
  // Performance test mode
  print("=== PERFORMANCE TEST MODE ===\n");
  test_filter_performance("Grayscale", ip_grayscale);
  test_filter_performance("Black & White", ip_blackwhite);
  test_filter_performance("Invert", ip_invert);
  test_filter_performance("Mirror", ip_mirror);
  test_filter_performance("Blur 3x3", ip_blur3x3);
  test_filter_performance("Sharpen 3x3", ip_sharpen3x3);
  test_filter_performance("Sobel", ip_sobel);
  print("=== TESTS COMPLETE ===\n");
  print("Now entering normal UI mode...\n");

  ui_draw_initial(); // Return to main menu
#endif

  // Normal UI operation (always runs)
  while (1) {
    process_ui_events();
    delay(1000);
  }
}
