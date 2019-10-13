/*
 * CANBridge.c
 *
 * Created: 12.10.2019 17:55:19
 * Author : Dmitry
 */ 

#include "DeviceConfig.h"

#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>


#include "UartLink.h"
#include "CAN/can.h"

#define CAN_PAYLOAD_OFFSET 5


#define LED_PORT		PORTC
#define LED_PIN			PC5
#define LED_DDR			DDRC

#define EOL() while(!(UCSR0A & (1 << UDRE0))); UDR0 = '\n';

#define CAN_PACKAGE_LEN 8

uint8_t uart_package_buffer[CAN_PACKAGE_LEN];
int uart_package_pos = 0;

uint16_t device_sid = CAN_SID;

void reset_handler()
{
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


void uart_received_handler(char ch)
{
	uint8_t buffer[CAN_PACKAGE_LEN];
	
	if(ch == 'M')
	{
		UCSR0B &= ~(1 << RXCIE0);	// Disabling UART RX interrupt
		
		uint8_t sidh = ULink_receive();
		uint8_t sidl = ULink_receive();
		
		device_sid = sidh;
		device_sid = device_sid << 8;
		device_sid = sidl;

		UCSR0B |= (1 << RXCIE0);	// Enabling UART RX interrupt
	}	
	else if(ch == 'I')
	{
		UCSR0B &= ~(1 << RXCIE0);	// Disabling UART RX interrupt
		
		for(int i = 0; i < CAN_PACKAGE_LEN; i++)
		{
			buffer[i] = ULink_receive();
		}
		
		can_load_tx0_buffer(CAN_SID, buffer, CAN_PACKAGE_LEN);
		can_rts(CAN_RTS_TXB0);		
		
		UCSR0B |= (1 << RXCIE0);	// Enabling UART RX interrupt
	}
	else if(ch == 'T')
	{
		ULink_send_info("Can testing command received\n");
		uint8_t status = can_read_status();
		uint8_t caninte = 0;
		can_read(CAN_REG_CANINTE, &caninte, 1);
		
		ULink_send_info("Status: ");		
		print_binary(status);
		while(!(UCSR0A & (1 << UDRE0)))
		{
		}
		UDR0 = '\n';
		
		ULink_send_info("CANINTE: ");
		print_binary(caninte);
		while(!(UCSR0A & (1 << UDRE0)))
		{
		}
		UDR0 = '\n';
		
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
}

void do_blink()
{
	LED_PORT |= (1 << LED_PIN);
	_delay_ms(200);
	LED_PORT &= ~(1 << LED_PIN);
	_delay_ms(200);
}

void  inline enable_power_reduction()
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

void can_data_received(uint16_t sid, uint8_t *data, uint8_t data_length)
{
	ULink_send_info("");
	
	while(!(UCSR0A & (1 << UDRE0)))
	{
	}
	UDR0 = sid >> 8;
	
	while(!(UCSR0A & (1 << UDRE0)))
	{
	}
	UDR0 = sid;
	
	for(int i = 0; i < CAN_PACKAGE_LEN; i++)
	{
		while(!(UCSR0A & (1 << UDRE0)))
		{
		}
		UDR0 = data[CAN_PAYLOAD_OFFSET + i];
	}
	
	while(!(UCSR0A & (1 << UDRE0)))
	{
	}
	UDR0 = '\n';
}


int main(void)
{
	MCUSR = 0;
	wdt_disable();
	
	//enable_power_reduction();
	
	ULink_set_received_handler(uart_received_handler);
	ULink_set_reset_request_handler(reset_handler);
	ULink_init();
	
	// Can initialization
	can_set_callback(can_data_received);
	can_init(CAN_MCU_INT_EN);
	
	while (1)
	{
		do_blink();
	}
}
