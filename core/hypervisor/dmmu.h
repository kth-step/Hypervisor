
#ifndef _DMMU_H_
#define _DMMU_H_

// Disabling aggressive flushing
//#define AGGRESSIVE_FLUSHING_HANDLERS

/* bft base and size definition */
#define DMMU_BFT_BASE_PY  (MB + HAL_PHYS_START)
#define DMMU_BFT_BASE_VA  (DMMU_BFT_BASE_PY - HAL_OFFSET)

#define DMMU_BFT_COUNT (1024 * 1024)
#define DMMU_BFT_SIZE  (DMMU_BFT_COUNT * sizeof(dmmu_entry_t))

#define __PACKED __attribute__ ((packed))

// To check if all the page tables are allocated form the region that is always chackable or not
//#define DEBUG_DMMU_CACHEABILITY_CHECKERS
//#define CHECK_PAGETABLES_CACHEABILITY

/* bft entry type */
enum dmmu_entry_type {
	DMMU_TYPE_DATA, DMMU_TYPE_L1PT, DMMU_TYPE_L2PT, DMMU_TYPE_INVALID
};

/* a single bft table */
typedef union dmmu_entry {
	uint32_t all;
	__PACKED struct {
		uint32_t refcnt:13;
		uint32_t x_refcnt:13;
		uint32_t dev_refcnt:4;
		uint32_t type:2;
	};
} dmmu_entry_t;

typedef __PACKED struct l1_pt {
	uint32_t typ:2;
	uint32_t pxn:1;
	uint32_t ns:1;
	uint32_t sbz:1;
	uint32_t dom:4;
	uint32_t dummy:1;
	uint32_t addr:22;
} l1_pt_t;

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

/* Error messages */
#define SUCCESS_MMU                 (0)
#define ERR_MMU_RESERVED_VA                 (1)
#define ERR_MMU_ENTRY_UNMAPPED 		        (2)
#define ERR_MMU_OUT_OF_RANGE_PA             (3)
#define ERR_MMU_SECTION_NOT_UNMAPPED        (4)
#define ERR_MMU_PH_BLOCK_NOT_WRITABLE       (5)
#define ERR_MMU_AP_UNSUPPORTED              (6)
#define ERR_MMU_BASE_ADDRESS_IS_NOT_ALIGNED (7)
#define ERR_MMU_ALREADY_L1_PT               (8)
#define ERR_MMU_ALREADY_L2_PT               (8)
#define ERR_MMU_SANITY_CHECK_FAILED         (9)
#define ERR_MMU_PT_REGION				    (10)
#define ERR_MMU_NO_UPDATE                   (11)
#define ERR_MMU_IS_NOT_L2_PT                (12)
#define ERR_MMU_XN_BIT_IS_ON                (13)
#define ERR_MMU_PT_NOT_UNMAPPED             (14)
#define ERR_MMU_REF_OVERFLOW                (15)
#define ERR_MMU_INCOMPATIBLE_AP             (16)
#define ERR_MMU_L2_UNSUPPORTED_DESC_TYPE    (17)
#define ERR_MMU_REFERENCE_L2                (18)
#define ERR_MMU_L1_BASE_IS_NOT_16KB_ALIGNED (19)
#define ERR_MMU_IS_NOT_L1_PT                (20)
#define ERR_MMU_REFERENCED				    (21)
#define ERR_MMU_FREE_ACTIVE_L1				(22)
#define ERR_MMU_SUPERSECTION				(23)
#define ERR_MMU_NEW_L1_NOW_WRITABLE			(24)
#define ERR_MMU_L2_BASE_OUT_OF_RANGE        (25)
#define ERR_MMU_NOT_CACHEABLE               (26)
#define ERR_MMU_OUT_OF_CACHEABLE_RANGE      (27)
#define ERR_MMU_NEW_L2_NOW_WRITABLE			(28)
#define ERR_MMU_X_REF_OVERFLOW                (29)
#define ERR_MMU_UNIMPLEMENTED               (-1)

#define PAGE_INFO_TYPE_DATA 0
#define PAGE_INFO_TYPE_L1PT 1
#define PAGE_INFO_TYPE_L2PT 2
#define PAGE_INFO_TYPE_INVALID 3

/* in tranelate.c */
int mmu_lookup_guest(addr_t vadr, addr_t * padr, int user_write);
int mmu_lookup_hv(addr_t vadr, addr_t * padr, int hv_write);
addr_t mmu_guest_pa_to_va(addr_t padr, hc_config * config);
void mmu_bft_region_set(addr_t start, size_t size, uint32_t refc, uint32_t x_refc,
			uint32_t typ);

#define DESC_TYPE_MASK 0b11
#define UNMAPPED_ENTRY 0

#define L1_SEC_DESC_MASK 0xFFF00000
#define L1_SEC_DESC_ATTR_MASK 0x000BFFFC
#define L1_BASE_MASK 0xFFFFC000
#define L2_BASE_MASK 0xFFFFF000
#define L1_PT_DESC_MASK 0xFFFFFC00
#define L1_PT_DESC_ATTR_MASK 0x000003FC
#define SECTION_SIZE (0x00100000)
#define PAGE_SIZE (0x00001000)
#define MAX_13BIT 8191
#define MAX_REFCNT MAX_13BIT
#define L2_DESC_ATTR_MASK 0x00000FFD

#define VA_TO_L1_IDX(va) (va >> 20)
#define L1_IDX_TO_PA(l1_base, idx) ((l1_base & 0xFFFFC000) | (idx << 2))
#define L2_IDX_TO_PA(l2_base, idx) ((l2_base & 0xFFFFF000) | ((0xFFF & idx) << 2))

#define L1_TYPE(l1_desc) (l1_desc & DESC_TYPE_MASK)

#define UNMAP_L1_ENTRY(l1_desc) (l1_desc & 0xfffffffc)
#define UNMAP_L2_ENTRY(l2_desc) (l2_desc & 0xfffffffc)
#define CREATE_L1_SEC_DESC(x, y) (L1_SEC_DESC_MASK & x) | (L1_SEC_DESC_ATTR_MASK & y) | (0b10)
#define CREATE_L1_PT_DESC(x, y) (L1_PT_DESC_MASK & x) | (L1_PT_DESC_ATTR_MASK & y) | (0x01)
#define CREATE_L2_DESC(x, y) (L2_BASE_MASK & x) | (L2_DESC_ATTR_MASK & y) | (0b10)
#define GET_L1_AP(sec) ((((uint32_t) sec->ap_3b) << 2) | ((uint32_t) sec->ap_0_1bs))
// old version taking a flat 32 bits
//#define GET_L2_AP(attrs) ((attrs >> 7) & 0b100) | (((attrs >> 4) & 0b11))
#define GET_L2_AP(pg_desc) ((((uint32_t) pg_desc->ap_3b) << 2) | ((uint32_t) pg_desc->ap_0_1bs))
#define L1_DESC_PXN(x) ((x & 0x4) >> 2)
#define L2_DESC_PA(l2_base_add, l2_idx) (l2_base_add | (l2_idx << 2) | 0)

#define START_PA_OF_SECTION(sec) (((uint32_t)sec->addr) << 20)
#define START_PA_OF_SPT(pt) (((uint32_t)pt->addr) << 12)
#define PA_OF_POINTED_PT(pt) (((uint32_t)pt->addr) << 10)

#define PA_TO_PH_BLOCK(pa) ((pa) >> 12)
#define START_PA_OF_BLOCK(pa) ((pa) << 12)
#define PT_PA_TO_PH_BLOCK(pa) ((pa) >> 2)

/* in tranelate.c */
int mmu_lookup_guest(addr_t vadr, addr_t * padr, int user_write);
int mmu_lookup_hv(addr_t vadr, addr_t * padr, int hv_write);

/*Function prototypes*/
dmmu_entry_t *get_bft_entry_by_block_idx(addr_t ph_block); /* Used by networking code in soc_cpsw.c */
int dmmu_switch_mm(addr_t l1_base_pa_add);

int dmmu_create_L1_pt(addr_t l1_base_pa_add);
int dmmu_l1_pt_map(addr_t va, addr_t l2_base_pa_add, uint32_t attrs);
int dmmu_unmap_L1_pt(addr_t l1_base_pa_add);
uint32_t dmmu_unmap_L1_pageTable_entry(addr_t va);

uint32_t dmmu_create_L2_pt(addr_t l2_base_pa_add);
int dmmu_l2_map_entry(addr_t l2_base_pa_add, uint32_t l2_idx,
		      addr_t page_pa_add, uint32_t attrs);
int dmmu_l2_unmap_entry(addr_t l2_base_pa_add, uint32_t l2_idx);
int dmmu_unmap_L2_pt(addr_t l2_base_pa_add);

#endif				/* _DMMU_H_ */
