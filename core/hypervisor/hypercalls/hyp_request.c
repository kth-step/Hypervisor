#include "types.h"
#include "hyper.h"
#include "hyp_request.h"
#include "dmmu.h"

extern virtual_machine *curr_vm;

enum dmmu_command {
	CMD_MAP_L1_SECTION, CMD_UNMAP_L1_PT_ENTRY, CMD_CREATE_L2_PT, CMD_MAP_L1_PT, CMD_MAP_L2_ENTRY, CMD_UNMAP_L2_ENTRY, CMD_FREE_L2, CMD_CREATE_L1_PT, CMD_SWITCH_ACTIVE_L1, CMD_FREE_L1
};

hypercall_request_t request_switch_L1Pt(addr_t l1_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_SWITCH_ACTIVE_L1;
	request.switch_L1Pt.l1_base_pa_add = l1_base_pa_add;
	return request;
}

hypercall_request_t request_create_L1Pt(addr_t l1_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_CREATE_L1_PT;
	request.create_L1Pt.l1_base_pa_add = l1_base_pa_add;
	return request;
}

hypercall_request_t request_map_L1Pt_pt(addr_t va, addr_t l2_base_pa_add, uint32_t attrs) {
	hypercall_request_t request;
	request.hypercall = CMD_MAP_L1_PT;
	request.map_L1Pt_pt.va = va;
	request.map_L1Pt_pt.l2_base_pa_add = l2_base_pa_add;
	request.map_L1Pt_pt.attrs = attrs;
	return request;
}

hypercall_request_t request_free_L1Pt(addr_t l1_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_FREE_L1;
	request.free_L1Pt.l1_base_pa_add = l1_base_pa_add;
	return request;
}

hypercall_request_t request_free_L2Pt(addr_t l2_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_FREE_L2;
	request.free_L2Pt.l2_base_pa_add = l2_base_pa_add;
	return request;
}

hypercall_request_t request_unmap_L1Pt_entry (addr_t  va) {
	hypercall_request_t request;
	request.hypercall = CMD_UNMAP_L1_PT_ENTRY;
	request.unmap_L1Pt_entry.va = va;
	return request;
}

hypercall_request_t request_create_L2Pt(addr_t l2_base_pa_add) {
	hypercall_request_t request;
	request.hypercall = CMD_CREATE_L2_PT;
	request.create_L2Pt.l2_base_pa_add = l2_base_pa_add;
	return request;
}

hypercall_request_t request_map_L2Pt_entry(addr_t l2_base_pa_add, uint32_t l2_idx, addr_t page_pa_add, uint32_t attrs) {
	hypercall_request_t request;
	request.hypercall = CMD_MAP_L2_ENTRY;
	request.map_L2Pt_entry.l2_base_pa_add = l2_base_pa_add;
	request.map_L2Pt_entry.l2_idx = l2_idx;
	request.map_L2Pt_entry.page_pa_add = page_pa_add;
	request.map_L2Pt_entry.attrs = attrs;
	return request;
}

hypercall_request_t request_unmap_L2Pt_entry(addr_t l2_base_pa_add, uint32_t l2_idx) {
	hypercall_request_t request;
	request.hypercall = CMD_UNMAP_L2_ENTRY;
	request.unmap_L2Pt_entry.l2_base_pa_add = l2_base_pa_add;
	request.unmap_L2Pt_entry.l2_idx = l2_idx;
	return request;
}

hypercall_request_t request_map_L1Pt_section(addr_t va, addr_t sec_base_add, uint32_t attrs) {
	hypercall_request_t request;
	request.hypercall = CMD_MAP_L1_SECTION;
	request.map_L1Pt_section.va = va;
	request.map_L1Pt_section.sec_base_add = sec_base_add;
	request.map_L1Pt_section.attrs = attrs;
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

void execute_next_request() {
	hypercall_request_t * pending_requests = (hypercall_request_t *)REQUESTS_BASE_VA;
	if (curr_vm->pending_request_index >= curr_vm->pending_request_counter)
		return;
	hypercall_request_t request = pending_requests[curr_vm->pending_request_index];
	switch(request.hypercall) {
		case CMD_SWITCH_ACTIVE_L1:
			dmmu_switch_mm(request.switch_L1Pt.l1_base_pa_add);
			break;
		case CMD_CREATE_L1_PT:
			dmmu_create_L1_pt(request.create_L1Pt.l1_base_pa_add);
			break;
		case CMD_MAP_L1_PT:
			dmmu_l1_pt_map(request.map_L1Pt_pt.va,
			               request.map_L1Pt_pt.l2_base_pa_add,
			               request.map_L1Pt_pt.attrs);
			break;
		case CMD_MAP_L1_SECTION:
			dmmu_map_L1_section(request.map_L1Pt_section.va,
			                      request.map_L1Pt_section.sec_base_add,
			                      request.map_L1Pt_section.attrs);
			break;
		case CMD_UNMAP_L1_PT_ENTRY:
			dmmu_unmap_L1_pageTable_entry(request.unmap_L1Pt_entry.va);
			break;
		case CMD_FREE_L1:
			dmmu_unmap_L1_pt(request.free_L1Pt.l1_base_pa_add);
			break;
		case CMD_CREATE_L2_PT:
			dmmu_create_L2_pt(request.create_L2Pt.l2_base_pa_add);
			break;
		case CMD_MAP_L2_ENTRY:
			dmmu_l2_map_entry(request.map_L2Pt_entry.l2_base_pa_add,
			                    request.map_L2Pt_entry.l2_idx,
			                    request.map_L2Pt_entry.page_pa_add,
			                    request.map_L2Pt_entry.attrs);
			break;
		case CMD_UNMAP_L2_ENTRY:
			dmmu_l2_unmap_entry(request.unmap_L2Pt_entry.l2_base_pa_add,
			                    request.unmap_L2Pt_entry.l2_idx);
			break;
		case CMD_FREE_L2:
			dmmu_unmap_L2_pt(request.free_L2Pt.l2_base_pa_add);
			break;
	}
	curr_vm->pending_request_index++; 
	return;
}


void execute_all_requests() {
	uint32_t req_index;
	for (req_index = curr_vm->pending_request_index; req_index < curr_vm->pending_request_counter ; req_index++)
	{
		execute_next_request();
	}
	reset_requests();
	return;
}


void dmmu_handler_2(uint32_t p03, uint32_t p1, uint32_t p2)
{
	uint32_t p0 = p03 & 0xF;
	uint32_t p3 = p03 >> 4;

	switch(p0) {
		case CMD_CREATE_L1_PT:
			push_request(request_create_L1Pt(p1));
			break;
		case CMD_MAP_L1_SECTION:
			push_request(request_map_L1Pt_section(p1,p2,p3));
			break;
		case CMD_MAP_L1_PT:
			push_request(request_map_L1Pt_pt(p1, p2, p3));
			break;
		case CMD_UNMAP_L1_PT_ENTRY:
			push_request(request_unmap_L1Pt_entry(p1));
			break;
		case CMD_FREE_L1:
			push_request(request_free_L1Pt(p1));
			break;
		case CMD_CREATE_L2_PT:
			push_request(request_create_L2Pt(p1));
			break;
		case CMD_MAP_L2_ENTRY:
			p3 = p03 & 0xFFFFFFF0;
			uint32_t idx = p2 >> 20;
			uint32_t attrs = p2 & 0xFFF;
			push_request(request_map_L2Pt_entry(p1, idx, p3, attrs));
			break;
		case CMD_UNMAP_L2_ENTRY:
			push_request(request_unmap_L2Pt_entry(p1, p2));
			break;
		case CMD_FREE_L2:
			push_request(request_free_L2Pt(p1));
			break;
		case CMD_SWITCH_ACTIVE_L1:
			push_request(request_switch_L1Pt(p1));
			break;
		default:
			return;
	}
}
