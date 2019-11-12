/*
 * UARTLink.c
 *
 * Created: 09.10.2019 15:13:44
 * Author : Dmitry
 */ 

#include "UartLink.h"

#ifndef F_CPU
#define F_CPU 8000000UL 
#endif


#define BAUD 9600

#include <avr/wdt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>

request_t reset_request_handler = NULL;
request_t prog_request_handler = NULL;
received_callback_t ulink_received_handler = NULL;

void ULink_set_reset_request_handler(request_t handler)
{
	reset_request_handler = handler;
}

void ULink_set_received_handler(received_callback_t handler)
{
	ulink_received_handler = handler;
}

void ULink_set_prog_request_handler(request_t handler)
{
	prog_request_handler = handler;
}

void ULink_init()
{
	unsigned int ubrr = F_CPU / 16 / BAUD - 1;
	
#if defined (__AVR_ATmega328p__)
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
	
	UCSR0B |= (1 << RXCIE0);
	
#elif defined (__AVR_ATmega8__)
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
	
	UCSRB |= (1 << RXCIE);	
#else
# warning Device is not supported
#endif
	
	// Enabling interrupt
	sei();
	
}

void ULink_send_char(char singleChar)
{
	while(!(UCSRXA & (1 << UDREX)))
	{
	}
	UDRX = singleChar;
}

void ULink_send(char* str)
{
	for(int i = 0; str[i] != '\0'; i++)
	{
		ULink_send_char(str[i]);
	}	
}

void ULink_send_command(char* message)
{	
	ULink_send("UC_");
	ULink_send(message);
}

void ULink_send_info(char* message)
{
	ULink_send("UI_");
	ULink_send(message);
}

void resolveUartCommand(char ch)
{
	if(ch == 'N')
	{
		ULink_send_command(DEV_NAME"\n");
	}	
	else if(ch == 'A')
	{
		ULink_send_command(ACK"\n");
	}
	
	else if(ch == 'V')
	{
		ULink_send_command(FIRM_VERS"\n");
	}
	else if (ch == 'R')
	{
		if(reset_request_handler != NULL)
		{
			reset_request_handler();
		}		
	}	
	else if(ch == 'P')
	{
		if(prog_request_handler != NULL)
		{
			prog_request_handler();
		}
	}
	else
	{
		if(ulink_received_handler != NULL)
		{
			ulink_received_handler(ch);
		}
	}
}

char ULink_receive()
{
	while ( !(UCSRXA & (1<<RXCX)) );
		
	return UDRX;
}

void ULink_received_handler()
{
	char data = UDRX;

	resolveUartCommand(data);
}

#if defined (__AVR_ATmega328p__)

ISR(USART_RX_vect)
{
	//ULink_send_info("RX interrupt!\n");
	ULink_received_handler();
}

#elif defined (__AVR_ATmega8__)

ISR(USART_RXC_vect)
{
	//ULink_send_info("RX interrupt!\n");
	ULink_received_handler();
}

#else
# warning Device is not supported
#endif