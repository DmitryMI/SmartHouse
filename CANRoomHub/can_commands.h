/*
 * mandatory_commands.h
 *
 * Created: 12.10.2019 16:54:20
 *  Author: Dmitry
 */

/*
0		1		2		3		4		5		6		7
-----------------------------------------------------------------
| ADDRH | ADDRL | COMDH | COMDL | DATA0 | DATA1 | DATA2 | DATA3 |
-----------------------------------------------------------------
*/ 


#ifndef MANDATORY_COMMANDS_H_
#define MANDATORY_COMMANDS_H_

#define CAN_PAYLOAD_OFFSET		5

#define CAN_OFFSET_ADDRH		CAN_PAYLOAD_OFFSET	
#define CAN_OFFSET_ADDRL		CAN_PAYLOAD_OFFSET + 1	
#define CAN_OFFSET_COMDH		CAN_PAYLOAD_OFFSET + 2
#define CAN_OFFSET_COMDL		CAN_PAYLOAD_OFFSET + 3
#define CAN_OFFSET_DATA			CAN_PAYLOAD_OFFSET + 4


#define CAN_CMD_WHOIS		0x20		// COMDH: 0010 0000		COMDL: 0000 0000
#define CAN_CMD_ACK			0x30		// COMDH: 0011 0000		COMDL: 0000 0000
#define CAN_CMD_RESET		0x40		// COMDH: 0100 0000		COMDL: 0000 0000
#define CAN_CMD_PROG_FLASH	0x51		// COMDH: 0101 0001		COMDL: xxxx xxxx - page number
#define CAN_CMD_PROG_EPROM	0x52		// COMDH: 0101 0010		COMDL: xxxx xxxx - page number
#define CAN_CMD_PROG_INFOR	0x53		// COMDH: 0101 0011		COMDL: 0000 0000
#define CAN_CMD_UNSUPPORTED	0xF0		// COMDH: 1111 0000		COMDL: 0000 0000

#define ROOM_CMD_ACTIVATE	0x11		// COMDH: 0001 0001		COMDL: xxxx xxxx - HubCode		DATA0: 0000 000x - state
#define ROOM_CMD_DEACTIVATE	0x12		// COMDH: 0001 0010		COMDL: xxxx xxxx - HubCode		DATA0: 0000 000x - state
#define ROOM_CMD_GET_STATE	0x13		// COMDH: 0001 0011		COMDL: xxxx xxxx - HubCode		DATA0: 0000 000x - state
#define ROOM_RET_STATE		0x14		// COMDH: 0001 0100		COMDL: xxxx xxxx - HubCode		DATA0: 0000 000x - state


#endif /* MANDATORY_COMMANDS_H_ */