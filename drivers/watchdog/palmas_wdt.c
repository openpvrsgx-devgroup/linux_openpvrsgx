/*
 * Driver for Watchdog part of Palmas PMIC Chips
 *
 * Copyright 2011-2013 Texas Instruments Inc.
 *
 * Author: Graeme Gregory <gg@slimlogic.co.uk>
 * Author: Ian Lartey <ian@slimlogic.co.uk>
 *
 * Based on twl4030_wdt.c
 *
 * Author: Timo Kokkonen <timo.t.kokkonen at nokia.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/watchdog.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/mfd/palmas.h>

struct palmas_wdt {
	struct palmas *palmas;
	struct watchdog_device wdt;
	struct device *dev;

	int timer_margin;
};

static int nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started "
	"(default=" __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

static int palmas_wdt_write(struct palmas *palmas, unsigned int data)
{
	unsigned int addr;

	addr = PALMAS_BASE_TO_REG(PALMAS_PMU_CONTROL_BASE, PALMAS_WATCHDOG);

	return palmas_write(palmas, PALMAS_PMU_CONTROL_BASE, addr, data);
}

static int palmas_wdt_start(struct watchdog_device *wdt)
{
	struct palmas_wdt *driver_data = watchdog_get_drvdata(wdt);
	struct palmas *palmas = driver_data->palmas;

	return palmas_wdt_write(palmas, driver_data->timer_margin |
				PALMAS_WATCHDOG_ENABLE);
}

static int palmas_wdt_stop(struct watchdog_device *wdt)
{
	struct palmas_wdt *driver_data = watchdog_get_drvdata(wdt);
	struct palmas *palmas = driver_data->palmas;

	return palmas_wdt_write(palmas, driver_data->timer_margin);
}

static int palmas_wdt_set_timeout(struct watchdog_device *wdt, unsigned timeout)
{
	struct palmas_wdt *driver_data = watchdog_get_drvdata(wdt);

	driver_data->timer_margin = fls(timeout) - 1;
	return 0;
}

static const struct watchdog_info palmas_wdt_info = {
	.options = WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE,
	.identity = "Palmas Watchdog",
	.firmware_version = 0,
};

static const struct watchdog_ops palmas_wdt_ops = {
	.owner = THIS_MODULE,
	.start = palmas_wdt_start,
	.stop = palmas_wdt_stop,
	.ping = palmas_wdt_start,
	.set_timeout = palmas_wdt_set_timeout,
};

static int palmas_wdt_probe(struct platform_device *pdev)
{
	struct palmas_wdt *driver_data;
	struct watchdog_device *palmas_wdt;
	int ret = 0;

	driver_data = devm_kzalloc(&pdev->dev, sizeof(*driver_data),
				   GFP_KERNEL);
	if (!driver_data)
		return -ENOMEM;

	driver_data->palmas = dev_get_drvdata(pdev->dev.parent);

	palmas_wdt = &driver_data->wdt;

	palmas_wdt->info = &palmas_wdt_info;
	palmas_wdt->ops = &palmas_wdt_ops;
	palmas_wdt->status = 0;
	palmas_wdt->timeout = 30;
	palmas_wdt->min_timeout = 1;
	palmas_wdt->max_timeout = 128;
	palmas_wdt->parent = &pdev->dev;

	watchdog_set_nowayout(palmas_wdt, nowayout);
	watchdog_set_drvdata(palmas_wdt, driver_data);

	ret = watchdog_register_device(&driver_data->wdt);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, driver_data);

	return 0;
}

static int palmas_wdt_remove(struct platform_device *pdev)
{
	struct watchdog_device *wdt = platform_get_drvdata(pdev);

	watchdog_unregister_device(wdt);

	return 0;
}

static struct of_device_id of_palmas_match_tbl[] = {
	{ .compatible = "ti,palmas-wdt", },
	{ .compatible = "ti,twl6035-wdt", },
	{ .compatible = "ti,twl6036-wdt", },
	{ .compatible = "ti,twl6037-wdt", },
	{ .compatible = "ti,tps65913-wdt", },
	{ .compatible = "ti,tps65914-wdt", },
	{ .compatible = "ti,tps80036-wdt", },
	{ /* end */ }
};

static struct platform_driver palmas_wdt_driver = {
	.probe = palmas_wdt_probe,
	.remove = palmas_wdt_remove,
	.driver = {
		.owner = THIS_MODULE,
		.of_match_table = of_palmas_match_tbl,
		.name = "palmas-wdt",
	},
};

module_platform_driver(palmas_wdt_driver);

MODULE_AUTHOR("Graeme Gregory <gg@slimlogic.co.uk>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:palmas-wdt");
MODULE_DEVICE_TABLE(of, of_palmas_match_tbl);
