/*
 * gpioport.c
 *
 * Created: 25.08.2019 17:06:08
 *  Author: Dmitry
 */ 

#include "gpioport.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#define TIMER_CUSTOM_PRESCALER 0

typedef enum state_enum
{
	IDLE, RECEIVING, SENDING
} state_t;


static uint8_t output_frame_buffer[sizeof(gpio_frame_t)];
static uint8_t input_frame_buffer[sizeof(gpio_frame_t)];

static int pos_bit;
static int pos_byte;
static gpio_callback_t recieve_callback;
static state_t state;



void gpioport_set_callback(gpio_callback_t callback)
{
	recieve_callback = callback;
}

void set_int0(int enabled)
{
	if(enabled)
	{
		GICR |= (1 << INT0);
	}
	else
	{
		GICR &= (1 << INT0);
	}
}

void set_int1(int enabled)
{
	if(enabled)
	{
		GICR |= (1 << INT1);
	}
	else
	{
		GICR &= (1 << INT1);
	}
}


void gpioport_init()
{
	sei();
	
	// Resetting all pins to input
	SDA_DDR &= ~(1 << SDA_PIN);
	SCL_DDR &= ~(1 << SCL_PIN);
	
	
	// Enabling timer0
	// Setting prescaler	
	//TCCR0 = (1 << CS00);					// No prescaler
	//TCCR0 = (1 << CS01);					// 8
	//TCCR0 = (1 << CS01) | (1 << CS00);		// 64
	TCCR0 = (1 << CS02);					// 256	
	//TCCR0 = (1 << CS02) | (1 << CS00);	// 1024
	
	// Enabling Overflow interrupt of Timer0
	TIMSK = (1 << TOIE0);
	
	
	// Enabling INT0 on falling edge
	MCUSR = (1 << ISC01)	
	set_int0(1);
}


void gpioport_send_raw(gpio_frame_t frame)
{
	
}

void gpioport_send(uint8_t dst, uint8_t cmd, uint8_t data[GPIO_FRAME_DATA_SIZE])
{
	
}



ISR(TIMER0_OVF_vect)	// Timer interrupt
{
	TCNT0 = TIMER_CUSTOM_PRESCALER;
	
	
}

ISR(INT0_vect)			// SCL interrupt
{
	
}
