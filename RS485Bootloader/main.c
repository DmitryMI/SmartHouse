/*
 * RS485Bootloader.c
 *
 * Created: 12.10.2019 16:32:01
 * Author : Dmitry
 */ 

#include <avr/io.h>

#define F_CPU	8000000UL
#define BAUD	9600UL

#include <util/delay.h>
#include <avr/boot.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "rs485_commands.h"

// ***CONFIGURABLE***
#define BOOT_VERS	1

#define LED_PORT	PORTC
#define LED_INP		PINC
#define LED_DDR		DDRC
#define LED_PIN		PC5

#ifndef RS485_SID
# warning "RS485_SID must be defined and unique"
#define RS485_SID 0x03
#endif
// ******************

#define PAGE_SIZE_BYTES SPM_PAGESIZE

uint32_t prog_page_address = 0;
uint8_t prog_byte_address = 0;

void inline exit_bootloader()
{
	MCUCR = (1 << IVCE);
	MCUCR = 0;
	
	#if defined (__AVR_ATmega328p__)
	asm("jmp 0x0000");
	#elif defined (__AVR_ATmega8__)
	asm("rjmp app_start");
	#endif
}

void inline reset_exit_timer()
{
	#if defined (__AVR_ATmega328P__)
	wdt_reset();
	#elif defined (__AVR_ATmega8__)
	TCNT1 = 0;
	#endif
}

ISR(TIMER1_OVF_vect)
{
	exit_bootloader();
}

void inline program_handle(uint8_t *package)
{
	uint8_t sreg = SREG;
	cli();
	
	uint16_t w1 = package[0];
	w1 += package[1] << 8;	
	uint16_t w2 = package[2];
	w2 += package[3] << 8;
	
	boot_page_fill(prog_page_address + prog_byte_address, w1);
	prog_byte_address += 2;
	boot_page_fill(prog_page_address + prog_byte_address, w2);
	prog_byte_address += 2;
	
	if(prog_byte_address == PAGE_SIZE_BYTES)
	{
		prog_byte_address = 0;
		
		eeprom_busy_wait();
		boot_page_erase(prog_page_address);
		boot_spm_busy_wait();	// Wait until the memory is erased.
		boot_page_write(prog_page_address);	// Store buffer in flash page.
		boot_spm_busy_wait();	// Wait until the memory is written.
		boot_rww_enable();
		
		prog_page_address += PAGE_SIZE_BYTES;
	}
	
	SREG = sreg;
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
	reset_exit_timer();
	return UDR0;
#elif defined (__AVR_ATmega8__)
	while ( !(UCSRA & (1<<RXC)) );
	reset_exit_timer();
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
	// rs485-data received
	//wdt_reset();
	reset_exit_timer();
	
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
	
	//uint8_t *data = package + CAN_OFFSET_DATA;
	
	uint8_t response[8] = {0};
		
	int mustRespond = 1;

	
	response[CAN_OFFSET_ADDRH - CAN_PAYLOAD_OFFSET] = sid >> 8;
	response[CAN_OFFSET_ADDRL - CAN_PAYLOAD_OFFSET] = sid;
	
	if(comdh == CAN_CMD_WHOIS)
	{
		response[CAN_OFFSET_COMDH - CAN_PAYLOAD_OFFSET] = CAN_CMD_ACK;
		response[CAN_OFFSET_COMDL - CAN_PAYLOAD_OFFSET] = BOOT_VERS;
	}
	else if(comdh == CAN_CMD_PROG_INFOR)
	{
		response[CAN_OFFSET_COMDH - CAN_PAYLOAD_OFFSET] = CAN_CMD_ACK;
		response[CAN_OFFSET_COMDL - CAN_PAYLOAD_OFFSET] = 0;
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = PAGE_SIZE_BYTES;
		response[CAN_OFFSET_DATA + 1  - CAN_PAYLOAD_OFFSET] = 0;
		response[CAN_OFFSET_DATA + 2  - CAN_PAYLOAD_OFFSET] = 0;
		
	}
	else if(comdh == CAN_CMD_RESET)
	{
		mustRespond = 0;
		
	}
	else if(comdh == CAN_CMD_PROG_FLASH)
	{
		int page_address =  PAGE_SIZE_BYTES * comdl;
		prog_page_address = page_address;
		program_handle(package + CAN_OFFSET_DATA);	
		
		if(prog_page_address == page_address)	
		{
			mustRespond = 0;
		}
		else
		{			
			response[CAN_OFFSET_COMDH  - CAN_PAYLOAD_OFFSET] = CAN_CMD_ACK;
			response[CAN_OFFSET_COMDL  - CAN_PAYLOAD_OFFSET] = 0;
		}

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

void init_exit_timer()
{
#if defined (__AVR_ATmega328P__)
	// Using WDT as 2.0 seconds timer causing interrupt
	cli();
	wdt_reset();
	WDTCSR = ((1<<WDCE) | (1<<WDE));
	WDTCSR = ((1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0)) & ~(1 << WDE);
	sei();
#elif defined (__AVR_ATmega8__)
	TIMSK |= (1 << TOIE1);
	sei();
	//enable interrupts
	//TCCR1B |= (1 << CS11) | (1 << CS10);
	TCCR1B |= (1 << CS12);
	// set prescaler to 256 and start the timer

#endif

}


int main(void)
{
	// Setting position of reset vectors table
	MCUCR = (1 << IVCE);
	MCUCR = (1 << IVSEL);
	
	// Disabling WDT from resetting the system
	MCUSR = 0;
	wdt_disable();
	
	// Initializing CAN
	rs485_init();
	
	init_exit_timer();
	
	LED_DDR |= (1 << LED_PIN);	
	LED_PORT |= (1 << LED_PIN);

	
    while (1) 
    {						
		load_tx();		
    }
}

