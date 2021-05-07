/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
/*
 * tps65185.c
 *
 * Based on the MAX1619 driver.
 * Copyright (C) 2003-2004 Alexey Fisher <fishor@mail.ru>
 *                         Jean Delvare <khali@linux-fr.org>
 *
 * The TPS65185 is a sensor chip made by Texass Instruments.
 * It reports up to two temperatures (its own plus up to
 * one external one).
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/mfd/tps6518x.h>

static int tps6518x_read(struct device *dev, enum hwmon_sensor_types type,
			 u32 attr, int channel, long *temp)
{
	struct tps6518x *tps6518x = dev_get_drvdata(dev);
	unsigned int regval;
	int err;

	if (attr != hwmon_temp_input)
		return -EOPNOTSUPP;

	err = regmap_update_bits(tps6518x->regmap,
				 REG_TPS65185_TMST1,
				 0x80, 0x80);
	if (err < 0)
		return err;

	do {
		msleep(2);
		err = regmap_read(tps6518x->regmap, REG_TPS65185_TMST1, &regval);
	} while (!(0x20 & regval));


	err = regmap_read(tps6518x->regmap, REG_TPS6518x_TMST_VAL, &regval);
	if (err < 0)
		return err;

	*temp = 1000 * (s8) regval;
	
	return 0;
}

static umode_t tps6518x_is_visible(const void *data,
				   enum hwmon_sensor_types type,
				   u32 attr, int channel)
{
	if (type != hwmon_temp)
		return 0;

	if (attr != hwmon_temp_input)
		return 0;

	return 0444;
}

static const struct hwmon_ops tps6518x_hwmon_ops = {
	.is_visible = tps6518x_is_visible,
	.read = tps6518x_read,
};

static const struct hwmon_channel_info *tps6518x_info[] = {
	HWMON_CHANNEL_INFO(chip, HWMON_C_REGISTER_TZ),
	HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
	NULL
};

static const struct hwmon_chip_info tps6518x_chip_info = {
	.ops = &tps6518x_hwmon_ops,
	.info = tps6518x_info,
};
/*
 * Real code
 */
static int tps6518x_sensor_probe(struct platform_device *pdev)
{
	struct tps6518x *tps6518x;
        struct device *hwmon_dev;

	tps6518x = dev_get_drvdata(pdev->dev.parent);
	pdev->dev.of_node = pdev->dev.parent->of_node;

	hwmon_dev = devm_hwmon_device_register_with_info(&pdev->dev,
							  "tps65185",
							  tps6518x,
							  &tps6518x_chip_info,
							  NULL);
        return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct platform_device_id tps6518x_sns_id[] = {
	{ "tps6518x-sns", 0},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, tps6518x_sns_id);

/*
 * Driver data (common to all clients)
 */
static struct platform_driver tps6518x_sensor_driver = {
	.probe = tps6518x_sensor_probe,
	.id_table = tps6518x_sns_id,
	.driver = {
		.name = "tps6518x_sensor",
	},
};

module_platform_driver(tps6518x_sensor_driver);

MODULE_DESCRIPTION("TPS6518x sensor driver");
MODULE_LICENSE("GPL");
