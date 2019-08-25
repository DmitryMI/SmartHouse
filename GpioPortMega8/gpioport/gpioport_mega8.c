/*
 * gpioport_mega8.c
 *
 * Created: 23.08.2019 15:08:09
 *  Author: Dmitry
 */ 

#include <stddef.h>
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
	PORTD |= 1 << PD4;
	current_gport->index_bit = 0;
	current_gport->index_byte = 0;
	
	for(int i = 0; i < GPIO_FRAME_DATA_SIZE; i++)
	{
		current_gport->data_buffer[i] = 0;
	}
}

void on_receiving_timeout()
{
	current_gport->state = IDLE;
	current_gport->timeout_timer.action = NULL;
	gpioport_reset(current_gport);
}


void gpioport_int_handler()
{
	current_gport->state = RECEIVING;
	current_gport->timeout_timer.action = on_receiving_timeout;
	current_gport->timeout_timer.nseconds_init = RECEIVING_TIMEOUT_NSECS;
	current_gport->timeout_timer.nseconds_left = RECEIVING_TIMEOUT_NSECS;
	
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

void gpioport_timer_handler()
{
	int nseconds = current_gport->timeout_timer.nseconds_pertick;
	current_gport->timeout_timer.nseconds_left -= nseconds;
	if(current_gport->timeout_timer.nseconds_left <= 0)
	{
		current_gport->timeout_timer.nseconds_left = 0;
		timeout_action_t action = current_gport->timeout_timer.action;
		if(action != NULL)
		{
			action();
		}		
	}
}

void gpioport_int_enable(int enabled)
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

void gpioport_set_callback(result_callback_t callback)
{
	current_gport->callback = callback;
}

void gpioport_begin(gpioport* gport, result_callback_t callback)
{		
	current_gport = gport;
	gpioport_reset();	
	current_gport->callback = callback;
	gpioport_int_enable(1);
}

void gpio_send_next()
{
	if(current_gport->state == SENDING_HIGH)
	{		
		current_gport->state = SENDING_LOW;
		
		byte cur_byte = current_gport->data_buffer[current_gport->index_byte];
		int bit = cur_byte & 1;
		if(bit)
		{
			SDA_PORT |= (1 << SDA_PIN);
		}
		else
		{
			SDA_PORT &= (1 << SCL_PIN);
		}
		
		SCL_PORT &= ~(1 << SCL_PIN);
		
		cur_byte = cur_byte >> 1;
		current_gport->index_bit++;
		if(current_gport->index_bit == 8)
		{
			current_gport->index_bit = 0;
			current_gport->index_byte++;
			if(current_gport->index_byte == sizeof(gpio_frame))
			{
				// End of transmittion
				current_gport->state = SENDING_LAST;
			}
		}	
		
	}
	else if(current_gport->state == SENDING_LOW)
	{
		current_gport->state = SENDING_HIGH;
		SCL_PORT |= (1 << SCL_PIN);
	}
	else if(current_gport->state == SENDING_LAST)
	{
		gpioport_reset(current_gport);
		current_gport->timeout_timer.action = NULL;
		current_gport->state = IDLE;
		gpioport_int_enable(1);
		SCL_PORT |= (1 << SCL_PIN);
		return;
	}
	
	current_gport->timeout_timer.nseconds_left = current_gport->timeout_timer.nseconds_init;
}


void gpioport_send(gpio_frame *frame)
{
	while(current_gport->state != IDLE)
	{
		// Waiting while state becomes IDLE
	}
	
	current_gport->state = SENDING_HIGH;
	
	//cli();
	gpioport_int_enable(0);
	
	//int ddr_sda_backup = SDA_DDR;
	//int ddr_scl_backup = SCL_DDR;
	
	SDA_DDR |= (1 << SDA_PIN);
	SCL_DDR |= (1 << SCL_PIN);
	
	current_gport->data_buffer[0] = frame->id_src;
	current_gport->data_buffer[1] = frame->id_dst;
	current_gport->data_buffer[2] = frame->cmd;
	for(int i = 0; i < GPIO_FRAME_DATA_SIZE; i++)
	{
		current_gport->data_buffer[i + 3] = frame->data[i];
	}
	current_gport->data_buffer[GPIO_FRAME_DATA_SIZE + 3] = frame->crc;
	
	current_gport->index_bit = 0;
	current_gport->index_byte = 0;
	
	current_gport->timeout_timer.action = gpio_send_next;
	current_gport->timeout_timer.nseconds_init = 50000;
	//current_gport->timeout_timer.nseconds_left = 50000;	
}

