/*
 * trusted_service.h
 *
 *  Created on: 30 mar 2011
 *      Author: Master
 */

#ifndef TRUSTED_SERVICE_H_
#define TRUSTED_SERVICE_H_

typedef struct hyperargs_ {
	char *source;
	char *dest;
} hyperargs;

typedef struct trusted_args_contract_ {
	char contract[50000];
	int *success;
} TrustedContractArgs;

typedef struct trusted_args_sign_ {
//      char signature[128];
	char *contract;		//[50000];
	char *modulus;
	int *success;
	char *signature;	//TODO remove only for testing
} TrustedSignArgs;

#define __PACKED __attribute__ ((packed))

typedef union dmmu_entry {
	uint32_t all;
	__PACKED struct {
		uint32_t refcnt:15;
		uint32_t x_refcnt:15;
		uint32_t type:2;
	};
} dmmu_entry_t;

typedef __PACKED struct l2_small {
	uint32_t xn:1;
	uint32_t cnst:1;	// cnst = 1
	uint32_t b:1;
	uint32_t c:1;
	uint32_t ap_0_1bs:2;
	uint32_t tex:3;
	uint32_t ap_3b:1;
	uint32_t s:1;
	uint32_t ng:1;
	uint32_t addr:20;
} l2_small_t;

typedef __PACKED struct l1_sec {
	uint32_t pxn:1;
	uint32_t typ:1;
	uint32_t b:1;
	uint32_t c:1;
	uint32_t xn:1;
	uint32_t dom:4;
	uint32_t dummy:1;
	uint32_t ap_0_1bs:2;
	uint32_t tex:3;
	uint32_t ap_3b:1;
	uint32_t s:1;
	uint32_t ng:1;
	uint32_t secIndic:1;
	uint32_t ns:1;
	uint32_t addr:12;
} l1_sec_t;

#define MB_BITS 20
#define MB (1UL << MB_BITS)
#define HAL_PHYS_START (0x80000000)
#define HAL_VIRT_START (0xF0000000)
#define HAL_OFFSET ((HAL_PHYS_START)-(HAL_VIRT_START))

/* bft base and size definition */
#define DMMU_BFT_BASE_PY  (MB + HAL_PHYS_START)
#define DMMU_BFT_BASE_VA  (DMMU_BFT_BASE_PY - HAL_OFFSET)


#define SHAREDRPC_FUNCTION __attribute__((section("shared_rpc_functions")))
#define SHAREDRPC_DATA __attribute__((section("shared_rpc_data")))

#define FLASH_DATA

#define L2_BASE_MASK 0xFFFFF000
#define L2_DESC_ATTR_MASK 0x00000FFD
#define L1_SEC_DESC_MASK 0xFFF00000
#define L1_SEC_DESC_ATTR_MASK 0x000BFFFC
#define DESC_TYPE_MASK 0b11

#define PA_TO_PH_BLOCK(pa) ((pa) >> 12)
#define CREATE_L2_DESC(x, y) (L2_BASE_MASK & x) | (L2_DESC_ATTR_MASK & y) | (0b10)
#define CREATE_L1_SEC_DESC(x, y) (L1_SEC_DESC_MASK & x) | (L1_SEC_DESC_ATTR_MASK & y) | (0b10)
#define GET_L2_AP(pg_desc) ((((uint32_t) pg_desc->ap_3b) << 2) | ((uint32_t) pg_desc->ap_0_1bs))
#define GET_L1_AP(sec) ((((uint32_t) sec->ap_3b) << 2) | ((uint32_t) sec->ap_0_1bs))

#define START_PA_OF_SECTION(sec) (((uint32_t)sec->addr) << 20)
#define L1_IDX_TO_PA(l1_base, idx) ((l1_base & 0xFFFFC000) | (idx << 2))

#define GUEST_PASTART 0x87600000
#define VA_FOR_PT_ACCESS_START 0x0
#define SECTION_SIZE (0x00100000)
#define GUEST_PSIZE (0x00600000)
#define PAGE_SIZE (0x00001000)

//Errors
#define ERR_MONITOR_BLOCK_WRITABLE 64
#define ERR_MONITOR_BLOCK_EXE 65
#define ERR_MONITOR_PAGE_WRITABLE_AND_EXE 66

#define ERR_MMU_OUT_OF_RANGE_PA             (3)

#endif				/* TRUSTED_SERVICE_H_ */
