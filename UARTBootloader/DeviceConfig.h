/*
 * DeviceConfig.h
 *
 * Created: 09.10.2019 15:57:11
 *  Author: Dmitry
 */ 


#ifndef DEVICECONFIG_H_
#define DEVICECONFIG_H_

#define BAUD	9600

#if defined __AVR_ATmega328p__
	#define DEV_NAME	"Atmega328p"
#elif defined __AVR_ATmega8__
	#define DEV_NAME	"Atmega8"
#endif


#define FIRM_VERS	"2.0"

#define ACK			"ACK"


#endif /* DEVICECONFIG_H_ */