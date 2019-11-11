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


#include "UartLink.h"
#include "CAN/can.h"
#include "can_commands.h"

#define CAN_FIRM_VERS		1

#define LED_PORT		PORTC
#define LED_PIN			PC5
#define LED_DDR			DDRC

#define EOL() while(!(UCSR0A & (1 << UDRE0))); UDR0 = '\n';

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

void print_binary(char ch)
{
	for(int i = 0; i < 8; i++)
	{
		while(!(UCSR0A & (1 << UDRE0)))
		{
			
		}
		
		if(ch & (1 << 7))
		{
			UDR0 = '1';
		}
		else
		{
			UDR0 = '0';
		}
		
		ch = ch << 1;
	}
}

void print_16(uint16_t ch)
{
	for(int i = 0; i < 16; i++)
	{
		while(!(UCSR0A & (1 << UDRE0)))
		{
			
		}
		
		if(ch & (1 << 15))
		{
			UDR0 = '1';
		}
		else
		{
			UDR0 = '0';
		}
		
		ch = ch << 1;
	}
}

void received_handler(char ch)
{
	if(ch == 'S')
	{
		ULink_send_info("No sleep mode :<\n");
	}
	else if(ch == 'T')
	{
		ULink_send_info("Can testing command received\n");
		uint8_t status = can_read_status();
				
		ULink_send_info("Status reading finished\n");
		ULink_send_info("");
		print_binary(status);
		while(!(UCSR0A & (1 << UDRE0)))
		{
			
		}
		UDR0 = '\n';		
		
		uint8_t canctrl;
		can_read(CAN_REG_CANCTRL, &canctrl, 1);
		ULink_send_info("CANCTRL register: ");
		print_binary(canctrl);
		while(!(UCSR0A & (1 << UDRE0)))
		{
			
		}
		UDR0 = '\n';
	}
	else if('F')
	{
		ULink_send_info("Testing CAN message sending and receiving. SID: \n");
		
		uint8_t data[] = {'a', 'b', 'c', 'd', 'e', 'f'};
		
		uint16_t sid = 551UL;
		
		print_16(sid);
		while(!(UCSR0A & (1 << UDRE0)))
		{
			
		}
		UDR0 = '\n';
		
		can_load_tx0_buffer(sid, data, 6);
		can_rts(CAN_RTS_TXB0);
		
		ULink_send_info("TX initiated. Wait for interrupt!\n");
	}
}

void uart_log(char* message)
{
	ULink_send_info(message);
	while(!(UCSR0A & (1 << UDRE0)));
	UDR0 = '\n';
}

void do_blink()
{
	LED_PORT |= (1 << LED_PIN);
	_delay_ms(50);
	LED_PORT &= ~(1 << LED_PIN);
	_delay_ms(1000);
}

void enable_power_reduction()
{
	ADCSRA &= ~(ADEN);	// Turning off ADC
	
	PRR =
	(1 << PRTWI)	|	// Disabling IIC
	(1 << PRTIM2)	|	// Disabling Timer2
	(1 << PRTIM0)	|	// Disabling Timer0
	(1 << PRTIM1)	|	// Disabling Timer1
	//(1 << PRSPI)	|	// Disabling SPI
	(1 << PRADC);		// Disabling ADC
}

ISR(INT0_vect)
{
	_can_int0_handler();
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
	
	//uint8_t *data = package + CAN_OFFSET_DATA;
	
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
		response[CAN_OFFSET_DATA - CAN_PAYLOAD_OFFSET] = 128;
		response[CAN_OFFSET_DATA + 1  - CAN_PAYLOAD_OFFSET] = 0;
		response[CAN_OFFSET_DATA + 2  - CAN_PAYLOAD_OFFSET] = 0;
		
	}	
	else if(comdh == CAN_CMD_RESET)
	{
		reset_handler();
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
	
	device_sid = eeprom_read_word(0);
	
	//enable_power_reduction();
	
	ULink_set_received_handler(received_handler);
	ULink_set_reset_request_handler(reset_handler);
	ULink_init();
	
	// Can initialization
	can_set_logger(uart_log);
	can_set_callback(can_data_received);
	can_init(CAN_MCU_INT_EN);
	
	// Set loopback mode
	//set_loopback_mode();
		
    while (1) 
    {
		do_blink();
    }
}

