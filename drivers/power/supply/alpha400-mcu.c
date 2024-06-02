// SPDX-License-Identifier: GPL-2.0
/*
 * An I2C driver for the MCU used on the Letux 400
 * Copyright 2012 Daniel Gloeckner
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/pm.h>

#define DRV_VERSION "2.0"

static struct i2c_client *mcu;

static int minipc_battery_get_properties(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LIPO;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = 7000000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = 8400000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = i2c_smbus_read_byte_data(mcu, 0xdb);
		if (ret >= 0) {
			/* resistor divider scales 8.4V to 3V */
			/* lpc915 is powered from 3.3V */
			/* ergo voltage = value * 2.8 * 3.3 / 255 */
			val->intval = ret * 616000 / 17;
			ret = 0;
		}
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static struct power_supply *minipc_battery, *minipc_psu;

static enum power_supply_property minipc_battery_properties[] = {
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static struct power_supply_desc minipc_battery_desc = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = minipc_battery_properties,
	.num_properties = ARRAY_SIZE(minipc_battery_properties),
	.get_property = minipc_battery_get_properties,
};

static int minipc_psu_get_properties(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = i2c_smbus_read_byte_data(mcu, 0xd9) & 1;
		if (val->intval < 0)
			ret = val->intval;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static enum power_supply_property minipc_psu_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static struct power_supply_desc minipc_psu_desc = {
	.name = "psu",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = minipc_psu_properties,
	.num_properties = ARRAY_SIZE(minipc_psu_properties),
	.get_property = minipc_psu_get_properties,
};

static void minipc_mcu_power_off(void)
{
	i2c_smbus_write_byte_data(mcu, 0xd8, 0x01);
}

static int minipc_mcu_probe(struct i2c_client *client)
{
	struct power_supply_config psy_cfg = {};

	dev_dbg(&client->dev, "%s\n", __func__);

	if (mcu)
		return -EBUSY;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	dev_info(&client->dev, "chip found, driver version " DRV_VERSION "\n");

	minipc_battery = power_supply_register(&client->dev,
					       &minipc_battery_desc,
					       &psy_cfg);

	minipc_battery_desc.use_for_apm = !!minipc_battery;

	minipc_psu = power_supply_register(&client->dev,
					   &minipc_psu_desc,
					   &psy_cfg);

	minipc_psu_desc.use_for_apm = !!minipc_psu;

	mcu = client;
	pm_power_off = minipc_mcu_power_off;

	return 0;
}

static void minipc_mcu_remove(struct i2c_client *client)
{
	if (!mcu)
		return;
	if (!minipc_battery_desc.use_for_apm)
		power_supply_unregister(minipc_battery);
	if (!minipc_psu_desc.use_for_apm)
		power_supply_unregister(minipc_psu);
	pm_power_off = NULL;
	mcu = NULL;
}

static const struct i2c_device_id minipc_mcu_id[] = {
	{ "minipc-mcu", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, minipc_mcu_id);

static const struct of_device_id minipc_mcu_of_matches[] = {
	{ .compatible = "skytone,alpha400-mcu", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, minipc_mcu_of_matches);

static struct i2c_driver minipc_mcu_driver = {
	.driver		= {
		.name	= "minipc-mcu",
		.of_match_table = of_match_ptr(minipc_mcu_of_matches),
	},
	.probe		= minipc_mcu_probe,
	.remove		= minipc_mcu_remove,
	.id_table	= minipc_mcu_id,
};

MODULE_AUTHOR("Daniel Gloeckner <daniel-gl@gmx.net>");
MODULE_DESCRIPTION("Skytone Alpha 400 LPC915 MCU driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

module_i2c_driver(minipc_mcu_driver);
