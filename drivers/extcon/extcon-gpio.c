// SPDX-License-Identifier: GPL-2.0-only
/*
 * extcon_gpio.c - Single-state GPIO extcon driver based on extcon class
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * Modified by MyungJoo Ham <myungjoo.ham@samsung.com> to support extcon
 * (originally switch class is supported)
 */

#include <linux/devm-helpers.h>
#include <linux/extcon-provider.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

/**
 * struct gpio_extcon_data - A simple GPIO-controlled extcon device state container.
 * @edev:		Extcon device.
 * @work:		Work fired by the interrupt.
 * @debounce_jiffies:	Number of jiffies to wait for the GPIO to stabilize, from the debounce
 *			value.
 * @gpiod:		GPIO descriptor for this external connector.
 * @extcon_id:		The unique id of specific external connector.
 * @debounce:		Debounce time for GPIO IRQ in ms.
 * @check_on_resume:	Boolean describing whether to check the state of gpio
 *			while resuming from sleep.
 */
struct gpio_extcon_data {
	struct extcon_dev *edev;
	struct delayed_work work;
	unsigned long debounce_jiffies;
	struct gpio_desc *gpiod;
	unsigned int extcon_id;
	unsigned long debounce;
	bool check_on_resume;
};

static void gpio_extcon_work(struct work_struct *work)
{
	int state;
	struct gpio_extcon_data	*data =
		container_of(to_delayed_work(work), struct gpio_extcon_data,
			     work);

	state = gpiod_get_value_cansleep(data->gpiod);
	extcon_set_state_sync(data->edev, data->extcon_id, state);
}

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	struct gpio_extcon_data *data = dev_id;

#ifdef DEBUG
	printk("extcon gpio_irq_handler\n");
#endif
	queue_delayed_work(system_power_efficient_wq, &data->work,
			      data->debounce_jiffies);
	return IRQ_HANDLED;
}

static int gpio_extcon_probe(struct platform_device *pdev)
{
	struct gpio_extcon_data *data;
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	unsigned long irq_flags;
	int irq;
	int ret;

#ifdef DEBUG
	printk("gpio_extcon_probe\n");
#endif
	data = devm_kzalloc(dev, sizeof(struct gpio_extcon_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (node) {
		u32 value;

		data->debounce_jiffies = 0;

		data->check_on_resume = of_property_read_bool(node, "check-on-resume");
		if(!of_property_read_u32(node, "debounce-delay-ms", &value))
			data->debounce_jiffies = value;
#ifdef DEBUG
		printk("extcon gpio %p\n", data->gpiod);
		printk("extcon debounce %lu\n", data->debounce_jiffies);
#endif
	}
	/*
	 * FIXME: extcon_id represents the unique identifier of external
	 * connectors such as EXTCON_USB, EXTCON_DISP_HDMI and so on. extcon_id
	 * is necessary to register the extcon device. But, it's not yet
	 * developed to get the extcon id from device-tree or others.
	 * On later, it have to be solved.
	 */
	if (data->extcon_id > EXTCON_NONE)
		return -EINVAL;

	data->gpiod = devm_gpiod_get(dev, "extcon", GPIOD_IN);
	if (IS_ERR(data->gpiod))
		return PTR_ERR(data->gpiod);
	irq = gpiod_to_irq(data->gpiod);
	if (irq <= 0)
		return irq;

	/*
	 * It is unlikely that this is an acknowledged interrupt that goes
	 * away after handling, what we are looking for are falling edges
	 * if the signal is active low, and rising edges if the signal is
	 * active high.
	 */
	if (gpiod_is_active_low(data->gpiod))
		irq_flags = IRQF_TRIGGER_FALLING;
	else
		irq_flags = IRQF_TRIGGER_RISING;

	/* Allocate the memory of extcon devie and register extcon device */
	data->edev = devm_extcon_dev_allocate(dev, &data->extcon_id);
	if (IS_ERR(data->edev)) {
		dev_err(dev, "failed to allocate extcon device\n");
		return -ENOMEM;
	}

	ret = devm_extcon_dev_register(dev, data->edev);
	if (ret < 0)
		return ret;

	ret = devm_delayed_work_autocancel(dev, &data->work, gpio_extcon_work);
	if (ret)
		return ret;

	/*
	 * Request the interrupt of gpio to detect whether external connector
	 * is attached or detached.
	 */
	ret = devm_request_any_context_irq(dev, irq,
					gpio_irq_handler, irq_flags,
					pdev->name, data);
	if (ret < 0)
		return ret;

	platform_set_drvdata(pdev, data);
	/* Perform initial detection */
	gpio_extcon_work(&data->work.work);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int gpio_extcon_resume(struct device *dev)
{
	struct gpio_extcon_data *data;

	data = dev_get_drvdata(dev);
	if (data->check_on_resume)
		queue_delayed_work(system_power_efficient_wq,
			&data->work, data->debounce_jiffies);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(gpio_extcon_pm_ops, NULL, gpio_extcon_resume);

static const struct of_device_id of_extcon_match_tbl[] = {
	{ .compatible = "extcon-gpio", },
	{ /* end */ }
};

MODULE_DEVICE_TABLE(of, of_extcon_match_tbl);

static struct platform_driver gpio_extcon_driver = {
	.probe		= gpio_extcon_probe,
	.driver		= {
		.name	= "extcon-gpio",
		.pm	= &gpio_extcon_pm_ops,
		.of_match_table = of_match_ptr(of_extcon_match_tbl),
	},
};

module_platform_driver(gpio_extcon_driver);

MODULE_AUTHOR("Mike Lockwood <lockwood@android.com>");
MODULE_DESCRIPTION("GPIO extcon driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:extcon-gpio");
