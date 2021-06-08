.section ".text.boot"  // Make sure the linker puts this at the start of the kernel image

#include "boot.h"

.global _start  // Execution starts here

_start:
	// Check processor ID is zero (executing on main core), else hang
	mrs x1, mpidr_el1
	and x1, x1, #3
	cbz x1, 2f
1:  // We're not on the main core, so hang in an infinite wait loop
	wfe
	b 1b
2:  // We're on the main core!

	// Set stack to start below our code
	ldr x1, =_start
	mov sp, x1

	// Clean the BSS section
	ldr  x1, =__bss_start // Start address
	ldr  x2, =__bss_size  // Size of the section
3:  cbz  x2, 4f           // Quit loop if zero
	str  xzr, [x1], #8
	sub  x2, x2, #1
	cbnz x2, 3b           // Loop if non-zero

	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0

	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0

	// Jump to our main() routine in C (make sure it doesn't return)
4:  bl main
	// In case it does return, halt the master core too
	b 1b
