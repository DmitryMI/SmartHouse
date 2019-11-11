/*
 * can.c
 *
 * Created: 11.10.2019 20:38:45
 *  Author: Dmitry
 */ 

#define F_CPU 8000000UL

#include "can.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

logger_t log_handler = NULL;
can_received_callback_t can_callback = NULL;

void can_set_logger(logger_t logger)
{
	log_handler = logger;
}

void can_set_callback(can_received_callback_t callback)
{
	can_callback = callback;
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

uint8_t can_read_status()
{
	uint8_t cmd = (1 << 7) | (1 << 5);		// 1010 0000
	
	SPI_PORT &= ~(1 << SPI_CS_CAN);	
	spi_putc(cmd);
	SPDR = 0xFF;
	uint8_t status = spi_getc();	
	SPI_PORT |= (1 << SPI_CS_CAN);
	return status;
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


void _can_int0_handler()
{
	// Check if it is really a low-level INT0 interrupt
	if(!(PIND & (1 << PD2)))
	{	
		const int package_length = 13;
		
		uint8_t package[package_length];
		can_readrxb(0, package, package_length);				
		
		uint16_t sid = 0;
		sid += package[0];
		sid = sid << 3;
		sid += (package[1] & ((1 << 7) | (1 << 6) | (1 << 5))) >> 5;		
		
		uint8_t dlc = package[4];		
		
		if(can_callback != NULL)
		{
			can_callback(sid, package, dlc);
		}		
	}
}

void can_enable_int0()
{
#if defined (__AVR_ATmega328P__)
	// Configuring interrupt on MCU
	DDRD &= ~(1 << PD2);						// Setting	INT0 pin to input
	EICRA &= ~(1 << ISC00) & ~(1 << ISC01);		// Low level INT0
	EIMSK |= (1 << INT0);						// Enabling INT0
	sei();
#elif (__AVR_ATmega8__)
	DDRD &= ~(1 << PD2);						// Setting	INT0 pin to input
	MCUCR &= ~(1 << ISC00) & ~(1 << ISC01);		// Low level INT0
	GICR |= (1 << INT0);						// Enabling INT0
	sei();
#endif	
}

void can_init(uint8_t flags)
{
	// Initializaing SPI
	// MOSI - output, SCK - output, SS - output, others - input
	SPI_DDR = (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_CS_MCU) | (1 << SPI_CS_CAN);
	
	// Setting CS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
	
	// Enable SPI, Set master mode, set clock divider to 128
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0) | (1 << SPR1);
	
	// Specification recommends to reset controller after power-up
	can_reset_controller();
	
	_delay_ms(100);
	
	if(flags & CAN_MCU_INT_EN)
	{
		// Configuring interrupt on MCU
		can_enable_int0();
		
		// Configuring interrupt on CAN-controller
		uint8_t caninte = CAN_INT_RX0IE;
		can_write(CAN_REG_CANINTE, &caninte, 1);
	}
	
	// Set normal operation mode
	uint8_t canctrl = (1 << 0) | (1 << 1) | (1 >> 2);
	//can_read(CAN_REG_CANCTRL, &canctrl, 1);
	canctrl &= CAN_OPMODE_RESET;
	canctrl |= CAN_OPMODE_NORMAL;
	can_write(CAN_REG_CANCTRL, &canctrl, 1);
}