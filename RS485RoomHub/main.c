/*
 * RS485RoomHub.c
 *
 * Created: 10.11.2019 12:40:58
 * Author : Dmitry
 */ 

#ifndef RS485_SID
# warning "RS485_SID must be defined and unique"
#define RS485_SID 0x02
#endif

#define F_CPU	8000000UL
#define BAUD	9600UL
#define ROM_VERS	2


#include <avr/io.h>
#include <util/delay.h>
#include <avr/boot.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "rs485_commands.h"

#define LED_PORT	PORTC
#define LED_INP		PINC
#define LED_DDR		DDRC
#define LED_PIN		PC5

#define DEVICE1_DDR	DDRC
#define DEVICE2_DDR	DDRC
#define DEVICE3_DDR	DDRC

#define DEVICE1_PORT	PORTC
#define DEVICE2_PORT	PORTC
#define DEVICE3_PORT	PORTC

#define DEVICE1_INDX	PC1
#define DEVICE2_INDX	PC2
#define DEVICE3_INDX	PC3

void reset_handler()
{
	cli();
	do
	{
		wdt_enable(WDTO_500MS);
		for(;;)
		{
		}
	} while(0);
}


void inline rs485_init()
{
	unsigned int ubrr = F_CPU / 16 / BAUD - 1;
	
	/*Set baud rate */
	#if defined (__AVR_ATmega328P__)
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
	
	#elif defined (__AVR_ATmega8__)
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
	#endif
}

void uart_putc(uint8_t b)
{
	#if defined (__AVR_ATmega328P__)

	while(!(UCSR0A & (1 << UDRE0)))
	{
	}
	UDR0 = b;
	
	#elif defined (__AVR_ATmega8__)
	while(!(UCSRA & (1 << UDRE)))
	{
	}
	UDR = b;
	#endif
}

uint8_t uart_getc()
{
	#if defined (__AVR_ATmega328P__)
	while ( !(UCSR0A & (1<<RXC0)) );
	return UDR0;
	#elif defined (__AVR_ATmega8__)
	while ( !(UCSRA & (1<<RXC)) );
	return UDR;
	#endif
}

int inline rs485_readrxb(uint8_t rxb_mask, uint8_t *data, int data_length)
{
	for(int i = 0; i < data_length; i++)
	{
		data[i] = uart_getc();
		//LED_INP |= (1 << LED_PIN);
		LED_PORT ^= (1 << LED_PIN);
	}
	
	return data_length;
}

void handle_hub_code(uint8_t cmd, uint8_t hub_code, uint8_t *response)
{
	if(cmd == ROOM_CMD_ACTIVATE)
	{
		switch(hub_code)
		{
			case 1:
			DEVICE1_PORT |= (1 << DEVICE1_INDX);
			break;
			case 2:
			DEVICE2_PORT |= (1 << DEVICE2_INDX);
			break;
			case 3:
			DEVICE3_PORT |= (1 << DEVICE3_INDX);
			break;
		}
	}
	else if(cmd == ROOM_CMD_DEACTIVATE)
	{
		switch(hub_code)
		{
			case 1:
			DEVICE1_PORT &= ~(1 << DEVICE1_INDX);
			break;
			case 2:
			DEVICE2_PORT &= ~(1 << DEVICE2_INDX);
			break;
			case 3:
			DEVICE3_PORT &= ~(1 << DEVICE3_INDX);
			break;
		}
	}
	
	switch(hub_code)
	{
		case 1:
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = DEVICE1_PORT & (1 << DEVICE1_INDX);
		break;
		case 2:
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = DEVICE2_PORT & (1 << DEVICE2_INDX);
		break;
		case 3:
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = DEVICE2_PORT & (1 << DEVICE2_INDX);
		break;
	}
	
}

void inline rs485_load_tx0_buffer(uint16_t sid, uint8_t *data, int data_length)
{
	uint8_t sidh = sid >> 8;
	uint8_t sidl = sid;
	
	uart_putc(sidh);
	uart_putc(sidl);
	
	// Sending bytes
	for(int i = 0; i < data_length; i++)
	{
		uart_putc(data[i]);
	}
}

void inline load_tx()
{
	
	// Reading data from rs485-controller
	const int package_length = 10;
	uint8_t package[package_length];
	rs485_readrxb(0, package, package_length);
	uint16_t sid = 0;
	sid += package[0];
	sid = sid << 8;
	sid += package[1];
	//uint8_t dlc = package[4];
	
	uint8_t addrh = package[CAN_OFFSET_ADDRH];
	uint8_t addrl = package[CAN_OFFSET_ADDRL];
	uint16_t addr = addrh;
	addr = addr << 8;
	addr += addrl;
	
	// BROADCAST MESSAGES ARE NOT ALLOWED IN RS-485!
	if(addr != RS485_SID)
	{
		return;
	}
	
	uint8_t comdh = package[CAN_OFFSET_COMDH];
	uint8_t comdl = package[CAN_OFFSET_COMDL];

	
	uint8_t response[8] = {0};
	
	int mustRespond = 1;

	
	response[CAN_OFFSET_ADDRH - CAN_PAYLOAD_OFFSET] = sid >> 8;
	response[CAN_OFFSET_ADDRL - CAN_PAYLOAD_OFFSET] = sid;
	
	if(comdh == CAN_CMD_WHOIS)
	{
		response[CAN_OFFSET_COMDH - CAN_PAYLOAD_OFFSET] = CAN_CMD_ACK;
		response[CAN_OFFSET_COMDL - CAN_PAYLOAD_OFFSET] = ROM_VERS;
	}
	else if(comdh == CAN_CMD_PROG_INFOR)
	{
		response[CAN_OFFSET_COMDH - CAN_PAYLOAD_OFFSET] = CAN_CMD_ACK;
		response[CAN_OFFSET_COMDL - CAN_PAYLOAD_OFFSET] = 0;
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = 64;
		response[CAN_OFFSET_DATA + 1  - CAN_PAYLOAD_OFFSET] = 0;
		response[CAN_OFFSET_DATA + 2  - CAN_PAYLOAD_OFFSET] = 0;
		
	}
	else if(comdh == CAN_CMD_RESET)
	{
		reset_handler();		
	}
	else if(comdh == CAN_CMD_PROG_FLASH)
	{
		response[CAN_OFFSET_COMDH  - CAN_PAYLOAD_OFFSET] = CAN_CMD_UNSUPPORTED;
	}
	else if(comdh == ROOM_CMD_ACTIVATE || comdh == ROOM_CMD_DEACTIVATE || comdh == ROOM_CMD_GET_STATE)
	{
		handle_hub_code(comdh, comdl, response);
	}
	else
	{
		response[CAN_OFFSET_COMDH  - CAN_PAYLOAD_OFFSET] = CAN_CMD_UNSUPPORTED;
	}
	
	if(mustRespond)
	{
		rs485_load_tx0_buffer(RS485_SID, response, 8);
	}
}




int main(void)
{
	MCUSR = 0;
	wdt_disable();
    
    // Initializing CAN
    rs485_init();   

    
    LED_DDR |= (1 << LED_PIN);
    LED_PORT |= (1 << LED_PIN);

	DEVICE1_DDR |= (1 << DEVICE1_INDX);
	DEVICE2_DDR |= (1 << DEVICE2_INDX);
	DEVICE3_DDR |= (1 << DEVICE3_INDX);
    
    while (1)
    {
	    load_tx();
    }
}

