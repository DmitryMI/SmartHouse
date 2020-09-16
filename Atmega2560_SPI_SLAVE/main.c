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

void send_Uart(unsigned char c)//	Отправка байта
{
	while(!(UCSRA&(1 <<UDRE)))	//	Устанавливается, когда регистр свободен
	{}
	UDR = c;
}

void send_int_Uart(unsigned int c)//	Отправка числа от 0000 до 9999
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

unsigned char getch_Uart(void)//	Получение байта
{
	while(!(UCSRA&(1<<RXC)))	//	Устанавливается, когда регистр свободен
	{}
	return UDR;
}

void send_Uart_str(unsigned char *s)//	Отправка строки
{
	while (*s != 0) send_Uart(*s++);
}


int init_UART(void)
{
	//	Установка скорости 9600
	UBRRH=0;	//	UBRR=f/(16*band)-1 f=8000000Гц band=9600,
	UBRRL=51;	//	нормальный асинхронный двунаправленный режим работы
	
	//			RXC			-	завершение приёма
	//			|TXC		-	завершение передачи
	//			||UDRE 		-	отсутствие данных для отправки
	//			|||FE		-	ошибка кадра
	//			||||DOR		-	ошибка переполнение буфера
	//			|||||PE		-	ошибка чётности
	//			||||||U2X	-	Двойная скорость
	//			|||||||MPCM	-	Многопроцессорный режим
	//			76543210
	UCSRA=0b00000000;

	//			RXCIE		-	прерывание при приёме данных
	//			|TXCIE		-	прерывание при завершение передачи
	//			||UDRIE		-	прерывание отсутствие данных для отправки
	//			|||RXEN		-	разрешение приёма
	//			||||TXEN	-	разрешение передачи
	//			|||||UCSZ2	-	UCSZ0:2 размер кадра данных
	//			||||||RXB8	-	9 бит принятых данных
	//			|||||||TXB8	-	9 бит переданных данных
	//			76543210
	UCSRB=0b00011000;	//	разрешен приём и передача по UART

	//			URSEL		-	всегда 1
	//			|UMSEL		-	режим:1-синхронный 0-асинхронный
	//			||UPM1		-	UPM0:1 чётность
	//			|||UPM0		-	UPM0:1 чётность
	//			||||USBS	-	топ биты: 0-1, 1-2
	//			|||||UCSZ1	-	UCSZ0:2 размер кадра данных
	//			||||||UCSZ0	-	UCSZ0:2 размер кадра данных
	//			|||||||UCPOL-	в синхронном режиме - тактирование
	//			76543210
	UCSRC=0b10000110;	//	8-битовая посылка
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

