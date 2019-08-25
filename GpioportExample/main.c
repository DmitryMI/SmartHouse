/*
 * GpioportExample.c
 *
 * Created: 25.08.2019 17:41:26
 * Author : Dmitry
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "../Gpioport/gpioport.h"


int main(void)
{
	//DDRD |= (1 << PD2);
	//PORTD |= (1 << PD2);
	
	gpioport_init();
	
    /* Replace with your application code */
    while (1) 
    {
    }
}