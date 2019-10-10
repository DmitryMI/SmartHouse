/*
 * NRF24L01_Sender.c
 *
 * Created: 08.10.2019 13:32:10
 * Author : Dmitry
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <stdio.h>
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

void SPI_MasterTransmit2(char cData, int releaseLine)
{
	// Setting SS to low
	PORT_SPI &= ~(1 << DD_SS);
	
	SPDR = cData;
	
	while(!(SPSR & (1 << SPIF)))
	{
		
	}
	
	// Setting SS to high
	if(releaseLine)
	{		
		PORT_SPI |= (1 << DD_SS);
	}
}

void SPI_MasterReleaseLine()
{
	PORT_SPI |= (1 << DD_SS);
}

void SPI_MasterTransmit_buf(uint8_t *cData, int len, int releaseLine)
{
	// Setting SS to low
	PORT_SPI &= ~(1 << DD_SS);
	
	for(int i = 0; i < len; i++)
	{			
		SPDR = cData[i];		
		while(!(SPSR & (1 << SPIF)))
		{
			
		}
	}
	
	// Setting SS to high
	if(releaseLine)
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


uint8_t NRF_ReadStatusRegister()
{
	SPI_MasterTransmit(0b11111111);
	while(!(SPSR & (1 << SPIF)))
	{
		
	}
	return SPDR;
}

void NRF_write_tx_payload(uint8_t *buffer, int len)
{
	SPI_MasterTransmit2(0b10100000, 0);		// W_TX_PAYLOAD instruction
	SPI_MasterTransmit_buf(buffer, len, 1);
}

uint8_t NRF_enable_TX_IRQ(int enabled)
{
	uint8_t cmdRead = (1 << 5);
	uint8_t cmdWrite = 0b00100000 | (1 << 5);
	
	// Writing register
	SPI_MasterTransmit2(cmdWrite, 0);	// R_REGISTER, MASK_TX_DS
	SPI_MasterTransmit2(!!enabled, 1);
	
	// Reading register
	SPI_MasterTransmit2(cmdRead, 0);	// R_REGISTER, MASK_TX_DS
	while(!(SPSR & (1 << SPIF)))
	{
		
	}
	return SPDR;
}

void NRF_set_self_address(uint8_t addr)
{
	
	
}

int main(void)
{
	UART_init();
	UART_send("UART initialized\n");
	SPI_MasterInit();
	UART_send("SPI initialized\n");
	
	UART_send("Reading status register...\n");	
	char sprintf_buffer[16];
	
	
	while (1)
	{
		uint8_t status = NRF_ReadStatusRegister();
		sprintf(sprintf_buffer, "Status: %#010x\n", status);
		UART_send(sprintf_buffer);
		_delay_ms(1000);
		
		sprintf(sprintf_buffer, "Sample text\n");
		UART_send("Writing 'SampleText' to TX buf\n");
		NRF_write_tx_payload(sprintf_buffer, 16);
		_delay_ms(2000);	
		
	}
}
