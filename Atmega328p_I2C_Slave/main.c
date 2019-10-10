/*
 * Atmega328p_I2C_Slave.c
 *
 * Created: 11.09.2019 10:08:56
 * Author : Dmitry
 */ 

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>

#define PORT_IIC	PORTC
#define PORT_IN_IIC		PINC
#define DDR_IIC		DDRC
#define PIN_SCL		PC5
#define PIN_SDA		PC4

#define DDR_LEDS1	DDRB
#define DDR_LEDS2	DDRD
#define PORT_LEDS1	PORTB
#define PORT_LEDS2	PORTD
#define PIN_LED0	PB0
#define PIN_LED1	PD7
#define PIN_LED2	PD6

typedef enum PIN_VALUE
{
	HIGH = 1, LOW = 0, NC = -1
} pin_value;

void set_leds_state(pin_value led0, pin_value led1, pin_value led2)
{
	DDR_LEDS1 |= (1 << PIN_LED0);
	DDR_LEDS2 |= (1 << PIN_LED1) | (1 << PIN_LED2);
	
	if(led0 == HIGH)
	{
		PORT_LEDS1 |= (1 << PIN_LED0);
	}
	else if (led0 == LOW)
	{
		PORT_LEDS1 &= ~(1 << PIN_LED0);
	}
	
	if(led1 == HIGH)
	{
		PORT_LEDS2 |= (1 << PIN_LED1);
	}
	else if(led1 == LOW)
	{
		PORT_LEDS2 &= ~(1 << PIN_LED1);
	}
	
	if(led2 == HIGH)
	{
		PORT_LEDS2 |= (1 << PIN_LED2);
	}
	else if(led2 == LOW)
	{
		PORT_LEDS2 &= ~(1 << PIN_LED2);
	}
}


int main(void)
{
	set_leds_state(HIGH, HIGH, HIGH);
	_delay_ms(2000);
	set_leds_state(LOW, LOW, LOW);
	
    /* Replace with your application code */
    while (1) 
    {
		if(PORT_IN_IIC & (1 << PIN_SCL) || PORT_IN_IIC & (1 << PIN_SDA) )
		{
			set_leds_state(HIGH, NC, NC);
		}
		else
		{
			set_leds_state(LOW, NC, NC);
		}
    }
}

