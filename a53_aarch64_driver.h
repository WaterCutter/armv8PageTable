/*
	author：water cutter
	date: 20240715
	brief: v8 aarch64, page table generating driver
*/

#ifndef _A53_DRIVER_H

/* page tables location */
#define LV1_PT_LOC_ADDR (0x0004020000)
#define LV2_PT_LOC_ADDR (LV1_PT_LOC_ADDR + 0x1000)

/* mmu switch status */
#define MMU_CACHE_OFF	(0)
#define MMU_CACHE_ON	(1)

/* Address space size and Granule size (for TCR_EL3) */
typedef enum _ADDRSPACE_GRANULE_SIZE{
	VA4G_PA_4G_Gra4K = 0x95002520,
	VA512G_PA1T_Gra4K = 0x95022519
}ADDRSPACE_GRANULE_SIZE;

typedef enum _TTB_DESC_TYPE{
	BLOCK_DESC = 0x01,
	TABLE_DESC = 0x03,
	LV3_PAGE_DESC = 0x3,
}TTB_DESC_TYPE;

/* tight to values in MAIR_EL3
 * refer to a53_aarch64_driver.s please */
typedef enum _MEM_AATR_IN_TTDESC{
	DEVICE_nGnRnE = 0x0<<2,
	NORMAL_NONCACHEABLE = 0x01<<2,
	NORMAL_WRITETHROUGH_RW_ALLOC = 0x02<<2,
	NORMAL_WRITEBACK_RW_ALLOC = 0x03<<3,
}MEM_AATR_IN_TTDESC;

typedef enum _NG_AF_SH_AP_NS{
	INDIFFERENCE = 0x74<<4,
}NG_AF_SH_AP_NS;

void setup_ttb(
		ADDRSPACE_GRANULE_SIZE agSize,
		unsigned long long ttbBaseAddr);

void mmu_cahche_switch(unsigned int status);

#endif
