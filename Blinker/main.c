/*
 * Blinker.c
 *
 * Created: 10.11.2019 18:04:10
 * Author : Dmitry
 */ 

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>


int main(void)
{
	DDRC |= (1 << PC5);	

    while (1) 
    {
		PORTC |= (1 << PC5);
		_delay_ms(100);
		PORTC &= ~(1 << PC5);
		_delay_ms(50);
    }
}

