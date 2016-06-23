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

void main_monitor()
{
#ifdef TEST_MONITOR_1	
	test_monitor_1();
#endif
#ifdef TEST_MONITOR_2	
	test_monitor_2();
#endif
	printf("TEST COMPLETED\n");
}

