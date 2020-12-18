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


#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/mfd/tps6518x.h>

#include <linux/gpio.h>

/*
 * Conversions
 */
static int temp_from_reg(int val)
{
	return val;
}



/*
 * Client data (each client gets its own)
 */
struct tps6518x_data {
	struct device *hwmon_dev;
	struct tps6518x *tps6518x;
};



int tps6518x_get_temperature(struct tps6518x *tps6518x,int *O_piTemperature)
{
	int iTemp;
	unsigned int reg_val;

	if(!tps6518x) {
		printk(KERN_ERR"%s(),65185 object error !!\n",__FUNCTION__);
		return -1;
	}

	/*
	 * begin Temperature conversion
	 */
	switch (tps6518x->revID & 0xff)
	{
	   case TPS65180_PASS1 :
	   case TPS65180_PASS2 :
	   case TPS65181_PASS1 :
	   case TPS65181_PASS2 :
		    reg_val = 0x80;
		    tps6518x_reg_write(tps6518x,REG_TPS65180_TMST_CONFIG, reg_val);
		    // wait for completion completed
		    while ((0x20 & reg_val) == 0)
		    {
		        msleep(1);
		        tps6518x_reg_read(tps6518x,REG_TPS65180_TMST_CONFIG, &reg_val);
		    }
	        break;
	   case TPS65185_PASS0 :
	   case TPS65186_PASS0 :
	   case TPS65185_PASS1 :
	   case TPS65186_PASS1 :
	   case TPS65185_PASS2 :
	   case TPS65186_PASS2 :
		    reg_val = 0x80;
		    tps6518x_reg_write(tps6518x,REG_TPS65185_TMST1, reg_val);
		    // wait for completion completed
		    while ((0x20 & reg_val) == 0)
		    {
		        msleep(1);
		        tps6518x_reg_read(tps6518x,REG_TPS65185_TMST1, &reg_val);
		    }
	        break;
	   default:
		break;	

	}


	tps6518x_reg_read(tps6518x,REG_TPS6518x_TMST_VAL, &reg_val);
	iTemp = temp_from_reg(reg_val);

	if(O_piTemperature) {
		printk("%s():temperature = %d\n",__FUNCTION__,iTemp);
		*O_piTemperature = iTemp;
	}
	
	return 0;
}

EXPORT_SYMBOL(tps6518x_get_temperature);

/*
 * Sysfs stuff
 */
static ssize_t show_temp_input(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct tps6518x_data *data = dev_get_drvdata(dev);
	int iTemp ;

	if(0==tps6518x_get_temperature(data->tps6518x,&iTemp)) {
		return snprintf(buf, PAGE_SIZE, "%d\n", iTemp);
	}
	else {
		return snprintf(buf, PAGE_SIZE, "?\n");
	}
}

static ssize_t show_intr_regs(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	unsigned int reg_val;
	unsigned int intr_reg_val;
	struct tps6518x_data *data = dev_get_drvdata(dev);
	/*
	 * get the interrupt status register value
	 */
	switch (data->tps6518x->revID)
	{
	   case TPS65180_PASS1 :
	   case TPS65180_PASS2 :
	   case TPS65181_PASS1 :
	   case TPS65181_PASS2 :
		    tps6518x_reg_read(data->tps6518x,REG_TPS65180_INT1, &intr_reg_val);
		    tps6518x_reg_read(data->tps6518x,REG_TPS65180_INT2, &reg_val);
		    intr_reg_val |= reg_val<<8;
	        break;
	   case TPS65185_PASS0 :
	   case TPS65186_PASS0 :
	   case TPS65185_PASS1 :
	   case TPS65186_PASS1 :
	   case TPS65185_PASS2 :
	   case TPS65186_PASS2 :
		    tps6518x_reg_read(data->tps6518x,REG_TPS65185_INT1, &intr_reg_val);
		    tps6518x_reg_read(data->tps6518x,REG_TPS65185_INT2, &reg_val);
		    intr_reg_val |= reg_val<<8;
	        break;
	   default:
		break;	

	}

	return snprintf(buf, PAGE_SIZE, "0x%04X\n", intr_reg_val);
}


static DEVICE_ATTR(temp_input, S_IRUGO, show_temp_input, NULL);
static DEVICE_ATTR(intr_input, S_IRUGO, show_intr_regs, NULL);

static struct attribute *tps6518x_attributes[] = {
	&dev_attr_temp_input.attr,
	&dev_attr_intr_input.attr,
	NULL
};

static const struct attribute_group tps6518x_group = {
	.attrs = tps6518x_attributes,
};

/*
 * Real code
 */
static int tps6518x_sensor_probe(struct platform_device *pdev)
{
	struct tps6518x_data *data;
	int err;
    printk("tps6518x_sensor_probe starting\n");

	data = kzalloc(sizeof(struct tps6518x_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	data->tps6518x = dev_get_drvdata(pdev->dev.parent);
	/* Register sysfs hooks */
	err = sysfs_create_group(&pdev->dev.kobj, &tps6518x_group);
	if (err)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove_files;
	}

	platform_set_drvdata(pdev, data);

    printk("tps6518x_sensor_probe success\n");
	return 0;

exit_remove_files:
	sysfs_remove_group(&pdev->dev.kobj, &tps6518x_group);
exit_free:
	kfree(data);
exit:
	return err;
}

static int tps6518x_sensor_remove(struct platform_device *pdev)
{
	struct tps6518x_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &tps6518x_group);

	kfree(data);
	return 0;
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
	.remove = tps6518x_sensor_remove,
	.id_table = tps6518x_sns_id,
	.driver = {
		.name = "tps6518x_sensor",
	},
};

module_platform_driver(tps6518x_sensor_driver);

MODULE_DESCRIPTION("TPS6518x sensor driver");
MODULE_LICENSE("GPL");
