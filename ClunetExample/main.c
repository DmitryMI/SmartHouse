#define F_CPU 1000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "../ClunetLib/clunet/clunet.h"

void data_received(unsigned char src_address, unsigned char dst_address, unsigned char command, char* data, unsigned char size)
{
	
}

int main (void)
{
	clunet_init();
	clunet_set_on_data_received(data_received);
	sei();

	char buffer[1];
	buffer[0] = 1;
	clunet_send(CLUNET_BROADCAST_ADDRESS, CLUNET_PRIORITY_MESSAGE, CLUNET_COMMAND_DEVICE_POWER_INFO, buffer, sizeof(buffer));
	
	while (1)
	{
		clunet_send(CLUNET_BROADCAST_ADDRESS, CLUNET_PRIORITY_MESSAGE, CLUNET_COMMAND_DEVICE_POWER_INFO, buffer, sizeof(buffer));
		//_delay_ms(1000);
	}
	return 0;
}