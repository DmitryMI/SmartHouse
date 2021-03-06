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
#include <avr/eeprom.h>

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

uint16_t CAN_SID;
// ******************

// Device specific configs
#if defined (__AVR_ATmega328P__)

#define INT0_ENABLE		EIMSK |= (1 << INT0);

#define TIMER_ENABLE	TIMSK1 |= (1 << TOIE1); \
						TCCR1B |= (1 << CS12);
						
#define EXIT_BOOT		asm("jmp 0");

#define SET_VECTORS		MCUCR = (1 << IVCE);	\
						MCUCR = (1 << IVSEL);
						
#define RESET_VECTORS	MCUCR = (1 << IVCE);	\
						MCUCR = 0;
						
#elif defined (__AVR_ATmega8__)

#define INT0_ENABLE		GICR |= (1 << INT0);

#define TIMER_ENABLE	TIMSK |= (1 << TOIE1); \
						TCCR1B |= (1 << CS12);
						
#define EXIT_BOOT		asm("rjmp app_start");

#define SET_VECTORS		GICR = (1 << IVCE);	\
						GICR = (1 << IVSEL);

#define RESET_VECTORS	GICR = (1 << IVCE);	\
						GICR = 0;

#else
# warning Device not supported
#endif



#define PACKAGE_SIZE_IN		13
#define PACKAGE_SIZE_OUT	8

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
	RESET_VECTORS;
	
	EXIT_BOOT;
}

ISR(TIMER1_OVF_vect)
{
	exit_bootloader();
}

void inline timer_reset()
{
	TCNT1 = 0;
}

void inline program_handle(uint8_t *package)
{	
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
}

void spi_putc(uint8_t b)				// INLINE increases size!
{
	SPDR = b;
	while(!(SPSR & (1 << SPIF)));
}

uint8_t inline spi_getc()
{
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

void inline can_cmd(uint8_t command)
{
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	spi_putc(command);
	SPI_PORT |= (1 << SPI_CS_CAN);
}

void inline can_rts()
{
	uint8_t cmd = (1 << 7) | CAN_RTS_TXB0;		// 1000 0nnn
	can_cmd(cmd);
}

void inline can_reset_controller()
{
	uint8_t cmd = (1 << 7) | (1 << 6);
	can_cmd(cmd);
}

void inline can_write(uint8_t reg_addr, uint8_t data)
{
	uint8_t write_cmd = (1 << 1);
	
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	spi_putc(write_cmd);
	spi_putc(reg_addr);
	
	//for(int i = 0; i < data_length; i++)
	//{
		spi_putc(data);
	//}
	
	SPI_PORT |= (1 << SPI_CS_CAN);
}

void inline can_readrxb(uint8_t *data)
{
	uint8_t cmd = (1 << 7) | (1 << 4);		// 1001 0nn0

	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	spi_putc(cmd);
	
	for(int i = 0; i < PACKAGE_SIZE_IN; i++)
	{
		SPDR = 0xFF;
		data[i] = spi_getc();
	}
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
}

void inline can_load_tx0_buffer(uint8_t *data)
{
	uint8_t cmd = (1 << 6);
	
	// Pulling CS low
	SPI_PORT &= ~(1 << SPI_CS_CAN);
	
	// Sending LOAD TXB instruction
	spi_putc(cmd);
	
	// Filling SID
	uint16_t sid = CAN_SID << 5;
	
	// Sid 11 - 3
	spi_putc(sid >> 8);
	
	// Sid 3 - 0
	spi_putc(sid);
	
	// Filling TXB0EID8
	spi_putc(0xFF);
	
	// Filling TXB0EID0
	spi_putc(0xFF);
	
	// Filling DLC
	spi_putc(PACKAGE_SIZE_OUT);
	
	// Sending bytes
	for(int i = 0; i < PACKAGE_SIZE_OUT; i++)
	{
		spi_putc(data[i]);
	}
	
	// Setting SS to high
	SPI_PORT |= (1 << SPI_CS_CAN);
}



void inline load_tx()
{
	// CAN-data received
	//wdt_reset();
	timer_reset();
	
	// Reading data from CAN-controller
	uint8_t package[PACKAGE_SIZE_IN];
	can_readrxb(package);
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
	
	//uint8_t response[8] = {0};
	uint8_t response[8];
		
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
		cli();
		program_handle(package + CAN_OFFSET_DATA);	
		sei();
		
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
		can_load_tx0_buffer(response);
		can_rts();
	}	
}


ISR(INT0_vect)
{		
	load_tx();
}

void wait100ms()
{
	_delay_ms(100);
}

void inline can_init()
{
	// Initializaing SPI
	SPI_DDR = (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_CS_MCU) | (1 << SPI_CS_CAN);
	SPI_PORT |= (1 << SPI_CS_CAN);
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

	can_reset_controller();
	
	// Configuring interrupt on MCU
	INT0_ENABLE;

	wait100ms();
	uint8_t caninte = CAN_INT_RX0IE;
	can_write(CAN_REG_CANINTE, caninte);

	// Set normal operation mode
	uint8_t canctrl = (1 << 0) | (1 << 1) | (1 << 2);
	can_write(CAN_REG_CANCTRL, canctrl);
}

int main(void)
{
	// Setting position of reset vectors table
	SET_VECTORS;
	
	// Disabling WDT from resetting the system
	MCUSR = 0;
	wdt_disable();
	
	CAN_SID = eeprom_read_word(0);
	
	// Initializing CAN
	can_init();
	
	// Using TIMER1 as 2.0 seconds timer causing interrupt		
	TIMER_ENABLE;
	
	sei();
	
	LED_DDR |= (1 << LED_PIN);	
	LED_PORT |= (1 << LED_PIN);
	
    while (1) 
    {

    }
}