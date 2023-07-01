/*
	author:		HeHy
	brief:		pre configuration for a53_aarch64_driver.c/.h
*/
.set ttb0_base, 0x0004020000


.global _init_mmu_cache
.type _init_mmu_cache "function"
_init_mmu_cache:
	// Disable L1 Caches
	MRS X0, SCTLR_EL3 // Read SCTLR_EL3.
	BIC X0, X0, #(0x1 << 2) // Disable D Cache.
	MSR SCTLR_EL3, X0 // Write SCTLR_EL3.
	// Invalidate Data cache to make the code general purpose.
	// Calculate the cache size first and loop through each set +
	// way.
	MOV X0, #0x0 // X0 = Cache level
	MSR CSSELR_EL1, x0 // 0x0 for L1 Dcache 0x2 for L2 Dcache.
	MRS X4, CCSIDR_EL1 // Read Cache Size ID.
	AND X1, X4, #0x7
	ADD X1, X1, #0x4 // X1 = Cache Line Size.
	LDR X3, =0x7FFF
	AND X2, X3, X4, LSR #13 // X2 = Cache Set Number ¨C 1.
	LDR X3, =0x3FF
	AND X3, X3, X4, LSR #3 // X3 = Cache Associativity Number ¨C 1.
	CLZ W4, W3 // X4 = way position in the CISW instruction.
	MOV X5, #0 // X5 = way counter way_loop.
	mmu_way_loop:
		MOV X6, #0 // X6 = set counter set_loop.
	mmu_set_loop:
		LSL X7, X5, X4
		ORR X7, X0, X7 // Set way.
		LSL X8, X6, X1
		ORR X7, X7, X8 // Set set.
		DC cisw, X7 // Clean and Invalidate cache line.
		ADD X6, X6, #1 // Increment set counter.
		CMP X6, X2 // Last set reached yet?
		BLE mmu_set_loop // If not, iterate set_loop,
		ADD X5, X5, #1 // else, next way.
		CMP X5, X3 // Last way reached yet?
		BLE mmu_way_loop // If not, iterate way_loop.

	LDR X1, =0xFFBB4400 // ATTR0 Device-nGnRnE
						// ATTR1 Normal Non-Cacheable.
	MSR MAIR_EL3, X1 	// ATTR2 Normal Cacheable, Write-through.
						// ATTR3 Normal Cacheable, Write-back.
	RET

.global _enable_mmu_cache
.type _enable_mmu_cache "function"
_enable_mmu_cache:
	MRS X0, SCTLR_EL3
	ORR X0, X0, #(0x1 << 2) // The C bit (data cache).
	ORR X0, X0, #(0x1 << 12) // The I bit (instruction cache).
	ORR X0, X0, #0x1 // The M bit (MMU).
	MSR SCTLR_EL3, X0
	DSB SY
	ISB
	RET

.global _disable_mmu_cache
.type _disable_mmu_cache "function"
_disable_mmu_cache:
	MRS X0, SCTLR_EL3
	MOV X0, #0
	MSR SCTLR_EL3, X0
	MSR TTBR0_EL3, XZR
	DSB SY
	ISB
	RET
