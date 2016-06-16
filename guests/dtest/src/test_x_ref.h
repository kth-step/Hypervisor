#define __PACKED __attribute__ ((packed))

typedef union dmmu_entry {
	uint32_t all;
	__PACKED struct {
		uint32_t refcnt:15;
		uint32_t x_refcnt:15;
		uint32_t type:2;
	};
} dmmu_entry_t;

void main_x_ref();

