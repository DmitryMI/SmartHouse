/*
 * Atmega2560_SPI_SLAVE.c
 *
 * Created: 10.09.2019 11:41:55
 * Author : Dmitry
 */ 

#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define PORT_SPI	PORTB
#define DDR_SPI		DDRB
#define DD_MOSI		PB2
#define DD_MISO		PB3
#define DD_SCK		PB1
#define DD_SS		PB0

#define PORT_LEDS	PORTC
#define DDR_LEDS	DDRC
#define DD_PIN0		PC5

void send_Uart(unsigned char c)//	�������� �����
{
	while(!(UCSRA&(1 <<UDRE)))	//	���������������, ����� ������� ��������
	{}
	UDR = c;
}

void send_int_Uart(unsigned int c)//	�������� ����� �� 0000 �� 9999
{
	unsigned char temp;
	c=c%10000;
	temp=c/100;
	send_Uart(temp/10+'0');
	send_Uart(temp%10+'0');
	temp=c%100;;
	send_Uart(temp/10+'0');
	send_Uart(temp%10+'0');
}

unsigned char getch_Uart(void)//	��������� �����
{
	while(!(UCSRA&(1<<RXC)))	//	���������������, ����� ������� ��������
	{}
	return UDR;
}

void send_Uart_str(unsigned char *s)//	�������� ������
{
	while (*s != 0) send_Uart(*s++);
}


int init_UART(void)
{
	//	��������� �������� 9600
	UBRRH=0;	//	UBRR=f/(16*band)-1 f=8000000�� band=9600,
	UBRRL=51;	//	���������� ����������� ��������������� ����� ������
	
	//			RXC			-	���������� �����
	//			|TXC		-	���������� ��������
	//			||UDRE 		-	���������� ������ ��� ��������
	//			|||FE		-	������ �����
	//			||||DOR		-	������ ������������ ������
	//			|||||PE		-	������ ��������
	//			||||||U2X	-	������� ��������
	//			|||||||MPCM	-	����������������� �����
	//			76543210
	UCSRA=0b00000000;

	//			RXCIE		-	���������� ��� ����� ������
	//			|TXCIE		-	���������� ��� ���������� ��������
	//			||UDRIE		-	���������� ���������� ������ ��� ��������
	//			|||RXEN		-	���������� �����
	//			||||TXEN	-	���������� ��������
	//			|||||UCSZ2	-	UCSZ0:2 ������ ����� ������
	//			||||||RXB8	-	9 ��� �������� ������
	//			|||||||TXB8	-	9 ��� ���������� ������
	//			76543210
	UCSRB=0b00011000;	//	�������� ���� � �������� �� UART

	//			URSEL		-	������ 1
	//			|UMSEL		-	�����:1-���������� 0-�����������
	//			||UPM1		-	UPM0:1 ��������
	//			|||UPM0		-	UPM0:1 ��������
	//			||||USBS	-	��� ����: 0-1, 1-2
	//			|||||UCSZ1	-	UCSZ0:2 ������ ����� ������
	//			||||||UCSZ0	-	UCSZ0:2 ������ ����� ������
	//			|||||||UCPOL-	� ���������� ������ - ������������
	//			76543210
	UCSRC=0b10000110;	//	8-������� �������
}




void SPI_SlaveInit(void)
{
	DDR_SPI = (1 << DD_MISO);
	
	SPCR = (1 << SPE) | (1 << SPIE);
	sei();
}

ISR(SPI_STC_vect)
{
	char cData = SPDR;
	send_Uart(cData);
	SPDR = 0x41;
}


int main(void)
{
    DDR_LEDS = (1 << DD_PIN0);
	PORT_LEDS |= (1 << DD_PIN0);	
	PORT_LEDS &= ~(1 << DD_PIN0);
	
	SPI_SlaveInit();
	
	init_UART();
	
    while (1) 
    {

    }
}

