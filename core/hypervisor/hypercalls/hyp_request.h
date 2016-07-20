#ifndef _HYP_REQUEST_H_
#define _HYP_REQUEST_H_

typedef struct hypercall_request_ {
	int  hypercall;
  	uint32_t curr_l1_base_add;
  	union {
		struct {
			addr_t l1_base_pa_add;
		} switch_mm;

		struct {
			addr_t l1_base_pa_add;
		} create_L1_pt;

		struct {
			addr_t va;
			addr_t l2_base_pa_add;
			uint32_t attrs;
		} l1_pt_map;

		struct {
			addr_t va;
			addr_t sec_base_add;
			uint32_t attrs;
		} map_L1_section;

		struct {
			addr_t  va;
		} unmap_L1_pageTable_entry;

		struct {
			addr_t l1_base_pa_add;
		} unmap_L1_pt;

		struct {
			addr_t l2_base_pa_add;
		} create_L2_pt;

		struct {
			addr_t l2_base_pa_add;
			uint32_t l2_idx;
			addr_t page_pa_add;
			uint32_t attrs;
		} l2_map_entry;

		struct {
			addr_t l2_base_pa_add;
			uint32_t l2_idx;
		} l2_unmap_entry;

		struct {
			addr_t l2_base_pa_add;
		} unmap_L2_pt;
	};
} hypercall_request_t;

hypercall_request_t request_dmmu_switch_mm(addr_t l1_base_pa_add);
hypercall_request_t request_dmmu_create_L1_pt(addr_t l1_base_pa_add);
hypercall_request_t request_dmmu_l1_pt_map(addr_t va, addr_t l2_base_pa_add, uint32_t attrs);
hypercall_request_t request_dmmu_unmap_L1_pt(addr_t l1_base_pa_add);
hypercall_request_t request_dmmu_unmap_L2_pt(addr_t l2_base_pa_add);
hypercall_request_t request_dmmu_unmap_L1_pageTable_entry(addr_t  va);
hypercall_request_t request_dmmu_create_L2_pt(addr_t l2_base_pa_add);
hypercall_request_t request_dmmu_l2_map_entry(addr_t l2_base_pa_add, uint32_t l2_idx, addr_t page_pa_add, uint32_t attrs);
hypercall_request_t request_dmmu_l2_unmap_entry(addr_t l2_base_pa_add, uint32_t l2_idx);
hypercall_request_t request_dmmu_map_L1_section(addr_t va, addr_t sec_base_add, uint32_t attrs);

void push_request(hypercall_request_t request);
void change_request(hypercall_request_t request, uint32_t index);
hypercall_request_t get_request(uint32_t index);

void reset_requests(void);
void pending_requests_init(void);


#endif
