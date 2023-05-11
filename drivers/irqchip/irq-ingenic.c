// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Ingenic XBurst SoCs IRQ support
 *  Copyright (C) 2009-2010, Lars-Peter Clausen <lars@metafoo.de>
 *  Copyright (C) 2021, 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/irqchip.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/cpuhotplug.h>

#include <asm/io.h>
#include <asm/mach-ingenic/smp.h>

#define JZ_REG_INTC_STATUS		0x00
#define JZ_REG_INTC_MASK		0x04
#define JZ_REG_INTC_SET_MASK	0x08
#define JZ_REG_INTC_CLEAR_MASK	0x0c
#define JZ_REG_INTC_PENDING		0x10
#define CHIP_SIZE				0x20

#define IF_ENABLED(cfg, ptr)	PTR_IF(IS_ENABLED(cfg), (ptr))

enum ingenic_intc_version {
	ID_JZ4730,
	ID_JZ4740,
	ID_JZ4725B,
	ID_JZ4760,
	ID_JZ4770,
	ID_JZ4775,
	ID_JZ4780,
	ID_X2000,
};

struct ingenic_intc_irqchip {
	struct irq_domain *domain;
	void __iomem *base;
	unsigned num_chips;

	struct irq_chip_generic *gc[];
};

struct ingenic_intc {
	enum ingenic_intc_version version;
	struct irq_domain *domain;
	struct ingenic_intc_irqchip __percpu *irqchips;

	int irq;
};

struct ingenic_intc *ingenic_intc;

static irqreturn_t ingenic_intc_cascade(int irq, void *data)
{
	struct ingenic_intc *intc = ingenic_intc;
	struct ingenic_intc_irqchip *irqchip = data;
	uint32_t pending;
	unsigned i;

	for (i = 0; i < irqchip->num_chips; i++) {
		pending = irq_reg_readl(irqchip->gc[i], JZ_REG_INTC_PENDING);
		if (!pending)
			continue;

		while (pending) {
			int bit = __fls(pending);

			irq = irq_linear_revmap(intc->domain, bit + (i * 32));
			generic_handle_irq(irq);
			pending &= ~BIT(bit);
		}
	}

	if(IS_ENABLED(CONFIG_MACH_JZ4780))
		IF_ENABLED(CONFIG_SMP, jz4780_smp_switch_irqcpu(smp_processor_id()));

	return IRQ_HANDLED;
}

static int __init ingenic_intc_setup_irqchip(unsigned int cpu)
{
	struct ingenic_intc *intc = ingenic_intc;
	struct ingenic_intc_irqchip *irqchip;
	struct irq_chip_type *ct;
	unsigned int i;

	if (intc->version >= ID_X2000)
		irqchip = per_cpu_ptr(intc->irqchips, cpu);
	else
		irqchip = intc->irqchips;

	for (i = 0; i < irqchip->num_chips; i++) {
		irqchip->gc[i] = irq_get_domain_generic_chip(intc->domain, i * 32);

		irqchip->gc[i]->wake_enabled = IRQ_MSK(32);
		irqchip->gc[i]->reg_base = irqchip->base + (i * CHIP_SIZE);

		ct = irqchip->gc[i]->chip_types;
		ct->regs.enable = JZ_REG_INTC_CLEAR_MASK;
		ct->regs.disable = JZ_REG_INTC_SET_MASK;
		ct->chip.irq_unmask = irq_gc_unmask_enable_reg;
		ct->chip.irq_mask = irq_gc_mask_disable_reg;
		ct->chip.irq_mask_ack = irq_gc_mask_disable_reg;
		ct->chip.irq_set_wake = irq_gc_set_wake;
		ct->chip.flags = IRQCHIP_MASK_ON_SUSPEND;

		/* Mask all irqs */
		irq_reg_writel(irqchip->gc[i], IRQ_MSK(32), JZ_REG_INTC_SET_MASK);
	}

	if (intc->version >= ID_X2000)
		enable_percpu_irq(intc->irq, IRQ_TYPE_NONE);

	return 0;
}

static const struct of_device_id __maybe_unused ingenic_intc_of_matches[] __initconst = {
	{ .compatible = "ingenic,jz4730-intc", .data = (void *) ID_JZ4730 },
	{ .compatible = "ingenic,jz4740-intc", .data = (void *) ID_JZ4740 },
	{ .compatible = "ingenic,jz4725b-intc", .data = (void *) ID_JZ4725B },
	{ .compatible = "ingenic,jz4760-intc", .data = (void *) ID_JZ4760 },
	{ .compatible = "ingenic,jz4770-intc", .data = (void *) ID_JZ4770 },
	{ .compatible = "ingenic,jz4775-intc", .data = (void *) ID_JZ4775 },
	{ .compatible = "ingenic,jz4780-intc", .data = (void *) ID_JZ4780 },
	{ .compatible = "ingenic,x2000-intc", .data = (void *) ID_X2000 },
	{ /* sentinel */ }
};

static int __init ingenic_intc_probe(struct device_node *np, unsigned num_chips)
{
	const struct of_device_id *id = of_match_node(ingenic_intc_of_matches, np);
	struct ingenic_intc_irqchip *irqchip;
	struct ingenic_intc *intc;
	void __iomem *base;
	unsigned int cpu;
	int ret;

	intc = kzalloc(sizeof(*intc), GFP_KERNEL);
	if (!intc)
		return -ENOMEM;

	intc->version = (enum ingenic_intc_version)id->data;

	if (intc->version >= ID_X2000)
		intc->irqchips = __alloc_percpu(struct_size(irqchip, gc, num_chips), 1);
	else
		intc->irqchips = kzalloc(struct_size(irqchip, gc, num_chips), GFP_KERNEL);

	if (!intc->irqchips) {
		ret = -ENOMEM;
		goto out_free;
	}

	base = of_io_request_and_map(np, 0, of_node_full_name(np));
	if (!base) {
		pr_err("%s: Failed to map INTC registers\n", __func__);
		ret = PTR_ERR(base);
		goto out_free_percpu;
	}

	intc->irq = irq_of_parse_and_map(np, 0);
	if (!intc->irq) {
		pr_crit("%s: Cannot to get INTC IRQ\n", __func__);
		ret = -EINVAL;
		goto out_unmap_base;
	}

	for (cpu = 0; cpu < num_possible_cpus(); cpu++) {
		if (intc->version >= ID_X2000) {
			irqchip = per_cpu_ptr(intc->irqchips, cpu);
			irqchip->base = base + 0x1000 * cpu;
		} else {
			irqchip = intc->irqchips;
			irqchip->base = base;
		}

		irqchip->num_chips = num_chips;
	}

	ingenic_intc = intc;

	return 0;

out_unmap_base:
	iounmap(base);
out_free_percpu:
	free_percpu(intc->irqchips);
out_free:
	kfree(intc);
	return ret;
}

static int __init ingenic_intc_of_init(struct device_node *np,
				       unsigned num_chips)
{
	struct ingenic_intc *intc;
	struct irq_domain *domain;
	int ret;

	ret = ingenic_intc_probe(np, num_chips);
	if (ret) {
		pr_crit("%s: Failed to initialize INTC clocks: %d\n", __func__, ret);
		return ret;
	}

	intc = ingenic_intc;
	if (IS_ERR(intc))
		return PTR_ERR(intc);

	domain = irq_domain_add_linear(np, num_chips * 32, &irq_generic_chip_ops, NULL);
	if (!domain) {
		ret = -ENOMEM;
		goto out_unmap_irq;
	}

	intc->domain = domain;

	ret = irq_alloc_domain_generic_chips(domain, 32, 1, "INTC",
					     handle_level_irq, 0, IRQ_NOPROBE | IRQ_LEVEL, 0);
	if (ret)
		goto out_domain_remove;

	if (intc->version >= ID_X2000) {
		ret = request_percpu_irq(intc->irq, ingenic_intc_cascade,
				"SoC intc cascade interrupt", intc->irqchips);
		if (ret) {
			pr_err("Failed to register SoC intc cascade interrupt\n");
			goto out_domain_remove;
		}

		/* Setup irqchips on each CPU core */
		ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "Ingenic XBurst: online",
					ingenic_intc_setup_irqchip, NULL);
		if (ret < 0) {
			pr_crit("%s: Unable to init percpu irqchips: %x\n", __func__, ret);
			goto out_domain_remove;
		}

	} else {
		ret = request_irq(intc->irq, ingenic_intc_cascade, IRQF_NO_SUSPEND,
				"SoC intc cascade interrupt", intc->irqchips);
		if (ret) {
			pr_err("Failed to register SoC intc cascade interrupt\n");
			goto out_domain_remove;
		}

		ret = ingenic_intc_setup_irqchip(0);
		if (ret < 0) {
			pr_crit("%s: Unable to init percpu irqchips: %x\n", __func__, ret);
			goto out_domain_remove;
		}
	}

	return 0;

out_domain_remove:
	irq_domain_remove(domain);
out_unmap_irq:
	irq_dispose_mapping(intc->irq);
	free_percpu(intc->irqchips);
	kfree(intc);
	return ret;
}

static int __init intc_1chip_of_init(struct device_node *np,
				     struct device_node *parent)
{
	return ingenic_intc_of_init(np, 1);
}
IRQCHIP_DECLARE(jz4730_intc, "ingenic,jz4730-intc", intc_1chip_of_init);
IRQCHIP_DECLARE(jz4740_intc, "ingenic,jz4740-intc", intc_1chip_of_init);
IRQCHIP_DECLARE(jz4725b_intc, "ingenic,jz4725b-intc", intc_1chip_of_init);

static int __init intc_2chip_of_init(struct device_node *np,
	struct device_node *parent)
{
	return ingenic_intc_of_init(np, 2);
}
IRQCHIP_DECLARE(jz4760_intc, "ingenic,jz4760-intc", intc_2chip_of_init);
IRQCHIP_DECLARE(jz4770_intc, "ingenic,jz4770-intc", intc_2chip_of_init);
IRQCHIP_DECLARE(jz4775_intc, "ingenic,jz4775-intc", intc_2chip_of_init);
IRQCHIP_DECLARE(jz4780_intc, "ingenic,jz4780-intc", intc_2chip_of_init);
IRQCHIP_DECLARE(x2000_intc, "ingenic,x2000-intc", intc_2chip_of_init);
