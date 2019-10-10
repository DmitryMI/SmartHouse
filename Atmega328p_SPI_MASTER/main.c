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
	
	// Enable SPI, Set master mode, set clock divider to 128
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

void UART_init()
{
	unsigned int ubrr = F_CPU / 16 / 1200 - 1;
	
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void UART_send_char(char singleChar)
{
	while(!(UCSR0A & (1 << UDRE0)))
	{
	}
	UDR0 = singleChar;
}

void UART_send(char* message)
{
	for(int i = 0; i < 32 && message[i] != '\0'; i++)
	{
		UART_send_char(message[i]);
	}
}

void NRF_SendCommand(uint8_t *ret_buffer, int ret_data_len)
{
	
}

int main(void)
{
	UART_init();
	UART_send("UART initialized");
    SPI_MasterInit();
	UART_send("SPI initialized");
	
	uint8_t buffer[32];
	
    while (1) 
    {
		
    }
}

