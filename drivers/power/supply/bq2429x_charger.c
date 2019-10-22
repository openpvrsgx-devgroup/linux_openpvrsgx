// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * BQ24296/7 battery charger and VSYS+OTG regulator
 *
 * Copyright (C) 2014 Rokchip
 * Original-author: 张晴 <zhangqing@rock-chips.com>
 * Original-author: yj <yangjie@rock-chips.com>
 * Original-author: Elaine Zhang <zhangqing@rock-chips.com>
 *
 * Copyright (C) 2016-2019 Golden Delicious Computers GmbH&Co. KG
 * Author: H. Nikolaus Schaller <hns@goldelico.com>
 * I found Rokchip code in some Android driver and modified it to
 * become useable for the Pyra handheld.
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/idr.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/param.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define VSYS_REGULATOR	0
#define OTG_REGULATOR	1
#define NUM_REGULATORS	2

/* I2C register defines */
#define INPUT_SOURCE_CONTROL_REGISTER		0x00
#define POWER_ON_CONFIGURATION_REGISTER		0x01
#define CHARGE_CURRENT_CONTROL_REGISTER		0x02
#define PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER		0x03
#define CHARGE_VOLTAGE_CONTROL_REGISTER		0x04
#define TERMINATION_TIMER_CONTROL_REGISTER		0x05
#define THERMAL_REGULATION_CONTROL_REGISTER		0x06
#define MISC_OPERATION_CONTROL_REGISTER		0x07
#define SYSTEM_STATS_REGISTER		0x08
#define FAULT_STATS_REGISTER		0x09
#define VENDOR_STATS_REGISTER		0x0A

/* REG00 input source control register value */
#define EN_HIZ_ENABLE	 1
#define EN_HIZ_DISABLE	 0
#define EN_HIZ_OFF	 7
#define EN_HIZ_MASK	 1

#define VINDPM_OFF		3
#define VINDPM_MASK		0xf

#define IINLIM_100MA		0
#define IINLIM_150MA		1
#define IINLIM_500MA		2
#define IINLIM_900MA		3
#define IINLIM_1200MA		4
#define IINLIM_1500MA		5
#define IINLIM_2000MA		6
#define IINLIM_3000MA		7
#define IINLIM_OFF		0
#define IINLIM_MASK		7

/* REG01 power-on configuration register value */
/* OTG Mode Current Config */
#define OTG_MODE_CURRENT_CONFIG_500MA	0x00
#define OTG_MODE_CURRENT_CONFIG_1300MA	0x01
#define OTG_MODE_CURRENT_CONFIG_OFF	0
#define OTG_MODE_CURRENT_CONFIG_MASK	0x01

/* VSYS Minimum */
#define SYS_MIN_OFF		1
#define SYS_MIN_MASK		0x7

/* Charge Mode Config */
#define CHARGE_MODE_CONFIG_CHARGE_DISABLE	0x00
#define CHARGE_MODE_CONFIG_CHARGE_BATTERY	0x01
#define CHARGE_MODE_CONFIG_OTG_OUTPUT		0x02
#define CHARGE_MODE_CONFIG_OFF			4
#define CHARGE_MODE_CONFIG_MASK			0x03

/* Watchdog */
#define WATCHDOG_RESET	0x40

/* Reset */
#define REGISTER_RESET_ENABLE	 1
#define REGISTER_RESET_DISABLE	 0
#define REGISTER_RESET_OFF	 7
#define REGISTER_RESET_MASK	 1

/* REG02 charge current limit register value */
#define CHARGE_CURRENT_64MA		0x01
#define CHARGE_CURRENT_128MA		0x02
#define CHARGE_CURRENT_256MA		0x04
#define CHARGE_CURRENT_512MA		0x08
#define CHARGE_CURRENT_1024MA		0x10
#define CHARGE_CURRENT_1536MA		0x18
#define CHARGE_CURRENT_2048MA		0x20
#define CHARGE_CURRENT_OFF		2
#define CHARGE_CURRENT_MASK		0x3f

/* REG03 Pre-Charge/Termination Current Control Register value */
/* Pre-Charge Current Limit */
#define PRE_CHARGE_CURRENT_LIMIT_128MA		0x00
#define PRE_CHARGE_CURRENT_LIMIT_256MA		0x01
#define PRE_CHARGE_CURRENT_LIMIT_OFF		4
#define PRE_CHARGE_CURRENT_LIMIT_MASK		0x0f
/* Termination Current Limit */
#define TERMINATION_CURRENT_LIMIT_128MA		0x00
#define TERMINATION_CURRENT_LIMIT_256MA		0x01
#define TERMINATION_CURRENT_LIMIT_OFF		0
#define TERMINATION_CURRENT_LIMIT_MASK		0x0f

/* REG04 Charge Voltage Register */
#define VREG_MASK	0x3f
#define VREG_OFF	2

/* REG05 Charge Termination/Timer control register value */
#define WATCHDOG_DISABLE	0
#define WATCHDOG_40S		1
#define WATCHDOG_80S		2
#define WATCHDOG_160S		3
#define WATCHDOG_OFF		4
#define WATCHDOG_MASK		3

/* REG06 boost voltage/thermal regulation register */
#define BOOSTV_OFF	4
#define BOOSTV_MASK	0xf

/* REG07 misc operation control register value */
#define DPDM_ENABLE	 1
#define DPDM_DISABLE	 0
#define DPDM_OFF	 7
#define DPDM_MASK	 1

/* REG08 system status register value */
#define VBUS_UNKNOWN		0
#define VBUS_USB_HOST		1
#define VBUS_ADAPTER_PORT	2
#define VBUS_OTG		3
#define VBUS_OFF		6
#define VBUS_MASK		3

#define CHRG_NO_CHARGING	0
#define CHRG_PRE_CHARGE		1
#define CHRG_FAST_CHARGE	2
#define CHRG_CHRGE_DONE		3
#define CHRG_OFF		4
#define CHRG_MASK		3

#define DPM_STAT	0x08
#define PG_STAT		0x04
#define THERM_STAT	0x02
#define VSYS_STAT	0x01

/* REG09 fault status register value */

#define WATCHDOG_FAULT	0x80
#define OTG_FAULT	0x40
#define CHRG_FAULT_OFF	4
#define CHRG_FAULT_MASK	0x3
#define BAT_FAULT	0x08
#define NTC_FAULT_OFF	0
#define NTC_FAULT_MASK	0x3

/* REG0a vendor status register value */
#define CHIP_BQ24296		0x20
#define CHIP_BQ24297		0x60

#define ID_BQ24296		0
#define ID_BQ24297		1

struct bq2429x_device_info {
	struct device			*dev;
	struct i2c_client		*client;
	const struct i2c_device_id	*id;

	struct power_supply		*usb;

	struct regulator_desc		desc[NUM_REGULATORS];
	struct device_node		*of_node[NUM_REGULATORS];
	struct regulator_dev		*rdev[NUM_REGULATORS];
	struct regulator_init_data	*pmic_init_data;

	struct mutex			var_lock;

	struct delayed_work		usb_detect_work;
	struct work_struct		irq_work;
	struct workqueue_struct	*workqueue;

	/* input to detect different input supplies */
	struct gpio_desc		*dc_det_pin;
	/* output connected to psel of bq24296 */
	struct gpio_desc		*psel_pin;

	/* status register values from last read */
	u8 r8, r9;
	/* second last read for change detection */
	u8 prev_r8, prev_r9;
	/* is power adapter plugged in */
	bool adapter_plugged;

	/* charging current limit */
	unsigned int	chg_current_uA;
	/* default current limit after plugin of USB power */
	unsigned int	usb_input_current_uA;
	/* alternate power source (not USB) */
	unsigned int	adp_input_current_uA;
	unsigned int	voltage_min_design_uV;
	unsigned int	battery_voltage_max_design_uV;
	unsigned int	max_VSYS_uV;
};

/* helper tables */

static const unsigned int iinlim_table[] = {
	100000,
	150000,
	500000,
	900000,
	1000000,
	1500000,
	2000000,
	3000000,
};

static const unsigned int vsys_VSEL_table[] = {
	3000000,
	3100000,
	3200000,
	3300000,
	3400000,
	3500000,
	3600000,
	3700000,
};

static const unsigned int otg_VSEL_table[] = {
	4550000,
	4614000,
	4678000,
	4742000,
	4806000,
	4870000,
	4934000,
	4998000,
	5062000,
	5126000,
	5190000,
	5254000,
	5318000,
	5382000,
	5446000,
	5510000,
};

/*
 * Common code for BQ24296 devices read
 */

static int bq2429x_i2c_reg8_read(const struct i2c_client *client,
				 const char reg, char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msgs[2];
	char reg_buf = reg;
	int ret;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 1;
	msgs[0].buf = &reg_buf;

	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].len = count;
	msgs[1].buf = (char *)buf;

	ret = i2c_transfer(adap, msgs, 2);

	return (ret == 2) ? count : ret;
}

static int bq2429x_i2c_reg8_write(const struct i2c_client *client,
				  const char reg, const char *buf, int count)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	char *tx_buf = kmalloc(count + 1, GFP_KERNEL);
	int ret;

	if (!tx_buf)
		return -ENOMEM;
	tx_buf[0] = reg;
	memcpy(tx_buf+1, buf, count);

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = count + 1;
	msg.buf = (char *)tx_buf;

	ret = i2c_transfer(adap, &msg, 1);
	kfree(tx_buf);
	return (ret == 1) ? count : ret;
}

static inline int bq2429x_read(struct i2c_client *client,
			       u8 reg, u8 buf[], unsigned int len)
{
	int ret;

	ret = bq2429x_i2c_reg8_read(client, reg, buf, len);
	return ret;
}

static inline int bq2429x_write(struct i2c_client *client,
				u8 reg, u8 const buf[], unsigned int len)
{
	int ret;

	ret = bq2429x_i2c_reg8_write(client, reg, buf, (int)len);
	return ret;
}

static int bq2429x_update_reg(struct i2c_client *client,
			      int reg, u8 value, u8 mask)
{
	u8 retval = 0;
	int ret;

	ret = bq2429x_read(client, reg, &retval, 1);
	if (ret < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);
		return ret;
	}
	ret = 0;

	dev_dbg(&client->dev, "%s %02x: ( %02x & %02x ) | %02x -> %02x\n",
		__func__, reg, retval, (u8) ~mask, value,
		(u8) ((retval & ~mask) | value));

	if ((retval & mask) != value) {
		retval = (retval & ~mask) | value;
		ret = bq2429x_write(client, reg, &retval, 1);
		if (ret < 0) {
			dev_err(&client->dev, "%s: err %d\n",
				__func__, ret);
			return ret;
		}
		ret = 0;
	}

	return ret;
}

/* sysfs tool to show all register values */

static ssize_t show_registers(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	u8 buffer;
	struct i2c_client *client = to_i2c_client(dev);
	struct bq2429x_device_info *di = i2c_get_clientdata(client);
	int len = 0;
	int i;

	for (i = 0; i < 11; i++) {
		int n;

		bq2429x_read(di->client, i, &buffer, 1);
		n = scnprintf(buf, 256, "reg %02x value %02x\n",
			      i, buffer);
		buf += n;
		len += n;
	}
	return len;
}

DEVICE_ATTR(registers, 0444, show_registers, NULL);

/* getter and setter functions with conversion to/from uA/uV */

static int bq2429x_get_vindpm_uV(struct bq2429x_device_info *di)
{
	u8 retval;
	int ret;

	ret = bq2429x_read(di->client, INPUT_SOURCE_CONTROL_REGISTER,
			   &retval, 1);
	if (ret < 0) {
		dev_err(&di->client->dev, "%s: err %d\n", __func__, ret);
		return ret;
	}

	return 3880000 + 80000*((retval >> VINDPM_OFF) & VINDPM_MASK);
}

static int bq2429x_input_current_limit_uA(struct bq2429x_device_info *di)
{
	int ret;
	u8 retval;

	ret = bq2429x_read(di->client, INPUT_SOURCE_CONTROL_REGISTER,
			   &retval, 1);
	if (ret < 0) {
		dev_err(&di->client->dev, "%s: err %d\n", __func__, ret);
		return ret;
	}

	if (((retval >> EN_HIZ_OFF) & EN_HIZ_MASK) == EN_HIZ_ENABLE)
		return 0;	// High-Z state

	return iinlim_table[(retval >> IINLIM_OFF) & IINLIM_MASK];
}

static int bq2429x_set_input_current_limit_uA(struct bq2429x_device_info *di,
					      int uA)
{
	u8 hiz = EN_HIZ_DISABLE;
	u8 data = 0;
	int ret;

	dev_dbg(di->dev, "%s(%d)\n", __func__, uA);

	if (uA < 80000)		/* includes negative current limit */
		hiz = EN_HIZ_ENABLE;
	else if (uA < 120000)
		data = 0;
	else if (uA < 400000)
		data = 1;
	else if (uA < 700000)
		data = 2;
	else if (uA < 1000000)
		data = 3;
	else if (uA < 1200000)
		data = 4;
	else if (uA < 1800000)
		data = 5;
	else if (uA < 2200000)
		data = 6;
	else
		data = 7;

	ret = bq2429x_update_reg(di->client,
				  INPUT_SOURCE_CONTROL_REGISTER,
				  (((data & IINLIM_MASK) << IINLIM_OFF) |
					(hiz << EN_HIZ_OFF)),
				  ((IINLIM_MASK << IINLIM_OFF) |
					(EN_HIZ_MASK << EN_HIZ_OFF)));
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set input current limit (0x%x)\n",
				__func__, data);
	}

	return ret;
}

static int bq2429x_get_charge_current_uA(struct bq2429x_device_info *di)
{
	int ret;
	u8 retval;

	ret = bq2429x_read(di->client, CHARGE_CURRENT_CONTROL_REGISTER, &retval,
			   1);
	dev_dbg(di->dev, "bq2429x: CHARGE_CURRENT_CONTROL_REGISTER %02x\n",
		retval);

	if (ret < 0) {
		dev_err(&di->client->dev, "%s: err %d\n", __func__, ret);
		return ret;
	}

	return 64000 * ((retval >> CHARGE_CURRENT_OFF) & CHARGE_CURRENT_MASK)
	       + 512000;
}

static int bq2429x_set_charge_current_uA(struct bq2429x_device_info *di, int uA)
{
	int data;
	int ret;

	if (uA < 0)
		return -EINVAL;

	data = (uA - 512000 + 32000) / 64000;
	data = min(0x27, max(data, 0));	/* limit to 512 mA .. 3008 mA */

	ret = bq2429x_update_reg(di->client,
				  CHARGE_CURRENT_CONTROL_REGISTER,
				  (data << CHARGE_CURRENT_OFF),
				  (CHARGE_CURRENT_MASK << CHARGE_CURRENT_OFF));
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set charge current limit (%d)\n",
			__func__, uA);
	}
	return ret;
}

static int bq2429x_get_precharge_current_uA(struct bq2429x_device_info *di)
{
	u8 retval;
	int ret;

	ret = bq2429x_read(di->client,
			   PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER,
			   &retval, 1);
	dev_dbg(di->dev, "bq2429x: PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER %02x\n",
		retval);

	if (ret < 0) {
		dev_err(&di->client->dev, "%s: err %d\n", __func__, ret);
		return ret;
	}

	return 128000 * ((retval >> PRE_CHARGE_CURRENT_LIMIT_OFF) &
			 PRE_CHARGE_CURRENT_LIMIT_MASK) + 128000;
}

static int bq2429x_set_precharge_current_uA(struct bq2429x_device_info *di, int uA)
{
	int data;
	int ret;

	if (uA < 0)
		return -EINVAL;

	data = (uA - 128000 + 64000) / 128000;
	data = min(0xf, max(data, 0));	/* limit to 128 mA .. 2048 mA */

	ret = bq2429x_update_reg(di->client,
				  PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER,
				  (data << PRE_CHARGE_CURRENT_LIMIT_OFF),
				  (PRE_CHARGE_CURRENT_LIMIT_MASK << PRE_CHARGE_CURRENT_LIMIT_OFF));
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set precharge charge current (%d)\n",
			__func__, uA);
	}
	return ret;
}

static int bq2429x_set_charge_term_current_uA(struct bq2429x_device_info *di, int uA)
{
	int data;
	int ret;

	if (uA < 0)
		return -EINVAL;

	data = (uA - 128000 + 64000) / 128000;
	data = min(0xf, max(data, 0));	/* limit to 128 mA .. 2048 mA */

	ret = bq2429x_update_reg(di->client,
				  PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER,
				  (data << TERMINATION_CURRENT_LIMIT_OFF),
				  (PRE_CHARGE_CURRENT_LIMIT_MASK << TERMINATION_CURRENT_LIMIT_OFF));
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set charge current limit (%d)\n",
			__func__, uA);
	}
	return ret;
}

static int bq2429x_en_hiz_disable(struct bq2429x_device_info *di)
{
	int ret;

	ret = bq2429x_update_reg(di->client,
				  INPUT_SOURCE_CONTROL_REGISTER,
				  EN_HIZ_DISABLE << EN_HIZ_OFF,
				  EN_HIZ_MASK << EN_HIZ_OFF);
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set en_hiz_disable\n",
			__func__);
	}
	return ret;
}

static int bq2429x_set_charge_mode(struct bq2429x_device_info *di, u8 mode)
{
	int ret;

	ret = bq2429x_update_reg(di->client,
				  POWER_ON_CONFIGURATION_REGISTER,
				  mode << CHARGE_MODE_CONFIG_OFF,
				  CHARGE_MODE_CONFIG_MASK << CHARGE_MODE_CONFIG_OFF);
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set charge mode(0x%x)\n",
				__func__, mode);
	}

	return ret;
}

static int bq2429x_get_vsys_voltage_uV(struct bq2429x_device_info *di)
{
	u8 retval;
	int ret;

	dev_dbg(di->dev, "%s\n", __func__);

	ret = bq2429x_read(di->client, POWER_ON_CONFIGURATION_REGISTER, &retval,
			   1);
	if (ret < 0)
		return ret;

	dev_dbg(di->dev, " => %d uV\n",
		vsys_VSEL_table[(retval >> SYS_MIN_OFF) & SYS_MIN_MASK]);

	return vsys_VSEL_table[(retval >> SYS_MIN_OFF) & SYS_MIN_MASK];
}

static int bq2429x_set_vsys_voltage_uV(struct bq2429x_device_info *di,
				       int min_uV, int max_uV)
{
	dev_dbg(di->dev, "%s(%d, %d)\n", __func__, min_uV, max_uV);

// revisit: the driver should select the voltage closest to min_uV by scanning vsys_VSEL_table

	return 0;	/* disabled/untested */

	/* set system voltage */

	return bq2429x_update_reg(di->client,
				  POWER_ON_CONFIGURATION_REGISTER,
				  0,	/* 3.0V + 0.2V */
				  SYS_MIN_MASK << SYS_MIN_OFF);
}

static int bq2429x_get_otg_voltage_uV(struct bq2429x_device_info *di)
{
	u8 retval;
	int ret;

	dev_dbg(di->dev, "%s\n", __func__);

	ret = bq2429x_read(di->client, THERMAL_REGULATION_CONTROL_REGISTER,
			   &retval, 1);
	if (ret < 0)
		return ret;

	dev_dbg(di->dev, " => %d uV\n",
		otg_VSEL_table[(retval >> BOOSTV_OFF) & BOOSTV_MASK]);

	return otg_VSEL_table[(retval >> BOOSTV_OFF) & BOOSTV_MASK];
}

static int bq2429x_set_otg_voltage_uV(struct bq2429x_device_info *di,
				      int min_uV, int max_uV)
{
	dev_dbg(di->dev, "%s(%d, %d)\n", __func__, min_uV, max_uV);

// revisit: the driver should select the voltage closest to min_uV by scanning otg_VSEL_table

	return 0;	/* disabled/untested */

	/* set OTG step up converter voltage */

	return bq2429x_update_reg(di->client,
				  THERMAL_REGULATION_CONTROL_REGISTER,
				  0,
				  BOOSTV_MASK << BOOSTV_OFF);
}

static int bq2429x_is_otg_enabled(struct bq2429x_device_info *di)
{ /* check if OTG converter is enabled */
	u8 retval;
	int ret;

	dev_dbg(di->dev, "%s\n", __func__);

	ret = bq2429x_read(di->client, POWER_ON_CONFIGURATION_REGISTER, &retval,
			   1);
	if (ret < 0)
		return 0;	/* assume disabled */

	/*
	 * we could alternatively check r8 for OTG mode
	 * return ((di->r8 >> VBUS_OFF) && VBUS_MASK) == VBUS_OTG;
	 * which one is better?
	 */

	/* check bit 5 of POWER_ON_CONFIGURATION_REGISTER */
	return ((retval >> CHARGE_MODE_CONFIG_OFF) & CHARGE_MODE_CONFIG_MASK)
	       == CHARGE_MODE_CONFIG_OTG_OUTPUT;
}

static int bq2429x_get_otg_current_limit_uA(struct bq2429x_device_info *di)
{
	u8 retval;
	int ret;

	dev_dbg(di->dev, "%s\n", __func__);

	ret = bq2429x_read(di->client, POWER_ON_CONFIGURATION_REGISTER, &retval,
			   1);
	if (ret < 0)
		return ret;

	return ((retval >> OTG_MODE_CURRENT_CONFIG_OFF) &
		 OTG_MODE_CURRENT_CONFIG_MASK) ?
		 1000000 : 1500000;	/* 1.0A or 1.5A */
}

static int bq2429x_set_otg_current_limit_uA(struct bq2429x_device_info *di,
				     int min_uA, int max_uA)
{
	int enable = true;
	int val = OTG_MODE_CURRENT_CONFIG_500MA;

	dev_dbg(di->dev, "%s(%d, %d)\n", __func__, min_uA, max_uA);

	/*
	 * set OTG current limit in bit 0 of POWER_ON_CONFIGURATION_REGISTER
	 * choose 1A for values < 1.25A and 1.5A for values above
	 */

	if (max_uA < 500000)
		enable = false;	/* disable OTG */
	else if (max_uA < 1250000)
		val = OTG_MODE_CURRENT_CONFIG_500MA;	/* enable 1A */
	else
		val = OTG_MODE_CURRENT_CONFIG_1300MA;	/* enable 1.5A */

	return bq2429x_update_reg(di->client,
		POWER_ON_CONFIGURATION_REGISTER,
		(enable << 5)|(val << OTG_MODE_CURRENT_CONFIG_OFF),
		(1 << 5)|
		(OTG_MODE_CURRENT_CONFIG_MASK << OTG_MODE_CURRENT_CONFIG_OFF));
}

/* initialize the chip */

static int bq2429x_get_vendor_id(struct bq2429x_device_info *di)
{
	u8 retval;
	int ret;

	/* get the vendor id */
	ret = bq2429x_read(di->client, VENDOR_STATS_REGISTER, &retval, 1);
	if (ret < 0)
		return ret;
	return retval;
}

static int bq2429x_init_registers(struct bq2429x_device_info *di)
{
	int max_uV;
	int bits;
	int ret;

	/* revisit: could read from monitored battery properties
	 * (precharge-current-microamp, charge-term-current-microamp)
	 */

	// bq2429x_set_precharge_current_uA(di->precharge_current_uA);

	/* set Pre-Charge Current Limit as 128mA */
	ret = bq2429x_update_reg(di->client,
		PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER,
		PRE_CHARGE_CURRENT_LIMIT_128MA << PRE_CHARGE_CURRENT_LIMIT_OFF,
		PRE_CHARGE_CURRENT_LIMIT_MASK << PRE_CHARGE_CURRENT_LIMIT_OFF);
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set pre-charge limit 128mA\n",
				__func__);
		return ret;
	}

	// bq2429x_set_charge_term_current_uA(di->charge_term_current_uA);

	/* set Termination Current Limit as 128mA */
	ret = bq2429x_update_reg(di->client,
		PRE_CHARGE_TERMINATION_CURRENT_CONTROL_REGISTER,
		TERMINATION_CURRENT_LIMIT_128MA << TERMINATION_CURRENT_LIMIT_OFF,
		TERMINATION_CURRENT_LIMIT_MASK << TERMINATION_CURRENT_LIMIT_OFF);
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set termination limit 128mA\n",
				__func__);
		return ret;
	}

	/*
	 * VSYS may be up to 150 mV above fully charged battery voltage
	 * if operating from VBUS.
	 * So to effectively limit VSYS we may have to lower the max. battery
	 * voltage.
	 */

	max_uV = di->max_VSYS_uV - 150000;
	max_uV = min_t(int, max_uV, (int) di->battery_voltage_max_design_uV);

	bits = (max_uV - 3504000) / 16000;
	bits = max(bits, 0);
	bits = min(bits, 63);

	dev_dbg(&di->client->dev, "%s(): translated vbatt_max=%u and VSYS_max=%u to VREG=%u (%02x)\n",
		__func__,
		di->battery_voltage_max_design_uV, di->max_VSYS_uV, max_uV,
		bits);

	/* revisit: bq2429x_set_charge_current_uA(di, ?); */

	ret = bq2429x_update_reg(di->client,
				  CHARGE_VOLTAGE_CONTROL_REGISTER,
				  bits << VREG_OFF,
				  VREG_MASK << VREG_OFF);
	if (ret < 0) {
		dev_err(&di->client->dev, "%s(): Failed to set max. battery voltage\n",
				__func__);
		return ret;
	}

	return 0;
}

/* handle USB detection and charging based on status registers */

static inline bool bq2429x_battery_present(struct bq2429x_device_info *di)
{ /* assume if there is an NTC fault there is no battery  */
	return ((di->r9 >> NTC_FAULT_OFF) & NTC_FAULT_MASK) == 0;
}

static inline bool bq2429x_input_present(struct bq2429x_device_info *di)
{ /* VBUS is available */
	return (di->r8 & PG_STAT) != 0;
}

static int bq2429x_battery_temperature_mC(struct bq2429x_device_info *di)
{
	/*
	 * since there is no ADC available for the NTC we deduce values
	 * revisit: during boost mode deduce values from BHOT and BCOLD
	 * settings
	 */

	if (di->r9 & 0x02)
		return -10000;	/* too cold (-10C) */
	else if (di->r9 & 0x01)
		return 60000;	/* too hot (60C) */
	return 22500;	/* ok (22.5C) */
}

static void bq2429x_input_available(struct bq2429x_device_info *di, bool state)
{ /* track external power input state and trigger actions on change */
	if (state && !di->adapter_plugged) {
		di->adapter_plugged = true;

		dev_notice(di->dev, "bq2429x: VBUS became available\n");

		power_supply_changed(di->usb);

		/* start charging */
		if (!bq2429x_battery_present(di))
			return;

		if (di->dc_det_pin) {
			/* detect alternate power supply */
			int ret = gpiod_get_value(di->dc_det_pin);

			if (ret == 0)
				bq2429x_set_input_current_limit_uA(di,
					di->adp_input_current_uA);
			else
				bq2429x_set_input_current_limit_uA(di,
					di->usb_input_current_uA);
		} else
			bq2429x_set_input_current_limit_uA(di,
					di->usb_input_current_uA);

		bq2429x_set_charge_current_uA(di, di->chg_current_uA);
		bq2429x_set_charge_mode(di, CHARGE_MODE_CONFIG_CHARGE_BATTERY);
	} else if (!state && di->adapter_plugged) {
		di->adapter_plugged = false;

		power_supply_changed(di->usb);

		dev_notice(di->dev, "bq2429x: VBUS became unavailable\n");
	}
}

static int bq2429x_usb_detect(struct bq2429x_device_info *di)
{
	int ret;

	/* lock if interrupt and polling occur at the same time */
	mutex_lock(&di->var_lock);

	dev_dbg(di->dev, "%s, line=%d\n", __func__, __LINE__);

	ret = bq2429x_read(di->client, SYSTEM_STATS_REGISTER, &di->r8, 1);
	if (ret != 1) {
		mutex_unlock(&di->var_lock);

		dev_err(&di->client->dev, "%s: err %d\n", __func__, ret);

		return ret;
	}

	ret = bq2429x_read(di->client, FAULT_STATS_REGISTER, &di->r9, 1);
	if (ret != 1) {
		mutex_unlock(&di->var_lock);

		dev_err(&di->client->dev, "%s: err %d\n", __func__, ret);

		return ret;
	}

	/* report changes to last state */
	if (di->r8 != di->prev_r8 || di->r9 != di->prev_r9)
		{
		char string[200];
		sprintf(string, "r8=%02x", di->r8);
		switch ((di->r8 >> 6) & 3) {
		case 1: strcat(string, " HOST"); break;
		case 2: strcat(string, " ADAP"); break;
		case 3: strcat(string, " OTG"); break;
		};
		switch ((di->r8 >> 4) & 3) {
		case 1: strcat(string, " PRECHG"); break;
		case 2: strcat(string, " FCHG"); break;
		case 3: strcat(string, " CHGTERM"); break;
		};
		if ((di->r8 >> 3) & 1)
			strcat(string, " INDPM");
		if ((di->r8 >> 2) & 1)
			strcat(string, " PWRGOOD");
		if ((di->r8 >> 1) & 1)
			strcat(string, " THERMREG");
		if ((di->r8 >> 0) & 1)
			strcat(string, " VSYSMIN");
		sprintf(string+strlen(string), " r9=%02x", di->r9);
		if ((di->r9 >> 7) & 1)
			strcat(string, " WDOG");
		if ((di->r9 >> 6) & 1)
			strcat(string, " OTGFAULT");
		switch ((di->r9 >> 4) & 3) {
		case 1: strcat(string, " UNPLUG"); break;
		case 2: strcat(string, " THERMAL"); break;
		case 3: strcat(string, " CHGTIME"); break;
		};
		if ((di->r9 >> 3) & 1)
			strcat(string, " BATFAULT");
		if ((di->r9 >> 2) & 1)
			strcat(string, " RESERVED");
		if ((di->r9 >> 1) & 1)
			strcat(string, " COLD");
		if ((di->r9 >> 0) & 1)
			strcat(string, " HOT");
		dev_notice(di->dev, "%s: %s\n", __func__, string);
		di->prev_r8 = di->r8, di->prev_r9 = di->r9;
		}

	if (((di->r8 >> 4) & 3) == 3) {
		/* charging  terminated */
		/* power_supply_changed(di->usb); */
	}

	/* handle (momentarily) disconnect of VBUS */
	if ((di->r9 >> CHRG_FAULT_OFF) & CHRG_FAULT_MASK)
		bq2429x_input_available(di, false);

	/* since we are polling slowly, VBUS may already be back again */
	bq2429x_input_available(di, bq2429x_input_present(di));

	mutex_unlock(&di->var_lock);

	return 0;
}

static void usb_detect_work_func(struct work_struct *wp)
{ /* polling if we have no interrupt configured */
	struct delayed_work *dwp =
		(struct delayed_work *)container_of(wp, struct delayed_work,
						    work);
	struct bq2429x_device_info *di =
		(struct bq2429x_device_info *)container_of(dwp,
				struct bq2429x_device_info, usb_detect_work);
	int ret;

	ret = bq2429x_usb_detect(di);

	if (ret == 0)
		schedule_delayed_work(&di->usb_detect_work, 1*HZ);
}

static void bq2729x_irq_work_func(struct work_struct *wp)
{ /* interrupt */
	struct bq2429x_device_info *di = (struct bq2429x_device_info *)container_of(wp, struct bq2429x_device_info, irq_work);

	dev_dbg(di->dev, "%s: di = %px\n", __func__, di);

	bq2429x_usb_detect(di);

	dev_dbg(di->dev, "%s: r8=%02x r9=%02x\n", __func__, di->r8, di->r9);
}

static irqreturn_t bq2729x_chg_irq_func(int irq, void *dev_id)
{
	struct bq2429x_device_info *di = dev_id;

	dev_dbg(di->dev, "%s\n", __func__);

	queue_work(di->workqueue, &di->irq_work);

	return IRQ_HANDLED;
}

/* regulator framework integration for VSYS and OTG */

static int bq2429x_get_vsys_voltage(struct regulator_dev *dev)
{
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	return bq2429x_get_vsys_voltage_uV(di);
}

static int bq2429x_set_vsys_voltage(struct regulator_dev *dev,
				    int min_uV, int max_uV,
				    unsigned int *selector)
{
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	/* how to handle *selector? */

	return bq2429x_set_vsys_voltage_uV(di, min_uV, max_uV);
}

static int bq2429x_get_otg_voltage(struct regulator_dev *dev)
{
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	return bq2429x_get_otg_voltage_uV(di);
}

static int bq2429x_set_otg_voltage(struct regulator_dev *dev,
				   int min_uV, int max_uV,
				   unsigned int *selector)
{
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	/* how to handle *selector? */

	return bq2429x_set_otg_voltage_uV(di, min_uV, max_uV);
}

static int bq2429x_get_otg_current_limit(struct regulator_dev *dev)
{
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	return bq2429x_get_otg_current_limit_uA(di);
}

static int bq2429x_set_otg_current_limit(struct regulator_dev *dev,
				     int min_uA, int max_uA)
{
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	/* how to handle *selector? */

	return bq2429x_set_otg_current_limit_uA(di, min_uA, max_uA);
}

static int bq2429x_otg_enable(struct regulator_dev *dev)
{ /* enable OTG step up converter */
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	/* check if battery is present and reject if no battery */
	if (!bq2429x_battery_present(di)) {
		dev_warn(&di->client->dev, "can enable otg only with installed battery and no overtemperature\n");
		return -EBUSY;
	}

	bq2429x_en_hiz_disable(di);

	mdelay(5);

	return bq2429x_set_charge_mode(di, CHARGE_MODE_CONFIG_OTG_OUTPUT);
	/* could check/wait with timeout that r8 indicates OTG mode */
}

static int bq2429x_otg_disable(struct regulator_dev *dev)
{ /* disable OTG step up converter */
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	return bq2429x_set_charge_mode(di, CHARGE_MODE_CONFIG_CHARGE_DISABLE);
	/* could check/wait with timeout that r8 indicates non-OTG mode */
}

static int bq2429x_otg_is_enabled(struct regulator_dev *dev)
{ /* check if OTG converter is enabled */
	struct bq2429x_device_info *di = rdev_get_drvdata(dev);

	return bq2429x_is_otg_enabled(di);
}

static struct regulator_ops vsys_ops = {
	.get_voltage = bq2429x_get_vsys_voltage,
	.set_voltage = bq2429x_set_vsys_voltage,
};

static struct regulator_ops otg_ops = {
	.get_voltage = bq2429x_get_otg_voltage,
	.set_voltage = bq2429x_set_otg_voltage,
	.get_current_limit = bq2429x_get_otg_current_limit,
	.set_current_limit = bq2429x_set_otg_current_limit,
	.enable = bq2429x_otg_enable,	/* turn on OTG mode */
	.disable = bq2429x_otg_disable,	/* turn off OTG mode */
	.is_enabled = bq2429x_otg_is_enabled,
};

static struct of_regulator_match bq2429x_regulator_matches[] = {
	[VSYS_REGULATOR] = { .name = "bq2429x-vsys"},
	[OTG_REGULATOR] = { .name = "bq2429x-otg"},
};

static int bq2429x_charger_suspend(struct device *dev, pm_message_t state)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq2429x_device_info *di = i2c_get_clientdata(client);

	/* revisit: we may want to turn off otg here */
	cancel_delayed_work_sync(&di->usb_detect_work);
	return 0;
}

static int bq2429x_charger_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq2429x_device_info *di = i2c_get_clientdata(client);

	schedule_delayed_work(&di->usb_detect_work, msecs_to_jiffies(50));
	return 0;
}

static void bq2429x_charger_shutdown(struct i2c_client *client)
{ /* make sure we turn off OTG mode on power down */
	struct bq2429x_device_info *di = i2c_get_clientdata(client);

	if (bq2429x_otg_is_enabled(di->rdev[1]))
		bq2429x_otg_disable(di->rdev[1]);
}

/* SYSFS interface */

/*
 * sysfs input_current_limit store
 * set the max current drawn from USB
 */

static ssize_t
bq2429x_input_current_limit_uA_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t n)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq2429x_device_info *di = i2c_get_clientdata(client);
	int cur = 0;
	int status = 0;

	status = kstrtoint(buf, 10, &cur);
	if (status)
		return status;
	if (cur < 0)
		return -EINVAL;

	status = bq2429x_set_input_current_limit_uA(di, cur);
	if (status < 0)
		return status;

	di->usb_input_current_uA = cur;	/* restore after unplug/replug */

	return n;
}

/*
 * sysfs input_current_limit show
 * reports current drawn from VBUS
 */

static ssize_t bq2429x_input_current_limit_uA_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq2429x_device_info *di = i2c_get_clientdata(client);
	int cur = bq2429x_input_current_limit_uA(di);

	if (cur < 0)
		return cur;

	return scnprintf(buf, PAGE_SIZE, "%u\n", cur);
}

/*
 * sysfs otg max current store
 */

static ssize_t
bq2429x_otg_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t n)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq2429x_device_info *di = i2c_get_clientdata(client);
	int cur = 0;
	int status = 0;

	status = kstrtoint(buf, 10, &cur);
	if (status)
		return status;
	if (cur < 0)
		return -EINVAL;

	dev_dbg(di->dev, "%s: set OTG max current %u uA\n", __func__, cur);

	bq2429x_en_hiz_disable(di);

	mdelay(5);

	status = bq2429x_set_otg_current_limit_uA(di, cur, cur);

	return (status < 0) ? status : n;
}

/*
 * sysfs otg max current show
 */

static ssize_t bq2429x_otg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq2429x_device_info *di = i2c_get_clientdata(client);
	int cur = bq2429x_get_otg_current_limit_uA(di);

	if (cur < 0)
		return cur;

	return scnprintf(buf, PAGE_SIZE, "%u\n", cur);
}

static DEVICE_ATTR(max_current, 0644, bq2429x_input_current_limit_uA_show,
			bq2429x_input_current_limit_uA_store);

static DEVICE_ATTR(otg, 0644, bq2429x_otg_show, bq2429x_otg_store);

/* power_supply interface */

static int bq2429x_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct bq2429x_device_info *di = power_supply_get_drvdata(psy);
	int ret;

	dev_dbg(di->dev, "%s,line=%d prop=%d\n", __func__, __LINE__, psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		switch ((di->r8 >> CHRG_OFF) & CHRG_MASK) {
		case CHRG_NO_CHARGING:
			val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		case CHRG_PRE_CHARGE:
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
			break;
		case CHRG_FAST_CHARGE:
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
			break;
		case CHRG_CHRGE_DONE:
			val->intval = POWER_SUPPLY_STATUS_FULL;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		switch ((di->r8 >> CHRG_OFF) & CHRG_MASK) {
		case CHRG_NO_CHARGING:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			break;
		case CHRG_PRE_CHARGE:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case CHRG_FAST_CHARGE:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		case CHRG_CHRGE_DONE:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		switch ((di->r9 >> CHRG_FAULT_OFF) & CHRG_FAULT_MASK) {
		case 0:
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
			break;
		case 1:
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
			break;
		case 2:
			val->intval = POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
			break;
		case 3:
			val->intval = POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		if(bq2429x_input_present(di)) {
			if ((di->r8 & DPM_STAT) != 0)
				val->intval = bq2429x_get_vindpm_uV(di);
			else
				/* power good: assume VBUS 5V */
				val->intval = 5000000;
		} else
			/* power not good: assume 0V */
			val->intval = 0;
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = bq2429x_input_current_limit_uA(di);
		dev_dbg(di->dev, "bq2429x CURRENT_MAX: %u mA\n", val->intval);
		break;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		switch ((di->r8 >> CHRG_OFF) & CHRG_MASK) {
		case CHRG_NO_CHARGING:
		case CHRG_CHRGE_DONE:
			/* assume no charging current */
			val->intval = 0;
			dev_dbg(di->dev, "bq2429x CURRENT_NOW: %u mA\n",
				val->intval);
			break;

		case CHRG_PRE_CHARGE:
			val->intval = bq2429x_input_current_limit_uA(di);
			ret = bq2429x_get_precharge_current_uA(di);
			/* report the lower of both */
			if (ret < val->intval)
				val->intval = ret;
			dev_dbg(di->dev, "bq2429x CURRENT_NOW: %u mA\n",
				val->intval);
			break;

		case CHRG_FAST_CHARGE:
			val->intval = bq2429x_input_current_limit_uA(di);
			ret = bq2429x_get_charge_current_uA(di);
			/* report the lower of both */
			if (ret < val->intval)
				val->intval = ret;
			dev_dbg(di->dev, "bq2429x CURRENT_NOW: %u mA\n", val->intval);
			break;
		}
		break;

	case POWER_SUPPLY_PROP_TEMP:
		val->intval = bq2429x_battery_temperature_mC(di) / 100;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = bq2429x_input_present(di);
		break;

	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = bq2429x_battery_present(di);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int bq2429x_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct bq2429x_device_info *di = power_supply_get_drvdata(psy);

	dev_dbg(di->dev, "%s,line=%d prop=%d\n", __func__, __LINE__, psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return bq2429x_set_input_current_limit_uA(di, val->intval);
	default:
		return -EPERM;
	}

	return 0;
}

static int bq2429x_writeable_property(struct power_supply *psy,
					enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return 1;
	default:
		break;
	}

	return 0;
}

static enum power_supply_property bq2429x_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_PRESENT,
// POWER_SUPPLY_PROP_HEALTH
};

static const struct power_supply_desc bq2429x_power_supply_desc[] = {
	[ID_BQ24296] = {
	.name			= "bq24296",
	.type			= POWER_SUPPLY_TYPE_USB,
	.properties		= bq2429x_charger_props,
	.num_properties		= ARRAY_SIZE(bq2429x_charger_props),
	.get_property		= bq2429x_get_property,
	.set_property		= bq2429x_set_property,
	.property_is_writeable	= bq2429x_writeable_property,
	},
	[ID_BQ24297] = {
	.name			= "bq24297",
	.type			= POWER_SUPPLY_TYPE_USB,
	.properties		= bq2429x_charger_props,
	.num_properties		= ARRAY_SIZE(bq2429x_charger_props),
	.get_property		= bq2429x_get_property,
	.set_property		= bq2429x_set_property,
	.property_is_writeable	= bq2429x_writeable_property,
	},
};

/* device tree support */

static int bq2429x_parse_dt(struct bq2429x_device_info *di)
{
	struct device_node *np;
	struct device_node *regulators;
	struct device_node *regulator_np;
	struct device_node *battery_np;
	struct of_regulator_match *matches;
	static struct regulator_init_data *reg_data;
	int idx = 0, count, ret;
	u32 val;

	dev_dbg(di->dev, "%s,line=%d\n", __func__, __LINE__);

	np = of_node_get(di->dev->of_node);
	if (!np) {
		dev_err(&di->client->dev, "could not find bq2429x DT node\n");
		return -EINVAL;
	}

	di->battery_voltage_max_design_uV = 4200000;	/* default for LiIon */
	di->voltage_min_design_uV = 3200000;
	di->adp_input_current_uA = 2048000;
	/* take defaults as set by U-Boot or power-on */
	di->chg_current_uA = bq2429x_get_charge_current_uA(di);
	di->usb_input_current_uA = bq2429x_input_current_limit_uA(di);

	of_property_read_u32(np, "ti,usb-input-current-microamp",
			     &di->usb_input_current_uA);
	of_property_read_u32(np, "ti,adp-input-current-microamp",
			     &di->adp_input_current_uA);

	battery_np = of_parse_phandle(np, "monitored-battery", 0);

	if (battery_np) {
		u32 value;

		of_property_read_u32(battery_np,
				"voltage-max-design-microvolt",
				&di->battery_voltage_max_design_uV);
		of_property_read_u32(battery_np,
				"voltage-min-design-microvolt",
				&di->voltage_min_design_uV);
		of_property_read_u32(battery_np,
				"constant-charge-current-max-microamp",
				&di->chg_current_uA);
		if (!of_property_read_u32(battery_np,
				"precharge-current-microamp",
				&value));
			bq2429x_set_precharge_current_uA(di, value);
		if (!of_property_read_u32(battery_np,
				"charge-term-current-microamp",
				&value));
			bq2429x_set_charge_term_current_uA(di, value);
		of_node_put(battery_np);
	}

	dev_info(di->dev, "%s,line=%u chg_current = %u usb_input_current = %u adp_input_current = %u bat_volt_max = %u\n", __func__,__LINE__,
		di->chg_current_uA, di->usb_input_current_uA,
		di->adp_input_current_uA, di->battery_voltage_max_design_uV);

	/*
	 * optional dc_det_pin
	 * if 0, charger is switched by driver to 2048mA, otherwise 512mA
	 * or whatever is defined in device tree
	 */
	di->dc_det_pin = gpiod_get_optional(&di->client->dev,
					    "dc-det", GPIOD_IN);

	if (IS_ERR(di->dc_det_pin)) {
		if (PTR_ERR(di->dc_det_pin) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		dev_err(&di->client->dev, "invalid det gpio: %ld\n", PTR_ERR(di->dc_det_pin));
		di->dc_det_pin = NULL;
	}

	/* we provide two regulators, VSYS and VOTG */
	regulators = of_get_child_by_name(np, "regulators");
	if (!regulators) {
		dev_err(&di->client->dev, "regulator node not found\n");
		return -EINVAL;
	}

	count = ARRAY_SIZE(bq2429x_regulator_matches);
	matches = bq2429x_regulator_matches;

	ret = of_regulator_match(&di->client->dev, regulators, matches, count);
	dev_dbg(di->dev, "%d matches\n", ret);

	if (ret < 0) {
		dev_err(&di->client->dev, "Error parsing regulator init data: %d\n",
			ret);
		return ret;
	}

	if (ret != count) {
		dev_err(&di->client->dev, "Found %d of expected %d regulators\n",
			ret, count);
		return -EINVAL;
	}

	regulator_np = of_get_next_child(regulators, NULL);	// get first regulator (vsys)
	if (!of_property_read_u32(regulator_np, "regulator-max-microvolt", &val)) {
		dev_err(&di->client->dev, "found regulator-max-microvolt = %u\n", val);
		di->max_VSYS_uV = val;	// limit by device tree
	}
	of_node_put(regulator_np);
	of_node_put(regulators);

	reg_data = devm_kzalloc(&di->client->dev,
				NUM_REGULATORS * sizeof(reg_data[0]),
				GFP_KERNEL);
	if (!reg_data)
		return -EINVAL;

	di->pmic_init_data = reg_data;

	for (idx = 0; idx < ret; idx++) {
		if (!matches[idx].init_data || !matches[idx].of_node)
			continue;

		memcpy(&reg_data[idx], matches[idx].init_data,
				sizeof(struct regulator_init_data));
	}

	return 0;
}

static const struct of_device_id bq2429x_charger_of_match[] = {
	{ .compatible = "ti,bq24296", .data = (void *) 0 },
	{ .compatible = "ti,bq24297", .data = (void *) 1 },
	{ },
};
MODULE_DEVICE_TABLE(of, bq2429x_charger_of_match);

static int bq2429x_charger_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	struct bq2429x_device_info *di;
	struct device_node *bq2429x_node;
	struct power_supply_config psy_cfg = { };
	struct regulator_config config = { };
	struct regulator_init_data *init_data;
	struct regulator_dev *rdev;
	int i;
	int ret;

	dev_dbg(di->dev, "%s,line=%d\n", __func__, __LINE__);

	bq2429x_node = of_node_get(client->dev.of_node);
	if (!bq2429x_node) {
		dev_warn(&client->dev, "could not find bq2429x DT node\n");
		return -EINVAL;
	}

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (di == NULL) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		return -ENOMEM;
	}

	di->dev = &client->dev;
	i2c_set_clientdata(client, di);
	di->client = client;
	di->id = id;
	di->prev_r8 = 0xff;
	di->prev_r9 = 0xff;

	ret = bq2429x_get_vendor_id(di);

	if (ret < 0) {
		dev_err(&di->client->dev,
			"%s(): Failed reading vendor register\n", __func__);
		return -EPROBE_DEFER;	// try again later
	}

	switch (ret) {
	case CHIP_BQ24296:
	case CHIP_BQ24297:
	default:
		dev_err(&client->dev, "not a bq2429x: %d %02x\n", ret, ret);
		return -ENODEV;
	}

	ret = bq2429x_parse_dt(di);

	if (ret < 0) {
		if (ret != -EPROBE_DEFER)
			dev_err(&client->dev, "failed to parse DT\n");
		return ret;
	}

	init_data = di->pmic_init_data;
	if (!init_data)
		return -EINVAL;

	mutex_init(&di->var_lock);
	di->workqueue = create_singlethread_workqueue("bq2429x_irq");
	INIT_WORK(&di->irq_work, bq2729x_irq_work_func);
	INIT_DELAYED_WORK(&di->usb_detect_work, usb_detect_work_func);

	ret = bq2429x_init_registers(di);
	if (ret < 0) {
		dev_err(&client->dev, "failed to initialize registers: %d\n",
			ret);
		return ret;
	}

	psy_cfg.drv_data = di;
	di->usb = devm_power_supply_register(&client->dev,
			&bq2429x_power_supply_desc[id->driver_data],
			&psy_cfg);
	if (IS_ERR(di->usb)) {
		ret = PTR_ERR(di->usb);
		dev_err(&client->dev,
			"failed to register as USB power_supply: %d\n", ret);
		return ret;
	}

	for (i = 0; i < NUM_REGULATORS; i++, init_data++) {
		/* Register the regulators */

		di->desc[i].id = i;
		di->desc[i].name = bq2429x_regulator_matches[i].name;
		di->desc[i].type = REGULATOR_VOLTAGE;
		di->desc[i].owner = THIS_MODULE;

		switch (i) {
		case VSYS_REGULATOR:
			di->desc[i].ops = &vsys_ops;
			di->desc[i].n_voltages = ARRAY_SIZE(vsys_VSEL_table);
			di->desc[i].volt_table = vsys_VSEL_table;
			break;
		case OTG_REGULATOR:
			di->desc[i].ops = &otg_ops;
			di->desc[i].n_voltages = ARRAY_SIZE(otg_VSEL_table);
			di->desc[i].volt_table = otg_VSEL_table;
			break;
		}

		config.dev = di->dev;
		config.init_data = init_data;
		config.driver_data = di;
		config.of_node = bq2429x_regulator_matches[i].of_node;

		rdev = devm_regulator_register(&client->dev, &di->desc[i],
					       &config);
		if (IS_ERR(rdev)) {
			dev_err(di->dev,
				"failed to register %s regulator %d %s\n",
				client->name, i, di->desc[i].name);
			return PTR_ERR(rdev);
		}

		/* save regulator reference for cleanup */
		di->rdev[i] = rdev;
	}

	ret = devm_request_threaded_irq(&client->dev, client->irq,
				NULL, bq2729x_chg_irq_func,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				client->name,
				di);
	if (ret < 0) {
		dev_warn(&client->dev, "failed to request chg_irq: %d - polling\n",
			 ret);
		client->irq = 0;
	}

	if (device_create_file(&client->dev, &dev_attr_max_current))
		dev_warn(&client->dev, "could not create sysfs file max_current\n");

	if (device_create_file(&client->dev, &dev_attr_otg))
		dev_warn(&client->dev, "could not create sysfs file otg\n");

	if (device_create_file(&client->dev, &dev_attr_registers))
		dev_warn(&client->dev, "could not create sysfs file registers\n");

	if (!client->irq)
		schedule_delayed_work(&di->usb_detect_work, 0);

	dev_dbg(di->dev, "%s ok", __func__);

	return 0;
}

static int bq2429x_charger_remove(struct i2c_client *client)
{
	struct bq2429x_device_info *di = i2c_get_clientdata(client);

	device_remove_file(di->dev, &dev_attr_max_current);
	device_remove_file(di->dev, &dev_attr_otg);
	device_remove_file(di->dev, &dev_attr_registers);
	return 0;
}

static const struct i2c_device_id bq2429x_charger_id[] = {
	{ "bq24296", ID_BQ24296 },
	{ "bq24297", ID_BQ24297 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, bq2429x_charger_id);

static struct i2c_driver bq2429x_charger_driver = {
	.probe = bq2429x_charger_probe,
	.remove = bq2429x_charger_remove,
	.shutdown = bq2429x_charger_shutdown,
	.id_table = bq2429x_charger_id,
	.driver = {
		.name = "bq2429x_charger",
		.of_match_table = of_match_ptr(bq2429x_charger_of_match),
		.suspend = bq2429x_charger_suspend,
		.resume = bq2429x_charger_resume,
	},
};

module_i2c_driver(bq2429x_charger_driver);

MODULE_AUTHOR("Rockchip");
MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("TI BQ24296/7 charger driver");
MODULE_LICENSE("GPL");
