#include "types.h"
#include "hyper.h"
#include "hyp_request.h"
#include "dmmu.h"

extern virtual_machine *curr_vm;

enum dmmu_command {
	CMD_MAP_L1_SECTION, CMD_UNMAP_L1_PT_ENTRY, CMD_CREATE_L2_PT, CMD_MAP_L1_PT, CMD_MAP_L2_ENTRY, CMD_UNMAP_L2_ENTRY, CMD_FREE_L2, CMD_CREATE_L1_PT, CMD_SWITCH_ACTIVE_L1, CMD_FREE_L1
};

hypercall_request_t request_dmmu_switch_mm(addr_t l1_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_SWITCH_ACTIVE_L1;
	request.switch_mm.l1_base_pa_add = l1_base_pa_add;
	return request;
}

hypercall_request_t request_dmmu_create_L1_pt(addr_t l1_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_CREATE_L1_PT;
	request.create_L1_pt.l1_base_pa_add = l1_base_pa_add;
	return request;
}

hypercall_request_t  request_dmmu_l1_pt_map(addr_t va, addr_t l2_base_pa_add, uint32_t attrs) {
	hypercall_request_t request;
	request.hypercall = CMD_MAP_L1_PT;
	request.l1_pt_map.va = va;
	request.l1_pt_map.l2_base_pa_add = l2_base_pa_add;
	request.l1_pt_map.attrs = attrs;
	return request;
}

hypercall_request_t request_dmmu_unmap_L1_pt(addr_t l1_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_FREE_L1;
	request.unmap_L1_pt.l1_base_pa_add = l1_base_pa_add;
	return request;
}

hypercall_request_t request_dmmu_unmap_L2_pt(addr_t l2_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_FREE_L2;
	request.unmap_L2_pt.l2_base_pa_add = l2_base_pa_add;
	return request;
}

hypercall_request_t request_dmmu_unmap_L1_pageTable_entry(addr_t  va) {
	hypercall_request_t request;
	request.hypercall = CMD_UNMAP_L1_PT_ENTRY;
	request.unmap_L1_pageTable_entry.va = va;
	return request;
}

hypercall_request_t request_dmmu_create_L2_pt(addr_t l2_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_CREATE_L2_PT;
	request.create_L2_pt.l2_base_pa_add = l2_base_pa_add;
	return request;
}

hypercall_request_t request_dmmu_l2_map_entry(addr_t l2_base_pa_add, uint32_t l2_idx, addr_t page_pa_add, uint32_t attrs) {
	hypercall_request_t request;
	request.hypercall = CMD_MAP_L2_ENTRY;
	request.l2_map_entry.l2_base_pa_add = l2_base_pa_add;
	request.l2_map_entry.l2_idx = l2_idx;
	request.l2_map_entry.page_pa_add = page_pa_add;
	request.l2_map_entry.attrs = attrs;
	return request;
}

hypercall_request_t request_dmmu_l2_unmap_entry(addr_t l2_base_pa_add, uint32_t l2_idx) {
	hypercall_request_t request;
	request.hypercall = CMD_UNMAP_L2_ENTRY;
	request.l2_unmap_entry.l2_base_pa_add = l2_base_pa_add;
	request.l2_unmap_entry.l2_idx = l2_idx;
	return request;
}

hypercall_request_t request_dmmu_map_L1_section(addr_t va, addr_t sec_base_add, uint32_t attrs) {
	hypercall_request_t request;
	request.hypercall = CMD_MAP_L1_SECTION;
	request.map_L1_section.va = va;
	request.map_L1_section.sec_base_add = sec_base_add;
	request.map_L1_section.attrs = attrs;
	return request;
}


#define REQUESTS_BASE_PY  ((4*MB) + HAL_PHYS_START)
#define REQUESTS_BASE_VA  (REQUESTS_BASE_PY - HAL_OFFSET)
void pending_requests_init(void) {
}

void reset_requests(void) {
	curr_vm->pending_request_counter = 0;
	curr_vm->pending_request_index = 0;
}

//#define ADV_DEBUG_CUR_REQ
void debug_current_request() {
	if (curr_vm->pending_request_counter >= MAX_PENDING_REQUESTS)
    		return;
	hypercall_request_t * pending_requests = (hypercall_request_t *)REQUESTS_BASE_VA;
	hypercall_request_t request = pending_requests[curr_vm->pending_request_index];
	printf("Request %d\n", request.hypercall);
		
#ifdef ADV_DEBUG_CUR_REQ
	switch (request.hypercall) {

		case CMD_CREATE_L1_PT:
			printf("create l1 pt l1 base addr is: %x\n", request.create_L1_pt.l1_base_pa_add);

		//No need for checks when freeing an L1
		case CMD_FREE_L1:
			printf("free L1 base addr is: %x\n", request.unmap_L1_pt.l1_base_pa_add);

		case CMD_MAP_L1_SECTION:
			printf("map_l1_section va is: %x\nmap_l1_section sec_base_add is: %x\nmap_l1_section attrs is: %x\n  ",
			        request.map_L1_section.va, request.map_L1_section.sec_base_add, request.map_L1_section.attrs);

		case CMD_MAP_L1_PT:
			printf("Map L1 base addr is: %x\n", request.unmap_L1_pt.l1_base_pa_add);

		case CMD_UNMAP_L1_PT_ENTRY:
			printf("Unmap L1 pt entry va is: %x\n", request.unmap_L1_pageTable_entry.va);

		case CMD_CREATE_L2_PT:
			printf("Create l2 pt, l2 pa base addr is: %x\n", request.create_L2_pt.l2_base_pa_add);

		case CMD_FREE_L2:
			printf("Free L2 l2 base pa addr is: %x\n", request.unmap_L2_pt.l2_base_pa_add);

		case CMD_MAP_L2_ENTRY:
			printf("map_l2_entry l2_base pa addr is: %x\nmap_l2_entry page_pa_add is: %x\nmap_l2_entry attrs is: %x\n  ",
			        request.l2_map_entry.l2_base_pa_add, request.l2_map_entry.page_pa_add, request.l2_map_entry.attrs);	
	
		case CMD_UNMAP_L2_ENTRY:
			printf("Unmap L2 entry base pa addr is: %x\n", request.l2_unmap_entry.l2_base_pa_add);

		case CMD_SWITCH_ACTIVE_L1:
			printf("Switch l1 base pa addr is: %x\n", request.switch_mm.l1_base_pa_add);

		default:
			printf("Default\n");
	}
#endif
}

void push_request(hypercall_request_t request) {
	if (curr_vm->pending_request_counter >= MAX_PENDING_REQUESTS)
    		return;
	hypercall_request_t * pending_requests = (hypercall_request_t *)REQUESTS_BASE_VA;
	pending_requests[curr_vm->pending_request_counter] = request;
	curr_vm->pending_request_counter++;
}

void change_request(hypercall_request_t request, uint32_t index) {
	if (index >= curr_vm->pending_request_counter)
    		return;
	hypercall_request_t * pending_requests = (hypercall_request_t *)REQUESTS_BASE_VA;
	pending_requests[index] = request;
}

hypercall_request_t get_request(uint32_t index) {
	hypercall_request_t request;
	if (index >= curr_vm->pending_request_counter)
		return request;
	hypercall_request_t * pending_requests = (hypercall_request_t *)REQUESTS_BASE_VA;
	return pending_requests[index];
}

uint32_t execute_next_request() {
	hypercall_request_t * pending_requests = (hypercall_request_t *)REQUESTS_BASE_VA;
	if (curr_vm->pending_request_index >= curr_vm->pending_request_counter)
		return 0;
	uint32_t res = 0;
	hypercall_request_t request = pending_requests[curr_vm->pending_request_index];
	switch(request.hypercall) {
		case CMD_SWITCH_ACTIVE_L1:
			res = dmmu_switch_mm(request.switch_mm.l1_base_pa_add);
			break;
		case CMD_CREATE_L1_PT:
			res = dmmu_create_L1_pt(request.create_L1_pt.l1_base_pa_add);
			break;
		case CMD_MAP_L1_PT:
			res = dmmu_l1_pt_map(request.l1_pt_map.va,
			               request.l1_pt_map.l2_base_pa_add,
			               request.l1_pt_map.attrs);
			break;
		case CMD_MAP_L1_SECTION:
			res = dmmu_map_L1_section(request.map_L1_section.va,
			                      request.map_L1_section.sec_base_add,
			                      request.map_L1_section.attrs);
			break;
		case CMD_UNMAP_L1_PT_ENTRY:
			res = dmmu_unmap_L1_pageTable_entry(request.unmap_L1_pageTable_entry.va);
			break;
		case CMD_FREE_L1:
			res = dmmu_unmap_L1_pt(request.unmap_L1_pt.l1_base_pa_add);
			break;
		case CMD_CREATE_L2_PT:
			res = dmmu_create_L2_pt(request.create_L2_pt.l2_base_pa_add);
			break;
		case CMD_MAP_L2_ENTRY:
			res = dmmu_l2_map_entry(request.l2_map_entry.l2_base_pa_add,
			                    request.l2_map_entry.l2_idx,
			                    request.l2_map_entry.page_pa_add,
			                    request.l2_map_entry.attrs);
			break;
		case CMD_UNMAP_L2_ENTRY:
			res = dmmu_l2_unmap_entry(request.l2_unmap_entry.l2_base_pa_add,
			                    request.l2_unmap_entry.l2_idx);
			break;
		case CMD_FREE_L2:
			res = dmmu_unmap_L2_pt(request.unmap_L2_pt.l2_base_pa_add);
			break;
	}
	curr_vm->pending_request_index++; 
	return res;
}


uint32_t execute_all_requests() {
	uint32_t req_index;
	uint32_t res = 0;
	for (req_index = curr_vm->pending_request_index; req_index < curr_vm->pending_request_counter ; req_index++)
	{
		uint32_t res1 = execute_next_request();
		if (res1 != 0){
			if (res == 0)
				res = res1;
			printf("Index of failed request is: %d\n", req_index);
		}
	}
	reset_requests();
	return res;
}


void dmmu_handler_2(uint32_t p03, uint32_t p1, uint32_t p2)
{
	uint32_t p0 = p03 & 0xF;
	uint32_t p3 = p03 >> 4;

	switch(p0) {
		case CMD_CREATE_L1_PT:
			push_request(request_dmmu_create_L1_pt(p1));
			break;
		case CMD_MAP_L1_SECTION:
			push_request(request_dmmu_map_L1_section(p1,p2,p3));
			break;
		case CMD_MAP_L1_PT:
			push_request(request_dmmu_l1_pt_map(p1, p2, p3));
			break;
		case CMD_UNMAP_L1_PT_ENTRY:
			push_request(request_dmmu_unmap_L1_pageTable_entry(p1));
			break;
		case CMD_FREE_L1:
			push_request(request_dmmu_unmap_L1_pt(p1));
			break;
		case CMD_CREATE_L2_PT:
			push_request(request_dmmu_create_L2_pt(p1));
			break;
		case CMD_MAP_L2_ENTRY:
			p3 = p03 & 0xFFFFFFF0;
			uint32_t idx = p2 >> 20;
			uint32_t attrs = p2 & 0xFFF;
			push_request(request_dmmu_l2_map_entry(p1, idx, p3, attrs));
			break;
		case CMD_UNMAP_L2_ENTRY:
			push_request(request_dmmu_l2_unmap_entry(p1, p2));
			break;
		case CMD_FREE_L2:
			push_request(request_dmmu_unmap_L2_pt(p1));
			break;
		case CMD_SWITCH_ACTIVE_L1:
			push_request(request_dmmu_switch_mm(p1));
			break;
		default:
			return;
	}
}
