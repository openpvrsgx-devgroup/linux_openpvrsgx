// SPDX-License-Identifier: GPL-2.0
/*
 * This file define the percpu IRQ handler for Ingenic CPU interrupts.
 * Copyright (c) 2014 Ingenic Semiconductor Co., Ltd.
 * Copyright (c) 2021 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 */

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <linux/irqdomain.h>

#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>
#include <asm/mipsmtregs.h>
#include <asm/setup.h>

static inline void unmask_ingenic_irq(struct irq_data *d)
{
	set_c0_status(IE_SW0 << d->hwirq);
}

static inline void mask_ingenic_irq(struct irq_data *d)
{
	clear_c0_status(IE_SW0 << d->hwirq);
}

static struct irq_chip ingenic_cpu_irq_controller = {
	.name			= "CPU IRQ",
	.irq_ack		= mask_ingenic_irq,
	.irq_mask		= mask_ingenic_irq,
	.irq_mask_ack	= mask_ingenic_irq,
	.irq_unmask		= unmask_ingenic_irq,
	.irq_eoi		= unmask_ingenic_irq,
	.irq_disable	= mask_ingenic_irq,
	.irq_enable		= unmask_ingenic_irq,
};

asmlinkage void __weak plat_irq_dispatch(void)
{
	unsigned long pending = read_c0_cause() & read_c0_status() & ST0_IM;

	if (!pending) {
		spurious_interrupt();
		return;
	}

	pending >>= CAUSEB_IP;
	if(pending) {
		if (pending & 8) {  		/* IPI */
			do_IRQ(MIPS_CPU_IRQ_BASE + 3);
		} else if(pending & 16) {	/* OST */
			do_IRQ(MIPS_CPU_IRQ_BASE + 4);
		} else if (pending & 4) {	/* INTC */
			do_IRQ(MIPS_CPU_IRQ_BASE + 2);
		} else {		/* Others */
			do_IRQ(MIPS_CPU_IRQ_BASE + __ffs(pending));
		}
	}
}

static int ingenic_cpu_intc_map(struct irq_domain *d, unsigned int irq,
							 irq_hw_number_t hw)
{
	static struct irq_chip *chip;

	chip = &ingenic_cpu_irq_controller;

	irq_set_percpu_devid(irq);
	irq_set_chip_and_handler(irq, chip, handle_percpu_devid_irq);

	return 0;
}

static const struct irq_domain_ops ingenic_cpu_intc_irq_domain_ops = {
	.map = ingenic_cpu_intc_map,
	.xlate = irq_domain_xlate_onecell,
};

static void __init __ingenic_cpu_irq_init(struct device_node *of_node)
{
	struct irq_domain *domain;

	/* Mask interrupts. */
	clear_c0_status(ST0_IM);
	clear_c0_cause(CAUSEF_IP);

	domain = irq_domain_add_legacy(of_node, 8, MIPS_CPU_IRQ_BASE, 0,
								   &ingenic_cpu_intc_irq_domain_ops, NULL);
	if (!domain)
		panic("Failed to add IRQ irqdomain for Ingenic CPU");
}

void __init ingenic_cpu_irq_init(void)
{
	__ingenic_cpu_irq_init(NULL);
}

int __init ingenic_cpu_irq_of_init(struct device_node *of_node,
								struct device_node *parent)
{
	__ingenic_cpu_irq_init(of_node);
	return 0;
}
IRQCHIP_DECLARE(cpu_intc, "ingenic,cpu-interrupt-controller", ingenic_cpu_irq_of_init);
