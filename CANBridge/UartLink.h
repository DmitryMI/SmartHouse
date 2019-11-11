#pragma once

#include <stdlib.h>

#if defined (__AVR_ATmega8__)

#define UCSRXB	UCSRB
#define UCSRXA	UCSRA
#define RXCIEX	RXCIE
#define UDREX	UDRE
#define UDRX	UDR
#define RXCX	RXC

#elif defined (__AVR_ATmega328p__)

#define UCSRXB	UCSR0B
#define UCSRXA	UCSR0A
#define RXCIEX	RXCIE0
#define UDREX	UDRE0
#define UDRX	UDR0
#define RXCX	RXC0

#endif

#include "DeviceConfig.h"

typedef void (*received_callback_t) (char ch);
typedef void (*request_t) ();

void ULink_init();

void ULink_send_command(char* message);

void ULink_send_info(char* message);

void ULink_set_reset_request_handler(request_t handler);

void ULink_set_prog_request_handler(request_t handler);

void ULink_set_received_handler(received_callback_t handler);

char ULink_receive();