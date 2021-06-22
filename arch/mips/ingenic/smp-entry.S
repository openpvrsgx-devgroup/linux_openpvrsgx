/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Ingenic XBurst SoCs SMP entry point
 * Copyright (c) 2013 Paul Burton <paul.burton@imgtec.com>
 * Copyright (c) 2021 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 */

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/cacheops.h>
#include <asm/cpu.h>
#include <asm/mipsregs.h>

#define CACHE_SIZE (32 * 1024)
#define CACHE_LINESIZE 32

.extern ingenic_cpu_entry_sp
.extern ingenic_cpu_entry_gp

.section .text.smp-entry
.balign 0x10000
.set noreorder
LEAF(ingenic_secondary_cpu_entry)
	mtc0			zero,			CP0_CAUSE

	li				t0,				ST0_CU0
	mtc0			t0,				CP0_STATUS

	/* detect prid */
	mfc0			t0,				CP0_PRID
	andi			t1,				t0,				PRID_IMP_XBURST2
	bne				zero,			t1,				2f

	/* cache setup */
	li				t0,				KSEG0
	ori				t1,				t0,				CACHE_SIZE
	mtc0			zero,			CP0_TAGLO,		0
1:	cache			Index_Store_Tag_I,				0(t0)
	cache			Index_Store_Tag_D,				0(t0)
	bne				t0,				t1,				1b
	addiu			t0,				t0,				CACHE_LINESIZE

	/* kseg0 cache attribute */
2:	mfc0			t0,				CP0_CONFIG,		0
	ori				t0,				t0,				CONF_CM_CACHABLE_NONCOHERENT
	mtc0			t0,				CP0_CONFIG,		0

	/* pagemask */
	mtc0			zero,			CP0_PAGEMASK,	0

	/* retrieve sp */
	la				t0,				ingenic_cpu_entry_sp
	lw				sp,				0(t0)

	/* retrieve gp */
	la				t0,				ingenic_cpu_entry_gp
	lw				gp,				0(t0)

	/* jump to the kernel in kseg0 */
	la				t0,				smp_bootstrap
	jr				t0
	nop
	END(ingenic_secondary_cpu_entry)
