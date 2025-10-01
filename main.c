/* main.c

   This file written 2024 by Artur Podobas and Pedro Antunes

   For copyright and licensing, see file COPYING */


/* Below functions are external and found in other files. */

#include "dtekv-lib.h"
#include "background.h"
#include "vga.h"


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

int mytime = 0x235957;
char textstring[] = "text, more text, and even more text!";
int timecount = 0;
int prime = 1234567;

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
// WORKS
void set_leds(int led_mask) {
  volatile int *leds = (volatile int *)0x04000000; // address of the LED's
  *leds = led_mask & 0x3FF; // 0x3FF = 001111111111, only care about the 10 lsb
}
/* Function to increment the 4 first LED's */
// WORKS
void increment_leds() {
  int led_value = 0;
  set_leds(led_value); 

  while (led_value < 0xF) { // run until led_value = 1111 (0xF)
    delay (1000); // change value to get accurate seconds
    led_value++;
    set_leds(led_value);
  }
}
/* Function to set a value to a desired display */
// DOES NOT WORK, error probably in row 42 how bits are treated
// 7segment works on passive high logic, meaning 0 = active
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
  volatile unsigned short *TMR1_STATUS = (unsigned short*) 0x04000020;

  if(*TMR1_STATUS & 0x1){
    *TMR1_STATUS = 0x0;
    time2string( textstring, mytime ); // Converts mytime to string

    set_displays(0, (int)textstring[7] - '0'); // ones of seconds
    set_displays(1, (int)textstring[6] - '0'); // tens of seconds
    set_displays(2, (int)textstring[4] - '0'); // ones of minutes
    set_displays(3, (int)textstring[3] - '0'); // tens of minutes
    set_displays(4, (int)textstring[1] - '0'); // ones of hours
    set_displays(5, (int)textstring[0] - '0'); // tens of hours

    display_string( textstring ); //Print out the string 'textstring'
    timecount++;
    if (timecount >= 10){
        tick( &mytime );     // Ticks the clock once
        timecount = 0;
      }
  }
}

/* Add your code here for initializing interrupts. */
void labinit(void)
{
  //initialize control register
  volatile unsigned short *TMR1_CONTROL = (unsigned short*) 0x04000024;

  // intitialize the time period to be 100 ms 
  // that gives us a period of 1/1000
  volatile unsigned short *TMR1_PERLO = (unsigned short*)  0x04000028;
  volatile unsigned short *TMR1_PERHI = (unsigned short*)  0x0400002c;


  // 10 timeout per sec
  // clock rate of 30 000 000 clock cycles per second means 30 MHz
  // but we want 10 Hz

  unsigned int period = (30000000/10) - 1; // we do minus 1 bcz the timers actual period is one cycle greater than that store in the registers, assumes we start from 0..., not 1...

  *(TMR1_PERLO) = period & 0xFFFF;
  *(TMR1_PERHI) = period >> 16;                          
  
  
  /*
    set control bits to do the following:
    - Start timer (bit 0 = 1)
    - Be continuous. allows timer to reload automatically after reach zero
    - Enable ITO (bit 2 = 1), this enables timeout flags which we want!   
  */
  *TMR1_CONTROL = 0x7;
  // *TMR1_CONTROL = 0x5; without continuous

  enable_interrupt();

}

int main(void) {
    vga_show_background();
}