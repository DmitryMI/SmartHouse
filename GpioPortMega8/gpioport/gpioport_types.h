/*
 * GpioPortTypes.h
 *
 * Created: 23.08.2019 14:36:59
 *  Author: Dmitry
 */ 


#ifndef GPIOPORTTYPES_H_
#define GPIOPORTTYPES_H_

#define GPIO_FRAME_DATA_SIZE 12

typedef unsigned char byte;
typedef unsigned char addr;
typedef unsigned char comm;

typedef struct gpio_frame_struct
{
	addr id_src;
	addr id_dst;
	
	comm cmd;
	
	byte data[GPIO_FRAME_DATA_SIZE];
	
	byte crc;
	
} gpio_frame;


typedef void (*result_callback_t)(gpio_frame* frame); 
typedef void (*timeout_action_t)(void);

typedef struct gpioport_timer_struct
{
	unsigned long nseconds_init;
	unsigned long nseconds_left;
	unsigned long nseconds_pertick;
	timeout_action_t action;
}gpioport_timer;

enum gpioport_state_t
{
	IDLE, SENDING_HIGH, SENDING_LOW, SENDING_LAST, RECEIVING	
};

typedef struct gpioport_struct 
{
	int index_byte;
	int index_bit;

	gpioport_timer timeout_timer;
	enum gpioport_state_t state;
	
	result_callback_t callback;
	byte data_buffer[sizeof(gpio_frame)];	
} gpioport;




#endif /* GPIOPORTTYPES_H_ */