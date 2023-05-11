// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic SoCs TCU IRQ driver
 * Copyright (C) 2019 Paul Cercueil <paul@crapouillou.net>
 * Copyright (C) 2020 周琰杰 (Zhou Yanjie) <zhouyanjie@wanyeetech.com>
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/cpuhotplug.h>
#include <linux/interrupt.h>
#include <linux/mfd/ingenic-tcu.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/overflow.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/sched_clock.h>

#include <dt-bindings/clock/ingenic,tcu.h>

static DEFINE_PER_CPU(call_single_data_t, ingenic_cevt_csd);

#define updateb(addr, mask, value) writeb(((readb(addr) & ~(mask)) | ((value) & (mask))), (addr))
#define updatew(addr, mask, value) writew(((readw(addr) & ~(mask)) | ((value) & (mask))), (addr))
#define updatel(addr, mask, value) writel(((readl(addr) & ~(mask)) | ((value) & (mask))), (addr))

struct ingenic_soc_info {
	unsigned int num_channels;
	bool jz4740_regs;
	int counter_width;
	u32 max_count;
};

struct ingenic_tcu_timer {
	unsigned int cpu;
	unsigned int channel;
	struct clock_event_device cevt;
	struct clk *clk;
	char name[8];
};

struct ingenic_tcu {
	const struct ingenic_soc_info *soc_info;
	void *base;
	struct regmap *map;
	struct device_node *np;
	struct clk *cs_clk;
	unsigned int cs_channel;
	struct clocksource cs;
	unsigned long pwm_channels_mask;
	struct ingenic_tcu_timer timers[];
};

static struct ingenic_tcu *ingenic_tcu;

extern unsigned int *ingenic_tcu_intc_affinity;

static u64 notrace ingenic_tcu_timer_read(void)
{
	struct ingenic_tcu *tcu = ingenic_tcu;
	unsigned int count;

	if (tcu->soc_info->jz4740_regs) {
// FIXME: should also bypass regmap
		regmap_read(tcu->map, TCU_REG_TCNTc(tcu->cs_channel), &count);
	} else {
// should use code similar to https://elixir.bootlin.com/linux/latest/source/drivers/clocksource/ingenic-ost.c#L86
// to ioremap during probe or init

		int timeout = 100;

		count = tcu->soc_info->max_count - readl(tcu->base + TCU_JZ4730_REG_TCNTc(tcu->cs_channel));

#if 0
		/* poll for the SF bit and then read OTCRD instead of OTCNT */
		while (!timeout && (readw(tcu->base + TCU_JZ4730_REG_TCSRc(tcu->cs_channel)) & TCU_JZ4730_TCSR_BUSY))
			timeout--;
		count = tcu->soc_info->max_count - readl(tcu->base + TCU_JZ4730_REG_TCRDc(tcu->cs_channel) + 0xc);
#endif
	}

	return count;
}

static u64 notrace ingenic_tcu_timer_cs_read(struct clocksource *cs)
{
	return ingenic_tcu_timer_read();
}

static inline struct ingenic_tcu *
to_ingenic_tcu(struct ingenic_tcu_timer *timer)
{
	return container_of(timer, struct ingenic_tcu, timers[timer->cpu]);
}

static inline struct ingenic_tcu_timer *
to_ingenic_tcu_timer(struct clock_event_device *evt)
{
	return container_of(evt, struct ingenic_tcu_timer, cevt);
}

static int ingenic_tcu_cevt_set_state_shutdown(struct clock_event_device *evt)
{
	struct ingenic_tcu_timer *timer = to_ingenic_tcu_timer(evt);
	struct ingenic_tcu *tcu = to_ingenic_tcu(timer);

	if (tcu->soc_info->jz4740_regs)
		regmap_write(tcu->map, TCU_REG_TECR, BIT(timer->channel));
	else
		updateb(tcu->base + TCU_JZ4730_REG_TER, BIT(timer->channel), 0);

	return 0;
}

static int ingenic_tcu_cevt_set_next(unsigned long next,
				     struct clock_event_device *evt)
{
	struct ingenic_tcu_timer *timer = to_ingenic_tcu_timer(evt);
	struct ingenic_tcu *tcu = to_ingenic_tcu(timer);

	if (next > tcu->soc_info->max_count)
		return -EINVAL;

	if (tcu->soc_info->jz4740_regs) {
		regmap_write(tcu->map, TCU_REG_TDFRc(timer->channel), next);
		regmap_write(tcu->map, TCU_REG_TCNTc(timer->channel), 0);
		regmap_write(tcu->map, TCU_REG_TESR, BIT(timer->channel));
	} else {
		writel(next, tcu->base + TCU_JZ4730_REG_TCNTc(timer->channel));
		updateb(tcu->base + TCU_JZ4730_REG_TER, BIT(timer->channel), BIT(timer->channel));
	}

	return 0;
}

static void ingenic_per_cpu_event_handler(void *info)
{
	struct clock_event_device *cevt = (struct clock_event_device *) info;

	cevt->event_handler(cevt);
}

static irqreturn_t ingenic_tcu_cevt_cb(int irq, void *dev_id)
{
	struct ingenic_tcu_timer *timer = dev_id;
	struct ingenic_tcu *tcu = to_ingenic_tcu(timer);
	call_single_data_t *csd;

	if (tcu->soc_info->jz4740_regs)
		regmap_write(tcu->map, TCU_REG_TECR, BIT(timer->channel));
	else {
		updateb(tcu->base + TCU_JZ4730_REG_TER, BIT(timer->channel), 0);
		/* reset underflow and IRQ */
		updateb(tcu->base + TCU_JZ4730_REG_TCSRc(timer->channel), TCU_JZ4730_TCSR_FLAG, 0);
		}

	if (timer->cevt.event_handler) {
		csd = &per_cpu(ingenic_cevt_csd, timer->cpu);
		csd->info = (void *) &timer->cevt;
		csd->func = ingenic_per_cpu_event_handler;
		smp_call_function_single_async(timer->cpu, csd);
	}

	return IRQ_HANDLED;
}

static struct clk *ingenic_tcu_get_clock(struct device_node *np, int id)
{
	struct of_phandle_args args;

	args.np = np;
	args.args_count = 1;
	args.args[0] = id;

	return of_clk_get_from_provider(&args);
}

static int ingenic_tcu_setup_cevt(unsigned int cpu)
{
	struct ingenic_tcu *tcu = ingenic_tcu;
	struct ingenic_tcu_timer *timer = &tcu->timers[cpu];
	unsigned int timer_virq;
	unsigned long rate;
	int err;

	timer->clk = ingenic_tcu_get_clock(tcu->np, timer->channel);
	if (IS_ERR(timer->clk))
		return PTR_ERR(timer->clk);

	err = clk_prepare_enable(timer->clk);
	if (err)
		goto err_clk_put;

	rate = clk_get_rate(timer->clk);
	if (!rate) {
		err = -EINVAL;
		goto err_clk_disable;
	}

	if (tcu->soc_info->jz4740_regs) {
		struct irq_domain *domain = irq_find_host(tcu->np);

		if (!domain) {
			err = -ENODEV;
			goto err_clk_disable;
		}

		timer_virq = irq_create_mapping(domain, timer->channel);
	} else {
		timer_virq = of_irq_get(tcu->np, timer->channel);
		if (!timer_virq) {
			err = -EINVAL;
			goto err_clk_disable;
		}
	}

	snprintf(timer->name, sizeof(timer->name), "TCU%u", timer->channel);

	err = request_irq(timer_virq, ingenic_tcu_cevt_cb, IRQF_TIMER,
			  timer->name, timer);
	if (err)
		goto err_irq_dispose_mapping;

	ingenic_tcu_intc_affinity[cpu] = BIT(timer->channel);

	timer->cpu = smp_processor_id();
	timer->cevt.cpumask = cpumask_of(smp_processor_id());
	timer->cevt.features = CLOCK_EVT_FEAT_ONESHOT;
	timer->cevt.name = timer->name;
	timer->cevt.rating = 200;
	timer->cevt.set_state_shutdown = ingenic_tcu_cevt_set_state_shutdown;
	timer->cevt.set_next_event = ingenic_tcu_cevt_set_next;

printk("%s: %d vs. %d\n", __func__, tcu->soc_info->max_count, 0xffff);

	clockevents_config_and_register(&timer->cevt, rate, 10, tcu->soc_info->max_count);

	if (!tcu->soc_info->jz4740_regs) {
		/* enable underflow interrupt */
		updatew(tcu->base + TCU_JZ4730_REG_TCSRc(timer->channel),
			TCU_JZ4730_TCSR_EN, TCU_JZ4730_TCSR_EN);
	}

	return 0;

err_irq_dispose_mapping:
	if (tcu->soc_info->jz4740_regs)
		irq_dispose_mapping(timer_virq);
err_clk_disable:
	clk_disable_unprepare(timer->clk);
err_clk_put:
	clk_put(timer->clk);
	return err;
}

static int __init ingenic_tcu_clocksource_init(struct device_node *np,
					       struct ingenic_tcu *tcu)
{
	unsigned int channel = tcu->cs_channel;
	struct clocksource *cs = &tcu->cs;
	unsigned long rate;
	int err;

	tcu->cs_clk = ingenic_tcu_get_clock(np, channel);
	if (IS_ERR(tcu->cs_clk))
		return PTR_ERR(tcu->cs_clk);

	err = clk_prepare_enable(tcu->cs_clk);
	if (err)
		goto err_clk_put;

	rate = clk_get_rate(tcu->cs_clk);
	if (!rate) {
		err = -EINVAL;
		goto err_clk_disable;
	}


	if (tcu->soc_info->jz4740_regs) {
		/* Reset channel */
		regmap_update_bits(tcu->map, TCU_REG_TCSRc(channel),
				   0xffff & ~TCU_TCSR_RESERVED_BITS, 0);

		/* Reset counter */
printk("%s: %d vs. %d\n", __func__, tcu->soc_info->max_count, 0xffff);

		regmap_write(tcu->map, TCU_REG_TDFRc(channel), tcu->soc_info->max_count);
		regmap_write(tcu->map, TCU_REG_TCNTc(channel), 0);

		/* Enable channel */
		regmap_write(tcu->map, TCU_REG_TESR, BIT(channel));
	} else {
		/* Reset all bits except parent clock mask and enable underflow interrupt */
		updatew(tcu->base + TCU_JZ4730_REG_TCSRc(channel),
			~TCU_JZ4730_TCSR_PARENT_CLOCK_MASK, TCU_JZ4730_TCSR_EN);

		/* Reset counter */
		writel(tcu->soc_info->max_count, tcu->base + TCU_JZ4730_REG_TCNTc(channel));
		writel(tcu->soc_info->max_count, tcu->base + TCU_JZ4730_REG_TRDRc(channel));

		/* Enable channel */
		updateb(tcu->base + TCU_JZ4730_REG_TER, BIT(channel), BIT(channel));
	}

	cs->name = "ingenic-timer";
	cs->rating = 200;
	cs->flags = CLOCK_SOURCE_IS_CONTINUOUS;

printk("%s: %d vs. %llu\n", __func__, tcu->soc_info->max_count, CLOCKSOURCE_MASK(16));

	cs->mask = tcu->soc_info->max_count;
	cs->read = ingenic_tcu_timer_cs_read;

	err = clocksource_register_hz(cs, rate);
	if (err)
		goto err_clk_disable;

	return 0;

err_clk_disable:
	clk_disable_unprepare(tcu->cs_clk);
err_clk_put:
	clk_put(tcu->cs_clk);
	return err;
}

static const struct ingenic_soc_info jz4740_soc_info = {
	.num_channels = 8,
	.jz4740_regs = true,
	.counter_width = 16,
	.max_count = CLOCKSOURCE_MASK(16),
};

static const struct ingenic_soc_info jz4725b_soc_info = {
	.num_channels = 6,
	.jz4740_regs = true,
	.counter_width = 16,
	.max_count = CLOCKSOURCE_MASK(16),
};

static const struct ingenic_soc_info jz4730_soc_info = {
	.num_channels = 3,
	.jz4740_regs = false,
	.counter_width = 26,
	.max_count = CLOCKSOURCE_MASK(26),
};

static const struct of_device_id ingenic_tcu_of_match[] = {
	{ .compatible = "ingenic,jz4730-tcu", .data = &jz4730_soc_info, },
	{ .compatible = "ingenic,jz4740-tcu", .data = &jz4740_soc_info, },
	{ .compatible = "ingenic,jz4725b-tcu", .data = &jz4725b_soc_info, },
	{ .compatible = "ingenic,jz4760-tcu", .data = &jz4740_soc_info, },
	{ .compatible = "ingenic,jz4770-tcu", .data = &jz4740_soc_info, },
	{ .compatible = "ingenic,x1000-tcu", .data = &jz4740_soc_info, },
	{ /* sentinel */ }
};

static int __init ingenic_tcu_init(struct device_node *np)
{
	const struct of_device_id *id = of_match_node(ingenic_tcu_of_match, np);
	const struct ingenic_soc_info *soc_info = id->data;
	struct ingenic_tcu_timer *timer;
	struct ingenic_tcu *tcu;
	struct regmap *map;
	struct resource res;
	unsigned int cpu;
	int ret, last_bit = -1;
	long rate;

	of_node_clear_flag(np, OF_POPULATED);

	if (soc_info->jz4740_regs) {
		map = device_node_to_regmap(np);
		if (IS_ERR(map))
			return PTR_ERR(map);
	}

	tcu = kzalloc(struct_size(tcu, timers, num_possible_cpus()),
		      GFP_KERNEL);
	if (!tcu)
		return -ENOMEM;

	if (!soc_info->jz4740_regs) {
		if (of_address_to_resource(np, 0, &res)) {
			ret = -ENOMEM;
			goto err_free_ingenic_tcu;
		}

		tcu->base = ioremap(res.start, resource_size(&res));
		if (!tcu->base) {
			ret = -ENOMEM;
			goto err_free_ingenic_tcu;
		}
	}

	ingenic_tcu_intc_affinity = kzalloc(sizeof(unsigned int) * num_possible_cpus(),
		      GFP_KERNEL);
	if (!ingenic_tcu_intc_affinity)
		return -ENOMEM;

	/*
	 * Enable all TCU channels for PWM use by default except channels 0/1,
	 * and channel 2 if target CPU is JZ4780/X2000 and SMP is selected.
	 */
	tcu->pwm_channels_mask = GENMASK(soc_info->num_channels - 1,
					 num_possible_cpus() + 1);
	of_property_read_u32(np, "ingenic,pwm-channels-mask",
			     (u32 *)&tcu->pwm_channels_mask);

	/* Verify that we have at least num_possible_cpus() + 1 free channels */
	if (hweight8(tcu->pwm_channels_mask) >
			soc_info->num_channels - num_possible_cpus() + 1) {
		pr_crit("%s: Invalid PWM channel mask: 0x%02lx\n", __func__,
			tcu->pwm_channels_mask);
		ret = -EINVAL;
		goto err_free_ingenic_tcu;
	}

	tcu->soc_info = soc_info;
	tcu->map = map;
	tcu->np = np;
	ingenic_tcu = tcu;

	for (cpu = 0; cpu < num_possible_cpus(); cpu++) {
		timer = &tcu->timers[cpu];

		timer->cpu = cpu;
		timer->channel = find_next_zero_bit(&tcu->pwm_channels_mask,
						  soc_info->num_channels,
						  last_bit + 1);
		last_bit = timer->channel;
	}

	tcu->cs_channel = find_next_zero_bit(&tcu->pwm_channels_mask,
					     soc_info->num_channels,
					     last_bit + 1);

	ret = ingenic_tcu_clocksource_init(np, tcu);
	if (ret) {
		pr_crit("%s: Unable to init clocksource: %d\n", __func__, ret);
		goto err_free_ingenic_tcu;
	}

	/* Setup clock events on each CPU core */
	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "Ingenic XBurst: online",
				ingenic_tcu_setup_cevt, NULL);
	if (ret < 0) {
		pr_crit("%s: Unable to start CPU timers: %d\n", __func__, ret);
		goto err_tcu_clocksource_cleanup;
	}

	/* Register the sched_clock at the end as there's no way to undo it */
	rate = clk_get_rate(tcu->cs_clk);

printk("%s: %d vs. %d\n", __func__, soc_info->counter_width, 16);

	sched_clock_register(ingenic_tcu_timer_read, soc_info->counter_width, rate);

	return 0;

err_tcu_clocksource_cleanup:
	clocksource_unregister(&tcu->cs);
	clk_disable_unprepare(tcu->cs_clk);
	clk_put(tcu->cs_clk);
err_free_ingenic_tcu:
	kfree(tcu);
	return ret;
}

TIMER_OF_DECLARE(jz4730_tcu_intc, "ingenic,jz4730-tcu", ingenic_tcu_init);
TIMER_OF_DECLARE(jz4740_tcu_intc,  "ingenic,jz4740-tcu",  ingenic_tcu_init);
TIMER_OF_DECLARE(jz4725b_tcu_intc, "ingenic,jz4725b-tcu", ingenic_tcu_init);
TIMER_OF_DECLARE(jz4760_tcu_intc,  "ingenic,jz4760-tcu",  ingenic_tcu_init);
TIMER_OF_DECLARE(jz4770_tcu_intc,  "ingenic,jz4770-tcu",  ingenic_tcu_init);
TIMER_OF_DECLARE(x1000_tcu_intc,  "ingenic,x1000-tcu",  ingenic_tcu_init);

static int __init ingenic_tcu_probe(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, ingenic_tcu);

	return 0;
}

static int ingenic_tcu_suspend(struct device *dev)
{
	struct ingenic_tcu *tcu = dev_get_drvdata(dev);
	unsigned int cpu;

	clk_disable(tcu->cs_clk);

	for (cpu = 0; cpu < num_online_cpus(); cpu++)
		clk_disable(tcu->timers[cpu].clk);

	return 0;
}

static int ingenic_tcu_resume(struct device *dev)
{
	struct ingenic_tcu *tcu = dev_get_drvdata(dev);
	unsigned int cpu;
	int ret;

	for (cpu = 0; cpu < num_online_cpus(); cpu++) {
		ret = clk_enable(tcu->timers[cpu].clk);
		if (ret)
			goto err_timer_clk_disable;
	}

	ret = clk_enable(tcu->cs_clk);
	if (ret)
		goto err_timer_clk_disable;

	return 0;

err_timer_clk_disable:
	for (; cpu > 0; cpu--)
		clk_disable(tcu->timers[cpu - 1].clk);
	return ret;
}

static const struct dev_pm_ops ingenic_tcu_pm_ops = {
	/* _noirq: We want the TCU clocks to be gated last / ungated first */
	.suspend_noirq = ingenic_tcu_suspend,
	.resume_noirq  = ingenic_tcu_resume,
};

static struct platform_driver ingenic_tcu_driver = {
	.driver = {
		.name	= "ingenic-tcu-timer",
		.pm	= pm_sleep_ptr(&ingenic_tcu_pm_ops),
		.of_match_table = ingenic_tcu_of_match,
	},
};
builtin_platform_driver_probe(ingenic_tcu_driver, ingenic_tcu_probe);
