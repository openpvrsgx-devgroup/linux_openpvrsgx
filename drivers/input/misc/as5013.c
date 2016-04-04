/*
	vsense.c

	Written by Gražvydas Ignotas <notasas@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.
*/

#define DEBUG
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/idr.h>
#include <linux/i2c/vsense.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_gpio.h>


#define VSENSE_INTERVAL		25

#define VSENSE_MODE_ABS		0
#define VSENSE_MODE_MOUSE	1
#define VSENSE_MODE_SCROLL	2
#define VSENSE_MODE_MBUTTONS	3

static DEFINE_IDR(vsense_proc_id);
static DEFINE_MUTEX(vsense_mutex);

struct vsense_drvdata {
	char dev_name[12];
	struct input_dev *input;
	struct i2c_client *client;
	//struct regulator *reg;
	int x_old, y_old;
	int irq_gpio;
	int mode;
	int proc_id;
	struct delayed_work work;

	struct proc_dir_entry *proc_root;
	int mouse_multiplier;	/* 24.8 */
	int scrollx_multiplier;
	int scrolly_multiplier;
	int scroll_counter;
	int scroll_steps;
	struct {
		int threshold_x;
		int threshold_y;
		int delay;
		int dblclick_stage;
		int state_l;
		int state_m;
		int state_r;
		int pos_active;
		int pos_prev;
		int pos_stable_counter;
	} mbutton;
};

//static
int as5013_i2c_write(struct i2c_client *client,
			    uint8_t aregaddr,
			    uint8_t avalue)
{
	uint8_t data[2] = { aregaddr, avalue };
	struct i2c_msg msg = {
		client->addr, /*I2C_M_IGNORE_NAK*/0, 2, data
	};
	int error;

	error = i2c_transfer(client->adapter, &msg, 1);
	return error < 0 ? error : 0;
}

static int as5013_i2c_read(struct i2c_client *client,
			   uint8_t aregaddr, uint8_t *value)
{
	uint8_t data[2] = { aregaddr };
	struct i2c_msg msg_set[2] = {
		{ client->addr, /* I2C_M_REV_DIR_ADDR */0, 1, data },
		{ client->addr, I2C_M_RD /*| I2C_M_NOSTART*/, 1, data }
	};
	int error;

	error = i2c_transfer(client->adapter, msg_set, ARRAY_SIZE(msg_set));
	if (error < 0)
		return error;

	//*value = data[0] & 0x80 ? -1 * (1 + ~data[0]) : data[0];
	*value = data[0];
	return 0;
}

enum nub_position {
	NUB_POS_CENTER = 0,
	NUB_POS_UP,
	NUB_POS_RIGHT,
	NUB_POS_DOWN,
	NUB_POS_LEFT,
};

static void release_mbuttons(struct vsense_drvdata *ddata)
{
	if (ddata->mbutton.state_l) {
		input_report_key(ddata->input, BTN_LEFT, 0);
		ddata->mbutton.state_l = 0;
	}
	if (ddata->mbutton.state_m) {
		input_report_key(ddata->input, BTN_MIDDLE, 0);
		ddata->mbutton.state_m = 0;
	}
	if (ddata->mbutton.state_r) {
		input_report_key(ddata->input, BTN_RIGHT, 0);
		ddata->mbutton.state_r = 0;
	}
	ddata->mbutton.pos_active = NUB_POS_CENTER;
}

static irqreturn_t as5013_axis_interrupt(int irq, void *dev_id)
{
	struct vsense_drvdata *ddata = dev_id;
	struct i2c_client *client = ddata->client;
	int ax = 0, ay = 0, rx = 0, ry = 0;
	int update_pending = 0;
	int ret, pos, l, m, r;
	uint8_t value;

	//dev_err(&client->dev, "irq\n");
//return IRQ_HANDLED;

	//if (unlikely(gpio_get_value(ddata->irq_gpio)))
	//	goto dosync;

	ret = as5013_i2c_read(client, 0x10, &value);
	if (ret < 0) {
		dev_err(&client->dev, "as5013_i2c_read(10) %d\n", ret);
		return IRQ_HANDLED;
	}
	ax = (int)(signed char)value;
	ax = -ax;

	ret = as5013_i2c_read(client, 0x11, &value);
	if (ret < 0) {
		dev_err(&client->dev, "as5013_i2c_read(11) %d\n", ret);
		return IRQ_HANDLED;
	}
	ay = (int)(signed char)value;

	//rx = (ax - ddata->x_old) / 4;
	//ry = (ay - ddata->y_old) / 4;
	if (abs(ax) > 10)
		rx = ax;
	if (abs(ay) > 10)
		ry = ay;
	ddata->x_old = ax;
	ddata->y_old = ay;

	update_pending = 1;

//dosync:
	switch (ddata->mode) {
	case VSENSE_MODE_MOUSE:
		rx = rx * ddata->mouse_multiplier / 256 / 8;
		ry = -ry * ddata->mouse_multiplier / 256 / 8;
		input_report_rel(ddata->input, REL_X, rx);
		input_report_rel(ddata->input, REL_Y, ry);
		break;
	case VSENSE_MODE_SCROLL:
		if (++(ddata->scroll_counter) < ddata->scroll_steps)
			return IRQ_HANDLED;
		ddata->scroll_counter = 0;
		ax = ax * ddata->scrollx_multiplier / 256 / 4;
		ay = ay * ddata->scrolly_multiplier / 256 / 4;
		input_report_rel(ddata->input, REL_HWHEEL, ax);
		input_report_rel(ddata->input, REL_WHEEL, ay);
		break;
	case VSENSE_MODE_MBUTTONS:
		if (!update_pending) {
			release_mbuttons(ddata);
			break;
		}

		pos = NUB_POS_CENTER;
		if      (ax / 4 >= ddata->mbutton.threshold_x)
			pos = NUB_POS_RIGHT;
		else if (ax / 4 <= -ddata->mbutton.threshold_x)
			pos = NUB_POS_LEFT;
		else if (ay / 4 >= ddata->mbutton.threshold_y)
			pos = NUB_POS_UP;
		else if (ay / 4 <= -ddata->mbutton.threshold_y)
			pos = NUB_POS_DOWN;

		if (pos != ddata->mbutton.pos_prev) {
			ddata->mbutton.pos_prev = pos;
			ddata->mbutton.pos_stable_counter = 0;
		}
		else
			ddata->mbutton.pos_stable_counter++;

		if (ddata->mbutton.pos_stable_counter < ddata->mbutton.delay)
			pos = ddata->mbutton.pos_active;

		if (pos != NUB_POS_UP)
			ddata->mbutton.dblclick_stage = 0;

		l = m = r = 0;
		switch (pos) {
		case NUB_POS_UP:
			ddata->mbutton.dblclick_stage++;
			switch (ddata->mbutton.dblclick_stage) {
				case 1: case 2: case 5: case 6:
				l = 1;
				break;
			}
			break;
		case NUB_POS_RIGHT:
			r = 1;
			break;
		case NUB_POS_DOWN:
			m = 1;
			break;
		case NUB_POS_LEFT:
			l = 1;
			break;
		}
		input_report_key(ddata->input, BTN_LEFT, l);
		input_report_key(ddata->input, BTN_RIGHT, r);
		input_report_key(ddata->input, BTN_MIDDLE, m);
		ddata->mbutton.pos_active = pos;
		ddata->mbutton.state_l = l;
		ddata->mbutton.state_m = m;
		ddata->mbutton.state_r = r;
		break;
	default:
		input_report_abs(ddata->input, ABS_X, ax * 2);
		input_report_abs(ddata->input, ABS_Y, -ay * 2);
		break;
	}
	input_sync(ddata->input);

	return IRQ_HANDLED;
}

static void vsense_work(struct work_struct *work)
{
	struct vsense_drvdata *ddata;
	struct i2c_client *client;
	uint8_t value = 0;
	int ret;

	ddata = container_of(work, struct vsense_drvdata, work.work);
	client = ddata->client;

	ret = as5013_i2c_read(client, 0x0f, &value);
	if (ret < 0) {
		dev_err(&client->dev, "as5013_i2c_read %d\n", ret);
	}
	//dev_info(&client->dev, "control: %02x\n", value);

	if (value & 1)
		as5013_axis_interrupt(0, ddata);

	schedule_delayed_work(&ddata->work, msecs_to_jiffies(VSENSE_INTERVAL));
}

static int vsense_open(struct input_dev *dev)
{
	struct vsense_drvdata *ddata = input_get_drvdata(dev);

	dev_dbg(&dev->dev, "vsense_open\n");

	cancel_delayed_work_sync(&ddata->work);
	schedule_delayed_work(&ddata->work, msecs_to_jiffies(VSENSE_INTERVAL));

	return 0;
}

static int vsense_input_register(struct vsense_drvdata *ddata, int mode)
{
	struct input_dev *input;
	int ret;

	input = input_allocate_device();
	if (input == NULL)
		return -ENOMEM;

	if (mode != VSENSE_MODE_ABS) {
		/* pretend to be a mouse */
		input_set_capability(input, EV_REL, REL_X);
		input_set_capability(input, EV_REL, REL_Y);
		input_set_capability(input, EV_REL, REL_WHEEL);
		input_set_capability(input, EV_REL, REL_HWHEEL);
		/* add fake buttons to fool X that this is a mouse */
		input_set_capability(input, EV_KEY, BTN_LEFT);
		input_set_capability(input, EV_KEY, BTN_RIGHT);
		input_set_capability(input, EV_KEY, BTN_MIDDLE);
	} else {
		input->evbit[BIT_WORD(EV_ABS)] = BIT_MASK(EV_ABS);
		input_set_abs_params(input, ABS_X, -256, 256, 0, 0);
		input_set_abs_params(input, ABS_Y, -256, 256, 0, 0);
	}

	input->name = ddata->dev_name;
	input->dev.parent = &ddata->client->dev;

	input->id.bustype = BUS_I2C;
	input->id.version = 0x0092;

	input->open = vsense_open;

	ddata->input = input;
	input_set_drvdata(input, ddata);

	ret = input_register_device(input);
	if (ret) {
		dev_err(&ddata->client->dev, "failed to register input device,"
			" error %d\n", ret);
		input_free_device(input);
		return ret;
	}

	return 0;
}

static void vsense_input_unregister(struct vsense_drvdata *ddata)
{
	cancel_delayed_work_sync(&ddata->work);
	input_unregister_device(ddata->input);
}

static int vsense_proc_mode_read(struct seq_file *m, void *p)
{
	struct vsense_drvdata *ddata = m->private;

	switch (ddata->mode) {
	case VSENSE_MODE_MOUSE:
		seq_printf(m, "mouse\n");
		break;
	case VSENSE_MODE_SCROLL:
		seq_printf(m, "scroll\n");
		break;
	case VSENSE_MODE_MBUTTONS:
		seq_printf(m, "mbuttons\n");
		break;
	default:
		seq_printf(m, "absolute\n");
		break;
	}

	return 0;
}

static ssize_t vsense_proc_mode_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *offs)
{
	struct vsense_drvdata *ddata =
		((struct seq_file *)file->private_data)->private;
	int mode = ddata->mode;
	char buff[32], *p;
	int ret;

	count = strncpy_from_user(buff, buffer,
			count < sizeof(buff) ? count : sizeof(buff) - 1);
	buff[count] = 0;

	p = buff + strlen(buff) - 1;
	while (p > buff && isspace(*p))
		p--;
	p[1] = 0;

	if (strcasecmp(buff, "mouse") == 0)
		mode = VSENSE_MODE_MOUSE;
	else if (strcasecmp(buff, "scroll") == 0)
		mode = VSENSE_MODE_SCROLL;
	else if (strcasecmp(buff, "mbuttons") == 0)
		mode = VSENSE_MODE_MBUTTONS;
	else if (strcasecmp(buff, "absolute") == 0)
		mode = VSENSE_MODE_ABS;
	else {
		dev_err(&ddata->client->dev, "unknown mode: %s\n", buff);
		return -EINVAL;
	}

	if (ddata->mode != VSENSE_MODE_ABS)
		release_mbuttons(ddata);

	if ((mode == VSENSE_MODE_ABS && ddata->mode != VSENSE_MODE_ABS) ||
	    (mode != VSENSE_MODE_ABS && ddata->mode == VSENSE_MODE_ABS)) {
		disable_irq(ddata->client->irq);
		vsense_input_unregister(ddata);
		ret = vsense_input_register(ddata, mode);
		if (ret)
			dev_err(&ddata->client->dev, "failed to re-register "
				"input as %d, code %d\n", mode, ret);
		else
			enable_irq(ddata->client->irq);
	}
	ddata->mode = mode;

	return count;
}

static int vsense_proc_int_read(struct seq_file *m, void *p)
{
	int *val = m->private;

	seq_printf(m, "%d\n", *val);
	return 0;
}

static int get_write_value(const char __user *buffer, size_t count, int *val)
{
	char buff[32];
	long lval = 0;
	int ret;

	count = strncpy_from_user(buff, buffer,
			count < sizeof(buff) ? count : sizeof(buff) - 1);
	buff[count] = 0;

	ret = strict_strtol(buff, 0, &lval);
	if (ret < 0)
		return ret;

	*val = lval;
	return count;
}

static ssize_t vsense_proc_int_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *offs)
{
	int *value = ((struct seq_file *)file->private_data)->private;
	int val;
	int ret;

	ret = get_write_value(buffer, count, &val);
	if (ret < 0)
		return ret;

	*value = val;
	return count;
}

static int vsense_proc_mult_read(struct seq_file *m, void *p)
{
	int *multiplier = m->private;
	int val = *multiplier * 100 / 256;
	return vsense_proc_int_read(m, &val);
}

static ssize_t vsense_proc_mult_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *offs)
{
	int *multiplier = ((struct seq_file *)file->private_data)->private;
	int ret, val, adj;

	ret = get_write_value(buffer, count, &val);
	if (ret < 0)
		return ret;
	if (val == 0)
		return -EINVAL;

	/* round to higher absolute value */
	adj = val < 0 ? -99 : 99;
	*multiplier = (val * 256 + adj) / 100;

	return ret;
}

static int vsense_proc_rate_read(struct seq_file *m, void *p)
{
	int *steps = m->private;
	int val = 1000 / VSENSE_INTERVAL / *steps;
	return vsense_proc_int_read(m, &val);
}

static ssize_t vsense_proc_rate_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *offs)
{
	int *steps = ((struct seq_file *)file->private_data)->private;
	int ret, val;

	ret = get_write_value(buffer, count, &val);
	if (ret < 0)
		return ret;
	if (val < 1)
		return -EINVAL;

	*steps = 1000 / VSENSE_INTERVAL / val;
	if (*steps < 1)
		*steps = 1;
	return ret;
}

static ssize_t vsense_proc_treshold_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *offs)
{
	int *value = ((struct seq_file *)file->private_data)->private;
	int ret, val;

	ret = get_write_value(buffer, count, &val);
	if (ret < 0)
		return ret;
	if (val < 1 || val > 32)
		return -EINVAL;

	*value = val;
	return ret;
}

static ssize_t
vsense_show_reset(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", 0);
}

static ssize_t
vsense_set_reset(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long new_reset;
	struct i2c_client *client;
	struct vsense_drvdata *ddata;
	int ret;

	ret = strict_strtoul(buf, 10, &new_reset);
	if (ret)
		return -EINVAL;

	client = to_i2c_client(dev);
	ddata = i2c_get_clientdata(client);

	return count;
}
static DEVICE_ATTR(reset, S_IRUGO | S_IWUSR,
	vsense_show_reset, vsense_set_reset);

#ifdef CONFIG_OF
static struct vsense_platform_data *
vsense_dt_init(struct i2c_client *client)
{
	// the task is to initialize dynamic pdata from the device tree properties
	struct device_node *np = client->dev.of_node;
	struct vsense_platform_data *pdata;

	pdata = devm_kzalloc(&client->dev,
						 sizeof(struct vsense_platform_data), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->gpio_irq = of_get_gpio(np, 0);
	pdata->gpio_reset = of_get_gpio(np, 1);	// not used

	return pdata;
}

static struct of_device_id as5013_dt_match[] = {
	{
	.compatible = "ams,as5013",
	},
	{},
};

MODULE_DEVICE_TABLE(of, as5013_dt_match);

#else
static struct vsense_platform_data *
vsense_dt_init(struct i2c_client *client)
{
	return ERR_PTR(-ENODEV);
}

#endif

#define VSENSE_PE(name, readf, writef) \
static int proc_open_##name(struct inode *inode, struct file *file) \
{ \
	return single_open(file, readf, PDE_DATA(inode)); \
} \
static const struct file_operations vsense_proc_##name = { \
	.open		= proc_open_##name, \
	.read		= seq_read, \
	.llseek		= seq_lseek, \
	.release	= seq_release, \
	.write		= writef, \
}

VSENSE_PE(mode, vsense_proc_mode_read, vsense_proc_mode_write);
VSENSE_PE(mouse_sensitivity, vsense_proc_mult_read, vsense_proc_mult_write);
VSENSE_PE(scrollx_sensitivity, vsense_proc_mult_read, vsense_proc_mult_write);
VSENSE_PE(scrolly_sensitivity, vsense_proc_mult_read, vsense_proc_mult_write);
VSENSE_PE(scroll_rate, vsense_proc_rate_read, vsense_proc_rate_write);
VSENSE_PE(mbutton_threshold, vsense_proc_int_read, vsense_proc_treshold_write);
VSENSE_PE(mbutton_threshold_y, vsense_proc_int_read, vsense_proc_treshold_write);
VSENSE_PE(mbutton_delay, vsense_proc_int_read, vsense_proc_int_write);

static int vsense_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct vsense_platform_data *pdata = dev_get_platdata(&client->dev);
	struct vsense_drvdata *ddata;
	uint8_t value = 0;
	char buff[32];
	int i, ret;

	if (pdata == NULL) {
		pdata = vsense_dt_init(client);
		if (IS_ERR(pdata)) {
			dev_err(&client->dev, "Needs entries in device tree\n");
			return PTR_ERR(pdata);
		}
	}

	//if (i2c_check_functionality(client->adapter, I2C_FUNC_PROTOCOL_MANGLING) == 0) {
	if (i2c_check_functionality(client->adapter, I2C_FUNC_I2C) == 0) {
		dev_err(&client->dev, "can't talk I2C?\n");
		return -EIO;
	}

	ret = as5013_i2c_read(client, 0x0c, &value);
	if (ret < 0) {
		dev_err(&client->dev, "as5013_i2c_read %d\n", ret);
		return ret;
	}
	dev_info(&client->dev, "ID code: %02x\n", value);

	ret = as5013_i2c_read(client, 0x0d, &value);
	if (ret < 0) {
		dev_err(&client->dev, "as5013_i2c_read %d\n", ret);
		return ret;
	}
	dev_info(&client->dev, "ID version: %02x\n", value);

	ret = as5013_i2c_read(client, 0x0e, &value);
	if (ret < 0) {
		dev_err(&client->dev, "as5013_i2c_read %d\n", ret);
		return ret;
	}
	dev_info(&client->dev, "silicon revision: %02x\n", value);

	for (i = 0; i < 10; i++) {
		ret = as5013_i2c_read(client, 0x0f, &value);
		if (ret < 0) {
			dev_err(&client->dev, "as5013_i2c_read %d\n", ret);
			return ret;
		}
		dev_info(&client->dev, "control: %02x\n", value);
		if ((value & 0xfe) == 0xf0)
			break;
	}

	if ((value & 0xfe) != 0xf0) {
		dev_err(&client->dev, "bad control: %02x\n", value);
		return -ENODEV;
	}

	ddata = kzalloc(sizeof(struct vsense_drvdata), GFP_KERNEL);
	if (ddata == NULL)
		return -ENOMEM;

	idr_preload(GFP_KERNEL);
	mutex_lock(&vsense_mutex);

	ret = idr_alloc(&vsense_proc_id, client, 0, 0, GFP_NOWAIT);
	if (ret >= 0) {
		ddata->proc_id = ret;

		// hack to get something like a mkdir -p ..
		if (idr_find_slowpath(&vsense_proc_id, ddata->proc_id ^ 1) == NULL)
			proc_mkdir("pandora", NULL);
	}

	mutex_unlock(&vsense_mutex);
	idr_preload_end();
	if (ret < 0) {
		dev_err(&client->dev, "failed to alloc idr,"
				" error %d\n", ret);
		goto err_idr;
	}

	if(gpio_is_valid(pdata->gpio_irq)) {
	ret = gpio_request_one(pdata->gpio_irq, GPIOF_IN, client->name);
	if (ret < 0) {
		dev_err(&client->dev, "failed to request GPIO %d,"
			" error %d\n", pdata->gpio_irq, ret);
		goto err_gpio_irq;
	}

	ret = gpio_to_irq(pdata->gpio_irq);
	if (ret < 0) {
		dev_err(&client->dev, "unable to get irq number for GPIO %d, "
			"error %d\n", pdata->gpio_irq, ret);
		goto err_gpio_to_irq;
	}
	client->irq = ret;
	} else
		client->irq = 0;

	snprintf(ddata->dev_name, sizeof(ddata->dev_name),
		 "nub%d", ddata->proc_id);

	INIT_DELAYED_WORK(&ddata->work, vsense_work);
	ddata->mode = VSENSE_MODE_ABS;
	ddata->client = client;
	ddata->irq_gpio = pdata->gpio_irq;
	ddata->mouse_multiplier = 170 * 256 / 100;
	ddata->scrollx_multiplier =
	ddata->scrolly_multiplier = 8 * 256 / 100;
	ddata->scroll_steps = 1000 / VSENSE_INTERVAL / 3;
	ddata->mbutton.threshold_x = 20;
	ddata->mbutton.threshold_y = 26;
	ddata->mbutton.delay = 1;
	i2c_set_clientdata(client, ddata);

#if 0
	ddata->reg = regulator_get(&client->dev, "vcc");
	if (IS_ERR(ddata->reg)) {
		ret = PTR_ERR(ddata->reg);
		dev_err(&client->dev, "unable to get regulator: %d\n", ret);
		goto err_regulator_get;
	}

	ret = regulator_enable(ddata->reg);
	if (ret) {
		dev_err(&client->dev, "unable to enable regulator: %d\n", ret);
		goto err_regulator_enable;
	}
#endif

	ret = vsense_input_register(ddata, ddata->mode);
	if (ret) {
		dev_err(&client->dev, "failed to register input device, "
			"error %d\n", ret);
		goto err_input_register;
	}

	if(client->irq) {
	ret = request_threaded_irq(client->irq, NULL,
				     as5013_axis_interrupt,
				     IRQF_ONESHOT |
				     //IRQF_TRIGGER_RISING |
				     IRQF_TRIGGER_FALLING,
				     "as5013_joystick", ddata);
	if (ret) {
		dev_err(&client->dev, "unable to claim irq %d, error %d\n",
			client->irq, ret);
		goto err_request_irq;
	}

	}

	dev_dbg(&client->dev, "probe %02x, gpio %i, irq %i, \"%s\"\n",
		client->addr, pdata->gpio_irq, client->irq, client->name);

	snprintf(buff, sizeof(buff), "pandora/nub%d", ddata->proc_id);
	ddata->proc_root = proc_mkdir(buff, NULL);
	if (ddata->proc_root != NULL) {
		proc_create_data("mode", S_IWUGO | S_IRUGO,
			ddata->proc_root, &vsense_proc_mode,
			ddata);
		proc_create_data("mouse_sensitivity", S_IWUGO | S_IRUGO,
			ddata->proc_root, &vsense_proc_mouse_sensitivity,
			&ddata->mouse_multiplier);
		proc_create_data("scrollx_sensitivity", S_IWUGO | S_IRUGO, ddata->proc_root,
			&vsense_proc_scrollx_sensitivity,
			&ddata->scrollx_multiplier);
		proc_create_data("scrolly_sensitivity", S_IWUGO | S_IRUGO, ddata->proc_root,
			&vsense_proc_scrolly_sensitivity,
			&ddata->scrolly_multiplier);
		proc_create_data("scroll_rate", S_IWUGO | S_IRUGO, ddata->proc_root,
			&vsense_proc_scroll_rate,
			&ddata->scroll_steps);
		proc_create_data("mbutton_threshold", S_IWUGO | S_IRUGO, ddata->proc_root,
			&vsense_proc_mbutton_threshold,
			&ddata->mbutton.threshold_x);
		proc_create_data("mbutton_threshold_y", S_IWUGO | S_IRUGO, ddata->proc_root,
			&vsense_proc_mbutton_threshold_y,
			&ddata->mbutton.threshold_y);
		proc_create_data("mbutton_delay", S_IWUGO | S_IRUGO, ddata->proc_root,
			&vsense_proc_mbutton_delay,
			&ddata->mbutton.delay);
	} else
		dev_err(&client->dev, "can't create proc dir");

	ret = device_create_file(&client->dev, &dev_attr_reset);

	//schedule_delayed_work(&ddata->work, msecs_to_jiffies(VSENSE_INTERVAL));

	return 0;

err_request_irq:
	vsense_input_unregister(ddata);
err_input_register:
//err_regulator_enable:
//	regulator_put(ddata->reg);
//err_regulator_get:
err_gpio_to_irq:
	gpio_free(pdata->gpio_irq);
err_gpio_irq:
//	gpio_free(pdata->gpio_reset);
	idr_remove(&vsense_proc_id, ddata->proc_id);
err_idr:
	kfree(ddata);
	return ret;
}

static int vsense_remove(struct i2c_client *client)
{
	struct vsense_drvdata *ddata;
	char buff[32];

	dev_dbg(&client->dev, "remove\n");

	ddata = i2c_get_clientdata(client);

	mutex_lock(&vsense_mutex);

	mutex_unlock(&vsense_mutex);

	device_remove_file(&client->dev, &dev_attr_reset);

	remove_proc_entry("mode", ddata->proc_root);
	remove_proc_entry("mouse_sensitivity", ddata->proc_root);
	remove_proc_entry("scrollx_sensitivity", ddata->proc_root);
	remove_proc_entry("scrolly_sensitivity", ddata->proc_root);
	remove_proc_entry("scroll_rate", ddata->proc_root);
	remove_proc_entry("mbutton_threshold", ddata->proc_root);
	remove_proc_entry("mbutton_threshold_y", ddata->proc_root);
	remove_proc_entry("mbutton_delay", ddata->proc_root);
	snprintf(buff, sizeof(buff), "pandora/nub%d", ddata->proc_id);
	remove_proc_entry(buff, NULL);

	mutex_lock(&vsense_mutex);
	idr_remove(&vsense_proc_id, ddata->proc_id);

	// hack..
	if (idr_find_slowpath(&vsense_proc_id, ddata->proc_id ^ 1) == NULL)
		remove_proc_entry("pandora", NULL);

	mutex_unlock(&vsense_mutex);

	free_irq(client->irq, ddata);
	vsense_input_unregister(ddata);
	gpio_free(ddata->irq_gpio);
	//regulator_put(ddata->reg);
	kfree(ddata);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int vsense_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct vsense_drvdata *ddata = i2c_get_clientdata(client);

	/* we can't process irqs while i2c is suspended and we can't
	 * ask the device to not generate them, so just disable instead */
	cancel_delayed_work_sync(&ddata->work);
	disable_irq(client->irq);

	return 0;
}

static int vsense_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	enable_irq(client->irq);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(vsense_pm_ops, vsense_i2c_suspend, vsense_i2c_resume);

static const struct i2c_device_id vsense_id[] = {
	{ "as5013", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, vsense_id);

static struct i2c_driver vsense_driver = {
	.driver = {
		.name	= "as5013",
		.owner	= THIS_MODULE,
		.pm	= &vsense_pm_ops,
		.of_match_table = of_match_ptr(as5013_dt_match),
	},
	.probe		= vsense_probe,
	.remove		= vsense_remove,
	.id_table	= vsense_id,
};

module_i2c_driver(vsense_driver);

MODULE_AUTHOR("Grazvydas Ignotas");
MODULE_DESCRIPTION("VSense navigation device driver");
MODULE_LICENSE("GPL");
