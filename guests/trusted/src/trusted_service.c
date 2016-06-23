/*
 * trustedservice.c
 *
 *  Created on: 30 mar 2011
 *      Author: Master
 */
#include <types.h>
#include "trusted_service.h"
#include "hypercalls.h"

enum dmmu_command {
	CMD_MAP_L1_SECTION, CMD_UNMAP_L1_PT_ENTRY, CMD_CREATE_L2_PT,
	CMD_MAP_L1_PT, CMD_MAP_L2_ENTRY, CMD_UNMAP_L2_ENTRY, CMD_FREE_L2,
	CMD_CREATE_L1_PT, CMD_SWITCH_ACTIVE_L1, CMD_FREE_L1
};


uint32_t call_checker(uint32_t param3, uint32_t param1, uint32_t param2)
{
	uint32_t p0 = param3 & 0xF;
	uint32_t p3 = param3 >> 4;

	switch (p0) {

	case CMD_CREATE_L1_PT:
		if (param1 % 2 != 0)
			return -1;
	case CMD_FREE_L1:
		return 0;
	case CMD_MAP_L1_SECTION:
		return 0;
	case CMD_MAP_L1_PT:
		return 0;
	case CMD_UNMAP_L1_PT_ENTRY:
		return 0;
	case CMD_CREATE_L2_PT:
		return 0;
	case CMD_FREE_L2:
		return 0;
	case CMD_MAP_L2_ENTRY:
		return 0;
	case CMD_UNMAP_L2_ENTRY:
		return 0;
	case CMD_SWITCH_ACTIVE_L1:
		return 0;
	default:
		return 0;
	}
	
	return 0;
}

void handler_rpc(unsigned callNum, uint32_t param3, uint32_t param1, uint32_t param2)
{
	printf("Monitor invoked with parameters: 0x%x 0x%x 0x%x 0x%x\n", callNum, param3, param1, param2);
	
	//finish_rpc(param3+4);
	uint32_t res = call_checker(param3, param1, param2);
	finish_rpc(res);
}

void finish_rpc(uint32_t value)
{
	ISSUE_HYPERCALL_REG1(HYPERCALL_END_RPC, value);
}

void _main(uint32_t param)
{
	/*Initialize the heap */
	init_heap();
}
