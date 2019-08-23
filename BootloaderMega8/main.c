/*
 * BootloaderMega8.c
 *
 * Created: 23.08.2019 11:40:11
 * Author : Dmitry
 */ 

//#define USE_BLANK_FIRMWAVE

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#define F_CPU 1000000
#include <util/delay.h>

/*#include "../GpioPortMega8/gpioport/gpioport_mega8.h"
#include "../GpioPortMega8/gpioport/gpioport_comm.h"
#include "../GpioPortMega8/gpioport/gpioport_types.h"
#include "../GpioPortMega8/gpioport/gpioport_errs.h"*/
#include "../GpioPortMega8/gpioport/gpioport.h"

#define BOOTLOADER_LED_DIR	DDRD
#define BOOTLOADER_LED_PRT	PORTD
#define BOOTLOADER_LED_PIN	PD4

gpioport* current_port;

void boot_program_page (uint32_t page, uint8_t *buf)
{
	if(buf == NULL)
		return;
	
	uint16_t i;
	uint8_t sreg;

	// Disable interrupts.

	sreg = SREG;
	cli();
	
	eeprom_busy_wait ();

	boot_page_erase (page);
	boot_spm_busy_wait ();      // Wait until the memory is erased.

	for (i=0; i < SPM_PAGESIZE; i+=2)
	{
		// Set up little-endian word.

		uint16_t w = *buf++;
		w += (*buf++) << 8;
		
		boot_page_fill (page + i, w);
	}

	boot_page_write (page);     // Store buffer in flash page.
	boot_spm_busy_wait();       // Wait until the memory is written.

	// Reenable RWW-section again. We need this if we want to jump back
	// to the application after bootloading.

	boot_rww_enable ();

	// Re-enable interrupts (if they were ever enabled).

	SREG = sreg;
}

ISR(INT0_vect)
{
	gpioport_int_handler();
}

void gpioport_callback(gpio_frame* frame)
{
	
}

int main(void)
{			
	gpioport_setup(gpioport_callback);
	
	_delay_ms(3000);
	    
	asm
	(
		"RJMP app_start"
	);
}

