/*
 * can.h
 *
 * Created: 11.10.2019 20:38:36
 *  Author: Dmitry
 */ 


#ifndef CAN_H_
#define CAN_H_

#include <avr/io.h>

// ***CONFIGURABLE***

#ifndef SPI_PORT
	#define SPI_PORT	PORTB
#endif
#ifndef SPI_DDR
	#define SPI_DDR		DDRB
#endif
#ifndef	SPI_MOSI
	#define SPI_MOSI		PB3
#endif
#ifndef SPI_MISO
	#define SPI_MISO		PB4
#endif
#ifndef SPI_SCK
	#define SPI_SCK		PB5
#endif
#ifndef SPI_CS_CAN
	#define SPI_CS_CAN		PB0
#endif
#ifndef SPI_CS_MCU
	#define SPI_CS_MCU		PB2
#endif

// ******************

#define CAN_INT_EN		(1 << 1)

#define CAN_REG_CANCTRL			0xF
#define CAN_OPMODE_RESET		(~(1 << 7) & ~(1 << 6) & ~(1 << 5))
#define CAN_OPMODE_NORMAL		0
#define CAN_OPMODE_SLEEP		(1 << 5)
#define CAN_OPMODE_LOOPBACK		(1 << 6)
#define CAN_OPMODE_LISTEN		((1 << 6) | (1 << 5))
#define CAN_OPMODE_CONFIG		(1 << 7)


#define CAN_REG_CANINTE			0x2B
#define CAN_INT_RX0IE			(1 << 0)
#define CAN_INT_RX1IE			(1 << 1)

typedef void (*logger_t)(char* message);



void can_init(uint8_t flags);
void can_set_logger(logger_t logger);
void can_reset_controller();

/* Call this function in INT0 ISR, if you are using CAN-interrupt */
void _can_int0_handler();

uint8_t can_read_status();
void can_write(uint8_t reg_addr, uint8_t *data_buffer, int data_length);
int can_read(uint8_t reg_addr, uint8_t *data_buffer, int data_length);



#endif /* CAN_H_ */