#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void timer_set_period(uint32_t period);
void timer_start(void);
void timer_stop(void);
uint32_t timer_get_status(void);

#endif // TIMER_H
