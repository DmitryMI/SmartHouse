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

void ULink_set_reset_request_handler(request_t handler)
{
	reset_request_handler = handler;
}

void ULink_set_prog_request_handler(request_t handler)
{
	prog_request_handler = handler;
}

void ULink_init()
{
	unsigned int ubrr = F_CPU / 16 / BAUD - 1;
	
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
	
	// Enabling interrupt
	sei();
	
	UCSR0B |= (1 << RXCIE0);
}

void ULink_send_char(char singleChar)
{
	while(!(UCSR0A & (1 << UDRE0)))
	{
	}
	UDR0 = singleChar;
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
	
	if(ch == 'A')
	{
		ULink_send_command(ACK"\n");
	}
	
	if(ch == 'V')
	{
		ULink_send_command(FIRM_VERS"\n");
	}

	if (ch == 'R')
	{
		if(reset_request_handler != NULL)
		{
			reset_request_handler();
		}		
	}
	
	if(ch == 'P')
	{
		if(prog_request_handler != NULL)
		{
			prog_request_handler();
		}
	}
}

char ULink_receive()
{
	while ( !(UCSR0A & (1<<RXC0)) );
		
	return UDR0;
}

void ULink_received_handler()
{
	char data = UDR0;

	resolveUartCommand(data);
}

ISR(USART_RX_vect)
{
	ULink_received_handler();
}
