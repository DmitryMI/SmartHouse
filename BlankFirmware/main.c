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
#include <avr/sleep.h>
#include <avr/interrupt.h>

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

void enter_powerdown_mode()
{
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();
	sleep_enable();
	sei();
	sleep_cpu();
	sleep_disable();
	sei();
}

void enter_idle_mode()
{
	set_sleep_mode(SLEEP_MODE_IDLE);
	cli();
	sleep_enable();
	sei();
	sleep_cpu();
	sleep_disable();
	sei();
}

void enable_power_reduction()
{
	ADCSRA &= ~(ADEN);	// Turning off ADC
	
	PRR = 
	(1 << PRTWI)	|	// Disabling IIC
	(1 << PRTIM2)	|	// Disabling Timer2
	(1 << PRTIM0)	|	// Disabling Timer0
	(1 << PRTIM1)	|	// Disabling Timer1
	(1 << PRSPI)	|	// Disabling SPI
	(1 << PRADC);		// Disabling ADC
}

void received_handler(char ch)
{
	if(ch == 'S')
	{
		ULink_send_info("Entering sleep mode...\n");
		enter_idle_mode();	
		ULink_send_info("Waking up from sleep mode...\n");	
	}
}


int main(void)
{
	MCUSR = 0;
	wdt_disable();
	
	enable_power_reduction();
	
	ULink_set_received_handler(received_handler);
	ULink_set_reset_request_handler(reset_handler);
	ULink_init();	
		
	uint8_t tickCounter = 0;

	while (1)
	{
		tickCounter++;
		//LED_PORT |= (1 << LED_PIN);
		
		asm("SBI 8, 5");

		_delay_ms(500);
		LED_PORT &= ~(1 << LED_PIN);
		turn_power(0);
		_delay_ms(500);
		
		do_blink();		
	}
}
