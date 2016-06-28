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
#define ISSUE_DMMU_HYPERCALL(type, p0, p1, p2) \
		syscall_dmmu(type | (p2 << 4), p0, p1);

#define ISSUE_DMMU_HYPERCALL_(type, p0, p1, p2, p3) \
		syscall_dmmu((type | (p2 & 0xFFFFFFF0)), p0, ((p1 << 20) | p3));

#define ISSUE_DMMU_QUERY_HYPERCALL(p0) \
		syscall_dmmu_query(p0);

#define __PACKED __attribute__ ((packed))
typedef union dmmu_entry {
	uint32_t all;
	__PACKED struct {
		uint32_t refcnt:15;
		uint32_t x_refcnt:15;
		uint32_t type:2;
	};
} dmmu_entry_t;

extern uint32_t va_base;
extern addr_t pstart;

extern uint32_t syscall_monitor(uint32_t r0, uint32_t r1, uint32_t r2);
//extern uint32_t syscall_dmmu(uint32_t r0, uint32_t r1, uint32_t r2);

void test_monitor_1()
{
	int res = 0;
	int x = 0+5;
	expect(0, "I add 0 to a number", 5, x);
	res = syscall_monitor(6,7,132);
	expect(1, "I add 4 to the parameter", 10, res);
	printf("Hello test 1\n");
}

void test_monitor_2()
{
	uint32_t va;
	uint32_t pa;
	uint32_t i;
	uint32_t res;

	pa = 0x0;
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L1_PT, pa, 0, 0);
	expect(0, "Creating L1 in PA outside the allowed guest memory",
	      3, res);

	va = va_base + 1 * 0x100000;
	pa = pstart + 1 * 0x100000;

	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmapping a valid writable page", 0, res);

	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_SECTION, va, pa, 0xc2e);
	expect(2, "Mapping a valid writable page", 0, res);
	
}

void test_monitor_3()
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
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmap 0xC0010000 that was mapping the L2", 0, res);

	//Creating a L2 page table at address pa
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L2_PT, pa, 0, 0);
	expect(2, "Creating the L2 in a non-referenced region", 0, res);

	//Mapping 0xC0010000 to the L2 page table just creaed
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_PT, va, pa, 0);
	expect(3, "Mapping 0xC01... using the L2 just created", 0, res);
	//end of L2 page table creation.
	//****************************************************

	//uint32_t  va2 = (va_base + 1 * 0x110000);
	//res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va2, 0, 0);
	//expect(9, "Unmapping the memory area where the created L2 is going to be mapped to", 0,
	//       res);
	
	uint32_t pga = pstart + 1 * 0x110000;
	//attrs = 0x31. Making the page address both writable and executable. Should fail.
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x31);
	expect(4, "Mapping a valid physical address into one of a valid L2",
	       -3, res);

	//Making it only writable. Should succeed.
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x30);
	expect(5, "Mapping a valid physical address into one of a valid L2",
	       0, res);

	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x30);
	expect(6, "Mapping a valid physical address into one of a valid L2",
	       0, res);

	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x21);
	expect(7, "Mapping a valid physical address into one of a valid L2",
	       1, res);
}

//******************************************************************************

void test_map_l2_w_nx()
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
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmap 0xC0010000 that was mapping the L2", 0, res);

	//Creating a L2 page table at address pa
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L2_PT, pa, 0, 0);
	expect(2, "Creating the L2 in a non-referenced region", 0, res);

	//Mapping 0xC0010000 to the L2 page table just creaed
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_PT, va, pa, 0);
	expect(3, "Mapping 0xC01... using the L2 just created", 0, res);
	//end of L2 page table creation.
	//****************************************************

	//The address where the L2 is going to be mapped to
	uint32_t pga = pstart + 1 * 0x110000;

	//attrs = 0x31 => Only writable.
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x31);
	expect(4, "Mapping a valid physical address into one of a valid L2",
	       0, res);
	res = ISSUE_DMMU_QUERY_HYPERCALL(pga);
	dmmu_entry_t bft_entry = (dmmu_entry_t) res;
	expect(5, "Reference counter", 1, bft_entry.refcnt);
	expect(5, "Executable reference counter", 0, bft_entry.x_refcnt);
	expect(5, "Type", 0, bft_entry.type);
	
	//attrs = 0x20 => Executable. Read only. Trying to map a physical address, with a writable ref counter of 1
	// into one of an L2 and setting nx bit to 0 => Making the memory region executable. Should Fail.
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x20);
	expect(6, "Mapping a writable memory region(ref cnt > 0) to an L2 pt entry and trying to make it executable",
	       64, res);
	
	uint32_t va2 = va + 0x200000;
	uint32_t pa2 = pa + 0x200000;
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va2, 0, 0);
	expect(7, "Unmap 0xC0010000 that was mapping the L2", 0, res);

	//Creating a L2 page table at address pa2
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L2_PT, pa2, 0, 0);
	expect(8, "Creating the L2 in a non-referenced region", 0, res);

	//attrs = 0x31 => Only writable, non executable. Should succeed.
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa2, 0, pga, 0x31);
	expect(9, "Mapping a valid physical address into one of a valid L2",
	       0, res);

	res = ISSUE_DMMU_QUERY_HYPERCALL(pga);
	bft_entry = (dmmu_entry_t) res;
	expect(10, "Reference counter", 2, bft_entry.refcnt);
	expect(10, "Executable reference counter", 0, bft_entry.x_refcnt);
	expect(10, "Type", 0, bft_entry.type);

}

void test_map_l2_nw_x()
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
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmap 0xC0010000 that was mapping the L2", 0, res);

	//Creating a L2 page table at address pa
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L2_PT, pa, 0, 0);
	expect(2, "Creating the L2 in a non-referenced region", 0, res);

	//Mapping 0xC0010000 to the L2 page table just creaed
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_PT, va, pa, 0);
	expect(3, "Mapping 0xC01... using the L2 just created", 0, res);
	//end of L2 page table creation.
	//****************************************************

	uint32_t pga = pstart + 1 * 0x110000;

	//attrs = 0x20 => The memory region at pga made: Read only. Executable.
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x20);
	expect(4, "Mapping a valid physical address into one of a valid L2",
	       0, res);
	res = ISSUE_DMMU_QUERY_HYPERCALL(pga);
	dmmu_entry_t bft_entry = (dmmu_entry_t) res;
	expect(5, "Reference counter", 0, bft_entry.refcnt);
	expect(5, "Executable reference counter", 1, bft_entry.x_refcnt);
	expect(5, "Type", 0, bft_entry.type);
	
	//attrs = 0x31 => Writable, Non executable. Trying to map a physical address, with an exe ref counter of 1
	// into one of an L2 and setting ap bit to 11(RW) => Making the memory region writable. Should Fail.
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x31);
	expect(6, "Mapping an exe memory region(x_ref_cnt > 0) to an L2 pt entry and trying to make it writable",
	       65, res);
	
	uint32_t va2 = va + 0x200000;
	uint32_t pa2 = pa + 0x200000;
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va2, 0, 0);
	expect(7, "Unmap 0xC0010000 that was mapping the L2", 0, res);

	//Creating a L2 page table at address pa2
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L2_PT, pa2, 0, 0);
	expect(8, "Creating the L2 in a non-referenced region", 0, res);

	//attrs = 0x20 => Executable, read only. Should succeed.
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa2, 0, pga, 0x20);
	expect(9, "Mapping a valid physical address into one of a valid L2",
	       0, res);

	res = ISSUE_DMMU_QUERY_HYPERCALL(pga);
	bft_entry = (dmmu_entry_t) res;
	expect(10, "Reference counter", 0, bft_entry.refcnt);
	expect(10, "Executable reference counter", 2, bft_entry.x_refcnt);
	expect(10, "Type", 0, bft_entry.type);

}

void test_map_l2_w_x()
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
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmap 0xC0010000 that was mapping the L2", 0, res);

	//Creating a L2 page table at address pa
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L2_PT, pa, 0, 0);
	expect(2, "Creating the L2 in a non-referenced region", 0, res);

	//Mapping 0xC0010000 to the L2 page table just creaed
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_PT, va, pa, 0);
	expect(3, "Mapping 0xC01... using the L2 just created", 0, res);
	//end of L2 page table creation.
	//****************************************************

	uint32_t pga = pstart + 1 * 0x110000;

	//attrs = 0x30 => Writable. Executable. Should fail
	res = ISSUE_DMMU_HYPERCALL_(CMD_MAP_L2_ENTRY, pa, 0, pga, 0x30);
	expect(4, "Mapping a valid physical address into one of a valid L2 and making the \
	       memory region both writable and exe.", 66, res);
}

//***************************************************************************

void test_map_l1_section_w_nx()
{
	uint32_t va;
	uint32_t pa;
	uint32_t res;
	dmmu_entry_t bft_entry;

	va = (va_base + 0x100000);
	pa = pstart + 1 * 0x100000;

	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmapping a valid writable page", 0, res);

	res = ISSUE_DMMU_QUERY_HYPERCALL(pa);
	bft_entry = (dmmu_entry_t) res;
	expect(2, "Reference counter", 0, bft_entry.refcnt);
	expect(2, "Executable reference counter", 0, bft_entry.x_refcnt);
	expect(2, "Type", 0, bft_entry.type);


	//attrs = 0xc10 => Writable. Not executable. Should succeed
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_SECTION, va, pa, 0xc10);
	expect(3, "Mapping a valid writable page", 0, res);
	
	res = ISSUE_DMMU_QUERY_HYPERCALL(pa);
	bft_entry = (dmmu_entry_t) res;
	expect(4, "Reference counter", 1, bft_entry.refcnt);
	expect(4, "Executable reference counter", 0, bft_entry.x_refcnt);
	expect(4, "Type", 0, bft_entry.type);
	
	va = va + 0x200000;
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmapping a valid writable page", 0, res);
	
	//attrs = 0x800. Read only, executable. Mapping a section to a writable memory region and trying to make it exe.
	//Should fail since the writable ref counter is greater than 0.
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_SECTION, va, pa, 0x800);
	expect(3, "Mapping a section to a writable memory region and setting it executable.", 64, res);
	
}

void test_map_l1_section_nw_x()
{
	uint32_t va;
	uint32_t pa;
	uint32_t res;
	dmmu_entry_t bft_entry;

	va = (va_base + 0x100000);
	pa = pstart + 1 * 0x100000;

	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmapping a valid writable page", 0, res);

	res = ISSUE_DMMU_QUERY_HYPERCALL(pa);
	bft_entry = (dmmu_entry_t) res;
	expect(2, "Reference counter", 0, bft_entry.refcnt);
	expect(2, "Executable reference counter", 0, bft_entry.x_refcnt);
	expect(2, "Type", 0, bft_entry.type);


	//attrs = 0x800 => Read only. Executable. Should succeed
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_SECTION, va, pa, 0x800);
	expect(3, "Mapping a valid writable page", 0, res);
	
	res = ISSUE_DMMU_QUERY_HYPERCALL(pa);
	bft_entry = (dmmu_entry_t) res;
	expect(4, "Reference counter", 0, bft_entry.refcnt);
	expect(4, "Executable reference counter", 1, bft_entry.x_refcnt);
	expect(4, "Type", 0, bft_entry.type);
	
	va = va + 0x200000;
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(5, "Unmapping a valid writable page", 0, res);
	
	//attrs = 0xc10. Writable. Not executable. Mapping a section to an exe memory region and trying to make it writable.
	//Should fail since the exe ref counter is greater than 0.
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_SECTION, va, pa, 0xc10);
	expect(6, "Mapping a section to an exe memory region and setting it writable.", 65, res);
	
}

void test_map_l1_section_w_x()
{
	uint32_t va;
	uint32_t pa;
	uint32_t res;
	dmmu_entry_t bft_entry;

	va = (va_base + 0x100000);
	pa = pstart + 1 * 0x100000;

	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(1, "Unmapping a valid writable page", 0, res);

	res = ISSUE_DMMU_QUERY_HYPERCALL(pa);
	bft_entry = (dmmu_entry_t) res;
	expect(2, "Reference counter", 0, bft_entry.refcnt);
	expect(2, "Executable reference counter", 0, bft_entry.x_refcnt);
	expect(2, "Type", 0, bft_entry.type);


	//attrs = 0xc00 => Writable. Executable. Trying to map a section to a memory region
	//and make it both writable and executable. Should fail.
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_SECTION, va, pa, 0xc00);
	expect(3, "Mapping a section to a memory region and setting it both \
	       writable and executable.", 66, res);
	
}

//***********************************************************************************************

void test_create_l1_pt_w_nx()
{
	uint32_t va = (va_base + 1 * 0x100000);
	uint32_t pa;
	uint32_t res;
	dmmu_entry_t bft_entry;

	pa = pstart + 1 * 0x100000;
	uint32_t pa2 = pa + 0x200000;
	uint32_t va2 = va + 0x200000;

	//(pa & 0xFFF00000) is the physical address where I am going to map a section. 0x802 are the attributes. 
	//=> The memory region where this section is going to be mapped to is read only and executable.
	//(pa & 0xFFF00000) | 0x802 => is the descriptor of a section
	*(int*)(va) = (pa2 & 0xFFF00000) | 0x802;
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(0, "Unmap 0xC0010000 that was mapping the L1", 0, res);

	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va2, 0, 0);
	expect(1, "Unmap 0xC0010000 that was mapping the L1", 0, res);

	//I map a section with attributes 0xc10 => Writable, non executable
	//to the address pa.
	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_SECTION, va2, pa2, 0xc10);
	expect(2, "Mapping a valid writable page", 0, res);

	//Trying to create a L1 page table at the address pa. 
	//va is the corresponding va of this pa. Previously we have written a section entry
	//at this address. The attrs for this section mapping were set to 0x802=>RO, exe. 
	//The pa where this section is to be mapped is set to the same pa as the section we mapped before
	//That memory region was previously made writable so this test should fail.
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L1_PT, pa, 0, 0);
	expect(3, "Creating an L1 on a non referenced part of the memory \
	       and having an entry with the exe bit set and corresponding to \
	       a section mapped to a writable memory region.", 
	       64, res);

	res = ISSUE_DMMU_QUERY_HYPERCALL(pa2);
	bft_entry = (dmmu_entry_t) res;
	expect(4, "Reference counter", 1, bft_entry.refcnt);
	expect(4, "Executable reference counter", 0, bft_entry.x_refcnt);
	expect(4, "Type", 0, bft_entry.type);

}

void test_create_l1_pt_nw_x()
{
	uint32_t va = (va_base + 1 * 0x100000);
	uint32_t pa;
	uint32_t res;
	dmmu_entry_t bft_entry;

	pa = pstart + 1 * 0x100000;
	uint32_t pa2 = pa + 0x200000;
	uint32_t va2 = va + 0x200000;

	*(int*)(va) = (pa2 & 0xFFF00000) | 0xc12;
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(0, "Unmap 0xC0010000 that was mapping the L1", 0, res);

	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va2, 0, 0);
	expect(1, "Unmap 0xC0010000 that was mapping the L1", 0, res);

	res = ISSUE_DMMU_HYPERCALL(CMD_MAP_L1_SECTION, va2, pa2, 0x800);
	expect(2, "Mapping a valid writable page", 0, res);

	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L1_PT, pa, 0, 0);
	expect(3, "Creating an L1 on a non referenced part of the memory \
	       and having an entry with the ap set to RW and corresponding to \
	       a section mapped to an exe memory region.", 
	       65, res);

	res = ISSUE_DMMU_QUERY_HYPERCALL(pa2);
	bft_entry = (dmmu_entry_t) res;
	expect(4, "Reference counter", 0, bft_entry.refcnt);
	expect(4, "Executable reference counter", 1, bft_entry.x_refcnt);
	expect(4, "Type", 0, bft_entry.type);

}


void test_create_l1_pt_w_x()
{
	uint32_t va = (va_base + 1 * 0x100000);
	uint32_t pa;
	uint32_t i;
	uint32_t res;
	dmmu_entry_t bft_entry;

	pa = pstart + 1 * 0x100000;
	//Writing a section descriptor into the memory va. Attrs are set to 0xc02 => 
	//The memory region it maps is set to both writable and executable
	*(int*)(va) = 0xc02;
	res = ISSUE_DMMU_HYPERCALL(CMD_UNMAP_L1_PT_ENTRY, va, 0, 0);
	expect(0, "Unmap 0xC0010000 that was mapping the L1", 0, res);
	
	//Creating a L1 page table in a memory region where there is an 
	//entry corresponding to a section and having both exe bit set to 1
	//and access permissions set to RW. This test should fail.
	res = ISSUE_DMMU_HYPERCALL(CMD_CREATE_L1_PT, pa, 0, 0);
	expect(1, "Creating an L1 on a non referenced part of the memory \
	       and having a section entry with both the exe bit set and \
	       ap set to RW.", 66, res);

	res = ISSUE_DMMU_QUERY_HYPERCALL(pa);
	bft_entry = (dmmu_entry_t) res;
	expect(2, "Reference counter", 0, bft_entry.refcnt);
	expect(2, "Executable reference counter", 0, bft_entry.x_refcnt);
	expect(2, "Type", 0, bft_entry.type);

}

//***********************************************************************************


void main_monitor()
{
#ifdef TEST_MONITOR_1	
	test_monitor_1();
#endif
#ifdef TEST_MONITOR_2	
	test_monitor_2();
#endif
#ifdef TEST_MONITOR_3	
	test_monitor_3();
#endif
#ifdef TEST_MAP_L2_W_NX	
	test_map_l2_w_nx();
#endif
#ifdef TEST_MAP_L2_NW_X	
	test_map_l2_nw_x();
#endif
#ifdef TEST_MAP_L2_W_X	
	test_map_l2_w_x();
#endif
#ifdef TEST_MAP_L1_SECTION_W_NX	
	test_map_l1_section_w_nx();
#endif
#ifdef TEST_MAP_L1_SECTION_NW_X	
	test_map_l1_section_nw_x();
#endif
#ifdef TEST_MAP_L1_SECTION_W_X	
	test_map_l1_section_w_x();
#endif
#ifdef TEST_CREATE_L1_PT_W_NX	
	test_create_l1_pt_w_nx();
#endif
#ifdef TEST_CREATE_L1_PT_NW_X	
	test_create_l1_pt_nw_x();
#endif
#ifdef TEST_CREATE_L1_PT_W_X	
	test_create_l1_pt_w_x();
#endif
	printf("TEST COMPLETED\n");
}

