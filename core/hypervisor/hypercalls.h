#ifndef HYPERCALLS_H_
#define HYPERCALLS_H_

#ifndef __ASSEMBLER__
#include "hyper.h"
#endif

/*Function prototypes*/

void hyper_panic(char *msg, uint32_t exit_code);
void hypercall_num_error(uint32_t hypercall_num);

void change_guest_mode(uint32_t mode);

void hypercall_guest_init();
void hypercall_interrupt_set(uint32_t interrupt, uint32_t op);
void hypercall_end_interrupt();

void hypercall_set_tls(uint32_t thread_id);

void hypercall_restore_linux_regs(uint32_t return_value, BOOL syscall);
void hypercall_restore_regs(uint32_t * regs);

#include "hyp_cache.h"
#include "hyp_mmu.h"

void hypercall_rpc(uint32_t rpc_op, void *arg);
void hypercall_end_rpc();
/***************************/

/**************
 // ASM macros
 **************/

#define STR(x) #x
#define HYPERCALL_NUM(n) "#"STR(n)

#define ISSUE_HYPERCALL(num)					    \
  asm volatile (						            \
		"SWI " HYPERCALL_NUM((num)) "         \n\t"	\
		 );

#define ISSUE_HYPERCALL_REG1(num, reg0)			   \
  asm volatile ("mov R0, %0 			\n\t"  	   \
		"SWI " HYPERCALL_NUM((num)) "\n\t"	       \
		::"r" (reg0) : "memory", "r0"		       \
		);

#define ISSUE_HYPERCALL_REG2(num, reg0, reg1)	   \
  asm volatile ("mov R0, %0 			\n\t"	   \
		"mov R1, %1			\n\t"		           \
		"SWI " HYPERCALL_NUM((num)) "\n\t"		   \
		::"r" (reg0), "r" (reg1) : "memory", "r0", "r1" \
		);

/*
 * Hypercalls
 *
 * *** important NOTE ***
 * If you change anything here, make sure the corresponding lines
 * in library/guest/include/hypercalls.h is also modified
 */

#define HYPERCALL_GUEST_INIT			1000

/*INTERRUPT */
#define HYPERCALL_INTERRUPT_SET     	1001
#define HYPERCALL_END_INTERRUPT     	1002
#define HYPERCALL_INTERRUPT_CTRL		1003

/*CACHE */
#define HYPERCALL_CACHE_OP				1004
#define HYPERCALL_TLB_OP				1005

/*PAGE TABLE*/
#define HYPERCALL_NEW_PGD				1006
#define HYPERCALL_FREE_PGD				1007
#define HYPERCALL_SWITCH_MM				1008
#define HYPERCALL_CREATE_SECTION    	1009
#define HYPERCALL_SET_PMD				1010
#define HYPERCALL_SET_PTE				1011

 /*CONTEXT*/
#define HYPERCALL_RESTORE_LINUX_REGS 	1012
#define HYPERCALL_RESTORE_REGS			1013
#define HYPERCALL_SET_CTX_ID			1014
#define HYPERCALL_SET_TLS_ID			1015
     /*RPC*/
#define HYPERCALL_RPC					1020
#define HYPERCALL_END_RPC				1021
     /*VFP*/
#define HYPERCALL_VFP					1022
#define HYPERCALL_DBG					1030
     /*VFP*/
#define HYPERCALL_UPDATE_MONITOR		1040
#define HYPERCALL_QUERY_BFT		1041
#define HYPERCALL_LINUX_INIT_END		1042
#endif				/* HYPERCALLS_H_ */
