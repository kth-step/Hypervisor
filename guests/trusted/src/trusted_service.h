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
		uint32_t refcnt:13;
		uint32_t x_refcnt:13;
		uint32_t dev_refcnt:4;
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


enum dmmu_command {
	CMD_MAP_L1_SECTION, CMD_UNMAP_L1_PT_ENTRY, CMD_CREATE_L2_PT, CMD_MAP_L1_PT, CMD_MAP_L2_ENTRY, CMD_UNMAP_L2_ENTRY, CMD_FREE_L2, CMD_CREATE_L1_PT, CMD_SWITCH_ACTIVE_L1, CMD_FREE_L1
};

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
#define START_PA_OF_SPT(pt) (((uint32_t)pt->addr) << 12)
#define L1_IDX_TO_PA(l1_base, idx) ((l1_base & 0xFFFFC000) | (idx << 2))
#define L2_DESC_PA(l2_base_add, l2_idx) (l2_base_add | (l2_idx << 2) | 0)

#define GUEST_PASTART 0x81000000
#define VA_FOR_PT_ACCESS_START 0xE8000000
#define SECTION_SIZE (0x00100000)
#define GUEST_PSIZE  (0x07000000)
#define PAGE_SIZE (0x00001000)

#define REQUESTS_BASE_PY  ((4*MB) + HAL_PHYS_START)
#define REQUESTS_BASE_VA  (REQUESTS_BASE_PY - HAL_OFFSET)

//Errors
#define SUCCESS 0 	
#define ERR_MONITOR_BLOCK_WRITABLE 64
#define ERR_MONITOR_BLOCK_EXE 65
#define ERR_MONITOR_PAGE_WRITABLE_AND_EXE 66

#define ERR_MMU_OUT_OF_RANGE_PA             (3)

#endif				/* TRUSTED_SERVICE_H_ */
