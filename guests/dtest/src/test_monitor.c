#include <types.h>

#include "hypercalls.h"

extern uint32_t va_base;
extern addr_t pstart;

extern uint32_t syscall_monitor(uint32_t r0, uint32_t r1, uint32_t r2);
extern uint32_t syscall_dmmu(uint32_t r0, uint32_t r1, uint32_t r2);

void test_monitor_1()
{
	int res = 0;
	int x = 0+5;
	expect(0, "I add 0 to a number", 5, x);
	res = syscall_dmmu(6,7,132);
	expect(1, "I add 4 to the parameter", 10, res);
	printf("Hello test 1\n");
}

void main_monitor()
{
#ifdef TEST_MONITOR_1	
	test_monitor_1();
#endif
	printf("TEST COMPLETED\n");
}

