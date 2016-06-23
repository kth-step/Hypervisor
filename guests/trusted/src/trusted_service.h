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

#endif				/* TRUSTED_SERVICE_H_ */
