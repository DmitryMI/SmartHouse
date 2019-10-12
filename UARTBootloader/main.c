/*
 * UARTBootloader.c
 *
 * Created: 08.10.2019 16:48:49
 * Author : Dmitry
 */ 

#include <avr/io.h>

#define F_CPU 8000000UL
#include "DeviceConfig.h"

#include <util/delay.h>
#include <avr/boot.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#define PAGE_SIZE_BYTES SPM_PAGESIZE

uint32_t prog_page_address = 0;

void program_page(uint32_t page_byte_addr, uint8_t *page_data)
{
	//cli();		// Disable interrupts.
	uint8_t sreg = SREG;
	eeprom_busy_wait();
	boot_page_erase(page_byte_addr);
	boot_spm_busy_wait();	// Wait until the memory is erased.

	for (int i = 0; i < PAGE_SIZE_BYTES; i += 2)
	{
		// Set up little-endian word.
		uint16_t w = *page_data++;
		w += (*page_data++) << 8;

		boot_page_fill(page_byte_addr + i, w);
	}
	
	
	boot_page_write(page_byte_addr);	// Store buffer in flash page.
	boot_spm_busy_wait();	// Wait until the memory is written.
	boot_rww_enable();

	SREG = sreg;
}

void UART_init()
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
	//sei();
	
	//UCSR0B |= (1 << RXCIE0);
}

void UART_send_char(char singleChar)
{
	while(!(UCSR0A & (1 << UDRE0)))
	{
	}
	UDR0 = singleChar;
}

void UART_send(char* str)
{
	for(int i = 0; str[i] != '\0'; i++)
	{
		UART_send_char(str[i]);
	}
}

void UART_send_command(char* message)
{
	UART_send("UC_");
	UART_send(message);
}

void UART_send_info(char* message)
{
	UART_send("UI_");
	UART_send(message);
}



char UART_receive()
{
	while ( !(UCSR0A & (1<<RXC0)) );
	wdt_reset();
	return UDR0;
}

void prog_handler()
{
	//UART_send_command(ACK"\n");
	uint8_t sreg_tmp = SREG;
	cli();

	unsigned char A[PAGE_SIZE_BYTES];	
	
	for (uint16_t i = 0; i < PAGE_SIZE_BYTES; i++)
	{
		A[i] = UART_receive();				
	}
	
	
	program_page(prog_page_address, A);
	prog_page_address += SPM_PAGESIZE;
	
	UART_send_command(ACK"\n");
	
	SREG = sreg_tmp;
}

void resolveUartCommand(char ch)
{
	if(ch == 'N')
	{
		UART_send_command(DEV_NAME"\n");
	}
	
	if(ch == 'A')
	{
		UART_send_command(ACK"\n");
	}
	
	if(ch == 'V')
	{
		UART_send_command(FIRM_VERS"\n");
	}

	if (ch == 'R')
	{
		
	}
	
	if(ch == 'P')
	{
		prog_handler();
	}
	
	if(ch == 'X')
	{
		UART_send_info("X command received!\n");
		asm("jmp 0x0000");
	}
}

ISR(WDT_vect)
{
	asm("jmp 0");
}

int main(void)
{	
	// Setting position of reset vectors table
	MCUCR |= (1 << IVCE);
	MCUCR |= (1 << IVSEL);
	
	// Disabling WDT from resetting the system
	MCUSR = 0;
	wdt_disable();
	
	// Using WDT as 2.0 seconds timer causing interrupt
	
	cli();
	wdt_reset();
	/* Start timed  sequence */
	WDTCSR = ((1<<WDCE) | (1<<WDE));
	/* Set new prescaler(time-out) value = 64K cycles (~0.5 s) */
	WDTCSR = ((1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0)) & ~(1 << WDE);
	sei();

	UART_init();

	DDRC |= (1 << 5);
	
	PORTC |= (1 << 5);

	while(1)
	{	
		char ch = UART_receive();
		
		// Communication in progress
		wdt_reset();
		
		if(PORTC & (1 << 5))
		{
			PORTC &= ~(1 << 5);
		}
		else
		{
			PORTC |= (1 << 5);
		}
		
		resolveUartCommand(ch);		
	}
}



