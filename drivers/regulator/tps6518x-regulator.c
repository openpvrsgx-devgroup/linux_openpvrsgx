/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/mfd/tps6518x.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

//#define TPS65185_V3P3_ENABLE		1

struct tps6518x_data {
	int num_regulators;
	struct tps6518x *tps6518x;
	struct regulator_dev **rdev;
};


static int tps6518x_pass_num = { 2 };
#if 0
static int tps6518x_vcom = { 0 };
#endif
//static int tps65180_current_Enable_Register = 0;

static int tps6518x_is_power_good(struct tps6518x *tps6518x);


/*
 * Regulator operations
 */
/* Convert uV to the VCOM register bitfield setting */

static int vcom_rs_to_uV(unsigned int reg_setting)
{
	if (reg_setting >= TPS65180_VCOM_MAX_SET)
		return TPS65180_VCOM_MAX_uV;
	return (reg_setting * TPS65180_VCOM_STEP_uV);
}
#if 0
static int vcom2_rs_to_uV(unsigned int reg_setting)
{
	if (reg_setting >= TPS65185_VCOM_MAX_SET)
		return TPS65185_VCOM_MAX_uV;
	return (reg_setting * TPS65185_VCOM_STEP_uV);
}
#endif


static int vcom_uV_to_rs(int uV)
{
	if (uV <= TPS65180_VCOM_MIN_uV)
		return TPS65180_VCOM_MIN_SET;
	if (uV >= TPS65180_VCOM_MAX_uV)
		return TPS65180_VCOM_MAX_SET;
	return uV / TPS65180_VCOM_STEP_uV;
}

static int vcom2_uV_to_rs(int uV)
{
	if (uV <= TPS65185_VCOM_MIN_uV)
		return TPS65185_VCOM_MIN_SET;
	if (uV >= TPS65185_VCOM_MAX_uV)
		return TPS65185_VCOM_MAX_SET;
	return (uV) / TPS65185_VCOM_STEP_uV;
}

static int epdc_pwr0_enable(struct regulator_dev *reg)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);

	gpiod_set_value(tps6518x->gpio_pmic_powerup, 1);

	return 0;

}

static int epdc_pwr0_disable(struct regulator_dev *reg)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);
	
	gpiod_set_value(tps6518x->gpio_pmic_powerup, 0);

	return 0;

}
static int tps6518x_v3p3_enable(struct regulator_dev *reg)
{
#ifdef TPS65185_V3P3_ENABLE//[
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);

	if(tps6518x->gpio_pmic_v3p3) {
		gpiod_set_value_cansleep(tps6518x->gpio_pmic_v3p3, 1);
	}
	else {
		ERR_MSG("epdc pmic v3p3 pin not available\n");
	}
#endif //]TPS65185_V3P3_ENABLE

	return 0;
}

static int tps6518x_v3p3_disable(struct regulator_dev *reg)
{
#ifdef TPS65185_V3P3_ENABLE//[
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);

	if(gpio_is_valid(tps6518x->gpio_pmic_v3p3)) {
		gpiod_set_value_cansleep(tps6518x->gpio_pmic_v3p3, 0);
	}
	else {
		ERR_MSG("epdc pmic v3p3 pin not available\n");
	}
#endif //]TPS65185_V3P3_ENABLE

	return 0;

}
static int tps6518x_v3p3_is_enabled(struct regulator_dev *reg)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);
	int gpio = gpiod_get_value_cansleep(tps6518x->gpio_pmic_powerup);

	if (gpio == 0)
		return 0;
	else
		return 1;
}


static int _tps6518x_vcom_set_voltage(struct tps6518x *tps6518x, int uV)
{
	int ret = 0;
	int converted_uv;
	/*
	 * this will not work on tps65182
	 */
	if (tps6518x->revID == 65182) {
		return 0;
	}
	
	switch (tps6518x->revID & 15)
	{
		case 0 : /* TPS65180 */
		case 1 : /* TPS65181 */
		case 4 : /* TPS65180-rev1 */
			ret = regmap_write(tps6518x->regmap,
					   REG_TPS65180_VCOM_ADJUST,
					   vcom_uV_to_rs(uV));
			break;
		case 5 : /* TPS65185 */
		case 6 : /* TPS65186 */

			converted_uv = vcom2_uV_to_rs(uV);
			gpiod_set_value_cansleep(tps6518x->gpio_pmic_wakeup,1);
			msleep(2);
			ret = regmap_write(tps6518x->regmap, REG_TPS65185_VCOM1,
					      converted_uv & 255);
			ret = regmap_update_bits(tps6518x->regmap, REG_TPS65185_VCOM2,
						    1, converted_uv >> 8);
			break;
		default :
		ret = -ENOENT;
	}

	if(ret > 0) {
		tps6518x->vcom_uV = uV;
	}

	return ret;
}

static int tps6518x_vcom_set_voltage(struct regulator_dev *reg,
					int minuV, int uV, unsigned *selector)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);
	return _tps6518x_vcom_set_voltage(tps6518x,uV);
}


static int _tps6518x_vcom_get_voltage(struct tps6518x *tps6518x)
{
	unsigned int reg, reg2;
	int vcomValue;

	/*
	 * this will not work on tps65182
	 */
	if (tps6518x->revID == 65182) {
		return 0;
	}
	
	switch (tps6518x->revID & 15)
	{
		case 0 : /* TPS65180 */
		case 1 : /* TPS65181 */
		case 4 : /* TPS65180-rev1 */
			regmap_read(tps6518x->regmap, REG_TPS65180_VCOM_ADJUST,
				    &reg);
			vcomValue = vcom_rs_to_uV(reg);
			break;
		case 5 : /* TPS65185 */
		case 6 : /* TPS65186 */
			regmap_read(tps6518x->regmap, REG_TPS65185_VCOM1, &reg);
			regmap_read(tps6518x->regmap, REG_TPS65185_VCOM2, &reg2);
			reg |= (reg2 & 1) << 8;
			vcomValue = reg * 10 * 1000;
			printk("%s() : vcom=%duV\n",__FUNCTION__,vcomValue);
			break;
		default:
			vcomValue = 0;
	}

	if(vcomValue) {
		tps6518x->vcom_uV = vcomValue;
	}
	
	return vcomValue;
}
static int tps6518x_vcom_get_voltage(struct regulator_dev *reg)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);
	return _tps6518x_vcom_get_voltage(tps6518x);
}

static int tps6518x_vcom_enable(struct regulator_dev *regulator)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(regulator);
	unsigned int reg;
	int vcomEnable = 0;

	/*
	 * check for the TPS65182 device
	 */
	if (tps6518x->revID == 65182)
	{
		gpiod_set_value_cansleep(tps6518x->gpio_pmic_vcom_ctrl,vcomEnable);
		return 0;
	}

	switch (tps6518x->revID & 15)
	{
		case 0 : /* TPS65180 */
		case 1 : /* TPS65181 */
		case 4 : /* TPS65180-rev1 */
			vcomEnable = 1;
			break;
		case 5 : /* TPS65185 */
		case 6 : /* TPS65186 */
			regmap_read(tps6518x->regmap, REG_TPS65185_VCOM2, &reg);
			// do not enable vcom if HiZ bit is set
			if (reg & (1<<VCOM_HiZ_LSH))
				vcomEnable = 0;
			else
				vcomEnable = 1;
			break;
		default:
			vcomEnable = 0;
	}
	gpiod_set_value_cansleep(tps6518x->gpio_pmic_vcom_ctrl, vcomEnable);

	return 0;
}

static int tps6518x_vcom_disable(struct regulator_dev *reg)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);

	gpiod_set_value_cansleep(tps6518x->gpio_pmic_vcom_ctrl,0);
	return 0;
}

static int tps6518x_vcom_is_enabled(struct regulator_dev *reg)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);

	int gpio = gpiod_get_value_cansleep(tps6518x->gpio_pmic_vcom_ctrl);
	if (gpio == 0)
		return 0;
	else
		return 1;
}


static int tps6518x_is_power_good(struct tps6518x *tps6518x)
{
	/*
	 * XOR of polarity (starting value) and current
	 * value yields whether power is good.
	 */
	return gpiod_get_value_cansleep(tps6518x->gpio_pmic_pwrgood) ^
		tps6518x->pwrgood_polarity;
}


static void tps6518x_int_func(struct work_struct *work)
{
	unsigned int reg, reg2;
	struct tps6518x *tps6518x = container_of(work, struct tps6518x, int_work);

	regmap_read(tps6518x->regmap, REG_TPS65180_INT1, &reg);
	regmap_read(tps6518x->regmap, REG_TPS65180_INT2, &reg2);
	dev_err(tps6518x->dev, "TPS6518X intr occured !!,INT1=0x%x,INT2=0x%x\n", reg, reg2);
}

static int tps6518x_wait_power_good(struct tps6518x *tps6518x)
{
	int i;

	msleep(1);

	for (i = 0; i < tps6518x->max_wait * 3; i++) 
	//for (i = 0; i < tps6518x->max_wait * 300; i++) 
	{
		if(0==gpiod_get_value_cansleep(tps6518x->gpio_pmic_intr)) {
			//tps6518x_int_func(&tps6518x->int_work);
			queue_work(tps6518x->int_workqueue,&tps6518x->int_work);
		}
		if (tps6518x_is_power_good(tps6518x)) {
			dev_dbg(tps6518x->dev, "%s():cnt=%d,PG=%d\n",__FUNCTION__,i,gpiod_get_value_cansleep(tps6518x->gpio_pmic_pwrgood));
			return 0;
		}

		msleep(1);
	}
	dev_err(tps6518x->dev,"%s():waiting(%d) for PG(%d) timeout\n",__FUNCTION__,i,gpiod_get_value_cansleep(tps6518x->gpio_pmic_pwrgood));
	return -ETIMEDOUT;
}

static int tps6518x_display_enable(struct regulator_dev *regulator)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(regulator);
	unsigned int reg;

	if (tps6518x->revID == 65182)
	{
		epdc_pwr0_enable(regulator);
	}
	else
	{
#if 0
		if(0==gpiod_get_value_cansleep(tps6518x->gpio_pmic_wakeup)) {
			printk("%s():wakeup ...\n",__FUNCTION__);
			tps6518x_chip_power(tps6518x,1,1,-1);
		}
#endif

		if(tps6518x->timing_need_restore) {
			if(tps6518x_setup_timings(tps6518x)>=0) {
				tps6518x->timing_need_restore=0;
			}
		}

		regmap_read(tps6518x->regmap, REG_TPS65180_ENABLE, &reg);
		/* enable display regulators */
		dev_dbg(tps6518x->dev, "%s(%d):wakeup=%d,powerup=%d,enREG=0x%x\n",__FUNCTION__,__LINE__,
				gpiod_get_value_cansleep(tps6518x->gpio_pmic_wakeup),
				gpiod_get_value_cansleep(tps6518x->gpio_pmic_powerup),
				reg);

		/* check VCOM_EN */
		regmap_update_bits(tps6518x->regmap, REG_TPS65180_ENABLE,
				   VDDH_EN | VPOS_EN | VEE_EN | VNEG_EN | VCOM_EN,
				   VDDH_EN | VPOS_EN | VEE_EN | VNEG_EN | VCOM_EN);

		epdc_pwr0_enable(regulator);

		regmap_update_bits(tps6518x->regmap, REG_TPS65180_ENABLE, ACTIVE, ACTIVE);
	}

	return tps6518x_wait_power_good(tps6518x);
}

static int tps6518x_display_disable(struct regulator_dev *regulator)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(regulator);

	if (tps6518x->revID == 65182)
	{
		epdc_pwr0_disable(regulator);
	}
	else
	{
		/* turn off display regulators */
		dev_dbg(tps6518x->dev, "%s(%d):wakeup=%d\n",__FUNCTION__,__LINE__,
				gpiod_get_value_cansleep(tps6518x->gpio_pmic_wakeup));

		regmap_update_bits(tps6518x->regmap, REG_TPS65180_ENABLE, STANDBY, STANDBY);

		epdc_pwr0_disable(regulator);
	}

	//msleep(tps6518x->max_wait);

	return 0;
}

static int tps6518x_display_is_enabled(struct regulator_dev *reg)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);
/*
	if (tps6518x->revID == 65182)
*/
		return gpiod_get_value_cansleep(tps6518x->gpio_pmic_powerup) ? 1:0;
/*
	else
		return tps6518x->iRegEnable & BITFMASK(ACTIVE);
*/
}

static int tps6518x_tmst_get_temperature(struct regulator_dev *reg)
{
	struct tps6518x *tps6518x = rdev_get_drvdata(reg);
	const int iDefaultTemp = 25;
	int iTemperature = iDefaultTemp;

	if(tps6518x_get_temperature(tps6518x,&iTemperature)<0) { 
		iTemperature = iDefaultTemp;
	}

	return iTemperature;
}

/*
 * Regulator operations
 */

static struct regulator_ops tps6518x_display_ops = {
	.enable = tps6518x_display_enable,
	.disable = tps6518x_display_disable,
	.is_enabled = tps6518x_display_is_enabled,
};

static struct regulator_ops tps6518x_vcom_ops = {
	.enable = tps6518x_vcom_enable,
	.disable = tps6518x_vcom_disable,
	.get_voltage = tps6518x_vcom_get_voltage,
	.set_voltage = tps6518x_vcom_set_voltage,
	.is_enabled = tps6518x_vcom_is_enabled,
};

static struct regulator_ops tps6518x_v3p3_ops = {
	.enable = tps6518x_v3p3_enable,
	.disable = tps6518x_v3p3_disable,
	.is_enabled = tps6518x_v3p3_is_enabled,
};

static struct regulator_ops tps6518x_tmst_ops = {
	.get_voltage = tps6518x_tmst_get_temperature,
};

/*
 * Regulator descriptors
 */
static struct regulator_desc tps6518x_reg[TPS6518x_NUM_REGULATORS] = {
{
	.name = "DISPLAY",
	.id = TPS6518x_DISPLAY,
	.ops = &tps6518x_display_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
},
{
	.name = "VCOM",
	.id = TPS6518x_VCOM,
	.ops = &tps6518x_vcom_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
},
{
	.name = "V3P3",
	.id = TPS6518x_V3P3,
	.ops = &tps6518x_v3p3_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
},
{
	.name = "TMST",
	.id = TPS6518x_TMST,
	.ops = &tps6518x_tmst_ops,
	.type = REGULATOR_VOLTAGE,
	.owner = THIS_MODULE,
},
};

#define CHECK_PROPERTY_ERROR_KFREE(prop) \
do { \
	int ret = of_property_read_u32(tps6518x->dev->of_node, \
					#prop, &tps6518x->prop); \
	if (ret < 0) { \
		return ret;	\
	}	\
} while (0);

#ifdef CONFIG_OF
static int tps6518x_pmic_dt_parse_pdata(struct platform_device *pdev,
					struct tps6518x_platform_data *pdata)
{
	struct tps6518x *tps6518x = dev_get_drvdata(pdev->dev.parent);
	struct device_node *pmic_np, *regulators_np, *reg_np;
	struct tps6518x_regulator_data *rdata;
	int i, ret = 0;

	pmic_np = of_node_get(tps6518x->dev->of_node);
	if (!pmic_np) {
		dev_err(&pdev->dev, "could not find pmic sub-node\n");
		return -ENODEV;
	}

#if 0
	if(0!=of_property_read_u32(pmic_np,"pwr_seq0",&tps6518x->pwr_seq0))
		dev_warn(&pdev->dev, "pwr_seq0 property not found");
	if(0!=of_property_read_u32(pmic_np,"pwr_seq1",&tps6518x->pwr_seq1))
		dev_warn(&pdev->dev, "pwr_seq1 property not found");
	if(0!=of_property_read_u32(pmic_np,"pwr_seq2",&tps6518x->pwr_seq2))
		dev_warn(&pdev->dev, "pwr_seq2 property not found");
#endif 

	if(0!=of_property_read_u32(pmic_np,"upseq0",&tps6518x->upseq0))
		dev_warn(&pdev->dev, "upseq0 property not found");
	if(0!=of_property_read_u32(pmic_np,"upseq1",&tps6518x->upseq1))
		dev_warn(&pdev->dev, "upseq1 property not found");
	if(0!=of_property_read_u32(pmic_np,"dwnseq0",&tps6518x->dwnseq0))
		dev_warn(&pdev->dev, "dwnseq0 property not found");
	if(0!=of_property_read_u32(pmic_np,"dwnseq1",&tps6518x->dwnseq1))
		dev_warn(&pdev->dev, "dwnseq1 property not found");

	regulators_np = of_find_node_by_name(pmic_np, "regulators");
	if (!regulators_np) {
		dev_err(&pdev->dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	pdata->num_regulators = of_get_child_count(regulators_np);
	dev_dbg(&pdev->dev, "num_regulators %d\n", pdata->num_regulators);

	rdata = devm_kzalloc(&pdev->dev, sizeof(*rdata) *
				pdata->num_regulators, GFP_KERNEL);
	if (!rdata) {
		of_node_put(regulators_np);
		dev_err(&pdev->dev, "could not allocate memory for"
			"regulator data\n");
		return -ENOMEM;
	}

	pdata->regulators = rdata;
	for_each_child_of_node(regulators_np, reg_np) {
		for (i = 0; i < ARRAY_SIZE(tps6518x_reg); i++)
			if (!of_node_cmp(reg_np->name, tps6518x_reg[i].name))
				break;

		if (i == ARRAY_SIZE(tps6518x_reg)) {
			dev_warn(&pdev->dev, "don't know how to configure"
				"regulator %s\n", reg_np->name);
			continue;
		}

		rdata->id = i;
		rdata->initdata = of_get_regulator_init_data(&pdev->dev,
							     reg_np, &tps6518x_reg[i]);
		rdata->reg_node = reg_np;
		rdata++;
	}
	of_node_put(regulators_np);

	tps6518x->max_wait = (6 + 6 + 6 + 6);

#ifdef TPS65185_V3P3_ENABLE //[
	tps6518x->gpio_pmic_v3p3 = devm_gpiod_get(&pdev->dev, "v3p3", GPIOD_OUT_HIGH);
	if (IS_ERR(tps6518x->gpio_pmic_v3p3)) {
		dev_err(&pdev->dev, "no epdc pmic v3p3 pin available\n");
		ret = PTR_ERR(tps6518x->gpio_pmic_v3p3);
		return err;
	}
#else
	tps6518x->gpio_pmic_v3p3 = NULL;
#endif //]TPS65185_V3P3_ENABLE

	return ret;
}
#else
static int tps6518x_pmic_dt_parse_pdata(struct platform_device *pdev,
					struct tps6518x *tps6518x)
{
	return 0;
}
#endif	/* !CONFIG_OF */

/*
 * Regulator init/probing/exit functions
 */
static int tps6518x_regulator_probe(struct platform_device *pdev)
{
	struct tps6518x *tps6518x = dev_get_drvdata(pdev->dev.parent);
	struct tps6518x_platform_data *pdata = tps6518x->pdata;
	struct tps6518x_data *priv;
	struct regulator_dev **rdev;
	struct regulator_config config = { };
	int size, i, ret = 0;

	dev_dbg(tps6518x->dev, "tps6518x_regulator_probe starting , of_node=%p\n",
				tps6518x->dev->of_node);

	//tps6518x->pwrgood_polarity = 1;
	if (tps6518x->dev->of_node) {
		
		ret = tps6518x_pmic_dt_parse_pdata(pdev, pdata);
		if (ret)
			return ret;
	}
	priv = devm_kzalloc(&pdev->dev, sizeof(struct tps6518x_data),
			       GFP_KERNEL);
	if (!priv)
		return -ENOMEM;


	size = sizeof(struct regulator_dev *) * pdata->num_regulators;
	priv->rdev = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
	if (!priv->rdev)
		return -ENOMEM;

	dev_info(&pdev->dev, "%s(%d): revID=%d,6518x@%p wakeup gpio%p=%d\n",__FUNCTION__,__LINE__,
			tps6518x->revID, tps6518x,tps6518x->gpio_pmic_wakeup,
			gpiod_get_value_cansleep(tps6518x->gpio_pmic_wakeup));

	rdev = priv->rdev;
	priv->num_regulators = pdata->num_regulators;
	platform_set_drvdata(pdev, priv);

	tps6518x->pass_num = tps6518x_pass_num;
	tps6518x->vcom_uV = _tps6518x_vcom_get_voltage(tps6518x);

	for (i = 0; i < pdata->num_regulators; i++) {
		int id = pdata->regulators[i].id;

		config.dev = tps6518x->dev;
		config.init_data = pdata->regulators[i].initdata;
		config.driver_data = tps6518x;
		config.of_node = pdata->regulators[i].reg_node;

		rdev[i] = regulator_register(&tps6518x_reg[id], &config);
		if (IS_ERR(rdev[i])) {
			ret = PTR_ERR(rdev[i]);
			dev_err(&pdev->dev, "regulator init failed for %d\n",
					id);
			rdev[i] = NULL;
			goto err;
		}
	}

	/*
	 * Set up PMIC timing values.
	 * Should only be done one time!  Timing values may only be
	 * changed a limited number of times according to spec.
	 */
	if(tps6518x_setup_timings(tps6518x)>0)
		tps6518x->timing_need_restore = 1;;

	INIT_WORK(&tps6518x->int_work, tps6518x_int_func);
	tps6518x->int_workqueue = create_singlethread_workqueue("tps6518x_INT");
	if(!tps6518x->int_workqueue) {
		dev_err(tps6518x->dev, "tps6518x int workqueue creating failed !\n");
	}

	dev_dbg(tps6518x->dev, "tps6518x_regulator_probe success\n");
	return 0;
err:
	while (--i >= 0)
		regulator_unregister(rdev[i]);
	return ret;
}

static int tps6518x_regulator_remove(struct platform_device *pdev)
{
	struct tps6518x_data *priv = platform_get_drvdata(pdev);
	struct regulator_dev **rdev = priv->rdev;
	int i;

	for (i = 0; i < priv->num_regulators; i++)
		regulator_unregister(rdev[i]);
	return 0;
}

static struct platform_driver tps6518x_regulator_driver = {
	.probe = tps6518x_regulator_probe,
	.remove = tps6518x_regulator_remove,
	.driver = {
		.name = "tps6518x-regulator",
	},
};

module_platform_driver(tps6518x_regulator_driver);

/* Module information */
MODULE_ALIAS("platform:tps6518x-regulator");
MODULE_DESCRIPTION("TPS6518x regulator driver");
MODULE_LICENSE("GPL");
