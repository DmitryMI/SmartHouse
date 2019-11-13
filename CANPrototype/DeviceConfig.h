/*
 * DeviceConfig.h
 *
 * Created: 09.10.2019 15:57:11
 *  Author: Dmitry
 */ 


#ifndef DEVICECONFIG_H_
#define DEVICECONFIG_H_

#define F_CPU		8000000UL
#define DEV_NAME	"Atmega328P CANPrototype"
#define FIRM_VERS	"1.0.0.0"

#define ACK			"ACK_FIRMWARE"

#if defined (__AVR_ATmega328P__)

	#define INT0_ENABLE		EIMSK |= (1 << INT0);

	#define TIMER_ENABLE	TIMSK1 |= (1 << TOIE1); \
	TCCR1B |= (1 << CS12);

	#define EXIT_BOOT		asm("jmp 0");

	#define SET_VECTORS		MCUCR = (1 << IVCE);	\
	MCUCR = (1 << IVSEL);

	#define RESET_VECTORS	MCUCR = (1 << IVCE);	\
	MCUCR = 0;

#elif defined (__AVR_ATmega8__)

	#define INT0_ENABLE		GICR |= (1 << INT0);

	#define TIMER_ENABLE	TIMSK |= (1 << TOIE1); \
	TCCR1B |= (1 << CS12);

	#define EXIT_BOOT		asm("rjmp app_start");

	#define SET_VECTORS		GICR = (1 << IVCE);	\
	GICR = (1 << IVSEL);

	#define RESET_VECTORS	GICR = (1 << IVCE);	\
	GICR = 0;

#else
# warning Device not supported
#endif


#endif /* DEVICECONFIG_H_ */