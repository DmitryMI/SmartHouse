/*
 * BlankFirmware.c
 *
 * Created: 23.08.2019 11:41:35
 * Author : Dmitry
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    /* Replace with your application code */
	DDRB |= 1 << PB0;
    while (1) 
    {
		PORTB |= 1 << PB0;
		_delay_ms(5);
		PORTB &= ~(1 << PB0);
		_delay_ms(1000);
    }
}

