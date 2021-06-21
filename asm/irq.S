// Credit: https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S

#include "aarch64/Entry.h"

// #define SAVE_VFP_REGS_ON_IRQ
// #define SAVE_VFP_REGS_ON_FIQ

#define ARM_IC_FIQ_CONTROL 0xfe00b20c



.global _ZN5Armaz3ARM10enableIRQsEv
_ZN5Armaz3ARM10enableIRQsEv:
	msr daifclr, #2
	ret

.global _ZN5Armaz3ARM11disableIRQsEv
_ZN5Armaz3ARM11disableIRQsEv:
	msr daifset, #2
	ret

.global _ZN5Armaz3ARM10initVectorEv
_ZN5Armaz3ARM10initVectorEv:
	adr x0, vectors
	msr vbar_el1, x0
	msr vbar_el2, x0
	ret



.macro vector handler
.align 7
b \handler
.endm




.align 11
.global vectors
vectors:
	// from current EL with sp_el0
	vector	_ZN5Armaz10Interrupts15SynchronousStubEv
	vector	_ZN5Armaz10Interrupts7IRQStubEv
	vector	_ZN5Armaz10Interrupts7FIQStubEv
	vector	_ZN5Armaz10Interrupts10SErrorStubEv

	// from current EL with sp_elx, x != 0
	vector	_ZN5Armaz10Interrupts15SynchronousStubEv
	vector	_ZN5Armaz10Interrupts7IRQStubEv
	vector	_ZN5Armaz10Interrupts7FIQStubEv
	vector	_ZN5Armaz10Interrupts10SErrorStubEv

	// from lower EL, target EL minus 1 is AArch64
	vector	HVCStub
	vector	_ZN5Armaz10Interrupts14UnexpectedStubEv
	vector	_ZN5Armaz10Interrupts14UnexpectedStubEv
	vector	_ZN5Armaz10Interrupts14UnexpectedStubEv

	// from lower EL, target EL minus 1 is AArch32
	vector	_ZN5Armaz10Interrupts14UnexpectedStubEv
	vector	_ZN5Armaz10Interrupts14UnexpectedStubEv
	vector	_ZN5Armaz10Interrupts14UnexpectedStubEv
	vector	_ZN5Armaz10Interrupts14UnexpectedStubEv














.macro stub name, exception
.global \name
\name:
	ldr x0, =lastlink
	str x30, [x0]
	ldr x0, =lastframe
	str x29, [x0]

	mrs	x0, esr_el1
	mrs	x1, spsr_el1
	mov	x2, x30 /* lr */
	mrs	x3, elr_el1
	mrs	x4, sp_el0
	mov	x5, sp
	mrs	x6, far_el1

	str	x6, [sp, #-16]!
	stp	x4, x5, [sp, #-16]!
	stp	x2, x3, [sp, #-16]!
	stp	x0, x1, [sp, #-16]!

	mov	x0, #\exception
	mov	x1, sp
	b   _ZN5Armaz10Interrupts16ExceptionHandlerEmPNS_10AbortFrameE /* never returns */
.endm




// Credit: https://github.com/rsta2/circle
.global _ZN5Armaz10Interrupts7SMCStubEv
_ZN5Armaz10Interrupts7SMCStubEv:
	ldr	x2, =SMCStack
	mov	sp, x2
	str	x30, [sp, #-16]!
	bl _ZN5Armaz10Interrupts20SecureMonitorHandlerEjj
	ldr x30, [sp], #16
	eret

stub _ZN5Armaz10Interrupts14UnexpectedStubEv,  0
stub _ZN5Armaz10Interrupts15SynchronousStubEv, 1
stub _ZN5Armaz10Interrupts10SErrorStubEv,      2




.global _ZN5Armaz10Interrupts7IRQStubEv
_ZN5Armaz10Interrupts7IRQStubEv:
	str x0, [sp, #-16]!
	ldr x0, =lastlink
	str x30, [x0]
	ldr x0, =lastframe
	str x29, [x0]
	ldr x0, [sp], #16

	stp	x29, x30, [sp, #-16]! // save x29, x30 onto stack

	mrs	x29, elr_el1 // save elr_el1, spsr_el1 onto stack
	mrs	x30, spsr_el1
	stp	x29, x30, [sp, #-16]!
	msr	DAIFClr, #1 // enable FIQ

#ifdef SAVE_VFP_REGS_ON_IRQ
	stp	q30, q31, [sp, #-32]! // save q0-q31 onto stack
	stp	q28, q29, [sp, #-32]!
	stp	q26, q27, [sp, #-32]!
	stp	q24, q25, [sp, #-32]!
	stp	q22, q23, [sp, #-32]!
	stp	q20, q21, [sp, #-32]!
	stp	q18, q19, [sp, #-32]!
	stp	q16, q17, [sp, #-32]!
	stp	q14, q15, [sp, #-32]!
	stp	q12, q13, [sp, #-32]!
	stp	q10, q11, [sp, #-32]!
	stp	q8, q9, [sp, #-32]!
	stp	q6, q7, [sp, #-32]!
	stp	q4, q5, [sp, #-32]!
	stp	q2, q3, [sp, #-32]!
	stp	q0, q1, [sp, #-32]!
#endif
	stp	x27, x28, [sp, #-16]! // save x0-x28 onto stack
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x17, x18, [sp, #-16]!
	stp	x15, x16, [sp, #-16]!
	stp	x13, x14, [sp, #-16]!
	stp	x11, x12, [sp, #-16]!
	stp	x9, x10, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!
	str	x0, [sp, #-16]!
	mov x0, #0x3210
	movk x0, #0x7654, lsl #16
	movk x0, #0xba98, lsl #32
	movk x0, #0xfedc, lsl #48
	str x0, [sp, #-16]!

	ldr	x0, =IRQReturnAddress // store return address for profiling
	str	x29, [x0]

	bl _ZN5Armaz10Interrupts16InterruptHandlerEv

	ldr x0, [sp], #16
	ldr	x0, [sp], #16 // restore x0-x28 from stack
	ldp	x1, x2, [sp], #16
	ldp	x3, x4, [sp], #16
	ldp	x5, x6, [sp], #16
	ldp	x7, x8, [sp], #16
	ldp	x9, x10, [sp], #16
	ldp	x11, x12, [sp], #16
	ldp	x13, x14, [sp], #16
	ldp	x15, x16, [sp], #16
	ldp	x17, x18, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
#ifdef SAVE_VFP_REGS_ON_IRQ
	ldp	q0, q1, [sp], #32 // restore q0-q31 from stack
	ldp	q2, q3, [sp], #32
	ldp	q4, q5, [sp], #32
	ldp	q6, q7, [sp], #32
	ldp	q8, q9, [sp], #32
	ldp	q10, q11, [sp], #32
	ldp	q12, q13, [sp], #32
	ldp	q14, q15, [sp], #32
	ldp	q16, q17, [sp], #32
	ldp	q18, q19, [sp], #32
	ldp	q20, q21, [sp], #32
	ldp	q22, q23, [sp], #32
	ldp	q24, q25, [sp], #32
	ldp	q26, q27, [sp], #32
	ldp	q28, q29, [sp], #32
	ldp	q30, q31, [sp], #32
#endif

	msr	DAIFSet, #1 // disable FIQ
	ldp	x29, x30, [sp], #16 // restore elr_el1, spsr_el1 from stack
	msr	elr_el1, x29
	msr	spsr_el1, x30

	ldp	x29, x30, [sp], #16 // restore x29, x30 from stack

	eret


.global _ZN5Armaz10Interrupts7FIQStubEv
_ZN5Armaz10Interrupts7FIQStubEv:
#ifdef SAVE_VFP_REGS_ON_FIQ
	stp	q30, q31, [sp, #-32]!
	stp	q28, q29, [sp, #-32]!
	stp	q26, q27, [sp, #-32]!
	stp	q24, q25, [sp, #-32]!
	stp	q22, q23, [sp, #-32]!
	stp	q20, q21, [sp, #-32]!
	stp	q18, q19, [sp, #-32]!
	stp	q16, q17, [sp, #-32]!
	stp	q14, q15, [sp, #-32]!
	stp	q12, q13, [sp, #-32]!
	stp	q10, q11, [sp, #-32]!
	stp	q8, q9, [sp, #-32]!
	stp	q6, q7, [sp, #-32]!
	stp	q4, q5, [sp, #-32]!
	stp	q2, q3, [sp, #-32]!
	stp	q0, q1, [sp, #-32]!
#endif
	stp	x29, x30, [sp, #-16]!
	stp	x27, x28, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	stp	x23, x24, [sp, #-16]!
	stp	x21, x22, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x17, x18, [sp, #-16]!
	stp	x15, x16, [sp, #-16]!
	stp	x13, x14, [sp, #-16]!
	stp	x11, x12, [sp, #-16]!
	stp	x9, x10, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!
	str	x0, [sp, #-16]!

	ldr	x2, =FIQData
	ldr	x1, [x2] // get FIQData.pHandler
	cmp	x1, #0 // is handler set?
	b.eq	2f
	ldr	x0, [x2, #8] // get FIQData.pParam
	blr	x1 // call handler

1:	ldr	x0, [sp], #16
	ldp	x1, x2, [sp], #16
	ldp	x3, x4, [sp], #16
	ldp	x5, x6, [sp], #16
	ldp	x7, x8, [sp], #16
	ldp	x9, x10, [sp], #16
	ldp	x11, x12, [sp], #16
	ldp	x13, x14, [sp], #16
	ldp	x15, x16, [sp], #16
	ldp	x17, x18, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x21, x22, [sp], #16
	ldp	x23, x24, [sp], #16
	ldp	x25, x26, [sp], #16
	ldp	x27, x28, [sp], #16
	ldp	x29, x30, [sp], #16
#ifdef SAVE_VFP_REGS_ON_FIQ
	ldp	q0, q1, [sp], #32
	ldp	q2, q3, [sp], #32
	ldp	q4, q5, [sp], #32
	ldp	q6, q7, [sp], #32
	ldp	q8, q9, [sp], #32
	ldp	q10, q11, [sp], #32
	ldp	q12, q13, [sp], #32
	ldp	q14, q15, [sp], #32
	ldp	q16, q17, [sp], #32
	ldp	q18, q19, [sp], #32
	ldp	q20, q21, [sp], #32
	ldp	q22, q23, [sp], #32
	ldp	q24, q25, [sp], #32
	ldp	q26, q27, [sp], #32
	ldp	q28, q29, [sp], #32
	ldp	q30, q31, [sp], #32
#endif

	eret

2:	ldr	x1, =ARM_IC_FIQ_CONTROL // disable FIQ (if handler is not set)
	mov	w0, #0
	str	w0, [x1]
	b 1b


HVCStub: // return to EL2h mode
	mrs	x0, spsr_el2
	bic	x0, x0, #0xf
	mov	x1, #9
	orr	x0, x0, x1
	msr	spsr_el2, x0
	eret


.data

.align 3
.global FIQData
FIQData: // matches TFIQData:
	.quad 0 // pHandler
	.quad 0 // pParam
	.word 0 // nFIQNumber (unused)

.align 3
.global IRQReturnAddress
IRQReturnAddress: .quad 0

.global lastlink
lastlink: .quad 0

.global lastframe
lastframe: .quad 0

.bss

.align 4
.space 128
SMCStack: