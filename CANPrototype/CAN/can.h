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


#define CAN_PACKAGE_PAYLOAD_OFFSET 5

#define CAN_RTS_TXB0	(1 << 0)
#define CAN_RTS_TXB1	(1 << 1)
#define CAN_RTS_TXB2	(1 << 2)
#define CAN_MCU_INT_EN		(1 << 1)

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


#define CAN_REG_CNF1			0x2A
// Bitrate configuration
#define CAN_CNF1_BRP0			(1 << 0)
#define CAN_CNF1_BRP1			(1 << 1)
#define CAN_CNF1_BRP2			(1 << 2)
#define CAN_CNF1_BRP3			(1 << 3)


#define CAN_REG_CNF2			0x29
#define CAN_CNF2_PRSEG0			(1 << 0)
#define CAN_CNF2_PRSEG1			(1 << 1)
#define CAN_CNF2_PRSEG2			(1 << 2)


#define CAN_REG_TXB0SIDL		0x32
#define CAN_EID16				(1 << 0)
#define CAN_EID17				(1 << 1)
#define CAN_EXIDE				(1 << 3)
#define CAN_SID0				(1 << 5)
#define CAN_SID1				(1 << 6)
#define CAN_SID2				(1 << 7)


#define CAN_REG_TXB0SIDH		0x31
#define CAN_SID3				(1 << 0)
#define CAN_SID4				(1 << 1)
#define CAN_SID5				(1 << 2)
#define CAN_SID6				(1 << 3)
#define CAN_SID7				(1 << 4)
#define CAN_SID8				(1 << 5)
#define CAN_SID9				(1 << 6)
#define CAN_SID10				(1 << 7)

#define CAN_REG_TXB0DLC			0x35
#define CAN_RTR					(1 << 6)
#define CAN_DLC3				(1 << 3)
#define CAN_DLC2				(1 << 2)
#define CAN_DLC1				(1 << 1)
#define CAN_DLC0				(1 << 0)

#define CAN_REG_TXB0D0			0x36

#define CAN_REG_TXB0CTRL		0x30
#define CAN_TX0_TXREQ			(1 << 3)


typedef void (*logger_t)(char* message);

typedef void (*can_received_callback_t)(uint16_t sid, uint8_t *data, uint8_t data_length);

void can_init(uint8_t flags);
void can_set_logger(logger_t logger);
void can_set_callback(can_received_callback_t callback);
void can_reset_controller();

/* Call this function in INT0 ISR, if you are using CAN-interrupt */
void _can_int0_handler();

uint8_t can_read_status();
void can_write(uint8_t reg_addr, uint8_t *data_buffer, int data_length);
int can_read(uint8_t reg_addr, uint8_t *data_buffer, int data_length);

/* sid: standard frame ID. DON'T USE MSB-BITS 16-12!*/
void can_fill_tx0_write(uint16_t sid, uint8_t *data, int data_length);

void can_load_tx0_buffer(uint16_t sid, uint8_t *data, int data_length);

void can_rts(uint8_t txb_mask);
int can_readrxb(uint8_t rxb_mask, uint8_t *data, int data_length);




#endif /* CAN_H_ */