/*
	author：water cutter
	date: 20240715
	brief: v8 aarch64, page table generating driver
*/

#include "a53_aarch64_driver.h"

extern void _init_mmu_cache();
//extern void _setup_ttb();
extern void _enable_mmu_cache();
extern void _disable_mmu_cache();
void mmu_cahche_switch(unsigned int status)
{
	switch(status){
	case MMU_CACHE_OFF:
		_init_mmu_cache();
		_disable_mmu_cache();
		break;
	case MMU_CACHE_ON:
		_init_mmu_cache();
		setup_ttb(VA4G_PA_4G_Gra4K, LV1_PT_LOC_ADDR);
		_enable_mmu_cache();
		break;
	}
}

static inline unsigned long long calFirstLvTableLen(ADDRSPACE_GRANULE_SIZE agSize)
{
	switch(agSize){
	case VA4G_PA_4G_Gra4K: return 4;
	case VA512G_PA1T_Gra4K: return 512;
	default:return 0;
	}
}

/* 4*512*2M */
static void setup_lv2_ttb(
		unsigned long long lv1ttbAddr,
		unsigned long long firstLvTableLen,
		unsigned long long lv2ttbAddr)
{
	unsigned long long lv2ttbBaseAddrs[firstLvTableLen];
	lv2ttbBaseAddrs[0] = lv2ttbAddr+512*0*8;
	lv2ttbBaseAddrs[1] = lv2ttbAddr+512*1*8;
	lv2ttbBaseAddrs[2] = lv2ttbAddr+512*2*8;
	lv2ttbBaseAddrs[3] = lv2ttbAddr+512*3*8;

	// change lv1 Desc[in lv ttb setup by setup_ttb()] from D_Block to D_Table
	unsigned long long* ttbPtr = (unsigned long long*) lv1ttbAddr;
	for(unsigned int lv1It=0; lv1It<firstLvTableLen; lv1It++){
		// keep low attr [ &((0xFFF)) ] of lv1 Desc
		ttbPtr[lv1It] = lv2ttbBaseAddrs[lv1It]|(ttbPtr[lv1It]&((0xFFF)))|TABLE_DESC; //set bit desc[1]
		unsigned long long l2EntryScale = 1<<21;
		/* add lv2 desc (D_Block type, granule 2M)
		 * change low attr [ &(~(0xFFF)) ] */
		for(unsigned int lv2It=0; lv2It<512; lv2It++){
			*(unsigned long long*)(lv2ttbBaseAddrs[lv1It] + lv2It*8) = ((lv1It*(1<<30) + l2EntryScale*lv2It)&(~(0xFFF)))|INDIFFERENCE|DEVICE_nGnRnE|BLOCK_DESC;
		}
	}
}

/* restriction: aAddr and eAddr should be 0x200000 aligned */
void set_cachebility_gran_2m(
		unsigned long long sAddr,
		unsigned long long eAddr,
		MEM_AATR_IN_TTDESC cachebility)
{
	// 2M == 2*1024*1024 == 0x200000
	unsigned long long sOrdinalnum = sAddr/0x200000;
	unsigned long long eOrdinalnum = eAddr/0x200000;
	unsigned long long* lv2PtPtr = (unsigned long long*)(LV2_PT_LOC_ADDR);
	for(unsigned long long blkIt = sOrdinalnum; blkIt<eOrdinalnum; blkIt++){
		lv2PtPtr[blkIt] = (lv2PtPtr[blkIt]&(~0xfff))|INDIFFERENCE|cachebility|BLOCK_DESC;
	}
}


/* lv1 page table, 4*1G */
void setup_ttb(
		ADDRSPACE_GRANULE_SIZE agSize,
		unsigned long long ttbBaseAddr)
{
	__asm("MSR TCR_EL3, X0");
	__asm("MSR TTBR0_EL3, X1");
	unsigned long long firstLvTableLen = calFirstLvTableLen(agSize);
	unsigned long long entryScale = 1<<30;
	for(unsigned long long i=0; i<firstLvTableLen;i++){
		/* ensure align by &~(0xFFF) */
		*(unsigned long long*)(ttbBaseAddr + i*8) = (entryScale*i)&(~(0xFFF))|INDIFFERENCE|DEVICE_nGnRnE|BLOCK_DESC;
	}
	setup_lv2_ttb(ttbBaseAddr, firstLvTableLen, LV2_PT_LOC_ADDR);
	set_cachebility_gran_2m(0x200000, 0x800000, NORMAL_WRITETHROUGH_RW_ALLOC);
	set_cachebility_gran_2m(0x200000, 0x800000, DEVICE_nGnRnE);

}

