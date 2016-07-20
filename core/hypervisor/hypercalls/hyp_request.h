#ifndef _HYP_REQUEST_H_
#define _HYP_REQUEST_H_

typedef struct hypercall_request_ {
	int  hypercall;
  	uint32_t curr_l1_base_add;
  	union {
		struct {
			addr_t l1_base_pa_add;
		} switch_L1Pt;

		struct {
			addr_t l1_base_pa_add;
		} create_L1Pt;

		struct {
			addr_t va;
			addr_t l2_base_pa_add;
			uint32_t attrs;
		} map_L1Pt_pt;

		struct {
			addr_t va;
			addr_t sec_base_add;
			uint32_t attrs;
		} map_L1Pt_section;

		struct {
			addr_t  va;
		} unmap_L1Pt_entry;

		struct {
			addr_t l1_base_pa_add;
		} free_L1Pt;

		struct {
			addr_t l2_base_pa_add;
		} create_L2Pt;

		struct {
			addr_t l2_base_pa_add;
			uint32_t l2_idx;
			addr_t page_pa_add;
			uint32_t attrs;
		} map_L2Pt_entry;

		struct {
			addr_t l2_base_pa_add;
			uint32_t l2_idx;
		} unmap_L2Pt_entry;

		struct {
			addr_t l2_base_pa_add;
		} free_L2Pt;
	};
} hypercall_request_t;

hypercall_request_t request_switch_L1Pt(addr_t l1_base_pa_add);
hypercall_request_t request_create_L1Pt(addr_t l1_base_pa_add);
hypercall_request_t request_map_L1Pt_pt(addr_t va, addr_t l2_base_pa_add, uint32_t attrs);
hypercall_request_t request_free_L1Pt(addr_t l1_base_pa_add);
hypercall_request_t request_free_L2Pt(addr_t l2_base_pa_add);
hypercall_request_t request_unmap_L1Pt_entry (addr_t  va);
hypercall_request_t request_create_L2Pt(addr_t l2_base_pa_add);
hypercall_request_t request_map_L2Pt_entry(addr_t l2_base_pa_add, uint32_t l2_idx, addr_t page_pa_add, uint32_t attrs);
hypercall_request_t request_unmap_L2Pt_entry(addr_t l2_base_pa_add, uint32_t l2_idx);
hypercall_request_t request_map_L1Pt_section(addr_t va, addr_t sec_base_add, uint32_t attrs);

void push_request(hypercall_request_t request);
void change_request(hypercall_request_t request, uint32_t index);
hypercall_request_t get_request(uint32_t index);

void reset_requests(void);
void pending_requests_init(void);


#endif
