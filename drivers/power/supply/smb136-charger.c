/*
 *  Basic SMB136 Battery Charger Driver
 *
 *  Copyright (C) 2016 OMAP4 AOSP Project
 *  Copyright (C) 2011 Samsung Electronics
 *  Ikkeun Kim <iks.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/power/smb136-charger.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

/* SMB136 Registers. */
#define SMB136_CHARGE_CURRENT			0x00
#define SMB136_INPUT_CURRENTLIMIT		0x01
#define SMB136_FLOAT_VOLTAGE			0x02
#define SMB136_CHARGE_CONTROL_A			0x03
#define SMB136_CHARGE_CONTROL_B			0x04
#define SMB136_PIN_ENABLE_CONTROL		0x05
#define SMB136_OTG_CONTROL			0x06
#define SMB136_SAFTY				0x09

#define SMB136_COMMAND_A			0x31
#define SMB136_STATUS_D				0x35
#define SMB136_STATUS_E				0x36

struct smb136_charger {
	struct i2c_client	*client;
	struct power_supply	*mains;
	struct power_supply	*usb;
	bool			mains_online;
	bool			usb_online;
	bool			usb_hc_mode;
	const struct smb136_charger_platform_data *pdata;
};

static int smb136_i2c_write(struct i2c_client *client, u8 reg, u8 data)
{
	int ret;

	if (!client)
		return -ENODEV;

	ret = i2c_smbus_write_byte_data(client, reg, data);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int smb136_hw_init(struct smb136_charger *smb)
{
	/* Change USB5/1/HC Control from Pin to I2C */
	smb136_i2c_write(smb->client, SMB136_PIN_ENABLE_CONTROL, 0x8);

	/* Disable Automatic Input Current Limit */
	/* Set it to 1.3A */
	smb136_i2c_write(smb->client, SMB136_INPUT_CURRENTLIMIT, 0xE6);

	/* Automatic Recharge Disabed */
	smb136_i2c_write(smb->client, SMB136_CHARGE_CONTROL_A, 0x8c);

	/* Safty timer Disabled */
	smb136_i2c_write(smb->client, SMB136_CHARGE_CONTROL_B, 0x28);

	/* Disable USB D+/D- Detection */
	smb136_i2c_write(smb->client, SMB136_OTG_CONTROL, 0x28);

	/* Set Output Polarity for STAT */
	smb136_i2c_write(smb->client, SMB136_FLOAT_VOLTAGE, 0xCA);

	/* Re-load Enable */
	smb136_i2c_write(smb->client, SMB136_SAFTY, 0x4b);

	return 0;
}

static void smb136_update_charger(struct smb136_charger *smb)
{
	if (smb->usb_hc_mode) {
		/* HC mode */
		smb136_i2c_write(smb->client, SMB136_COMMAND_A, 0x8c);

		/* Set charging current limit to 1.5A */
		smb136_i2c_write(smb->client, SMB136_CHARGE_CURRENT, 0xF4);

		dev_info(&smb->client->dev,
			"charging current limit set to 1.5A\n");
	} else if (smb->usb_online) {
		/* USBIN 500mA mode */
		smb136_i2c_write(smb->client, SMB136_COMMAND_A, 0x88);

		/* Set charging current limit to 500mA */
		smb136_i2c_write(smb->client, SMB136_CHARGE_CURRENT, 0x14);

		dev_info(&smb->client->dev,
			"charging current limit set to 0.5A\n");
	} else {
		/* USB 100mA Mode, USB5/1 Current Levels */
		/* Prevent in-rush current */
		smb136_i2c_write(smb->client, SMB136_COMMAND_A, 0x80);
		udelay(10);

		/* Set charge current to 100mA */
		/* Prevent in-rush current */
		smb136_i2c_write(smb->client, SMB136_CHARGE_CURRENT, 0x14);
		udelay(10);
	}
}

static int smb136_mains_get_property(struct power_supply *psy,
				     enum power_supply_property prop,
				     union power_supply_propval *val)
{
	struct smb136_charger *smb = power_supply_get_drvdata(psy);

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = smb->mains_online;
		return 0;
	default:
		return -EINVAL;
	}
	return -EINVAL;
}

static int smb136_mains_set_property(struct power_supply *psy,
				     enum power_supply_property prop,
				     const union power_supply_propval *val)
{
	struct smb136_charger *smb = power_supply_get_drvdata(psy);
	bool oldval;

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		oldval = smb->mains_online;

		smb->mains_online = val->intval;

		if (smb->mains_online != oldval)
			power_supply_changed(psy);
		return 0;
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static int smb136_mains_property_is_writeable(struct power_supply *psy,
					     enum power_supply_property prop)
{
	return 0;
}

static enum power_supply_property smb136_mains_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int smb136_usb_get_property(struct power_supply *psy,
				   enum power_supply_property prop,
				   union power_supply_propval *val)
{
	struct smb136_charger *smb = power_supply_get_drvdata(psy);

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = smb->usb_online;
		return 0;

	case POWER_SUPPLY_PROP_USB_HC:
		val->intval = smb->usb_hc_mode;
		return 0;

	default:
		break;
	}
	return -EINVAL;
}

static int smb136_usb_set_property(struct power_supply *psy,
				   enum power_supply_property prop,
				   const union power_supply_propval *val)
{
	int ret = -EINVAL;
	struct smb136_charger *smb = power_supply_get_drvdata(psy);
	bool oldval;

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		oldval = smb->usb_online;
		smb->usb_online = val->intval;

		if (smb->usb_online != oldval)
			power_supply_changed(psy);
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_USB_HC:
		smb->usb_hc_mode = val->intval;
		break;
	default:
		break;
	}

	smb136_update_charger(smb);

	return ret;
}

static int smb136_usb_property_is_writeable(struct power_supply *psy,
					    enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_USB_HC:
		return 1;
	default:
		break;
	}

	return 0;
}

static int smb136_get_battery_info(struct smb136_charger *smb)
{
	struct smb347_charger_platform_data *pdata = (void *)smb->pdata;
	struct power_supply_battery_info info = {};
	struct power_supply *supply;
	int err;

	if (smb->mains)
		supply = smb->mains;
	else
		supply = smb->usb;

	err = power_supply_get_battery_info(supply, &info);
	if (err == -ENXIO || err == -ENODEV)
		return 0;
	if (err)
		return err;

	return 0;
}

static struct smb136_charger_platform_data
			*smb136_get_platdata(struct device *dev)
{
	struct smb136_charger_platform_data *pdata;

	if (dev->of_node) {
		pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	} else {
		pdata = dev_get_platdata(dev);
	}

	return pdata;
}

static enum power_supply_property smb136_usb_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_USB_HC,
};

static const struct power_supply_desc smb136_mains_desc = {
	.name		= "smb136-mains",
	.type		= POWER_SUPPLY_TYPE_MAINS,
	.get_property	= smb136_mains_get_property,
	.set_property	= smb136_mains_set_property,
	.property_is_writeable	= smb136_mains_property_is_writeable,
	.properties	= smb136_mains_properties,
	.num_properties	= ARRAY_SIZE(smb136_mains_properties),
};

static const struct power_supply_desc smb136_usb_desc = {
	.name		= "smb136-usb",
	.type		= POWER_SUPPLY_TYPE_USB,
	.get_property	= smb136_usb_get_property,
	.set_property	= smb136_usb_set_property,
	.property_is_writeable	= smb136_usb_property_is_writeable,
	.properties	= smb136_usb_properties,
	.num_properties	= ARRAY_SIZE(smb136_usb_properties),
};

static int smb136_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
    printk("smb136: probe started");
	//static char *battery[] = { "max17042-battery" };
    struct power_supply_config mains_usb_cfg = {};
	struct device *dev = &client->dev;
	struct smb136_charger *smb;
	int ret;

	smb = devm_kzalloc(dev, sizeof(*smb), GFP_KERNEL);
	if (!smb)
		return -ENOMEM;

	smb->pdata = smb136_get_platdata(dev);
	if (!smb->pdata)
		return -ENODEV;

	i2c_set_clientdata(client, smb);

	smb->client = client;

    //mains_usb_cfg.supplied_to = battery;
    //mains_usb_cfg.num_supplicants = ARRAY_SIZE(battery);
    mains_usb_cfg.drv_data = smb;
    mains_usb_cfg.of_node = dev->of_node;

	smb->mains = devm_power_supply_register(dev, &smb136_mains_desc,
						   &mains_usb_cfg);

	if (IS_ERR(smb->mains)){
        printk("smb136: mains error");
		return PTR_ERR(smb->mains);
    }

	smb->usb = devm_power_supply_register(dev, &smb136_usb_desc,
					 &mains_usb_cfg);
	if (IS_ERR(smb->usb)){
        printk("smb136: usb error");
		return PTR_ERR(smb->usb);
    }

	ret = smb136_get_battery_info(smb);
	if (ret){
        printk("smb136: smb136_get_battery_info");
		return ret;
    }

    ret = smb136_hw_init(smb);
	if (ret < 0){
        printk("smb136: smb136_hw_init");
		return ret;
    }

	printk("smb136 probed\n");

	return 0;
}

static int smb136_remove(struct i2c_client *client)
{
	struct smb136_charger *smb = i2c_get_clientdata(client);

	power_supply_unregister(smb->usb);
	power_supply_unregister(smb->mains);

	return 0;
}

static const struct i2c_device_id smb136_id[] = {
	{ "smb136", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, smb136_id);

static const struct of_device_id smb136_of_match[] = {
	{ .compatible = "summit,smb136" },
	{ },
};
MODULE_DEVICE_TABLE(of, smb136_of_match);

static struct i2c_driver smb136_i2c_driver = {
	.driver = {
		.name	= "smb136",
		.of_match_table = smb136_of_match,
	},
	.id_table	= smb136_id,
	.probe	= smb136_probe,
	.remove	= smb136_remove,
    .id_table = smb136_id,
};

module_i2c_driver(smb136_i2c_driver);

MODULE_DESCRIPTION("SMB136 battery charger driver");
MODULE_LICENSE("GPL");
