/*
 * Minos smc call code
 *
 * Copyright (c) 2019 Le Min
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>

	.macro SMCCC instr
	.cfi_startproc
	\instr	#0
	ldr	x4, [sp]
	stp	x0, x1, [x4, #0]
	stp	x2, x3, [x4, #16]
	ret
	.cfi_endproc
	.endm

GTEXT(__arm_smccc_smc)
SECTION_FUNC(TEXT, __arm_smccc_smc)
	SMCCC	smc

GTEXT(__arm_smccc_hvc)
SECTION_FUNC(TEXT, __arm_smccc_hvc)
	SMCCC	hvc
