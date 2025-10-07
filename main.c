// main.c

#include "dtekv-lib.h"
#include "vga.h"
#include "image_processing.h"
#include "background.h"
#include "ui_state.h"
#include "performance_analysis.h"

extern void print(const char*);
extern void printc(char);
extern void print_dec(unsigned int);
extern void print_hex32(unsigned int x);
extern void display_string(char*);
extern void time2string(char*,int);
extern void enable_interrupt();
extern void tick(int*);
extern void delay(int);
extern int nextprime( int );

int mytime = 0x000000; // initial time: 00:00:00
char textstring[] = "text, more text, and even more text!";
int timecount = 0;

const int segment_map[10] = {
  0x3F, // 0 00111111
  0x06, // 1 00000110
  0x5B, // 2 01011011
  0x4F, // 3 01001111
  0x66, // 4 01100110
  0x6D, // 5 01101101
  0x7D, // 6 01111101
  0x07, // 7 00000111
  0x7F, // 8 01111111
  0x6F, // 9 01101111
};

/* Function to turn LED's on or off*/
void set_leds(int led_mask) {
  volatile int *leds = (volatile int *)0x04000000; // address of the LED's
  *leds = led_mask & 0x3FF; // 0x3FF = 001111111111, only care about the 10 lsb
}

/* Function to set a value to a desired display */
void set_displays (int display_number, int value) {
  volatile int *display = (volatile int *)0x04000050 + (display_number * 0x4);
  *display = ~segment_map[value] & 0xFF; // 0xFF = 11111111, 7 lsb
}

/* Function to get value of switches */
int get_sw(void) {
  volatile int *switches = (volatile int *)0x04000010;
  return *switches & 0x3FF; // 0x3FF = 001111111111, 10 lsb
}

/* Function to get value of button (KEY 1) */
int get_btn (void) {
  volatile int *button = (volatile int *)0x040000d0;
  return *button & 0x1; // 0x1 = 0001, 1 lsb
}

/* Below is the function that will be called when an interrupt is triggered. */
void handle_interrupt(unsigned cause) 
{  
  handle_interrupt_ui(cause);
}

void *memcpy(void *dest, const void *src, unsigned int n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (unsigned int i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

extern volatile unsigned char * const BUF0;
extern volatile unsigned char * const BUF1;
extern volatile unsigned int  * const VGA_CTRL_PTR;

/* Normal launch
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
    delay(100);
  }
}