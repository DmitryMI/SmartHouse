/*
 * can.c
 *
 * Created: 11.10.2019 20:38:45
 *  Author: Dmitry
 */ 

#include "can.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>

logger_t log_handler = NULL;

void can_set_logger(logger_t logger)
{
	log_handler = logger;
}

void _can_int0_handler()
{
	// Check if it is really a low-level INT0 interrupt
	if(PIND & (1 << PD2))
	{
		if(log_handler != NULL)
		{
			log_handler("CAN interrupt appeared!");
		}
	}
}

void can_reset_controller()
{
	uint8_t cmd = (1 << 7) | (1 << 6);
	
	log_handler("Sending RESET intruction...\n");
	
	// Pulling CS low
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	SPDR = cmd;
	
	while(!(SPSR & (1 << SPIF)));
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);	
}

uint8_t can_read_status()
{
	uint8_t cmd = (1 << 7) | (1 << 5);		// 1010 0000
	uint8_t status;

	// Pulling CS low
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	SPDR = cmd;	
	while(!(SPSR & (1 << SPIF)));
	
	SPDR = 0xFF;
	while(!(SPSR & (1 << SPIF)));
	status = SPDR;
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
	
	return status;
}

int can_read(uint8_t reg_addr, uint8_t *data_buffer, int data_length)
{
	uint8_t read_cmd = (1 << 1) | (1 << 0);
	
	// Pulling CS low
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	// Sending READ intstruction
	SPDR = read_cmd;
	while(!(SPSR & (1 << SPIF)));
	
	// Sending address byte
	SPDR = reg_addr;
	while(!(SPSR & (1 << SPIF)));
	
	
	for(int i = 0; i < data_length; i++)
	{
		SPDR = 0xFF;
		while(!(SPSR & (1<<SPIF)));
		data_buffer[i] = SPDR;
	}
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
	
	return data_length;
}

void can_write(uint8_t reg_addr, uint8_t *data_buffer, int data_length)
{
	uint8_t write_cmd = (1 << 1);
	
	// Pulling CS low
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	// Sending WRITE intstruction
	SPDR = write_cmd;	
	while(!(SPSR & (1 << SPIF)));
	
	// Sending address byte
	SPDR = reg_addr;
	while(!(SPSR & (1 << SPIF)));
	
	// Sending bytes
	for(int i = 0; i < data_length; i++)
	{
		SPDR = data_buffer[i];
		while(!(SPSR & (1 << SPIF)));
	}
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
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
	
	if(flags & CAN_INT_EN)
	{
		// Configuring interrupt on MCU
		DDRD &= ~(1 << PD2);						// Setting	INT0 pin to input
		EICRA &= ~(1 << ISC00) & ~(1 << ISC01);		// Low level INT0
		EIMSK |= (1 << INT0);						// Enabling INT0
		sei();
		
		// Configuring interrupt on CAN-controller
		// TODO
	}
}