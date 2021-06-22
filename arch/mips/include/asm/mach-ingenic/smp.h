/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Ingenic XBurst SoCs SMP definitions
 * Copyright (c) 2013 Paul Burton <paul.burton@imgtec.com>
 * Copyright (c) 2021 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 */

#ifndef __MIPS_ASM_MACH_INGENIC_SMP_H__
#define __MIPS_ASM_MACH_INGENIC_SMP_H__


/*
 * Ingenic XBurst SMP definitions.
 */

#define read_c0_corectrl()				__read_32bit_c0_register($12, 2)
#define write_c0_corectrl(val)			__write_32bit_c0_register($12, 2, val)

#define read_c0_corestatus()			__read_32bit_c0_register($12, 3)
#define write_c0_corestatus(val)		__write_32bit_c0_register($12, 3, val)

#define read_c0_reim()					__read_32bit_c0_register($12, 4)
#define write_c0_reim(val)				__write_32bit_c0_register($12, 4, val)

#define read_c0_mailbox0()				__read_32bit_c0_register($20, 0)
#define write_c0_mailbox0(val)			__write_32bit_c0_register($20, 0, val)

#define read_c0_mailbox1()				__read_32bit_c0_register($20, 1)
#define write_c0_mailbox1(val)			__write_32bit_c0_register($20, 1, val)

#define read_c0_mailbox2()				__read_32bit_c0_register($20, 2)
#define write_c0_mailbox2(val)			__write_32bit_c0_register($20, 2, val)

#define read_c0_mailbox3()				__read_32bit_c0_register($20, 3)
#define write_c0_mailbox3(val)			__write_32bit_c0_register($20, 3, val)

#define smp_clr_pending(mask) do {		\
		unsigned int stat;				\
		stat = read_c0_corestatus();	\
		stat &= ~((mask) & 0xff);		\
		write_c0_corestatus(stat);		\
	} while (0)

/* Core Control register */
#define CORECTRL_SLEEP1M_SHIFT			17
#define CORECTRL_SLEEP1M				(_ULCAST_(0x1) << CORECTRL_SLEEP1M_SHIFT)
#define CORECTRL_SLEEP0M_SHIFT			16
#define CORECTRL_SLEEP0M				(_ULCAST_(0x1) << CORECTRL_SLEEP0M_SHIFT)
#define CORECTRL_RPC1_SHIFT				9
#define CORECTRL_RPC1					(_ULCAST_(0x1) << CORECTRL_RPC1_SHIFT)
#define CORECTRL_RPC0_SHIFT				8
#define CORECTRL_RPC0					(_ULCAST_(0x1) << CORECTRL_RPC0_SHIFT)
#define CORECTRL_SWRST1_SHIFT			1
#define CORECTRL_SWRST1					(_ULCAST_(0x1) << CORECTRL_SWRST1_SHIFT)
#define CORECTRL_SWRST0_SHIFT			0
#define CORECTRL_SWRST0					(_ULCAST_(0x1) << CORECTRL_SWRST0_SHIFT)

/* Core Status register */
#define CORESTATUS_SLEEP1_SHIFT			17
#define CORESTATUS_SLEEP1				(_ULCAST_(0x1) << CORESTATUS_SLEEP1_SHIFT)
#define CORESTATUS_SLEEP0_SHIFT			16
#define CORESTATUS_SLEEP0				(_ULCAST_(0x1) << CORESTATUS_SLEEP0_SHIFT)
#define CORESTATUS_IRQ1P_SHIFT			9
#define CORESTATUS_IRQ1P				(_ULCAST_(0x1) << CORESTATUS_IRQ1P_SHIFT)
#define CORESTATUS_IRQ0P_SHIFT			8
#define CORESTATUS_IRQ0P				(_ULCAST_(0x1) << CORESTATUS_IRQ8P_SHIFT)
#define CORESTATUS_MIRQ1P_SHIFT			1
#define CORESTATUS_MIRQ1P				(_ULCAST_(0x1) << CORESTATUS_MIRQ1P_SHIFT)
#define CORESTATUS_MIRQ0P_SHIFT			0
#define CORESTATUS_MIRQ0P				(_ULCAST_(0x1) << CORESTATUS_MIRQ0P_SHIFT)

/* Reset Entry & IRQ Mask register */
#define REIM_ENTRY_SHIFT				16
#define REIM_ENTRY						(_ULCAST_(0xffff) << REIM_ENTRY_SHIFT)
#define REIM_IRQ1M_SHIFT				9
#define REIM_IRQ1M						(_ULCAST_(0x1) << REIM_IRQ1M_SHIFT)
#define REIM_IRQ0M_SHIFT				8
#define REIM_IRQ0M						(_ULCAST_(0x1) << REIM_IRQ0M_SHIFT)
#define REIM_MBOXIRQ1M_SHIFT			1
#define REIM_MBOXIRQ1M					(_ULCAST_(0x1) << REIM_MBOXIRQ1M_SHIFT)
#define REIM_MBOXIRQ0M_SHIFT			0
#define REIM_MBOXIRQ0M					(_ULCAST_(0x1) << REIM_MBOXIRQ0M_SHIFT)

/*
 * Ingenic XBurst2 SMP definitions.
 */

#define INGENIC_XBURST2_THREADS_PER_CORE	2

#define CCU_BASE						ioremap(0x12200000, 4)

#define CCU_MAILBOX_BASE				CCU_BASE + 0x1000

#define read_ccu_cssr()					readl(CCU_BASE + 0x20)

#define read_ccu_csrr()					readl(CCU_BASE + 0x40)
#define write_ccu_csrr(val)				writel(val, CCU_BASE + 0x40)

#define read_ccu_mscr()					readl(CCU_BASE + 0x60)
#define write_ccu_mscr(val)				writel(val, CCU_BASE + 0x60)

#define read_ccu_pimr()					readl(CCU_BASE + 0x120)
#define write_ccu_pimr(val)				writel(val, CCU_BASE + 0x120)

#define read_ccu_mimr()					readl(CCU_BASE + 0x160)
#define write_ccu_mimr(val)				writel(val, CCU_BASE + 0x160)

#define read_ccu_oimr()					readl(CCU_BASE + 0x1a0)
#define write_ccu_oimr(val)				writel(val, CCU_BASE + 0x1a0)

#define read_ccu_rer()					readl(CCU_BASE + 0xf00)
#define write_ccu_rer(val)				writel(val, CCU_BASE + 0xf00)

#define read_ccu_mbr0()					readl(CCU_BASE + 0x1000)
#define write_ccu_mbr0(val)				writel(val, CCU_BASE + 0x1000)

#define read_ccu_mbr1()					readl(CCU_BASE + 0x1004)
#define write_ccu_mbr1(val)				writel(val, CCU_BASE + 0x1004)

#define read_ccu_mbr2()					readl(CCU_BASE + 0x1008)
#define write_ccu_mbr2(val)				writel(val, CCU_BASE + 0x1008)

#define read_ccu_mbr3()					readl(CCU_BASE + 0x100c)
#define write_ccu_mbr3(val)				writel(val, CCU_BASE + 0x100c)

/* Core Sleep Status register */
#define CSSR_SS15_SHIFT					15
#define CSSR_SS15						(_ULCAST_(0x1) << CSSR_SS15_SHIFT)
#define CSSR_SS14_SHIFT					14
#define CSSR_SS14						(_ULCAST_(0x1) << CSSR_SS14_SHIFT)
#define CSSR_SS13_SHIFT					13
#define CSSR_SS13						(_ULCAST_(0x1) << CSSR_SS13_SHIFT)
#define CSSR_SS12_SHIFT					12
#define CSSR_SS12						(_ULCAST_(0x1) << CSSR_SS12_SHIFT)
#define CSSR_SS11_SHIFT					11
#define CSSR_SS11						(_ULCAST_(0x1) << CSSR_SS11_SHIFT)
#define CSSR_SS10_SHIFT					10
#define CSSR_SS10						(_ULCAST_(0x1) << CSSR_SS10_SHIFT)
#define CSSR_SS9_SHIFT					9
#define CSSR_SS9						(_ULCAST_(0x1) << CSSR_SS9_SHIFT)
#define CSSR_SS8_SHIFT					8
#define CSSR_SS8						(_ULCAST_(0x1) << CSSR_SS8_SHIFT)
#define CSSR_SS7_SHIFT					7
#define CSSR_SS7						(_ULCAST_(0x1) << CSSR_SS7_SHIFT)
#define CSSR_SS6_SHIFT					6
#define CSSR_SS6						(_ULCAST_(0x1) << CSSR_SS6_SHIFT)
#define CSSR_SS5_SHIFT					5
#define CSSR_SS5						(_ULCAST_(0x1) << CSSR_SS5_SHIFT)
#define CSSR_SS4_SHIFT					4
#define CSSR_SS4						(_ULCAST_(0x1) << CSSR_SS4_SHIFT)
#define CSSR_SS3_SHIFT					3
#define CSSR_SS3						(_ULCAST_(0x1) << CSSR_SS3_SHIFT)
#define CSSR_SS2_SHIFT					2
#define CSSR_SS2						(_ULCAST_(0x1) << CSSR_SS2_SHIFT)
#define CSSR_SS1_SHIFT					1
#define CSSR_SS1						(_ULCAST_(0x1) << CSSR_SS1_SHIFT)
#define CSSR_SS0_SHIFT					0
#define CSSR_SS0						(_ULCAST_(0x1) << CSSR_SS0_SHIFT)

/* Core Software Reset Mask register */
#define CSRR_SRE15_SHIFT				15
#define CSRR_SRE15						(_ULCAST_(0x1) << CSRR_SRE15_SHIFT)
#define CSRR_SRE14_SHIFT				14
#define CSRR_SRE14						(_ULCAST_(0x1) << CSRR_SRE14_SHIFT)
#define CSRR_SRE13_SHIFT				13
#define CSRR_SRE13						(_ULCAST_(0x1) << CSRR_SRE13_SHIFT)
#define CSRR_SRE12_SHIFT				12
#define CSRR_SRE12						(_ULCAST_(0x1) << CSRR_SRE12_SHIFT)
#define CSRR_SRE11_SHIFT				11
#define CSRR_SRE11						(_ULCAST_(0x1) << CSRR_SRE11_SHIFT)
#define CSRR_SRE10_SHIFT				10
#define CSRR_SRE10						(_ULCAST_(0x1) << CSRR_SRE10_SHIFT)
#define CSRR_SRE9_SHIFT					9
#define CSRR_SRE9						(_ULCAST_(0x1) << CSRR_SRE9_SHIFT)
#define CSRR_SRE8_SHIFT					8
#define CSRR_SRE8						(_ULCAST_(0x1) << CSRR_SRE8_SHIFT)
#define CSRR_SRE7_SHIFT					7
#define CSRR_SRE7						(_ULCAST_(0x1) << CSRR_SRE7_SHIFT)
#define CSRR_SRE6_SHIFT					6
#define CSRR_SRE6						(_ULCAST_(0x1) << CSRR_SRE6_SHIFT)
#define CSRR_SRE5_SHIFT					5
#define CSRR_SRE5						(_ULCAST_(0x1) << CSRR_SRE5_SHIFT)
#define CSRR_SRE4_SHIFT					4
#define CSRR_SRE4						(_ULCAST_(0x1) << CSRR_SRE4_SHIFT)
#define CSRR_SRE3_SHIFT					3
#define CSRR_SRE3						(_ULCAST_(0x1) << CSRR_SRE3_SHIFT)
#define CSRR_SRE2_SHIFT					2
#define CSRR_SRE2						(_ULCAST_(0x1) << CSRR_SRE2_SHIFT)
#define CSRR_SRE1_SHIFT					1
#define CSRR_SRE1						(_ULCAST_(0x1) << CSRR_SRE1_SHIFT)
#define CSRR_SRE0_SHIFT					0
#define CSRR_SRE0						(_ULCAST_(0x1) << CSRR_SRE0_SHIFT)

/* Memory Subsystem Control register */
#define MSCR_PSEL_SHIFT					24
#define MSCR_PSEL						(_ULCAST_(0x1) << MSCR_PSEL_SHIFT)
#define MSCR_L2SIZEQTR_SHIFT			9
#define MSCR_L2SIZEQTR					(_ULCAST_(0x1) << MSCR_L2SIZEQTR_SHIFT)
#define MSCR_L2SIZEHALF_SHIFT			8
#define MSCR_L2SIZEHALF					(_ULCAST_(0x1) << MSCR_L2SIZEHALF_SHIFT)
#define MSCR_QOSE_SHIFT					3
#define MSCR_QOSE						(_ULCAST_(0x1) << MSCR_QOSE_SHIFT)
#define MSCR_DISPFB1_SHIFT				2
#define MSCR_DISPFB1					(_ULCAST_(0x1) << MSCR_DISPFB1_SHIFT)
#define MSCR_DISPFB2_SHIFT				1
#define MSCR_DISPFB2					(_ULCAST_(0x1) << MSCR_DISPFB2_SHIFT)
#define MSCR_DISL2C_SHIFT				0
#define MSCR_DISL2C						(_ULCAST_(0x1) << MSCR_DISL2C_SHIFT)

/* Peripheral IRQ Mask register */
#define PIMR_IM15_SHIFT					15
#define PIMR_IM15						(_ULCAST_(0x1) << PIMR_IM15_SHIFT)
#define PIMR_IM14_SHIFT					14
#define PIMR_IM14						(_ULCAST_(0x1) << PIMR_IM14_SHIFT)
#define PIMR_IM13_SHIFT					13
#define PIMR_IM13						(_ULCAST_(0x1) << PIMR_IM13_SHIFT)
#define PIMR_IM12_SHIFT					12
#define PIMR_IM12						(_ULCAST_(0x1) << PIMR_IM12_SHIFT)
#define PIMR_IM11_SHIFT					11
#define PIMR_IM11						(_ULCAST_(0x1) << PIMR_IM11_SHIFT)
#define PIMR_IM10_SHIFT					10
#define PIMR_IM10						(_ULCAST_(0x1) << PIMR_IM10_SHIFT)
#define PIMR_IM9_SHIFT					9
#define PIMR_IM9						(_ULCAST_(0x1) << PIMR_IM9_SHIFT)
#define PIMR_IM8_SHIFT					8
#define PIMR_IM8						(_ULCAST_(0x1) << PIMR_IM8_SHIFT)
#define PIMR_IM7_SHIFT					7
#define PIMR_IM7						(_ULCAST_(0x1) << PIMR_IM7_SHIFT)
#define PIMR_IM6_SHIFT					6
#define PIMR_IM6						(_ULCAST_(0x1) << PIMR_IM6_SHIFT)
#define PIMR_IM5_SHIFT					5
#define PIMR_IM5						(_ULCAST_(0x1) << PIMR_IM5_SHIFT)
#define PIMR_IM4_SHIFT					4
#define PIMR_IM4						(_ULCAST_(0x1) << PIMR_IM4_SHIFT)
#define PIMR_IM3_SHIFT					3
#define PIMR_IM3						(_ULCAST_(0x1) << PIMR_IM3_SHIFT)
#define PIMR_IM2_SHIFT					2
#define PIMR_IM2						(_ULCAST_(0x1) << PIMR_IM2_SHIFT)
#define PIMR_IM1_SHIFT					1
#define PIMR_IM1						(_ULCAST_(0x1) << PIMR_IM1_SHIFT)
#define PIMR_IM0_SHIFT					0
#define PIMR_IM0						(_ULCAST_(0x1) << PIMR_IM0_SHIFT)

/* Mailbox IRQ Mask register */
#define MIMR_IM15_SHIFT					15
#define MIMR_IM15						(_ULCAST_(0x1) << MIMR_IM15_SHIFT)
#define MIMR_IM14_SHIFT					14
#define MIMR_IM14						(_ULCAST_(0x1) << MIMR_IM14_SHIFT)
#define MIMR_IM13_SHIFT					13
#define MIMR_IM13						(_ULCAST_(0x1) << MIMR_IM13_SHIFT)
#define MIMR_IM12_SHIFT					12
#define MIMR_IM12						(_ULCAST_(0x1) << MIMR_IM12_SHIFT)
#define MIMR_IM11_SHIFT					11
#define MIMR_IM11						(_ULCAST_(0x1) << MIMR_IM11_SHIFT)
#define MIMR_IM10_SHIFT					10
#define MIMR_IM10						(_ULCAST_(0x1) << MIMR_IM10_SHIFT)
#define MIMR_IM9_SHIFT					9
#define MIMR_IM9						(_ULCAST_(0x1) << MIMR_IM9_SHIFT)
#define MIMR_IM8_SHIFT					8
#define MIMR_IM8						(_ULCAST_(0x1) << MIMR_IM8_SHIFT)
#define MIMR_IM7_SHIFT					7
#define MIMR_IM7						(_ULCAST_(0x1) << MIMR_IM7_SHIFT)
#define MIMR_IM6_SHIFT					6
#define MIMR_IM6						(_ULCAST_(0x1) << MIMR_IM6_SHIFT)
#define MIMR_IM5_SHIFT					5
#define MIMR_IM5						(_ULCAST_(0x1) << MIMR_IM5_SHIFT)
#define MIMR_IM4_SHIFT					4
#define MIMR_IM4						(_ULCAST_(0x1) << MIMR_IM4_SHIFT)
#define MIMR_IM3_SHIFT					3
#define MIMR_IM3						(_ULCAST_(0x1) << MIMR_IM3_SHIFT)
#define MIMR_IM2_SHIFT					2
#define MIMR_IM2						(_ULCAST_(0x1) << MIMR_IM2_SHIFT)
#define MIMR_IM1_SHIFT					1
#define MIMR_IM1						(_ULCAST_(0x1) << MIMR_IM1_SHIFT)
#define MIMR_IM0_SHIFT					0
#define MIMR_IM0						(_ULCAST_(0x1) << MIMR_IM0_SHIFT)

/* OST IRQ Mask register */
#define OIMR_IM15_SHIFT					15
#define OIMR_IM15						(_ULCAST_(0x1) << OIMR_IM15_SHIFT)
#define OIMR_IM14_SHIFT					14
#define OIMR_IM14						(_ULCAST_(0x1) << OIMR_IM14_SHIFT)
#define OIMR_IM13_SHIFT					13
#define OIMR_IM13						(_ULCAST_(0x1) << OIMR_IM13_SHIFT)
#define OIMR_IM12_SHIFT					12
#define OIMR_IM12						(_ULCAST_(0x1) << OIMR_IM12_SHIFT)
#define OIMR_IM11_SHIFT					11
#define OIMR_IM11						(_ULCAST_(0x1) << OIMR_IM11_SHIFT)
#define OIMR_IM10_SHIFT					10
#define OIMR_IM10						(_ULCAST_(0x1) << OIMR_IM10_SHIFT)
#define OIMR_IM9_SHIFT					9
#define OIMR_IM9						(_ULCAST_(0x1) << OIMR_IM9_SHIFT)
#define OIMR_IM8_SHIFT					8
#define OIMR_IM8						(_ULCAST_(0x1) << OIMR_IM8_SHIFT)
#define OIMR_IM7_SHIFT					7
#define OIMR_IM7						(_ULCAST_(0x1) << OIMR_IM7_SHIFT)
#define OIMR_IM6_SHIFT					6
#define OIMR_IM6						(_ULCAST_(0x1) << OIMR_IM6_SHIFT)
#define OIMR_IM5_SHIFT					5
#define OIMR_IM5						(_ULCAST_(0x1) << OIMR_IM5_SHIFT)
#define OIMR_IM4_SHIFT					4
#define OIMR_IM4						(_ULCAST_(0x1) << OIMR_IM4_SHIFT)
#define OIMR_IM3_SHIFT					3
#define OIMR_IM3						(_ULCAST_(0x1) << OIMR_IM3_SHIFT)
#define OIMR_IM2_SHIFT					2
#define OIMR_IM2						(_ULCAST_(0x1) << OIMR_IM2_SHIFT)
#define OIMR_IM1_SHIFT					1
#define OIMR_IM1						(_ULCAST_(0x1) << OIMR_IM1_SHIFT)
#define OIMR_IM0_SHIFT					0
#define OIMR_IM0						(_ULCAST_(0x1) << OIMR_IM0_SHIFT)

extern void ingenic_smp_init(void);
extern void ingenic_secondary_cpu_entry(void);

extern void jz4780_smp_wait_irqoff(void);
extern void jz4780_smp_switch_irqcpu(int cpu);

#endif /* __MIPS_ASM_MACH_INGENIC_SMP_H__ */
