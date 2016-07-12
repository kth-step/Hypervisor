#include <types.h>

#include "hypercalls.h"

extern uint32_t va_base;
extern addr_t pstart;

enum dmmu_command {
	CMD_MAP_L1_SECTION, CMD_UNMAP_L1_PT_ENTRY, CMD_CREATE_L2_PT,
	    CMD_MAP_L1_PT, CMD_MAP_L2_ENTRY, CMD_UNMAP_L2_ENTRY, CMD_FREE_L2,
	    CMD_CREATE_L1_PT, CMD_SWITCH_ACTIVE_L1, CMD_FREE_L1
};




extern uint32_t syscall_dmmu(uint32_t r0, uint32_t r1, uint32_t r2);
extern uint32_t syscall_make_req(uint32_t r0, uint32_t r1, uint32_t r2);
#define ISSUE_DMMU_HYPERCALL(type, p0, p1, p2) \
		syscall_dmmu(type | (p2 << 4), p0, p1);

#define ISSUE_DMMU_HYPERCALL_(type, p0, p1, p2, p3) \
		syscall_dmmu((type | (p2 & 0xFFFFFFF0)), p0, ((p1 << 20) | p3));

#define ISSUE_DMMU_QUERY_HYPERCALL(p0) \
		syscall_dmmu_query(p0);

#define ISSUE_DMMU_QUERY_HYPERCALL(p0) \
		syscall_dmmu_query(p0);

#define ISSUE_DMMU_HYPERCALL_REQ(type, p0, p1, p2) \
	       syscall_make_req(type | (p2 << 4), p0, p1);

#define ISSUE_DMMU_HYPERCALL_REQ_(type, p0, p1, p2, p3) \
	       syscall_make_req((type | (p2 & 0xFFFFFFF0)), p0, ((p1 << 20) | p3));

#define ISSUE_DMMU_HYPERCALL_END_REQ() \
	       syscall_end_req();

#define __PACKED __attribute__ ((packed))

typedef union dmmu_entry {
	uint32_t all;
	__PACKED struct {
		uint32_t refcnt:13;
		uint32_t x_refcnt:13;
		uint32_t dev_refcnt:4;
		uint32_t type:2;
	};
} dmmu_entry_t;

extern uint32_t va_base;
extern addr_t pstart;

extern uint32_t syscall_monitor(uint32_t r0, uint32_t r1, uint32_t r2);
extern uint32_t syscall_dmmu(uint32_t r0, uint32_t r1, uint32_t r2);
//extern uint32_t syscall_dmmu_2(uint32_t r0, uint32_t r1, uint32_t r2);

#define MAX_PENDING_REQUESTS (10*1024)
void test_dmmu_api_2_push()
{
	// we use the base_address + 1MB
	uint32_t va;
	uint32_t pa;
	uint32_t* res;
	
	va = va_base + 1 * 0x100000;
	pa = pstart + 1 * 0x100000;
		
	//Unmapping 0xC0010000
	ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	res = ISSUE_DMMU_HYPERCALL_END_REQ();
	
	uint32_t i = 0;
	for (i = 0; i < 3; i++)
	{
		printf("After exiting result is: %d\n", *(res+i));
	}
	
}

void test_dmmu_api_2_push_exe()
{
	uint32_t va;
	uint32_t pa;
	uint32_t res;
	dmmu_entry_t bft_entry;

	va = (va_base + 0x100000);
	pa = pstart + 1 * 0x100000;

	ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	//expect(1, "Unmapping a valid writable page", 0, res);


	//attrs = 0xc10 => Writable. Not executable. Should succeed
	ISSUE_DMMU_HYPERCALL_REQ(CMD_MAP_L1_SECTION, va, pa, 0xc10);
	//expect(3, "Mapping a valid writable page", 0, res);
	
	//Trying to write in the memory region just set as writable
	*(int*)(va) = 7;
	//expect(3, "The value is: ", 7, *(int*) (va));	
	
	va = va + 0x200000;
	ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	//expect(1, "Unmapping a valid writable page", 0, res);
	
	//attrs = 0x800. Read only, executable. Mapping a section to a writable memory region and trying to make it exe.
	//Should fail since the writable ref counter is greater than 0.
	ISSUE_DMMU_HYPERCALL_REQ(CMD_MAP_L1_SECTION, va, pa, 0x800);
	//expect(3, "Mapping a section to a writable memory region and setting it executable.", 64, res);
	
	ISSUE_DMMU_HYPERCALL_END_REQ();	
}

void test_req_list_create_empty_l2()
{
	// we use the base_address + 1MB
	uint32_t va = (va_base + 1 * 0x100000);
	uint32_t pa;
	uint32_t i;
	uint32_t res;
	for (i=0; i<1024; i++) {
		uint32_t va1 = (va + i * 4);
		*((uint32_t*)va1) = 0;
	}


	//Trying to create a L2 pt in a referenced region
	pa = pstart + 1 * 0x100000;
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L2_PT, pa, 0, 0);
	//expect(1, "Creating the L2 in a referenced region", 21, res);

	//Unmapping 0xC0010000
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	//expect(3, "Unmap 0xC0010000 that was mapping the L2", 0, res);
	
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_MAP_L1_PT, va, pa, 0);
	//expect(5, "Mapping 0xC01... using the L2 just created but still typed D", 12, res);

	//Creating a L2 page table at address pa
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L2_PT, pa, 0, 0);
	//expect(6, "Creating the L2 in a non-referenced region", 0, res);

	//Mapping 0xC0010000 to the L2 page table just creaed
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_MAP_L1_PT, va, pa, 0);
	//expect(7, "Mapping 0xC01... using the L2 just created", 0, res);
	
	//end of L2 page table creation.
	//****************************************************
	uint32_t pga = pstart + 1 * 0x110000;
	uint32_t pa2 = pa + 1 * 0x100000;
	res = ISSUE_DMMU_HYPERCALL_REQ_(CMD_MAP_L2_ENTRY, pa2, 0, pga, 0);
	//expect(10, "Mapping a valid physical address into one of a non valid L2",
	//       12, res);

	ISSUE_DMMU_HYPERCALL_END_REQ();
}

void test_req_list_create_l1_pt()
{
	uint32_t va = (va_base + 1 * 0x100000);
	uint32_t pa;
	uint32_t i;
	uint32_t res;


	pa = pstart + 1 * 0x100000;
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L1_PT, pa, 0, 0);
	//expect(0, "Creating an L1 on referenced part of the memory", 
	//       21, res);

	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	//expect(0, "Unmap 0xC0010000 that was mapping the L1", 0, res);
	
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L1_PT, pa, 0, 0);
	//expect(0, "Creating an L1 on a non referenced part of the memory", 
	//      0, res);
	
	//Create another L1 page table
	uint32_t pa2 = pstart + 1 * 0x300000;
	uint32_t va2 = va_base + 1 * 0x300000;
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L1_PT, pa2, 0, 0);
	//expect(0, "Creating an L1 on a non referenced part of the memory", 
	//       21, res);

	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va2, 0, 0);
	//expect(0, "Unmap 0xC0010000 that was mapping the L1", 0, res);

	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L1_PT, pa2, 0, 0);
	//expect(0, "Creating an L1 on a non referenced part of the memory", 
	//       0, res);

	ISSUE_DMMU_HYPERCALL_END_REQ();		
}

void test_req_list_l1_pt_map() {
	uint32_t va = (va_base + 1 * 0x100000);
	uint32_t pa;
	uint32_t i;
	uint32_t res;

	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	//expect(0, "Unmap 0xC0100000 that was mapping the L1", 0, res);
	
	//Create an L1
	pa = pstart + 1 * 0x100000;
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L1_PT, pa, 0, 0);
	//expect(0, "Creating an L1 on a non referenced part of the memory", 
	//       0, res);

	//Create an L2 to map
	pa = pstart + 1 * 0x110000;
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L2_PT, pa, 0, 0);
	//expect(0, "Successful creation of a new L2", 0, res);

	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_MAP_L1_PT, va, pa, 0);
	//expect(0, "Mapping again the vm to the L1 page just created", 
	//       0, res);

	pa = pstart + 1 * 0x200000;
	//Mapping the L1 to a region which is not L2 page table
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_MAP_L1_PT, va, pa, 0);
	//expect(0, "Mapping again the vm to the L1 page just created", 
	//       12, res);

	//Unmapping a region of memory
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va + 0x200000, 0, 0);
	//expect(0, "Unmap 0xC0300000 that was mapping the L1", 0, res);

	//Mapping a region which is not L1 to an L2
	pa = pstart + 1 * 0x110000;
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_MAP_L1_PT, va + 0x200000, pa, 0);
	//expect(0, "Mapping a region which is not L1 to an L2", 
	//       0, res);

	ISSUE_DMMU_HYPERCALL_END_REQ();
}


void test_req_list_map_l2_w_nx()
{
	// we use the base_address + 1MB
	uint32_t va = (va_base + 1 * 0x100000);
	uint32_t pa;
	uint32_t i;
	uint32_t res;
	for (i=0; i<1024; i++) {
		uint32_t va1 = (va + i * 4);
		*((uint32_t*)va1) = 0;
	}

	pa = pstart + 1 * 0x100000;

	//Unmapping 0xC0010000
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	//expect(1, "Unmap 0xC0010000 that was mapping the L2", 0, res);

	//Creating a L2 page table at address pa
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L2_PT, pa, 0, 0);
	//expect(2, "Creating the L2 in a non-referenced region", 0, res);

	//Mapping 0xC0010000 to the L2 page table just creaed
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_MAP_L1_PT, va, pa, 0);
	//expect(3, "Mapping 0xC01... using the L2 just created", 0, res);
	
	//****************************************************

	//The address where the L2 is going to be mapped to
	uint32_t pga = pstart + 1 * 0x110000;

	//attrs = 0x31 => Only writable.
	res = ISSUE_DMMU_HYPERCALL_REQ_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x31);
	//expect(4, "Mapping a valid physical address into one of a valid L2",
	//       0, res);
	
	//attrs = 0x20 => Executable. Read only. Trying to map a physical address, with a writable ref counter of 1
	// into one of an L2 and setting nx bit to 0 => Making the memory region executable. Should Fail.
	res = ISSUE_DMMU_HYPERCALL_REQ_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x20);
	//expect(6, "Mapping a writable memory region(ref cnt > 0) to an L2 pt entry and trying to make it executable",
	//       64, res);
	
	uint32_t va2 = va + 0x200000;
	uint32_t pa2 = pa + 0x200000;
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_UNMAP_L1_PT_ENTRY, va2, 0, 0);
	//expect(7, "Unmap 0xC0010000 that was mapping the L2", 0, res);

	//Creating a L2 page table at address pa2
	res = ISSUE_DMMU_HYPERCALL_REQ(CMD_CREATE_L2_PT, pa2, 0, 0);
	//expect(8, "Creating the L2 in a non-referenced region", 0, res);

	//attrs = 0x31 => Only writable, non executable. Should succeed.
	res = ISSUE_DMMU_HYPERCALL_REQ_(CMD_MAP_L2_ENTRY, pa2, 0, pga, 0x31);
	//expect(9, "Mapping a valid physical address into one of a valid L2",
	//       0, res);

	ISSUE_DMMU_HYPERCALL_END_REQ();
}



void main_dmmu_api_2()
{
#ifdef TEST_DMMU_API_2_PUSH	
	test_dmmu_api_2_push();
#endif
#ifdef TEST_DMMU_API_2_PUSH_EXE	
	test_dmmu_api_2_push_exe();
#endif
#ifdef TEST_REQ_LIST_CREATE_EMPTY_L2	
	test_req_list_create_empty_l2();
#endif
#ifdef TEST_REQ_LIST_CREATE_L1_PT	
	test_req_list_create_l1_pt();
#endif
#ifdef TEST_REQ_LIST_L1_PT_MAP	
	test_req_list_l1_pt_map();
#endif
#ifdef TEST_REQ_LIST_MAP_L2_W_NX	
	test_req_list_map_l2_w_nx();
#endif

printf("TEST COMPLETED\n");

}

