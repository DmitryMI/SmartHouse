/*
 * can.c
 *
 * Created: 11.10.2019 20:38:45
 *  Author: Dmitry
 */ 

#include "can.h"
#include "../DeviceConfig.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>

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

void can_reset_controller()
{
	uint8_t cmd = (1 << 7) | (1 << 6);
	
	if(log_handler != NULL)
	{
		log_handler("Sending RESET intruction...\n");
	}
	
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

void can_fill_tx0_write(uint16_t sid, uint8_t *data, int data_length)
{
	// Filling SID
	sid = sid << 5;
	can_write(CAN_REG_TXB0SIDH, (uint8_t*)(&sid), 2);
	
	// Filling transmit buffer size
	can_write(CAN_REG_TXB0DLC, (uint8_t*)(&data_length), 1);
	
	// Writing data
	can_write(CAN_REG_TXB0D0, data, data_length);
}

void can_rts(uint8_t txb_mask)
{
	uint8_t cmd = (1 << 7) | txb_mask;		// 1000 0nnn

	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	SPDR = cmd;
	while(!(SPSR & (1 << SPIF)));
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
}

int can_readrxb(uint8_t rxb_mask, uint8_t *data, int data_length)
{
	uint8_t cmd = (1 << 7) | (1 << 4) | rxb_mask;		// 1001 0nn0

	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	SPDR = cmd;
	while(!(SPSR & (1 << SPIF)));
	
	for(int i = 0; i < data_length; i++)
	{
		SPDR = 0xFF;
		while(!(SPSR & (1<<SPIF)));
		data[i] = SPDR;
	}
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
	
	return data_length;
}

void can_load_tx0_buffer(uint16_t sid, uint8_t *data, int data_length)
{
	uint8_t cmd = (1 << 6);
	
	// Pulling CS low
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	// Sending LOAD TXB intstruction
	SPDR = cmd;
	while(!(SPSR & (1 << SPIF)));
	
	// Filling SID
	sid = sid << 5;
	
	// Sid 11 - 3
	SPDR = sid >> 8;
	while(!(SPSR & (1 << SPIF)));
	
	// Sid 3 - 0
	SPDR = sid;
	while(!(SPSR & (1 << SPIF)));
	
	// Filling TXB0EID8
	SPDR = 0xFF;
	while(!(SPSR & (1 << SPIF)));
	
	// Filling TXB0EID0
	SPDR = 0xFF;
	while(!(SPSR & (1 << SPIF)));
	
	// Filling DLC
	SPDR = data_length;
	while(!(SPSR & (1 << SPIF)));
	
	// Sending bytes
	for(int i = 0; i < data_length; i++)
	{
		SPDR = data[i];
		while(!(SPSR & (1 << SPIF)));
	}
	
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
	
	if(flags & CAN_MCU_INT_EN)
	{
		// Configuring interrupt on MCU		
		//EIMSK |= (1 << INT0);						// Enabling INT0
		INT0_ENABLE;
		sei();
		
		// Configuring interrupt on CAN-controller
		uint8_t caninte;
		can_read(CAN_REG_CANINTE, &caninte, 1);
		caninte |= CAN_INT_RX0IE;
		can_write(CAN_REG_CANINTE, &caninte, 1);
	}
	
	// Set normal operation mode
	uint8_t canctrl;
	can_read(CAN_REG_CANCTRL, &canctrl, 1);
	canctrl &= CAN_OPMODE_RESET;
	canctrl |= CAN_OPMODE_NORMAL;
	can_write(CAN_REG_CANCTRL, &canctrl, 1);
}