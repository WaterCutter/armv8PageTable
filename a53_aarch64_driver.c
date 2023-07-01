/*
 * Author		HeHy
 * Date			20230216
 * Brief		cache&mmu related c interface,
 * 				lv1/lv2/lv3 page tables total consume 4K+16K+16K=36K space
 * */

/*
 * ------------------------------------------------------------------------
 *
 *
 * */
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

/* if and only if cache ram in 4 part
 * 4*512*4K */
static void setup_lv3_ttb()
{
	unsigned long long sAddrs[4] = {
			RAM_P0_BEGIN_ADDR,
			RAM_P1_BEGIN_ADDR,
			RAM_P2_BEGIN_ADDR,
			RAM_P3_BEGIN_ADDR};
	for(unsigned long long ramIt = 0; ramIt<4; ramIt++){
		// find the 2M area the ram belongs to
		unsigned long long sOrdinalnum = sAddrs[ramIt]/0x200000;
		unsigned long long* lv2PtPtr = (unsigned long long*)(LV2_PT_LOC_ADDR);
		// change lv2 page table desc from D_Block to D_Table
		unsigned long long lv3PgAddr = (LV3_PT_LOC_ADDR + ramIt*512*8);
		lv2PtPtr[sOrdinalnum] = lv3PgAddr|(lv2PtPtr[sOrdinalnum]&(0xfff))|TABLE_DESC;
		// add lv3 desc to lv3 table loaction
		for(unsigned long long lv3It = 0; lv3It < 512; lv3It++){
			*(unsigned long long*) (lv3PgAddr + lv3It*8) = (sAddrs[ramIt] + 0x1000*lv3It)&(~(0xFFF))|INDIFFERENCE|DEVICE_nGnRnE|LV3_PAGE_DESC;
		}
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

/*
 lv3 page table (4k granule, each for 2M address space):
 ---------------------------------------------------------------------
 	 |		P0		|		P1		|		P2		|		P3		|
 ---------------------------------------------------------------------
 page table size: 4*512*8 = 16KB

 different with lv1 and lv2 page tables, need mapping design
*/
void set_cachebility_gran_4k(
		unsigned long long part,
		unsigned long long sAddr,
		unsigned long long eAddr,
		MEM_AATR_IN_TTDESC cachebility)
{

	unsigned long long beginAddrs[4] = {
				RAM_P0_BEGIN_ADDR,
				RAM_P1_BEGIN_ADDR,
				RAM_P2_BEGIN_ADDR,
				RAM_P3_BEGIN_ADDR};
	/* find ram the area belongs to */
	unsigned long long baseAddr = beginAddrs[part];
	unsigned long long sOrdinalnum = (sAddr-baseAddr)/0x1000;
	unsigned long long eOrdinalnum = (eAddr-baseAddr)/0x1000;
	unsigned long long* lv3PtPtr = (unsigned long long*)(LV3_PT_LOC_ADDR+part*0x1000);
	/* change attr in lv3 page table */
	for(unsigned long long pgIt = sOrdinalnum; pgIt<eOrdinalnum; pgIt++){
		lv3PtPtr[pgIt] = (lv3PtPtr[pgIt]&(~0xfff))|INDIFFERENCE|cachebility|LV3_PAGE_DESC;
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
	setup_lv3_ttb();
	set_cachebility_gran_2m(0x200000, 0x800000, NORMAL_WRITETHROUGH_RW_ALLOC);
	set_cachebility_gran_2m(0x200000, 0x800000, DEVICE_nGnRnE);
	set_cachebility_gran_4k(3,RAM_P3_BEGIN_ADDR,RAM_P3_BEGIN_ADDR+0x4000,NORMAL_WRITETHROUGH_RW_ALLOC);

}

