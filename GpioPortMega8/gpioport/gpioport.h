/*
 * gpioport.h
 *
 * Created: 23.08.2019 16:08:37
 *  Author: Dmitry
 */ 


#ifndef GPIOPORT_H_
#define GPIOPORT_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#include "gpioport_mega8.h"
#include "gpioport_comm.h"
#include "gpioport_types.h"
#include "gpioport_errs.h"

void gpioport_setup(result_callback callback)
{
	gpioport_begin(callback);
	sei();	
}


#endif /* GPIOPORT_H_ */