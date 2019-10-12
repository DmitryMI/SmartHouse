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


#include "UartLink.h"
#include "CAN/can.h"

#define LED_PORT		PORTC
#define LED_PIN			PC5
#define LED_DDR			DDRC

#define EOL() while(!(UCSR0A & (1 << UDRE0))); UDR0 = '\n';

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
	_delay_ms(200);
	LED_PORT &= ~(1 << LED_PIN);
	_delay_ms(200);
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

void set_loopback_mode()
{
	uint8_t canctrl;
	can_read(CAN_REG_CANCTRL, &canctrl, 1);
	canctrl &= CAN_OPMODE_RESET;
	canctrl |= CAN_OPMODE_LOOPBACK;
	can_write(CAN_REG_CANCTRL, &canctrl, 1);
}

void can_data_received(uint16_t sid, uint8_t *data, uint8_t data_length)
{
	ULink_send_info("Data received! SID: \n");
	
	print_16(sid);
	while(!(UCSR0A & (1 << UDRE0)))
	{		
	}
	UDR0 = '\n';
	
	for(int i = 0; i < data_length; i++)
	{		
		while(!(UCSR0A & (1 << UDRE0)))
		{
		}
		UDR0 = data[CAN_PACKAGE_PAYLOAD_OFFSET + i];
	}
	
	while(!(UCSR0A & (1 << UDRE0)))
	{
	}
	UDR0 = '\n';
	
	for(int i = 0; i < CAN_PACKAGE_PAYLOAD_OFFSET + data_length; i++)
	{
		print_binary(data[i]);
		while(!(UCSR0A & (1 << UDRE0)))
		{
		}
		UDR0 = ' ';
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

