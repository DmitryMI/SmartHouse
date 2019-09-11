/*
 * Atmega328p_SPI_MASTER.c
 *
 * Created: 10.09.2019 11:22:56
 * Author : Dmitry
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#define PORT_SPI	PORTB
#define DDR_SPI		DDRB
#define DD_MOSI		PB3
#define DD_MISO		PB4
#define DD_SCK		PB5
#define DD_SS		PB2

void SPI_MasterInit(void)
{
	// MOSI - output, SCK - output, SS - output, others - input
	DDR_SPI = (1 << DD_MOSI) | (1 << DD_SCK) | (1 << DD_SS);
	
	// Setting SS to high
	PORT_SPI |= (1 << DD_SS);
	
	// Enable SPI, Set master mode, set clock devider to 128
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0) | (1 << SPR1);
}

void SPI_MasterTransmit(char cData)
{
	// Setting SS to low
	PORT_SPI &= ~(1 << DD_SS);
	
	SPDR = cData;
		
	while(!(SPSR & (1 << SPIF)))
	{
		
	}
	
	// Setting SS to high
	PORT_SPI |= (1 << DD_SS);
}

int main(void)
{
	DDRB |= 1 << PB5;
	PORTB |= 1 << PB5;
	_delay_ms(1000);
	PORTB &= ~(1 << PB5);
	
    SPI_MasterInit();
	
    while (1) 
    {
		SPI_MasterTransmit('0');
		_delay_ms(500);
		SPI_MasterTransmit('1');		
		_delay_ms(500);
		SPI_MasterTransmit('2');
		_delay_ms(500);
    }
}

