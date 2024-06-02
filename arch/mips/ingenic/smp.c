// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic XBurst SoCs SMP support
 * Copyright (c) 2013 Paul Burton <paul.burton@imgtec.com>
 * Copyright (c) 2021 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 */

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/sched.h>
#include <linux/sched/hotplug.h>
#include <linux/sched/task_stack.h>
#include <linux/smp.h>
#include <linux/tick.h>

#include <asm/mach-ingenic/smp.h>
#include <asm/r4kcache.h>
#include <asm/smp-ops.h>
#include <asm/tlbflush.h>

struct ingenic_xburst2_mailbox {
	void *__iomem *base;

	spinlock_t lock;
};

u32 ingenic_cpu_entry_sp;
u32 ingenic_cpu_entry_gp;

static struct ingenic_xburst2_mailbox __percpu *ingenic_xburst2_mailbox;

static struct clk *cpu_clock_gates[CONFIG_NR_CPUS] = { NULL };

static struct cpumask cpu_running;

static DEFINE_SPINLOCK(smp_lock);

#define XBURST_TAGLO_DIRTY_MASK	0xc

static inline __always_inline notrace void ingenic_wback_dcache(void)
{
	unsigned long start = INDEX_BASE;
	unsigned long end = start + current_cpu_data.dcache.waysize;
	unsigned long ws_inc = 1UL << current_cpu_data.dcache.waybit;
	unsigned long ws_end =
		current_cpu_data.dcache.ways << current_cpu_data.dcache.waybit;
	unsigned long ws, addr, tmp;

	/*
	 * Doing a writeback/invalidate on the whole cache has a significant
	 * performance cost. In this loop we instead only writeback/invalidate
	 * cache lines which are marked dirty. To do this we load the tag at
	 * each index and check the (Ingenic-specific) dirty bits, and only
	 * perform the operation if they are set. There is still a performance
	 * cost to this but it is nowhere near as high as blasting the whole
	 * cache.
	 */
	for (ws = 0; ws < ws_end; ws += ws_inc) {
		for (addr = start; addr < end; addr += 0x80) {
			__asm__ __volatile__ (
			"	.set	push							\n"
			"	.set	noreorder						\n"
			"	.set	mips32r2						\n"
			"	cache	%2, 0x00(%1)					\n"
			"	mfc0	%0, "__stringify(CP0_TAGLO)"	\n"
			"	and		%0, %0, %3						\n"
			"	beq		$0, %0, 1f						\n"
			"	cache	%2, 0x20(%1)					\n"
			"	cache	%4, 0x00(%1)					\n"
			"1:	mfc0	%0, "__stringify(CP0_TAGLO)"	\n"
			"	and		%0, %0, %3						\n"
			"	beq		$0, %0, 2f						\n"
			"	cache	%2, 0x40(%1)					\n"
			"	cache	%4, 0x20(%1)					\n"
			"2:	mfc0	%0, "__stringify(CP0_TAGLO)"	\n"
			"	and		%0, %0, %3						\n"
			"	beq		$0, %0, 3f						\n"
			"	cache	%2, 0x60(%1)					\n"
			"	cache	%4, 0x40(%1)					\n"
			"3:	mfc0	%0, "__stringify(CP0_TAGLO)"	\n"
			"	and		%0, %0, %3						\n"
			"	beq		$0, %0, 4f						\n"
			"	nop										\n"
			"	cache	%4, 0x60(%1)					\n"
			"4:	.set	pop								\n"

			:	"=&r"	(tmp)
			:	"r"		(addr | ws),
				"i"		(Index_Load_Tag_D),
				"i"		(XBURST_TAGLO_DIRTY_MASK),
				"i"		(Index_Writeback_Inv_D));
		}
	}
}

/*
 * The Ingenic XBurst SMP variant has to write back dirty cache lines before
 * executing wait. The CPU & cache clock will be gated until we return from
 * the wait, and if another core attempts to access data from our data cache
 * during this time then it will lock up.
 */
void jz4780_smp_wait_irqoff(void)
{
	unsigned long pending = read_c0_cause() & read_c0_status() & CAUSEF_IP;

	/*
	 * Going to idle has a significant overhead due to the cache flush so
	 * try to avoid it if we'll immediately be woken again due to an IRQ.
	 */
	if (!need_resched() && !pending) {
		ingenic_wback_dcache();

		__asm__ __volatile__ (
		"	.set push		\n"
		"	.set mips32r2	\n"
		"	sync			\n"
		"	wait			\n"
		"	.set pop		\n");
	}

	local_irq_enable();
}

void jz4780_smp_switch_irqcpu(int cpu)
{
	int status;

	spin_lock(&smp_lock);

	if (cpu_online(!cpu)) {
		status = read_c0_reim();
		status &= ~(REIM_IRQ1M | REIM_IRQ0M);
		status |= REIM_IRQ0M << (!cpu);
		write_c0_reim(status);
	}

	spin_unlock(&smp_lock);
}

static irqreturn_t ingenic_xburst_mbox_handler(int irq, void *dev_id)
{
	int cpu = smp_processor_id();
	u32 action, status;

	spin_lock(&smp_lock);

	switch (cpu) {
	case 0:
		action = read_c0_mailbox0();
		write_c0_mailbox0(0);
		break;
	case 1:
		action = read_c0_mailbox1();
		write_c0_mailbox1(0);
		break;
	case 2:
		action = read_c0_mailbox2();
		write_c0_mailbox2(0);
		break;
	case 3:
		action = read_c0_mailbox3();
		write_c0_mailbox3(0);
		break;
	default:
		panic("Unhandled CPU %d!", cpu);
	}

	/* clear pending mailbox interrupt */
	status = read_c0_corestatus();
	status &= ~(CORESTATUS_MIRQ0P << cpu);
	write_c0_corestatus(status);

	spin_unlock(&smp_lock);

	if (action & SMP_RESCHEDULE_YOURSELF)
		scheduler_ipi();
	if (action & SMP_CALL_FUNCTION)
		generic_smp_call_function_interrupt();

	return IRQ_HANDLED;
}

static irqreturn_t ingenic_xburst2_mbox_handler(int irq, void *dev_id)
{
	struct ingenic_xburst2_mailbox *mailbox = dev_id;
	unsigned int action = 0;

	spin_lock(&mailbox->lock);
	action = readl(mailbox->base);
	writel(0, mailbox->base);
	spin_unlock(&mailbox->lock);

	if (!action) {
		pr_err("SMP[%d]:invalid mailboxes action is NULL\n",smp_processor_id());
		goto ipi_finish;
	}

	if (action & SMP_RESCHEDULE_YOURSELF)
		scheduler_ipi();
	if (action & SMP_CALL_FUNCTION)
		generic_smp_call_function_interrupt();

ipi_finish:
	return IRQ_HANDLED;
}

static void ingenic_smp_setup(void)
{
	struct device_node *cpu_node;
	u32 addr = KSEG1ADDR((u32)&ingenic_secondary_cpu_entry);
	int cpu = 0;
	u32 val;

	for_each_of_cpu_node(cpu_node) {
		__cpu_number_map[cpu] = cpu;
		__cpu_logical_map[cpu] = cpu;
		set_cpu_possible(cpu++, true);
	}

	switch (current_cpu_type()) {
	case CPU_XBURST:

		/* mask mailbox interrupts for this core */
		val = read_c0_reim();
		val &= ~REIM_MBOXIRQ0M;
		write_c0_reim(val);

		/* clear mailboxes & pending mailbox IRQs */
		write_c0_mailbox0(0);
		write_c0_mailbox1(0);
		write_c0_mailbox2(0);
		write_c0_mailbox3(0);
		write_c0_corestatus(0);

		/* set reset entry point */
		WARN_ON(addr & ~REIM_ENTRY);
		val &= ~REIM_ENTRY;
		val |= addr & REIM_ENTRY;

		/* unmask mailbox interrupts for this core */
		val |= REIM_MBOXIRQ0M;
		write_c0_reim(val);
		break;
	case CPU_XBURST2:

		/* declare we are SMT capable */
		smp_num_siblings = 2;

		/* mask mailbox interrupts for this core */
		val = read_ccu_mimr();
		val &= ~MIMR_IM0;
		write_ccu_mimr(val);

		/* clear mailboxes */
		write_ccu_mbr0(0);
		write_ccu_mbr1(0);
		write_ccu_mbr2(0);
		write_ccu_mbr3(0);

		/* set reset entry point */
		write_ccu_rer(addr);

		/* unmask mailbox interrupts for this core */
		val |= MIMR_IM0;
		write_ccu_mimr(val);
		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}

	set_c0_status(STATUSF_IP3);

	cpumask_set_cpu(cpu, &cpu_running);
}

static void ingenic_xburst_smp_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *cpu_node;
	unsigned ctrl;
	int cpu, err;

	/* setup the mailbox IRQ */
	err = request_irq(MIPS_CPU_IRQ_BASE + 3, ingenic_xburst_mbox_handler,
			IRQF_PERCPU | IRQF_NO_THREAD, "core mailbox", NULL);
	if (err)
		pr_err("request_irq() on core mailbox failed\n");

	ctrl = read_c0_corectrl();

	for_each_of_cpu_node(cpu_node) {
		cpu = of_cpu_node_to_id(cpu_node);
		if (cpu < 0) {
			pr_err("Failed to read index of %s\n", cpu_node->full_name);
			continue;
		}

		/* use reset entry point from REIM register */
		ctrl |= CORECTRL_RPC0 << cpu;

		cpu_clock_gates[cpu] = of_clk_get(cpu_node, 0);
		if (IS_ERR(cpu_clock_gates[cpu])) {
			cpu_clock_gates[cpu] = NULL;
			continue;
		}
	}

	write_c0_corectrl(ctrl);
}

static void ingenic_xburst2_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *cpu_node;
	struct ingenic_xburst2_mailbox *mailbox;
	unsigned pimr, oimr;
	int cpu, err;

	ingenic_xburst2_mailbox = alloc_percpu(struct ingenic_xburst2_mailbox);
	if(!ingenic_xburst2_mailbox)
		pr_err("Failed to alloc percpu mailbox\n");

	/* setup the mailbox IRQ */
	err = request_percpu_irq(MIPS_CPU_IRQ_BASE + 3, ingenic_xburst2_mbox_handler,
			"core mailbox", ingenic_xburst2_mailbox);
	if (err)
		pr_err("request_percpu_irq() on core mailbox failed\n");

	pimr = read_ccu_pimr();
	oimr = read_ccu_oimr();

	for_each_of_cpu_node(cpu_node) {
		cpu = of_cpu_node_to_id(cpu_node);
		if (cpu < 0) {
			pr_err("Failed to read index of %s\n", cpu_node->full_name);
			continue;
		}

		/* unmask peripheral and OST interrupts for each core */
		pimr |= PIMR_IM0 << cpu;
		oimr |= OIMR_IM0 << cpu;

		mailbox = per_cpu_ptr(ingenic_xburst2_mailbox, cpu);

		mailbox->base = CCU_MAILBOX_BASE + cpu * 4;

		enable_percpu_irq(MIPS_CPU_IRQ_BASE + 3, IRQ_TYPE_NONE);

		spin_lock_init(&mailbox->lock);

		cpu_clock_gates[cpu] = of_clk_get(cpu_node, 0);
		if (IS_ERR(cpu_clock_gates[cpu])) {
			cpu_clock_gates[cpu] = NULL;
			continue;
		}
	}

	write_ccu_pimr(pimr);
	write_ccu_oimr(oimr);
}

/*
 * Tell the hardware to boot CPUx, runs on CPU0.
 */
static int ingenic_boot_secondary(int cpu, struct task_struct *idle)
{
	unsigned long flags;
	int err;
	u32 val;

	spin_lock_irqsave(&smp_lock, flags);

	/* ensure the core is in reset */
	switch (current_cpu_type()) {
	case CPU_XBURST:
		val = read_c0_corectrl();
		val |= CORECTRL_SWRST0 << cpu;
		write_c0_corectrl(val);
		break;
	case CPU_XBURST2:
		val = read_ccu_csrr();
		val |= CSRR_SRE0 << cpu;
		write_ccu_csrr(val);
		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}

	/* ungate core clock */
	if (cpu_clock_gates[cpu]) {
		err = clk_prepare(cpu_clock_gates[cpu]);
		if (err)
			pr_err("Failed to prepare CPU clock gate\n");

		err = clk_enable(cpu_clock_gates[cpu]);
		if (err)
			pr_err("Failed to ungate core clock\n");
	}

	/* set entry sp/gp register values */
	ingenic_cpu_entry_sp = __KSTK_TOS(idle);
	ingenic_cpu_entry_gp = (u32)task_thread_info(idle);
	smp_wmb();

	/* take the core out of reset */
	switch (current_cpu_type()) {
	case CPU_XBURST:
		val &= ~(CORECTRL_SWRST0 << cpu);
		write_c0_corectrl(val);
		break;
	case CPU_XBURST2:
		val &= ~(CSRR_SRE0 << cpu);
		write_ccu_csrr(val);
		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}

	cpumask_set_cpu(cpu, &cpu_running);

	spin_unlock_irqrestore(&smp_lock, flags);

	return 0;
}

/*
 * Early setup, runs on secondary CPU after cache probe.
 */
static void ingenic_init_secondary(void)
{
	int cpu = smp_processor_id();

	switch (current_cpu_type()) {
	case CPU_XBURST:
		break;
	case CPU_XBURST2:
		cpu_set_core(&current_cpu_data, cpu_logical_map(cpu) / INGENIC_XBURST2_THREADS_PER_CORE);
		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}
}

/*
 * Late setup, runs on secondary CPU before entering the idle loop.
 */
static void ingenic_smp_finish(void)
{
	int cpu = smp_processor_id();
	u32 val;

	spin_lock(&smp_lock);

	/* unmask mailbox interrupts for this core */
	switch (current_cpu_type()) {
	case CPU_XBURST:
		val = read_c0_reim();
		val |= REIM_MBOXIRQ0M << cpu;
		write_c0_reim(val);
		break;
	case CPU_XBURST2:
		val = read_ccu_mimr();
		val |= MIMR_IM0 << cpu;
		write_ccu_mimr(val);
		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}

	spin_unlock(&smp_lock);

	/* unmask interrupts for this core */
	change_c0_status(ST0_IM, STATUSF_IP4 | STATUSF_IP3 |
			STATUSF_IP2 | STATUSF_IP1 | STATUSF_IP0);

	/* force broadcast timer */
	tick_broadcast_force();
}

static void ingenic_xburst_send_ipi_single_locked(int cpu, unsigned int action)
{
	u32 mbox;

	switch (cpu) {
	case 0:
		mbox = read_c0_mailbox0();
		write_c0_mailbox0(mbox | action);
		break;
	case 1:
		mbox = read_c0_mailbox1();
		write_c0_mailbox1(mbox | action);
		break;
	case 2:
		mbox = read_c0_mailbox2();
		write_c0_mailbox2(mbox | action);
		break;
	case 3:
		mbox = read_c0_mailbox3();
		write_c0_mailbox3(mbox | action);
		break;
	default:
		panic("Unhandled CPU %d!", cpu);
	}
}

static void ingenic_xburst_send_ipi_single(int cpu, unsigned int action)
{
	unsigned long flags;

	spin_lock_irqsave(&smp_lock, flags);
	ingenic_xburst_send_ipi_single_locked(cpu, action);
	spin_unlock_irqrestore(&smp_lock, flags);
}

static void ingenic_xburst2_send_ipi_single(int cpu, unsigned int action)
{
	struct ingenic_xburst2_mailbox *mailbox = per_cpu_ptr(ingenic_xburst2_mailbox, cpu);
	unsigned int val;
	unsigned long flags;

	spin_lock_irqsave(&mailbox->lock, flags);
	val = readl(mailbox->base);
	writel(action | val, mailbox->base);
	spin_unlock_irqrestore(&mailbox->lock, flags);
}

static void ingenic_xburst_send_ipi_mask(const struct cpumask *mask,
				 unsigned int action)
{
	int cpu;

	for_each_cpu(cpu, mask)
		ingenic_xburst_send_ipi_single(cpu, action);
}

static void ingenic_xburst2_send_ipi_mask(const struct cpumask *mask,
				 unsigned int action)
{
	int cpu;

	for_each_cpu(cpu, mask)
		ingenic_xburst2_send_ipi_single(cpu, action);
}

#ifdef CONFIG_HOTPLUG_CPU
static int ingenic_cpu_disable(void)
{
	int cpu = smp_processor_id();
	unsigned int val;

	local_irq_disable();

	set_cpu_online(cpu, false);
	calculate_cpu_foreign_map();

	spin_lock(&smp_lock);

	switch (current_cpu_type()) {
	case CPU_XBURST:
		val = read_c0_reim();
		if (val & (REIM_IRQ0M << cpu)) {
			val &= ~(REIM_IRQ0M << cpu);
			val |= REIM_IRQ0M; /* peripheral irq to cpu0 */
			write_c0_reim(val);
		}

		break;
	case CPU_XBURST2:
		val = read_ccu_pimr();
		if (val & (PIMR_IM0 << cpu)) {
			val &= ~(PIMR_IM0 << cpu);
			val |= (PIMR_IM0); /* peripheral irq to cpu0 */
			write_ccu_pimr(val);
		}

		val = read_ccu_oimr();
		if (val & (OIMR_IM0 << cpu)) {
			val &= ~(OIMR_IM0 << cpu);
			val |= (OIMR_IM0); /* OST irq to cpu0 */
			write_ccu_oimr(val);
		}

		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}

	spin_unlock(&smp_lock);

	clear_tasks_mm_cpumask(cpu);

	return 0;
}

static void ingenic_cpu_die(unsigned int cpu)
{
	unsigned long flags;
	unsigned int val;

	local_irq_save(flags);

	cpumask_clear_cpu(cpu, &cpu_running);

	wmb();

	switch (current_cpu_type()) {
	case CPU_XBURST:
		while (!(read_c0_corestatus() & (CORESTATUS_SLEEP0 << cpu)));

		val = read_c0_corectrl();
		val |= CORECTRL_SWRST0 << cpu;
		write_c0_corectrl(val);
		break;
	case CPU_XBURST2:
		while (!(read_ccu_cssr() & (CSSR_SS0 << cpu)));

		val = read_ccu_csrr();
		val |= CSRR_SRE0 << cpu;
		write_ccu_csrr(val);
		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}

	clk_disable_unprepare(cpu_clock_gates[cpu]);

	local_irq_restore(flags);
}

static void __ingenic_play_dead(void)
{
	__asm__ __volatile__ (
	"	.set	push		\n"
	"	.set	mips32r2	\n"
	"	sync				\n"
	"	wait				\n"
	"	.set	pop			\n");
}

static void ingenic_xburst_play_dead(void) {
	void (*do_play_dead)(void) = (void *)KSEG1ADDR(__ingenic_play_dead);
	int cpu = smp_processor_id();

	local_irq_disable();

	switch (cpu) {
	case 0:
		write_c0_mailbox0(0);
		break;
	case 1:
		write_c0_mailbox1(0);
		break;
	}

	smp_clr_pending(CORESTATUS_MIRQ0P << cpu);

	while(1) {
		while(cpumask_test_cpu(cpu, &cpu_running))
			;
		blast_icache32();
		blast_dcache32();

		do_play_dead();
	}
}

static void ingenic_xburst2_play_dead(void)
{
	void (*do_play_dead)(void) = (void *)KSEG1ADDR(__ingenic_play_dead);
	int cpu = smp_processor_id();
	unsigned int val;

	val = read_ccu_mimr();
	val &= ~(MIMR_IM0 << cpu);
	write_ccu_mimr(val);
	idle_task_exit();
	local_irq_disable();
	local_flush_tlb_all();

	/* isn't smt thread core */
	if(!cpu_online(((cpu + 1) & 1) | (cpu & (~1))))
		blast_dcache32();

	do_play_dead();

	pr_err("ERROR:SMP[%d]: shouldn't run here.", cpu);

	while(1)
		do_play_dead();
}

void play_dead(void)
{
	switch (current_cpu_type()) {
	case CPU_XBURST:
		ingenic_xburst_play_dead();
		break;
	case CPU_XBURST2:
		ingenic_xburst2_play_dead();
		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}
}
#endif

static struct plat_smp_ops ingenic_xburst_smp_ops = {
	.send_ipi_single	= ingenic_xburst_send_ipi_single,
	.send_ipi_mask		= ingenic_xburst_send_ipi_mask,
	.init_secondary		= ingenic_init_secondary,
	.smp_finish			= ingenic_smp_finish,
	.boot_secondary		= ingenic_boot_secondary,
	.smp_setup			= ingenic_smp_setup,
	.prepare_cpus		= ingenic_xburst_smp_prepare_cpus,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable		= ingenic_cpu_disable,
	.cpu_die			= ingenic_cpu_die,
#endif
};

struct plat_smp_ops ingenic_xburst2_smp_ops = {
	.send_ipi_single	= ingenic_xburst2_send_ipi_single,
	.send_ipi_mask		= ingenic_xburst2_send_ipi_mask,
	.init_secondary		= ingenic_init_secondary,
	.smp_finish			= ingenic_smp_finish,
	.boot_secondary		= ingenic_boot_secondary,
	.smp_setup			= ingenic_smp_setup,
	.prepare_cpus		= ingenic_xburst2_prepare_cpus,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable		= ingenic_cpu_disable,
	.cpu_die			= ingenic_cpu_die,
#endif
};

void __init ingenic_smp_init(void)
{
	switch (current_cpu_type()) {
	case CPU_XBURST:
		register_smp_ops(&ingenic_xburst_smp_ops);
		break;
	case CPU_XBURST2:
		register_smp_ops(&ingenic_xburst2_smp_ops);
		break;
	default:
		panic("Unknown Ingenic CPU type.");
	}
}
