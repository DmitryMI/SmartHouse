/*
 * Atmega2560_SPI_SLAVE.c
 *
 * Created: 10.09.2019 11:41:55
 * Author : Dmitry
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define PORT_SPI	PORTB
#define DDR_SPI		DDRB
#define DD_MOSI		PB2
#define DD_MISO		PB3
#define DD_SCK		PB1
#define DD_SS		PB0

#define PORT_LEDS	PORTA
#define DDR_LEDS	DDRA
#define DD_PIN0		PA0
#define DD_PIN1		PA1
#define DD_PIN2		PA2

void SPI_SlaveInit(void)
{
	DDR_SPI = (1 << DD_MISO);
	
	SPCR = (1 << SPE) | (1 << SPIE);
	sei();
}

ISR(SPI_STC_vect)
{
	char cData = SPDR;
	
	if(cData == '0')
	{
		PORT_LEDS |= (1 << DD_PIN0);
		PORT_LEDS &= ~(1 << DD_PIN1);
		PORT_LEDS &= ~(1 << DD_PIN2);
	}
	else if(cData == '1')
	{
		PORT_LEDS &= ~(1 << DD_PIN0);
		PORT_LEDS |= (1 << DD_PIN1);
		PORT_LEDS &= ~(1 << DD_PIN2);
	}
	else if(cData == '2')
	{
		PORT_LEDS &= ~(1 << DD_PIN0);
		PORT_LEDS &= ~(1 << DD_PIN1);
		PORT_LEDS |= (1 << DD_PIN2);
	}
	else
	{
		PORT_LEDS &= ~(1 << DD_PIN0);
		PORT_LEDS &= ~(1 << DD_PIN1);
		PORT_LEDS &= ~(1 << DD_PIN2);
	}
}

int main(void)
{
    DDR_LEDS = (1 << DD_PIN0) | (1 << DD_PIN1) | (1 << DD_PIN2);
	PORT_LEDS |= (1 << DD_PIN0) | (1 << DD_PIN1) | (1 << DD_PIN2);
	_delay_ms(2000);
	
	PORT_LEDS &= ~(1 << DD_PIN0) & ~(1 << DD_PIN1) & ~(1 << DD_PIN2);
	
	SPI_SlaveInit();
	
    while (1) 
    {
    }
}

