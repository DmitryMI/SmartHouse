/*
 * gpioport_mega8.c
 *
 * Created: 23.08.2019 15:08:09
 *  Author: Dmitry
 */ 

#include "gpioport_mega8.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define SCL_PIN GPIOPORT_SCL_PIN
#define SDA_PIN GPIOPORT_SDA_PIN

#ifndef GPIOPORT_SCL_PIN
#define GPIOPORT_SCL_PIN 2
#endif

#ifndef GPIOPORT_SDA_PIN
#define GPIOPORT_SDA_PIN 3
#endif

#ifndef GPIOPORT_SCL_PORT
#define GPIOPORT_SCL_PORT D
#endif

#ifndef GPIOPORT_SCL_PORT
#define GPIOPORT_SDA_PORT D
#endif

//#define SCL_PORT OUTPORT(GPIOPORT_SCL_PORT)
//#define SDA_PORT OUTPORT(GPIOPORT_SDA_PORT)
//#define SDA_INPUT INPORT(GPIOPORT_SDA_PORT)

#define CONC(a, b) a ## b

#define SCL_PORT PORT ## D
#define SDA_PORT PORT ## D
#define SDA_INPUT PIN ## D

#define SCL_DDR DDR ## D
#define SDA_DDR  DDR ## D


static gpioport* current_gport;


void gpioport_reset()
{
	current_gport->index_bit = 0;
	current_gport->index_byte = 0;
	
	for(int i = 0; i < GPIO_FRAME_DATA_SIZE; i++)
	{
		current_gport->data_buffer[i] = 0;
	}
}


void gpioport_int_handler()
{
	int bit = (SDA_INPUT | (1 << SDA_PIN));
	
	if(bit)
	{
		current_gport->data_buffer[current_gport->index_byte] |= (1 << current_gport->index_bit);
	}
	else
	{
		current_gport->data_buffer[current_gport->index_byte] &= ~(1 << current_gport->index_bit);
	}
	
	current_gport->index_bit++;
	
	if(current_gport->index_bit == 8)
	{
		current_gport->index_byte++;
		current_gport->index_bit = 0;
	}
	
	
	if(current_gport->index_byte == sizeof(gpio_frame))
	{
		gpio_frame* received_frame = (gpio_frame*)(current_gport->data_buffer);
		current_gport->callback(received_frame);
		gpioport_reset(current_gport);
	}
}

void gpioport_interrupt(int enabled)
{
	// Setting up INT0
	SCL_DDR &= (1 << SCL_PIN);
	SDA_DDR &= (1 << SDA_PIN);
	
	if(enabled)
	{		
		GICR |= (1 << INT0);
		MCUCR |= (1 << ISC01);
	}
	else
	{
		GICR &= ~(1 << INT0);
	}
}

void gpioport_begin(result_callback callback)
{	
	gpioport_reset();	
	current_gport->callback = callback;
	gpioport_interrupt(1);
}

