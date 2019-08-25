/*
 * gpioport.h
 *
 * Created: 25.08.2019 16:47:53
 *  Author: Dmitry
 */ 


#ifndef GPIOPORT_H_
#define GPIOPORT_H_

#include <stdint.h>

// Configuration
#define SDA_PORT	PORTD
#define SCL_PORT	PORTD
#define SDA_DDR		DDRD
#define SCL_DDR		DDRD
#define SDA_IN		PIND
#define SCL_IN		PIND
#define SDA_PIN		PD3
#define SCL_PIN		PD2

#define GPIO_FRAME_DATA_SIZE 12



typedef void (*gpio_callback_t)(uint8_t src, uint8_t dst, uint8_t cmd, uint8_t data[GPIO_FRAME_DATA_SIZE]);

typedef struct gpio_frame_struct
{
	uint8_t addr_src;
	uint8_t addr_dst;
	uint8_t command;
	
	uint8_t data[GPIO_FRAME_DATA_SIZE];
	
	uint8_t crc;
		
} gpio_frame_t;

void gpioport_init();
void gpioport_set_callback(gpio_callback_t);

void gpioport_send_raw(gpio_frame_t frame);
void gpioport_send(uint8_t dst, uint8_t cmd, uint8_t data[GPIO_FRAME_DATA_SIZE]);



#endif /* GPIOPORT_H_ */