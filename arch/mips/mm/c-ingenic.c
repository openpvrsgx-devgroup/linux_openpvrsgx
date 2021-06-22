// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic XBurst SoCs specific cache driver
 * Copyright (c) 2020 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 */

#include <linux/cpu_pm.h>
#include <linux/highmem.h>
#include <linux/kernel.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/smp.h>

#include <asm/bootinfo.h>
#include <asm/cache.h>
#include <asm/cacheops.h>
#include <asm/cpu.h>
#include <asm/mach-ingenic/smp.h>
#include <asm/mmu_context.h>
#include <asm/r4kcache.h>
#include <asm/traps.h>

static unsigned long dcache_size __read_mostly;
static unsigned long icache_size __read_mostly;
static unsigned long scache_size __read_mostly;

struct flush_icache_range_args {
	unsigned long start;
	unsigned long end;
	bool user;
};

static void ingenic_cache_noop(void) {}
static void ingenic_cache_noop0(void) {while(1) printk("-------0\n");}
static void ingenic_cache_noop1(void) {while(1) printk("-------1\n");}
static void ingenic_cache_noop2(void) {while(1) printk("-------2\n");}
static void ingenic_cache_noop7(void) {while(1) printk("-------7\n");}

static void (*ingenic_blast_scache)(void);
static void (*ingenic_blast_scache_range)(unsigned long start, unsigned long end);

static void ingenic_blast_scache_setup(void)
{
	unsigned long sc_lsize = cpu_scache_line_size();

	if (sc_lsize == 32)
		ingenic_blast_scache = blast_scache32;
	else if (sc_lsize == 64)
		ingenic_blast_scache = blast_scache64;
	else if (sc_lsize == 128)
		ingenic_blast_scache = blast_scache128;

	ingenic_blast_scache_range = blast_scache_range;
}

static inline void ingenic_on_each_cpu(void (*func)(void *info), void *info)
{
	preempt_disable();

	if (mips_machtype == MACH_INGENIC_JZ4780)
		smp_call_function_many(&cpu_foreign_map[smp_processor_id()], func, info, 1);

	func(info);

	preempt_enable();
}

static inline void local_ingenic_flush_cache_all(void *args)
{
	blast_dcache32();
	blast_icache32();
}

static void ingenic_flush_cache_all(void)
{
	ingenic_on_each_cpu(local_ingenic_flush_cache_all, NULL);
}

static inline int has_valid_asid(const struct mm_struct *mm)
{
	unsigned int cpu;

	if (mips_machtype >= MACH_INGENIC_X2000)
		for_each_cpu(cpu, cpu_present_mask)
			if (cpu_context(cpu, mm))
				return 1;
	else
		return cpu_context(smp_processor_id(), mm);

	return 0;
}

static inline void local_ingenic_flush_cache_range(void * args)
{
	struct vm_area_struct *vma = args;

	if (!has_valid_asid(vma->vm_mm))
		return;

	blast_dcache32();
	blast_icache32();
}

static void ingenic_flush_cache_range(struct vm_area_struct *vma,
						     unsigned long start, unsigned long end)
{
	if (vma->vm_flags & VM_EXEC)
		ingenic_on_each_cpu(local_ingenic_flush_cache_range, vma);
}

static void ingenic_flush_cache_page(struct vm_area_struct *vma,
						     unsigned long addr, unsigned long pfn)
{
	int exec = vma->vm_flags & VM_EXEC;
	pte_t *ptep;

	preempt_disable();

	if (!has_valid_asid(vma->vm_mm))
		goto out;

	addr &= PAGE_MASK;
	ptep = pte_offset_kernel(pmd_off(vma->vm_mm, addr), addr);

	if (!(pte_present(*ptep)))
		goto out;

	if ((vma->vm_mm == current->active_mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		if (exec) {
			blast_dcache32_page(addr);
			blast_icache32_page(addr);
		}

		goto out;
	}

	if (exec) {
		blast_dcache32_page_indexed(addr);
		blast_icache32_page_indexed(addr);
	}

out:
	preempt_enable();
}

static inline void __local_ingenic_flush_icache_range(unsigned long start,
						     unsigned long end, bool user)
{
	if (end - start >= icache_size) {
		blast_dcache32();
		blast_icache32();
	} else if (user) {
		protected_blast_dcache_range(start, end);
		protected_blast_icache_range(start, end);
	} else {
		blast_dcache_range(start, end);
		blast_icache_range(start, end);
	}
}

static inline void local_ingenic_flush_icache_range(unsigned long start,
						     unsigned long end)
{
	__local_ingenic_flush_icache_range(start, end, false);
}

static inline void local_ingenic_flush_icache_user_range(unsigned long start,
						     unsigned long end)
{
	__local_ingenic_flush_icache_range(start, end, true);
}

static inline void local_ingenic_flush_icache_range_ipi(void *args)
{
	struct flush_icache_range_args *fir_args = args;
	unsigned long start = fir_args->start;
	unsigned long end = fir_args->end;
	bool user = fir_args->user;

	__local_ingenic_flush_icache_range(start, end, user);
}

static void __ingenic_flush_icache_range(unsigned long start, unsigned long end, bool user)
{
	struct flush_icache_range_args args;

	args.start = start;
	args.end = end;
	args.user = user;

	ingenic_on_each_cpu(local_ingenic_flush_icache_range_ipi, &args);
}

static void ingenic_flush_icache_range(unsigned long start, unsigned long end)
{
	return __ingenic_flush_icache_range(start, end, false);
}

static void ingenic_flush_icache_user_range(unsigned long start, unsigned long end)
{
	return __ingenic_flush_icache_range(start, end, true);
}

static void ingenic_dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
	preempt_disable();

	if (size >= dcache_size)
		blast_dcache32();
	else
		blast_dcache_range(addr, addr + size);

	if (size >= scache_size)
		ingenic_blast_scache();
	else
		ingenic_blast_scache_range(addr, addr + size);

	preempt_enable();

	__sync();
}

static void ingenic_xburst_dma_cache_inv(unsigned long addr, unsigned long size)
{
	preempt_disable();
	write_c0_ingenic_errctl(XBURST_ERRCTL_WST_EN);

	if (size >= dcache_size)
		blast_dcache32();
	else
		blast_inv_dcache_range(addr, addr + size);

	write_c0_ingenic_errctl(XBURST_ERRCTL_WST_DIS);
	preempt_enable();

	__sync();
}

static void ingenic_xburst2_dma_cache_inv(unsigned long addr, unsigned long size)
{
	unsigned long lsize = cpu_scache_line_size();
	unsigned long almask = ~(lsize - 1);

	preempt_disable();

	if (size >= dcache_size)
		blast_dcache32();
	else
		blast_inv_dcache_range(addr, addr + size);

	if (size >= scache_size) {
		ingenic_blast_scache();
	} else {
		cache_op(Hit_Writeback_Inv_SD, addr & almask);
		cache_op(Hit_Writeback_Inv_SD, (addr + size - 1) & almask);
		blast_inv_scache_range(addr, addr + size);
	}

	preempt_enable();

	__sync();
}

static void probe_pcache(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;
	unsigned long config1;

	config1 = read_c0_config1();

	c->icache.linesz = 2 << ((config1 >> 19) & 7);
	c->icache.sets = 32 << (((config1 >> 22) + 1) & 7);
	c->icache.ways = 1 + ((config1 >> 16) & 7);

	icache_size = c->icache.ways * c->icache.sets * c->icache.linesz;
	if (!icache_size)
		panic("Invalid Primary instruction cache size.");

	c->icache.waysize = c->icache.sets * c->icache.linesz;
	c->icache.waybit = __ffs(icache_size / c->icache.ways);

	c->dcache.linesz = 2 << ((config1 >> 10) & 7);
	c->dcache.sets = 32 << (((config1 >> 13) + 1) & 7);
	c->dcache.ways = 1 + ((config1 >> 7) & 7);

	dcache_size = c->dcache.ways * c->dcache.sets * c->dcache.linesz;
	if (!dcache_size)
		panic("Invalid Primary data cache size.");

	c->dcache.waysize = c->dcache.sets * c->dcache.linesz;
	c->dcache.waybit = __ffs(dcache_size / c->dcache.ways);

	c->options |= MIPS_CPU_PREFETCH;

	switch (mips_machtype) {

	/* Physically indexed icache */
	case MACH_INGENIC_JZ4730:
	case MACH_INGENIC_JZ4740:
		c->icache.flags |= MIPS_CACHE_PINDEX;
		break;

	/* Physically indexed icache and dcache */
	case MACH_INGENIC_JZ4725B:
	case MACH_INGENIC_JZ4755:
	case MACH_INGENIC_JZ4760:
	case MACH_INGENIC_X2000:
	case MACH_INGENIC_X2000E:
		c->icache.flags |= MIPS_CACHE_PINDEX;
		c->dcache.flags |= MIPS_CACHE_PINDEX;
		break;
	}

	pr_info("Primary instruction cache %ldkiB, %s, %d-way, %d sets, linesize %d bytes.\n",
			icache_size >> 10, c->icache.flags & MIPS_CACHE_PINDEX ? "PIPT" : "VIPT",
			c->icache.ways, c->icache.sets, c->icache.linesz);

	pr_info("Primary data cache %ldkiB, %s, %d-way, %d sets, linesize %d bytes.\n",
			dcache_size >> 10, c->dcache.flags & MIPS_CACHE_PINDEX ? "PIPT" : "VIPT",
			c->dcache.ways, c->dcache.sets, c->dcache.linesz);
}

static void probe_scache(void)
{
	struct cpuinfo_mips *c = &current_cpu_data;
	unsigned int config1, config2;

	/* Mark as not present until probe completed */
	c->scache.flags |= MIPS_CACHE_NOT_PRESENT;

	ingenic_blast_scache = (void *)ingenic_cache_noop;
	ingenic_blast_scache_range = (void *)ingenic_cache_noop;

	if (current_cpu_type() == CPU_XBURST2)
		/* Check the DisL2C bit */
		if (read_ccu_mscr() & MSCR_DISL2C)
			return;

	/* Does this Ingenic CPU have a config2 register? */
	config1 = read_c0_config1();
	if (!(config1 & MIPS_CONF_M))
		return;

	config2 = read_c0_config2();

	c->scache.linesz = 2 << ((config2 >> 4) & 0xf);
	c->scache.sets = 64 << ((config2 >> 8) & 0xf);
	c->scache.ways = 1 + ((config2 >> 0) & 0xf);

	switch (mips_machtype) {

	/*
	 * According to config2 it would be 5-ways, but that is
	 * contradicted by all documentation.
	 */
	case MACH_INGENIC_JZ4770:
	case MACH_INGENIC_JZ4775:
		c->scache.ways = 4;
		break;

	/*
	 * According to config2 it would be 8-ways and 256-sets,
	 * but that is contradicted by all documentation.
	 */
	case MACH_INGENIC_JZ4780:
		c->scache.sets = 1024;
		c->scache.ways = 4;
		break;

	/*
	 * According to config2 it would be 5-ways and 512-sets,
	 * but that is contradicted by all documentation.
	 */
	case MACH_INGENIC_X1000:
	case MACH_INGENIC_X1000E:
		c->scache.sets = 256;
		c->scache.ways = 4;
		break;
	}

	scache_size = c->scache.ways * c->scache.sets * c->scache.linesz;
	if (!scache_size)
		return ;

	c->scache.waysize = c->scache.sets * c->scache.linesz;
	c->scache.waybit = __ffs(scache_size / c->scache.ways);

	c->scache.flags &= ~MIPS_CACHE_NOT_PRESENT;

	if (mips_machtype >= MACH_INGENIC_X2000) {
		c->scache.flags |= MIPS_CACHE_PINDEX;

		/* Enable the L2 prefetch unit */
		write_ccu_mscr(read_ccu_mscr() & ~MSCR_DISPFB2);
	} else {
		write_c0_ingenic_errctl(XBURST_ERRCTL_WST_DIS);
	}

	ingenic_blast_scache_setup();

	pr_info("Unified secondary cache %ldkiB, %s, %d-way, %d sets, linesize %d bytes.\n",
			scache_size >> 10, c->scache.flags & MIPS_CACHE_PINDEX ? "PIPT" : "VIPT",
			c->scache.ways, c->scache.sets, c->scache.linesz);
}

static int cca = -1;

static int __init cca_setup(char *str)
{
	get_option(&str, &cca);

	return 0;
}

early_param("cca", cca_setup);

static void ingenic_coherency_setup(void)
{
	if (cca < 0 || cca > 7)
		cca = read_c0_config() & CONF_CM_CMASK;
	_page_cachable_default = cca << _CACHE_SHIFT;

	pr_debug("Using cache attribute %d\n", cca);
	change_c0_config(CONF_CM_CMASK, cca);
}

static void ingenic_cache_error_setup(void)
{
	extern char __weak except_vec2_generic;

	set_uncached_handler(0x100, &except_vec2_generic, 0x80);
}

void ingenic_cache_init(void)
{
	extern void build_clear_page(void);
	extern void build_copy_page(void);

	probe_pcache();
	probe_scache();

	__flush_cache_vmap = ingenic_cache_noop0;
	__flush_cache_vunmap = ingenic_cache_noop1;
	__flush_cache_all = ingenic_flush_cache_all;

	__local_flush_icache_user_range = local_ingenic_flush_icache_user_range;
	__flush_icache_user_range = ingenic_flush_icache_user_range;

	__flush_kernel_vmap_range = (void *)ingenic_cache_noop2;

	flush_cache_range = ingenic_flush_cache_range;
	flush_cache_page = ingenic_flush_cache_page;
	flush_cache_mm = (void *)ingenic_cache_noop;
	flush_cache_all = ingenic_cache_noop;

	local_flush_icache_range = local_ingenic_flush_icache_range;
	flush_icache_range = ingenic_flush_icache_range;
	flush_icache_all = ingenic_cache_noop;

	local_flush_data_cache_page = (void *)ingenic_cache_noop7;
	flush_data_cache_page = blast_dcache32_page;

	_dma_cache_wback_inv = ingenic_dma_cache_wback_inv;
	_dma_cache_wback = ingenic_dma_cache_wback_inv;

	if (current_cpu_type() == CPU_XBURST)
		_dma_cache_inv = ingenic_xburst_dma_cache_inv;
	else if (current_cpu_type() == CPU_XBURST2)
		_dma_cache_inv = ingenic_xburst2_dma_cache_inv;
	else
		panic("Unknown Ingenic CPU type.");

	build_clear_page();
	build_copy_page();

	local_ingenic_flush_cache_all(NULL);

	ingenic_coherency_setup();
	board_cache_error_setup = ingenic_cache_error_setup;
}

static int ingenic_cache_pm_notifier(struct notifier_block *self,
						     unsigned long cmd, void *v)
{
	switch (cmd) {
	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		ingenic_coherency_setup();
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block ingenic_cache_pm_notifier_block = {
	.notifier_call = ingenic_cache_pm_notifier,
};

int __init ingenic_cache_init_pm(void)
{
	return cpu_pm_register_notifier(&ingenic_cache_pm_notifier_block);
}
arch_initcall(ingenic_cache_init_pm);
