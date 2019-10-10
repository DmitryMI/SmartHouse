/*
 * Atmega328p_IIC_Master.c
 *
 * Created: 11.09.2019 10:08:08
 * Author : Dmitry
 */ 

#include <avr/io.h>
#include <stdio.h>

#define F_CPU 16000000UL

#include <util/delay.h>

#define PORT_IIC	PORTC
#define DDR_IIC		DDRC
#define PIN_SCL		PC5
#define PIN_SDA		PC4

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


int main(void)
{
	UART_init();
	
    DDR_IIC |= (1 << PIN_SCL);
	DDR_IIC |= (1 << PIN_SDA);
	
	PORT_IIC |= (1 << PIN_SCL);
	PORT_IIC |= (1 << PIN_SDA);
	
	UART_send("Starting...\n");
	
	_delay_ms(2000);
	
	UART_send("Delay finished...\n");
	
	PORT_IIC &= ~(1 << PIN_SCL);
	PORT_IIC &= ~(1 << PIN_SDA);
	
	uint8_t tickCounter = 0;
	
	char buffer[16];
	
    while (1) 
    {		
		sprintf(buffer, "Tick: %d\n", tickCounter);
		UART_send(buffer);
		tickCounter++;
		_delay_ms(100);
    }
}

