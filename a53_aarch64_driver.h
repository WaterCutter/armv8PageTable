/*
 * Author		HeHy
 * Date			20230216
 * Brief
 * */

#ifndef _A53_DRIVER_H

/* page tables location */
#define LV1_PT_LOC_ADDR (0x0004020000)
#define LV2_PT_LOC_ADDR (LV1_PT_LOC_ADDR + 0x1000)
#define LV3_PT_LOC_ADDR (LV2_PT_LOC_ADDR + 512*4*8)

/* 4k granule size area */
#define RAM_SIZE (256*1024)//0x100000
#define RAM_P0_BEGIN_ADDR (0x10000000)
#define RAM_P0_END_ADDR (0x100FFFFF)
#define RAM_P1_BEGIN_ADDR (0x20000000)
#define RAM_P1_END_ADDR (0x200FFFFF)
#define RAM_P2_BEGIN_ADDR (0x30000000)
#define RAM_P2_END_ADDR (0x300FFFFF)
#define RAM_P3_BEGIN_ADDR (0x4000000)
#define RAM_P3_END_ADDR (0x100FFFFF)

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
