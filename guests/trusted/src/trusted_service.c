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

dmmu_entry_t *get_bft_entry_by_block_idx(addr_t ph_block)
{
	dmmu_entry_t *bft = (dmmu_entry_t *) DMMU_BFT_BASE_VA;
	return &bft[ph_block];
}

uint32_t map_l2_entry_checker(uint32_t param3, uint32_t param1, uint32_t param2)
{
	printf("Entered the map_l2_entry case\n");
	//The physical address to where the L2 entry is going to be mapped
	uint32_t pga = param3 & 0xFFFFFFF0;
	uint32_t idx = param2 >> 20;
	uint32_t attrs = param2 & 0xFFF;
	uint32_t new_l2_desc = CREATE_L2_DESC(pga, attrs);
	l2_small_t *pg_desc = (l2_small_t *) (&new_l2_desc);
	uint32_t ap = GET_L2_AP(pg_desc);
	uint32_t ph_block = PA_TO_PH_BLOCK(pga);

	dmmu_entry_t *bft_entry = get_bft_entry_by_block_idx(ph_block);

	if (bft_entry->refcnt > 0 && pg_desc->xn == 0)
		return ERR_MONITOR_BLOCK_WRITABLE;
	else if (bft_entry->x_refcnt > 0 && ap == 3)
		return ERR_MONITOR_BLOCK_EXE;
	else if (pg_desc->xn == 0 && ap == 3)
		return ERR_MONITOR_PAGE_WRITABLE_AND_EXE;
	return 0;
}

uint32_t map_l1_section_checker(uint32_t param3, uint32_t va, uint32_t sec_base_add)
{
	printf("Entered the map_l1_section case\n");
	uint32_t attrs = param3 >> 4;
	uint32_t l1_desc = CREATE_L1_SEC_DESC(sec_base_add, attrs);
	l1_sec_t *l1_sec_desc = (l1_sec_t *) (&l1_desc);
	uint32_t ap = GET_L1_AP(l1_sec_desc);
	
	uint32_t sec_idx;
	for (sec_idx = 0; (sec_idx < 256); sec_idx++) {
		uint32_t ph_block =
			    PA_TO_PH_BLOCK(START_PA_OF_SECTION(l1_sec_desc)) | (sec_idx);
		dmmu_entry_t *bft_entry = get_bft_entry_by_block_idx(ph_block);

		if (bft_entry->refcnt > 0 && l1_sec_desc->xn == 0)
			return ERR_MONITOR_BLOCK_WRITABLE;
		else if (bft_entry->x_refcnt > 0 && ap == 3)
			return ERR_MONITOR_BLOCK_EXE;
		else if (l1_sec_desc->xn == 0 && ap == 3)
			return ERR_MONITOR_PAGE_WRITABLE_AND_EXE;	
	}
	return 0;
}

uint32_t create_l1_pt_checker(uint32_t l1_base_pa_add)
{
	uint32_t ph_block;
	
	ph_block = PA_TO_PH_BLOCK(l1_base_pa_add);
	return 0;
}

uint32_t create_l2_pt_checker(uint32_t l2_base_pa_add)
{
	return 0;
}

uint32_t call_checker(uint32_t param3, uint32_t param1, uint32_t param2)
{
	//The type of the call
	uint32_t p0 = param3 & 0xF;

	switch (p0) {

	case CMD_CREATE_L1_PT:
		return create_l1_pt_checker(param1);

	//No need for checks when freeing an L1
	case CMD_FREE_L1:
		return 0;

	case CMD_MAP_L1_SECTION:
		return map_l1_section_checker(param3, param1, param2);

	//No need for checks when mapping an L1 page table
	case CMD_MAP_L1_PT:
		return 0;

	//No need for checks when unmapping an entry
	case CMD_UNMAP_L1_PT_ENTRY:
		return 0;

	case CMD_CREATE_L2_PT:
		return create_l2_pt_checker(param1);

	//No need for checks when freeing an L2
	case CMD_FREE_L2:
		return 0;

	case CMD_MAP_L2_ENTRY:
		return map_l2_entry_checker(param3, param1, param2);
			
	//No need for checks when unmapping an L2
	case CMD_UNMAP_L2_ENTRY:
		return 0;

	//No need for checks when switching pages
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
