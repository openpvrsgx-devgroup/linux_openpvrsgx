// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2010, Lars-Peter Clausen <lars@metafoo.de>
 *  Copyright (C) 2017, 2019 Paul Boddie <paul@boddie.org.uk>
 *  JZ4730 platform PWM support
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/regmap.h>

#define NUM_PWM 2

#define JZ4730_PWM_REG_CTR		0x00	/* 8-bit */
#define JZ4730_PWM_REG_PER		0x04	/* 16-bit */
#define JZ4730_PWM_REG_DUT		0x08	/* 16-bit */

#define JZ4730_PWM_OFFSET		0x1000	/* register spacing */

#define JZ4730_PWM_CTR_EN		0x80	/* enable */
#define JZ4730_PWM_CTR_SD		0x40	/* shutdown */
#define JZ4730_PWM_CTR_PRESCALE		0x3f	/* mask */
#define JZ4730_PWM_CTR_PRESCALE_LIMIT	0x40	/* actual limit, not encoded */

#define JZ4730_PWM_PER_PERIOD		0x3ff	/* mask */
#define JZ4730_PWM_PER_PERIOD_LIMIT	0x400	/* actual limit, not encoded */

#define JZ4730_PWM_DUT_FULL		0x400
#define JZ4730_PWM_DUT_DUTY		0x3ff	/* mask */

struct jz4730_pwm_chip {
	struct pwm_chip chip;
	struct regmap *map;
	struct clk *clk;
};

static inline struct jz4730_pwm_chip *to_jz4730(struct pwm_chip *chip)
{
	return container_of(chip, struct jz4730_pwm_chip, chip);
}

/* Update the register by loading, updating and storing the register value. */

static void pwm_jz4730_update_reg(struct pwm_chip *chip, struct pwm_device *pwm,
					u32 reg, u32 affected, u32 value)
{
	struct jz4730_pwm_chip *jzpc = to_jz4730(chip);
	u32 offset = pwm->hwpwm * JZ4730_PWM_OFFSET;

	regmap_update_bits(jzpc->map, offset + reg, affected, value);
}

static int jz4730_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm, enum pwm_polarity polarity)
{
	if (polarity != PWM_POLARITY_NORMAL)
		return -EINVAL;

	pwm_jz4730_update_reg(chip, pwm, JZ4730_PWM_REG_CTR,
		JZ4730_PWM_CTR_EN, JZ4730_PWM_CTR_EN);
	return 0;
}

static void jz4730_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	pwm_jz4730_update_reg(chip, pwm, JZ4730_PWM_REG_CTR,
		JZ4730_PWM_CTR_EN, 0);
}

static int jz4730_pwm_is_enabled(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct jz4730_pwm_chip *jzpc = to_jz4730(chip);
	u32 offset = pwm->hwpwm * JZ4730_PWM_OFFSET;
	unsigned int val;

	regmap_read(jzpc->map, offset + JZ4730_PWM_REG_CTR, &val);
	return !!(val & JZ4730_PWM_CTR_EN);
}

static int jz4730_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			     int duty_ns, int period_ns)
{
	struct jz4730_pwm_chip *jzpc = to_jz4730(chip);

	/* The prescaler seems to be a simple quantity, not a 4**n
	 * progression as seen in the JZ4740 TCSR:PRESCALE field.
	 *
	 * Thus, the PWM frequency can be calculated as follows, given
	 * observed values:
	 *
	 * PWM frequency = clock frequency / prescaler / period count
	 * = 3686400Hz / 64 / 300
	 * = 192Hz
	 *
	 * The above formula can be rewritten as follows:
	 *
	 * PWM frequency = clock frequency / (prescaler * period count)
	 * clock period * prescaler * period count = PWM period
	 * prescaler * period count = PWM period / clock period
	 * prescaler * period count = PWM period * clock frequency
	 *
	 * With the prescaler and period count apparently acting
	 * together as a larger quantity, the prescaler needs populating
	 * when the period count would otherwise exceed the limit of its
	 * field.
	 */

	unsigned long long tmp;
	unsigned long period, duty;
	unsigned int prescaler = 1;
	bool is_enabled;

	/* Get the PWM period as a multiple of the clock period. */

	tmp = (unsigned long long)clk_get_rate(jzpc->clk) * period_ns;
	do_div(tmp, 1000000000);
	period = tmp;

	if (period > JZ4730_PWM_PER_PERIOD_LIMIT) {
		prescaler = period / JZ4730_PWM_PER_PERIOD_LIMIT;
		period = period % JZ4730_PWM_PER_PERIOD_LIMIT;
	}

	if (prescaler > JZ4730_PWM_CTR_PRESCALE_LIMIT)
		return -EINVAL;

	/* Get the duty duration as a multiple of the clock period. */

	tmp = (unsigned long long)period * duty_ns;
	do_div(tmp, period_ns);
	duty = tmp;

	if (duty >= period)
		duty = period;

	is_enabled = jz4730_pwm_is_enabled(chip, pwm);
	if (is_enabled)
		jz4730_pwm_disable(chip, pwm);

	/* It may be the case that setting duty to period still causes
	 * one cycle to be low. This is what happens with output compare
	 * on PIC32, for instance.
	 */

	pwm_jz4730_update_reg(chip, pwm, JZ4730_PWM_REG_DUT,
		JZ4730_PWM_DUT_DUTY, duty);

	/* The period and prescaler encoding may employ an offset, with
	 * a stored value of zero representing one.
	 */

	pwm_jz4730_update_reg(chip, pwm, JZ4730_PWM_REG_PER,
		JZ4730_PWM_PER_PERIOD, period - 1);

	pwm_jz4730_update_reg(chip, pwm, JZ4730_PWM_REG_CTR,
		JZ4730_PWM_CTR_PRESCALE, prescaler - 1);

	if (is_enabled)
		jz4730_pwm_enable(chip, pwm, PWM_POLARITY_NORMAL);

	return 0;
}

static int jz4730_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			    const struct pwm_state *state)
{
	int err;
	bool enabled = pwm->state.enabled;

	if (state->polarity != pwm->state.polarity && pwm->state.enabled) {
		jz4730_pwm_disable(chip, pwm);
		enabled = false;
	}

	if (!state->enabled) {
		if (enabled)
			jz4730_pwm_disable(chip, pwm);

		return 0;
	}

	err = jz4730_pwm_config(pwm->chip, pwm, state->duty_cycle, state->period);
	if (err)
		return err;

	if (!enabled)
		err = jz4730_pwm_enable(chip, pwm, state->polarity);

	return err;
}

static const struct regmap_config ingenic_pwm_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static const struct pwm_ops jz4730_pwm_ops = {
	.apply = jz4730_pwm_apply,
};

static int jz4730_pwm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct jz4730_pwm_chip *jzpc;
	void __iomem *base;

	jzpc = devm_kzalloc(&pdev->dev, sizeof(*jzpc), GFP_KERNEL);
	if (!jzpc)
		return -ENOMEM;

	jzpc->clk = devm_clk_get(&pdev->dev, "ext");
	if (IS_ERR(jzpc->clk))
		return PTR_ERR(jzpc->clk);

	base = devm_ioremap_resource(dev,
			platform_get_resource(pdev, IORESOURCE_MEM, 0));
	if (IS_ERR(base)) {
		dev_err(dev, "Failed to ioremap registers\n");
		return PTR_ERR(base);
	}

	jzpc->map = devm_regmap_init_mmio(dev, base,
			&ingenic_pwm_regmap_config);
	if (IS_ERR(jzpc->map)) {
		dev_err(dev, "Failed to create regmap\n");
		return PTR_ERR(jzpc->map);
	}

	jzpc->chip.dev = &pdev->dev;
	jzpc->chip.ops = &jz4730_pwm_ops;
	jzpc->chip.npwm = NUM_PWM;

	platform_set_drvdata(pdev, jzpc);

	return pwmchip_add(&jzpc->chip);
}

static int jz4730_pwm_remove(struct platform_device *pdev)
{
	struct jz4730_pwm_chip *jzpc = platform_get_drvdata(pdev);

	pwmchip_remove(&jzpc->chip);

	return 0;
}

static const struct of_device_id jz4730_pwm_dt_ids[] = {
	{ .compatible = "ingenic,jz4730-pwm", },
	{},
};
MODULE_DEVICE_TABLE(of, jz4730_pwm_dt_ids);

static struct platform_driver jz4730_pwm_driver = {
	.driver = {
		.name = "jz4730-pwm",
		.of_match_table = of_match_ptr(jz4730_pwm_dt_ids),
	},
	.probe = jz4730_pwm_probe,
	.remove = jz4730_pwm_remove,
};
module_platform_driver(jz4730_pwm_driver);

MODULE_AUTHOR("Paul Boddie <paul@boddie.org.uk>");
MODULE_DESCRIPTION("Ingenic JZ4730 PWM driver");
MODULE_ALIAS("platform:jz4730-pwm");
MODULE_LICENSE("GPL");
