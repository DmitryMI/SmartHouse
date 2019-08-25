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

#define RECEIVING_TIMEOUT_NSECS 300000


void gpioport_int_enable(int enabled);

void gpioport_begin(gpioport* gport, result_callback_t callback);

void gpioport_int_handler();
void gpioport_timer_handler();
void gpioport_set_callback(result_callback_t callback);

void gpioport_send(gpio_frame *frame);

#endif /* GPIOPORT_MEGA8_H_ */