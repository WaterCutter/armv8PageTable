//
	.set stack_top, 0x000403F000
	.set stack_top_el1, 0x000402F000

	.global main_app
_reset:
	B	reset_handler
.balign 0x10
reset_handler:
	//* Initialize VBAR_EL3.
	LDR X1, = el3_vector_table
	MSR VBAR_EL3, X1
//	LDR X1, = el2_vector_table
//	MSR VBAR_EL2, X1
	LDR X1, = el1_vector_table
	MSR VBAR_EL1, X1

	//* Enabling sync exception
	// Routing SERROR IRQ and FIQ
	MRS X0, SCR_EL3		// EL3
	ORR X0, X0, #(1<<3) // The EA bit.
	ORR X0, X0, #(1<<1) // The IRQ bit.
	ORR X0, X0, #(1<<2) // The FIQ bit.
	MSR SCR_EL3, X0
	MRS X0, HCR_EL2		// EL2
	ORR X0, X0, #(1<<5) // The AMO bit.
	ORR X0, X0, #(1<<4) // The IMO bit.
	ORR X0, X0, #(1<<3) // The FMO bit.
	MSR HCR_EL2, X0
	// Enable SError, IRQ and FIQ
	MSR DAIFClr, #0x7

	//* Initialize the stack pointer.
	// Using same stack in different Exception Level
	LDR X1, =stack_top
	//* Slecet sp for current Exception Level
	//*	0: SP_EL0	1: SP_ELn	default: 1
	MSR SPSel, #1
	MOV SP, X1

	//* Initializing system control registers
	MSR HCR_EL2, XZR
	LDR X1, =0x30C50838
	MSR SCTLR_EL2, X1
	MSR SCTLR_EL1, X1

_start_done:
	B main_app

/*
==========================================================================
	ELx to ELx
	----------------------------------------------------------------------
	configuration option				switch
	Target Exception Level				SPSR_ELn.M[3:0]
										0b0000 EL0t
										0b0100 EL1t
										0b0101 EL1h
										0b1000 EL2t
										0b1001 EL2h
										0b1100 EL3t
										0b1101 EL3h
	----------------------------------------------------------------------
	Execution State of target ELn		SCR_EL3/HCR_EL2.RW | SPSR_ELn.M[4]
	----------------------------------------------------------------------
	Security Mode of target Eln			SCR_EL3/HCR_EL2.NS
	----------------------------------------------------------------------

	Constrain:
		The Execution state specified in SPSR_ELx must match the
		configuration in either SCR_EL3.RW or HCR_EL2.RW, or this
		generatesan illegal exception return.

==========================================================================
202301152115 hhy*/

.global _el3_to_el1
.type _el3_to_el1, "function"
_el3_to_el1:
	// Initialize the SCTLR_EL1 register before entering EL1
	MSR SCTLR_EL1, XZR
	// Determine the EL1 Execution state
	MRS X0, SCR_EL3
	ORR X0, X0, #(1<<10) // RW, EL2 Execution state is AArch64
	// Determine the EL1 Security Mode
	AND X0, X0, #0xFFFFFFFFFFFFFFFE // NS, EL1 is Secure world
	MSR SCR_EL3, x0
	MOV X0, #0b00101
	MSR SPSR_EL3, X0 // M[4:0]=01001 EL1h, must match SCR_EL3.RW
	// Determine EL2 entry.
	ADR X0, el1_entry
	MSR ELR_EL3, X0
	ERET

//=============================================================================================================
// ELx Entry
//=============================================================================================================
el1_entry:
	LDR X1, =stack_top_el1
	MOV SP, X1
	RET

//=============================================================================================================
// Exception Vector Table for ELn(n=1,2,3)
//=============================================================================================================

.balign 0x800
el3_vector_table:
	/* ---------------------------------------------------------------------
	 * Current EL with SP_EL0 : 0x0 - 0x200
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el3_sync_exception_sp_el0:
	b	el3_sync_exception_sp_el0
el3_end_sync_exception_sp_el0:

.balign 0x80
el3_irq_sp_el0:
	b	el3_irq_sp_el0
el3_end_irq_sp_el0:

.balign 0x80
el3_fiq_sp_el0:
	b	el3_fiq_sp_el0
el3_end_fiq_sp_el0:

.balign 0x80
el3_serror_sp_el0:
	b	el3_serror_sp_el0
el3_end_serror_sp_el0:

	/* ---------------------------------------------------------------------
	 * Current EL with SP_ELx: 0x200 - 0x400
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el3_sync_exception_sp_elx:
	b	el3_sync_exception_sp_elx
el3_end_sync_exception_sp_elx:

.balign 0x80
el3_irq_sp_elx:
	b	el3_irq_sp_elx
el3_end_irq_sp_elx:

.balign 0x80
el3_fiq_sp_elx:
	b	el3_fiq_sp_elx
el3_end_fiq_sp_elx:

.balign 0x80
el3_serror_sp_elx:
	b	el3_serror_sp_elx
el3_end_serror_sp_elx:

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch64 : 0x400 - 0x600
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el3_sync_exception_aarch64:
	//b	el3_sync_exception_aarch64
	ERET
el3_end_sync_exception_aarch64:

.balign 0x80
el3_irq_aarch64:
	b	el3_irq_aarch64
el3_end_irq_aarch64:

.balign 0x80
el3_fiq_aarch64:
	b	el3_fiq_aarch64
el3_end_fiq_aarch64:

.balign 0x80
el3_serror_aarch64:
	b	el3_serror_aarch64
el3_end_serror_aarch64:

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch32 : 0x600 - 0x800
	 * ---------------------------------------------------------------------
	 */

.balign 0x80
el3_sync_exception_aarch32:
	b	el3_sync_exception_aarch32
el3_end_sync_exception_aarch32:

.balign 0x80
el3_irq_aarch32:
	b	el3_irq_aarch32
el3_end_irq_aarch32:

.balign 0x80
el3_fiq_aarch32:
	b	el3_fiq_aarch32
el3_end_fiq_aarch32:

.balign 0x80
el3_serror_aarch32:
	b	el3_serror_aarch32
el3_end_serror_aarch32:

//============================================================

.balign 0x800
el2_vector_table:
	/* ---------------------------------------------------------------------
	 * Current EL with SP_EL0 : 0x0 - 0x200
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el2_sync_exception_sp_el0:
	b	el2_sync_exception_sp_el0
el2_end_sync_exception_sp_el0:

.balign 0x80
el2_irq_sp_el0:
	b	el2_irq_sp_el0
el2_end_irq_sp_el0:

.balign 0x80
el2_fiq_sp_el0:
	b	el2_fiq_sp_el0
el2_end_fiq_sp_el0:

.balign 0x80
el2_serror_sp_el0:
	b	el2_serror_sp_el0
el2_end_serror_sp_el0:

	/* ---------------------------------------------------------------------
	 * Current EL with el2_SP_ELx: 0x200 - 0x400
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el2_sync_exception_sp_elx:
	b	el2_sync_exception_sp_elx
el2_end_sync_exception_sp_elx:

.balign 0x80
el2_irq_sp_elx:
	b	el2_irq_sp_elx
el2_end_irq_sp_elx:

.balign 0x80
el2_fiq_sp_elx:
	b	el2_fiq_sp_elx
el2_end_fiq_sp_elx:

.balign 0x80
el2_serror_sp_elx:
	b	el2_serror_sp_elx
el2_end_serror_sp_elx:

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch64 : 0x400 - 0x600
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el2_sync_exception_aarch64:
	b	el2_sync_exception_aarch64
el2_end_sync_exception_aarch64:

.balign 0x80
el2_irq_aarch64:
	b	el2_irq_aarch64
el2_end_irq_aarch64:

.balign 0x80
el2_fiq_aarch64:
	b	el2_fiq_aarch64
el2_end_fiq_aarch64:

.balign 0x80
el2_serror_aarch64:
	b	el2_serror_aarch64
el2_end_serror_aarch64:

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch32 : 0x600 - 0x800
	 * ---------------------------------------------------------------------
	 */

.balign 0x80
el2_sync_exception_aarch32:
	b	el2_sync_exception_aarch32
el2_end_sync_exception_aarch32:

.balign 0x80
el2_irq_aarch32:
	b	el2_irq_aarch32
el2_end_irq_aarch32:

.balign 0x80
el2_fiq_aarch32:
	b	el2_fiq_aarch32
el2_end_fiq_aarch32:

.balign 0x80
el2_serror_aarch32:
	b	el2_serror_aarch32
el2_end_serror_aarch32:

//======================================================

.balign 0x800
el1_vector_table:
	/* ---------------------------------------------------------------------
	 * Current EL with SP_EL0 : 0x0 - 0x200
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el1_sync_exception_sp_el0:
	b	el1_sync_exception_sp_el0
el1_end_sync_exception_sp_el0:

.balign 0x80
el1_irq_sp_el0:
	b	el1_irq_sp_el0
el1_end_irq_sp_el0:

.balign 0x80
el1_fiq_sp_el0:
	b	el1_fiq_sp_el0
el1_end_fiq_sp_el0:

.balign 0x80
el1_serror_sp_el0:
	b	el1_serror_sp_el0
el1_end_serror_sp_el0:

	/* ---------------------------------------------------------------------
	 * Current EL with el1_SP_ELx: 0x200 - 0x400
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el1_sync_exception_sp_elx:
	b	el1_sync_exception_sp_elx
el1_end_sync_exception_sp_elx:

.balign 0x80
el1_irq_sp_elx:
	b	el1_irq_sp_elx
el1_end_irq_sp_elx:

.balign 0x80
el1_fiq_sp_elx:
	b	el1_fiq_sp_elx
el1_end_fiq_sp_elx:

.balign 0x80
el1_serror_sp_elx:
	b	el1_serror_sp_elx
el1_end_serror_sp_elx:

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch64 : 0x400 - 0x600
	 * ---------------------------------------------------------------------
	 */
.balign 0x80
el1_sync_exception_aarch64:
	b	el1_sync_exception_aarch64
el1_end_sync_exception_aarch64:

.balign 0x80
el1_irq_aarch64:
	b	el1_irq_aarch64
el1_end_irq_aarch64:

.balign 0x80
el1_fiq_aarch64:
	b	el1_fiq_aarch64
el1_end_fiq_aarch64:

.balign 0x80
el1_serror_aarch64:
	b	el1_serror_aarch64
el1_end_serror_aarch64:

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch32 : 0x600 - 0x800
	 * ---------------------------------------------------------------------
	 */

.balign 0x80
el1_sync_exception_aarch32:
	b	el1_sync_exception_aarch32
el1_end_sync_exception_aarch32:

.balign 0x80
el1_irq_aarch32:
	b	el1_irq_aarch32
el1_end_irq_aarch32:

.balign 0x80
el1_fiq_aarch32:
	b	el1_fiq_aarch32
el1_end_fiq_aarch32:

.balign 0x80
el1_serror_aarch32:
	b	el1_serror_aarch32
el1_end_serror_aarch32:

