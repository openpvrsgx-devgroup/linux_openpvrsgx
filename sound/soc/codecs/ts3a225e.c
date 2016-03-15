/*
 * Copyright (c) 2016 Golden Delicious Comp. GmbH&Co KG
 *
 * simple driver just polling the device and doing some printk
 * should be extended (merged?) with ts3a227e driver
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/workqueue.h>

#include <sound/core.h>
#include <sound/jack.h>
#include <sound/soc.h>

struct ts3a225e {
	struct	device *dev;
	struct  regmap *regmap;
	struct	mutex lock;
	struct	delayed_work work;
	u8	chip_id;
	int	last_reg;
	int	irq;
	struct	completion done;
};

#define DRIVER_NAME		"ts3a225e"
#define TS3A225_I2C_ADDRESS	0x3b
#define POLL_INTERVAL		msecs_to_jiffies(1000)

#define TS3A225_CHIP_ID_REG	0x01
#define TS3A225_DAT1_REG	0x05
#define TS3A225_INT_REG		0x06

static const unsigned short normal_i2c[] = { TS3A225_I2C_ADDRESS,
							I2C_CLIENT_END };

static void ts3a225_state_changed(struct device *dev)
{
	struct ts3a225e *data = dev_get_drvdata(dev);
	int ret;
	unsigned int reg;

	ret = regmap_read(data->regmap, TS3A225_INT_REG, &reg);
	if (ret < 0)
		return;

	if (reg != data->last_reg) {
		if (reg & 0x01)
			printk("ts3a225e: TRS headset plugged");
		else if (data->last_reg & 0x01)
			printk("ts3a225e: TRS headset removed");
		if (reg & 0x02)
			printk("ts3a225e:  microphone plugged");
		else if (data->last_reg & 0x02)
			printk("ts3a225e:  microphone removed");
		if (reg & 0x80)
			printk("ts3a225e:    detection failed");
		data->last_reg = reg;

		ret = regmap_read(data->regmap, TS3A225_DAT1_REG, &reg);
		if (ret < 0)
			return;
		if (reg & 0x80)
			printk("; GND on Ring 2 band");
		else
			printk("; GND on Sleeve band");
		switch ((reg >> 3) & 0x07) {
			case 0:	printk("; Tip-Sleeve < 400 Ohm"); break;
			case 1:	printk("; Tip-Sleeve < 800 Ohm"); break;
			case 2:	printk("; Tip-Sleeve <1200 Ohm"); break;
			case 3:	printk("; Tip-Sleeve <1600 Ohm"); break;
			case 4:	printk("; Tip-Sleeve <2000 Ohm"); break;
			case 5:	printk("; Tip-Sleeve <2400 Ohm"); break;
			case 6:	printk("; Tip-Sleeve <2800 Ohm"); break;
			case 7:	printk("; Tip-Sleeve >2800 Ohm"); break;
		}
		switch ((reg >> 0) & 0x07) {
			case 0:	printk("; Tip-Ring2 < 400 Ohm\n"); break;
			case 1:	printk("; Tip-Ring2 < 800 Ohm\n"); break;
			case 2:	printk("; Tip-Ring2 <1200 Ohm\n"); break;
			case 3:	printk("; Tip-Ring2 <1600 Ohm\n"); break;
			case 4:	printk("; Tip-Ring2 <2000 Ohm\n"); break;
			case 5:	printk("; Tip-Ring2 <2400 Ohm\n"); break;
			case 6:	printk("; Tip-Ring2 <2800 Ohm\n"); break;
			case 7:	printk("; Tip-Ring2 >2800 Ohm\n"); break;
		}
	}
}

static irqreturn_t ts3a225_isr(int irq, void *devid)
{
	struct ts3a225e *data = devid;

//	complete(&data->done);

	return IRQ_HANDLED;
}

static void ts3a225_work(struct work_struct *work)
{
	struct ts3a225e *data = container_of(work, struct ts3a225e,
		work.work);
	struct device *dev = data->dev;

	ts3a225_state_changed(dev);

	schedule_delayed_work(&data->work, POLL_INTERVAL);
}

static int ts3a225_detect(struct device *dev)
{
	struct ts3a225e *data = dev_get_drvdata(dev);
	unsigned int id;
	int ret;

	ret = regmap_read(data->regmap, TS3A225_CHIP_ID_REG, &id);
	if (ret < 0)
		return ret;

	if (id != data->chip_id)
		return -ENODEV;

	return 0;
}

static int ts3a225_i2c_detect(struct i2c_client *client,
			     struct i2c_board_info *info)
{
	if (client->addr != TS3A225_I2C_ADDRESS)
		return -ENODEV;

	return ts3a225_detect(&client->dev);
}

static struct regmap_config ts3a225_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8

	// define IRQ register as volatile
};

int ts3a225_probe(struct device *dev, struct regmap *regmap, int irq)
{
	struct ts3a225e *data;
	int err = 0;

	data = kzalloc(sizeof(struct ts3a225e), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}

	dev_set_drvdata(dev, data);
	data->dev = dev;
	data->regmap = regmap;
	data->irq = irq;
	data->last_reg = -1;

	if (data->irq > 0) {
		err = devm_request_irq(dev, data->irq, ts3a225_isr,
					      IRQF_TRIGGER_RISING, "ts3a225",
					      data);
		if (err < 0)
			goto exit_free;
	} else {
		INIT_DELAYED_WORK(&data->work, ts3a225_work);
		schedule_delayed_work(&data->work, POLL_INTERVAL);
	}


#if OLD

	/* Register sysfs hooks */
	err = sysfs_create_group(&dev->kobj, &ts3a225_attr_group);
	if (err)
		goto exit_free;

#endif
	dev_info(dev, "Successfully initialized %s!\n", DRIVER_NAME);

	return 0;

exit_free:
	kfree(data);
exit:
	return err;
}

static int ts3a225_i2c_probe(struct i2c_client *client,
				      const struct i2c_device_id *id)
{
	int err;
	struct regmap *regmap = devm_regmap_init_i2c(client,
						     &ts3a225_regmap_config);

	if (IS_ERR(regmap)) {
		err = PTR_ERR(regmap);
		dev_err(&client->dev, "Failed to init regmap: %d\n", err);
		return err;
	}

	return ts3a225_probe(&client->dev, regmap, client->irq);
}

static int ts3a225_i2c_remove(struct i2c_client *client)
{
	return 0;
//	return ts3a225_remove(&client->dev);
}

static const struct i2c_device_id ts3a225_id[] = {
	{ "ts3a225e", 0x02 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ts3a225_id);

static const struct of_device_id ts3a225e_of_match[] = {
	{ .compatible = "ti,ts3a225e", },
	{ }
};
MODULE_DEVICE_TABLE(of, ts3a225e_of_match);

static struct i2c_driver ts3a225_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(ts3a225e_of_match),
	},
	.id_table	= ts3a225_id,
	.probe		= ts3a225_i2c_probe,
	.remove		= ts3a225_i2c_remove,

	.detect		= ts3a225_i2c_detect,
	.address_list	= normal_i2c
};

module_i2c_driver(ts3a225_i2c_driver);

MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("TS3A225 I2C bus driver");
MODULE_LICENSE("GPL");
