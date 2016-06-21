/*
 * trustedservice.c
 *
 *  Created on: 30 mar 2011
 *      Author: Master
 */
#include <types.h>
#include "trusted_service.h"
#include "hypercalls.h"



void handler_rpc(unsigned callNum, uint32_t params)
{
	printf("Monitor invoked with parameters: 0x%x 0x%x\n", callNum, params);
	finish_rpc(params+4);
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
