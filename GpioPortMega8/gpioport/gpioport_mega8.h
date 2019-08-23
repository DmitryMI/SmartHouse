/*
 * gpioport_mega8.h
 *
 * Created: 23.08.2019 14:49:16
 *  Author: Dmitry
 */ 


#ifndef GPIOPORT_MEGA8_H_
#define GPIOPORT_MEGA8_H_

#include "gpioport_types.h"
#include "utils.h"


void gpioport_interrupt(int enabled);

void gpioport_begin(result_callback callback);

void gpioport_int_handler();
void gpioport_timer_handler();
void gpioport_set_callback();

#endif /* GPIOPORT_MEGA8_H_ */