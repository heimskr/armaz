//==================================================================
// Copyright (C) Arm Limited, 2019 All rights reserved.
//
// The example code is provided to you as an aid to learning when working
// with Arm-based technology, including but not limited to programming tutorials.
// Arm hereby grants to you, subject to the terms and conditions of this Licence,
// a non-exclusive, non-transferable, non-sub-licensable, free-of-charge licence,
// to use and copy the Software solely for the purpose of demonstration and
// evaluation.
//
// You accept that the Software has not been tested by Arm therefore the Software
// is provided as is, without warranty of any kind, express or implied. In no
// event shall the authors or copyright holders be liable for any claim, damages
// or other liability, whether in action or contract, tort or otherwise, arising
// from, out of or in connection with the Software or the use of Software.
//
// ------------------------------------------------------------

.type _ZN5Armaz3GIC11getAffinityEv, @function
.cfi_startproc
.global _ZN5Armaz3GIC11getAffinityEv
_ZN5Armaz3GIC11getAffinityEv:
	mrs  x0, MPIDR_EL1
	ubfx x1, x0, #32, #8
	bfi  w0, w1, #24, #8
	ret
.cfi_endproc

.type _ZN5Armaz3GIC15setPriorityMaskEj, @function
.global _ZN5Armaz3GIC15setPriorityMaskEj
_ZN5Armaz3GIC15setPriorityMaskEj:
	msr S3_0_C4_C6_0, x0 // ICC_PMR_EL1
	ret

.type _ZN5Armaz3GIC16enableGroup0IntsEv, @function
.global _ZN5Armaz3GIC16enableGroup0IntsEv
_ZN5Armaz3GIC16enableGroup0IntsEv:
	mov w0, #1
	msr S3_0_C12_C12_6, x0 // ICC_IGRPEN0_EL1
	isb
	ret

.type _ZN5Armaz3GIC16enableGroup1IntsEv, @function
.global _ZN5Armaz3GIC16enableGroup1IntsEv
_ZN5Armaz3GIC16enableGroup1IntsEv:
	mrs x0, S3_0_C12_C12_7 // ICC_IGRPEN1_EL1
	orr w0, w0, #1
	msr S3_0_C12_C12_7, x0 // ICC_IGRPEN1_EL1
	isb
	ret
