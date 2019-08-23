/*
 * gpioport_comm.h
 *
 * Created: 23.08.2019 14:42:04
 *  Author: Dmitry
 */ 


#ifndef GPIOPORT_COMM_H_
#define GPIOPORT_COMM_H_

#define		WHOIS	0x00	// Request of device info

#define		RESET	0x10	// Order to reboot

#define		UPDST	0x21	// Firmware update start
#define		UPDEN	0x22	// Firmware update end
#define		UPDCO	0x23	// Firmware update continue

#define		RESPS	0x30	// Single-frame response

#define		SPCFC	0xFF	// Specific command

#endif /* GPIOPORT_COMM_H_ */