/*
 * CANBootloader.c
 *
 * Created: 12.10.2019 16:32:01
 * Author : Dmitry
 */ 

#include <avr/io.h>

#define F_CPU 8000000UL

#include <util/delay.h>
#include <avr/boot.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "can_commands.h"

// ***CONFIGURABLE***
#define BOOT_VERS	1

#define LED_PORT	PORTC
#define LED_DDR		DDRC
#define LED_PIN		PC5

#define SPI_PORT	PORTB
#define SPI_DDR		DDRB
#define SPI_MOSI	PB3
#define SPI_MISO	PB4
#define SPI_SCK		PB5
#define SPI_CS_CAN	PB0
#define SPI_CS_MCU	PB2

#ifndef CAN_SID
# warning "CAN_SID must be defined and unique"
#define CAN_SID 0x7FE
#endif
// ******************

#define CAN_REG_CANINTE			0x2B
#define CAN_INT_RX0IE			(1 << 0)

#define CAN_REG_CANCTRL			0xF
#define CAN_OPMODE_RESET		(~(1 << 7) & ~(1 << 6) & ~(1 << 5))
#define CAN_OPMODE_NORMAL		0

#define CAN_RTS_TXB0	(1 << 0)

#define PAGE_SIZE_BYTES SPM_PAGESIZE

uint32_t prog_page_address = 0;
uint8_t prog_byte_address = 0;

void inline exit_bootloader()
{
	MCUCR = (1 << IVCE);
	MCUCR = 0;
	
	asm("jmp 0");
}

ISR(WDT_vect)
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

void spi_putc(uint8_t b)
{
	SPDR = b;
	while(!(SPSR & (1 << SPIF)));
}

uint8_t spi_getc()
{
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

void inline can_reset_controller()
{
	uint8_t cmd = (1 << 7) | (1 << 6);
	SPI_PORT &= ~(1 << SPI_CS_CAN);	
	spi_putc(cmd);
	SPI_PORT |= (1 << SPI_CS_CAN);
}


int inline can_read(uint8_t reg_addr, uint8_t *data_buffer, int data_length)
{
	uint8_t read_cmd = (1 << 1) | (1 << 0);
	
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	spi_putc(read_cmd);
	spi_putc(reg_addr);
	
	for(int i = 0; i < data_length; i++)
	{
		SPDR = 0xFF;
		data_buffer[i] = spi_getc();
	}
	SPI_PORT |= (1 << SPI_CS_CAN);	
	return data_length;
}

void inline can_write(uint8_t reg_addr, uint8_t *data_buffer, int data_length)
{
	uint8_t write_cmd = (1 << 1);
	
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	spi_putc(write_cmd);
	spi_putc(reg_addr);
	
	for(int i = 0; i < data_length; i++)
	{
		spi_putc(data_buffer[i]);
	}
	
	SPI_PORT |= (1 << SPI_CS_CAN);
}

int inline can_readrxb(uint8_t rxb_mask, uint8_t *data, int data_length)
{
	uint8_t cmd = (1 << 7) | (1 << 4) | rxb_mask;		// 1001 0nn0

	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	spi_putc(cmd);
	
	for(int i = 0; i < data_length; i++)
	{
		SPDR = 0xFF;
		data[i] = spi_getc();
	}
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
	
	return data_length;
}

void inline can_load_tx0_buffer(uint16_t sid, uint8_t *data, int data_length)
{
	uint8_t cmd = (1 << 6);
	
	// Pulling CS low
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	// Sending LOAD TXB intstruction
	spi_putc(cmd);
	
	// Filling SID
	sid = sid << 5;
	
	// Sid 11 - 3
	spi_putc(sid >> 8);
	
	// Sid 3 - 0
	spi_putc(sid);
	
	// Filling TXB0EID8
	spi_putc(0xFF);
	
	// Filling TXB0EID0
	spi_putc(0xFF);
	
	// Filling DLC
	spi_putc(data_length);
	
	// Sending bytes
	for(int i = 0; i < data_length; i++)
	{
		spi_putc(data[i]);
	}
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
}

void inline can_rts(uint8_t txb_mask)
{
	uint8_t cmd = (1 << 7) | txb_mask;		// 1000 0nnn

	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	spi_putc(cmd);
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
}

void inline load_tx()
{
	// CAN-data received
	wdt_reset();
	
	// Reading data from CAN-controller
	const int package_length = 13;
	uint8_t package[package_length];
	can_readrxb(0, package, package_length);
	uint16_t sid = 0;
	sid += package[0];
	sid = sid << 3;
	sid += (package[1] & ((1 << 7) | (1 << 6) | (1 << 5))) >> 5;
	//uint8_t dlc = package[4];
	
	uint8_t addrh = package[CAN_OFFSET_ADDRH];
	uint8_t addrl = package[CAN_OFFSET_ADDRL];
	uint16_t addr = addrh;
	addr = addr << 8;
	addr += addrl;
	
	if(addr != 0 && addr != CAN_SID)
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
		can_load_tx0_buffer(CAN_SID, response, 8);
		can_rts(CAN_RTS_TXB0);
	}	
}


ISR(INT0_vect)
{	
	/*EIMSK &= ~(1 << INT0);
	DDRB |= (1 << PB1);
	PORTB |= (1 << PB1);*/
	
	DDRB |= (1 << PB1);
	PORTB |= (1 << PB1);
	
	load_tx();
}

void inline can_init()
{
	// Initializaing SPI
	SPI_DDR = (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_CS_MCU) | (1 << SPI_CS_CAN);
	SPI_PORT |= (1 << SPI_CS_CAN);
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

	can_reset_controller();
	
	// Configuring interrupt on MCU
	DDRD &= ~(1 << PD2);						// Setting	INT0 pin to input
	EICRA &= ~(1 << ISC00) & ~(1 << ISC01);		// Low level INT0
	EIMSK |= (1 << INT0);						// Enabling INT0
	sei();
	
	
	// Configuring interrupt on CAN-controller
	/*uint8_t caninte = CAN_INT_RX0IE;	
	can_write(CAN_REG_CANINTE, &caninte, 1);*/
	
	
	uint8_t caninte = 0;
	_delay_ms(100);
	caninte = CAN_INT_RX0IE;
	can_write(CAN_REG_CANINTE, &caninte, 1);

	// Set normal operation mode
	uint8_t canctrl = (1 << 0) | (1 << 1) | (1 << 2);
	can_write(CAN_REG_CANCTRL, &canctrl, 1);
}

/* Can be used in polling*/
uint8_t inline can_read_status()
{
	uint8_t cmd = (1 << 7) | (1 << 5);		// 1010 0000
	uint8_t status;

	// Pulling CS low
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	spi_putc(cmd);
	
	SPDR = 0xFF;
	status = spi_getc();
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
	
	return status;
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
	can_init();
	
	// Using WDT as 2.0 seconds timer causing interrupt		
	cli();
	wdt_reset();
	WDTCSR = ((1<<WDCE) | (1<<WDE));
	WDTCSR = ((1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0)) & ~(1 << WDE);
	sei();
	
	LED_DDR |= (1 << LED_PIN);	
	LED_PORT |= (1 << LED_PIN);
	
    while (1) 
    {
		_delay_ms(100);
		LED_PORT ^= (1 << LED_PIN);
		/*LED_PORT &= ~(1 << LED_PIN);
		//_delay_ms(100);
		LED_PORT |= (1 << LED_PIN);*/
    }
}

