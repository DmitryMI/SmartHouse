/*
 * CANPrototype.c
 *
 * Created: 11.10.2019 20:33:07
 * Author : Dmitry
 */

#include "DeviceConfig.h" 



#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <avr/eeprom.h>

#include "CAN/can.h"
#include "can_commands.h"

#define CAN_FIRM_VERS		1


#define LED_PORT		PORTC
#define LED_PIN			PC5
#define LED_DDR			DDRC

#define EOL() while(!(UCSR0A & (1 << UDRE0))); UDR0 = '\n';

#define DEVICE1_DDR	DDRC
#define DEVICE2_DDR	DDRC
#define DEVICE3_DDR	DDRC

#define DEVICE1_PORT	PORTC
#define DEVICE2_PORT	PORTC
#define DEVICE3_PORT	PORTC

#define DEVICE1_INDX	PC1
#define DEVICE2_INDX	PC2
#define DEVICE3_INDX	PC3


uint16_t device_sid;

void reset_handler()
{
	cli();
	do
	{
		wdt_enable(WDTO_500MS);
		for(;;)
		{
		}
	} while(0);
}


void do_blink()
{
	LED_PORT |= (1 << LED_PIN);
	_delay_ms(50);
	LED_PORT &= ~(1 << LED_PIN);
	_delay_ms(2000);
}

void enable_power_reduction()
{
	/*ADCSRA &= ~(ADEN);	// Turning off ADC
	
	PRR =
	(1 << PRTWI)	|	// Disabling IIC
	(1 << PRTIM2)	|	// Disabling Timer2
	(1 << PRTIM0)	|	// Disabling Timer0
	(1 << PRTIM1)	|	// Disabling Timer1
	//(1 << PRSPI)	|	// Disabling SPI
	(1 << PRADC);		// Disabling ADC*/
}

ISR(INT0_vect)
{
	_can_int0_handler();
}

void handle_hub_code(uint8_t cmd, uint8_t hub_code, uint8_t *response)
{
	if(cmd == ROOM_CMD_ACTIVATE)
	{
		switch(hub_code)
		{
			case 1:
			DEVICE1_PORT |= (1 << DEVICE1_INDX);
			break;
			case 2:
			DEVICE2_PORT |= (1 << DEVICE2_INDX);
			break;
			case 3:
			DEVICE3_PORT |= (1 << DEVICE3_INDX);
			break;
		}
	}
	else if(cmd == ROOM_CMD_DEACTIVATE)
	{
		switch(hub_code)
		{
			case 1:
			DEVICE1_PORT &= ~(1 << DEVICE1_INDX);
			break;
			case 2:
			DEVICE2_PORT &= ~(1 << DEVICE2_INDX);
			break;
			case 3:
			DEVICE3_PORT &= ~(1 << DEVICE3_INDX);
			break;
		}
	}
	
	switch(hub_code)
	{
		case 1:
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = (DEVICE1_PORT & (1 << DEVICE1_INDX)) != 0;
		break;
		case 2:
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = (DEVICE2_PORT & (1 << DEVICE2_INDX)) != 0;
		break;
		case 3:
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = (DEVICE3_PORT & (1 << DEVICE3_INDX)) != 0;
		break;
	}
	
	response[CAN_OFFSET_COMDH - CAN_PAYLOAD_OFFSET] = ROOM_RET_STATE;
	response[CAN_OFFSET_COMDL - CAN_PAYLOAD_OFFSET] = hub_code;
}



void inline can_resolve(uint16_t sid, uint8_t *package)
{		
	uint8_t addrh = package[CAN_OFFSET_ADDRH];
	uint8_t addrl = package[CAN_OFFSET_ADDRL];
	uint16_t addr = addrh;
	addr = addr << 8;
	addr += addrl;
	
	if(addr != 0 && addr != device_sid)
	{
		return;
	}
	
	uint8_t comdh = package[CAN_OFFSET_COMDH];
	uint8_t comdl = package[CAN_OFFSET_COMDL];
		
	uint8_t response[8] = {0};
	
	int mustRespond = 1;
	
	response[CAN_OFFSET_ADDRH - CAN_PAYLOAD_OFFSET] = sid >> 8;
	response[CAN_OFFSET_ADDRL - CAN_PAYLOAD_OFFSET] = sid;
	
	if(comdh == CAN_CMD_WHOIS)
	{
		response[CAN_OFFSET_COMDH - CAN_PAYLOAD_OFFSET] = CAN_CMD_ACK;
		response[CAN_OFFSET_COMDL - CAN_PAYLOAD_OFFSET] = CAN_FIRM_VERS;
	}
	else if(comdh == CAN_CMD_PROG_INFOR)
	{
		response[CAN_OFFSET_COMDH - CAN_PAYLOAD_OFFSET] = CAN_CMD_ACK;
		response[CAN_OFFSET_COMDL - CAN_PAYLOAD_OFFSET] = 0;
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = SPM_PAGESIZE;
		response[CAN_OFFSET_DATA + 1  - CAN_PAYLOAD_OFFSET] = 0;
		response[CAN_OFFSET_DATA + 2  - CAN_PAYLOAD_OFFSET] = 0;
		
	}	
	else if(comdh == CAN_CMD_RESET)
	{
		reset_handler();
	}
	else if(comdh == ROOM_CMD_ACTIVATE || comdh == ROOM_CMD_DEACTIVATE || comdh == ROOM_CMD_GET_STATE)
	{
		handle_hub_code(comdh, comdl, response);
	}
	else
	{
		response[CAN_OFFSET_COMDH  - CAN_PAYLOAD_OFFSET] = CAN_CMD_UNSUPPORTED;
	}
	
	if(mustRespond)
	{
		can_load_tx0_buffer(device_sid, response, 8);
		can_rts(CAN_RTS_TXB0);
	}
}

void can_data_received(uint16_t sid, uint8_t *data, uint8_t data_length)
{
	can_resolve(sid, data);
}


int main(void)
{
	MCUSR = 0;
	wdt_disable();
	
	// Setting position of reset vectors table
	RESET_VECTORS;
	
	device_sid = eeprom_read_word(0);	

	can_set_callback(can_data_received);
	can_init(CAN_MCU_INT_EN);
		
    while (1) 
    {
		do_blink();
    }
}