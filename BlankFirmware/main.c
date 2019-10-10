/*
 * BlankFirmware.c
 *
 * Created: 23.08.2019 11:41:35
 * Author : Dmitry
 */ 

#define F_CPU 8000000UL

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "UartLink.h"


#define LED_PORT		PORTC
#define LED_PIN			PC5
#define LED_DDR			DDRC

#define RELAY_PORT		PORTC
#define RELAY_PIN		PC4
#define RELAY_DDR		DDRC

//void do_blink (void) __attribute__ ((section ("at2000.text")));

void do_blink()
{
	LED_PORT |= (1 << LED_PIN);
	_delay_ms(100);
	LED_PORT &= ~(1 << LED_PIN);
	_delay_ms(100);
}

void turn_power(int on)
{
	RELAY_DDR |= (1 << RELAY_PIN);
	
	if(on)
	{
		RELAY_PORT |= (1 << RELAY_PIN);
	}
	else
	{
		RELAY_PORT &= ~(1 << RELAY_PIN);
	}
	
	//_delay_ms(400);
}

void reset_handler()
{
	do                          
	{                           
		wdt_enable(WDTO_500MS);  
		for(;;)                 
		{                       
		}                       
	} while(0);
}


int main(void)
{
	MCUSR = 0;
	wdt_disable();
	
	LED_DDR |= (1 << LED_PIN);
	LED_PORT |= (1 << LED_PIN);
	_delay_ms(1000);
	LED_PORT &= ~(1 << LED_PIN);
	
	//turn_power(1);
	
	ULink_set_reset_request_handler(reset_handler);
	ULink_init();	
		
	uint8_t tickCounter = 0;

	while (1)
	{
		tickCounter++;
		LED_PORT |= (1 << LED_PIN);
		//turn_power(1);
		_delay_ms(500);
		LED_PORT &= ~(1 << LED_PIN);
		turn_power(0);
		_delay_ms(500);
		
		do_blink();		
	}
}
