/*
	pandora_nub.c

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
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_gpio.h>


#define PANDORA_NUB_INTERVAL		25

#define PANDORA_NUB_MODE_ABS		0
#define PANDORA_NUB_MODE_MOUSE	1
#define PANDORA_NUB_MODE_SCROLL	2
#define PANDORA_NUB_MODE_MBUTTONS	3

static DEFINE_IDR(pandora_nub_proc_id);
static DEFINE_MUTEX(pandora_nub_mutex);

/* Reset state is shared between both nubs, so we keep
 * track of it here.
 */
static int pandora_nub_reset_state;
static int pandora_nub_reset_refcount;

struct pandora_nub_platform_data {
	int gpio_irq;
	int gpio_reset;
};

struct pandora_nub_drvdata {
	char dev_name[12];
	struct input_dev *input;
	struct i2c_client *client;
	struct regulator *reg;
	struct delayed_work work;
	int reset_gpio;
	int irq_gpio;
	int mode;
	int proc_id;
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

enum nub_position {
	NUB_POS_CENTER = 0,
	NUB_POS_UP,
	NUB_POS_RIGHT,
	NUB_POS_DOWN,
	NUB_POS_LEFT,
};

static void release_mbuttons(struct pandora_nub_drvdata *ddata)
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

static void pandora_nub_work(struct work_struct *work)
{
	struct pandora_nub_drvdata *ddata;
	int ax = 0, ay = 0, rx = 0, ry = 0;
	int update_pending = 0;
	signed char buff[4];
	int ret, pos, l, m, r;

	ddata = container_of(work, struct pandora_nub_drvdata, work.work);

	if (unlikely(gpio_get_value(ddata->irq_gpio)))
		goto dosync;

	ret = i2c_master_recv(ddata->client, buff, sizeof(buff));
	if (unlikely(ret != sizeof(buff))) {
		dev_err(&ddata->client->dev, "read failed with %i\n", ret);
		goto dosync;
	}

	rx = (signed int)buff[0];
	ry = (signed int)buff[1];
	ax = (signed int)buff[2];
	ay = (signed int)buff[3];

	schedule_delayed_work(&ddata->work, msecs_to_jiffies(PANDORA_NUB_INTERVAL));
	update_pending = 1;

dosync:
	switch (ddata->mode) {
	case PANDORA_NUB_MODE_MOUSE:
		rx = rx * ddata->mouse_multiplier / 256;
		ry = -ry * ddata->mouse_multiplier / 256;
		input_report_rel(ddata->input, REL_X, rx);
		input_report_rel(ddata->input, REL_Y, ry);
		break;
	case PANDORA_NUB_MODE_SCROLL:
		if (++(ddata->scroll_counter) < ddata->scroll_steps)
			return;
		ddata->scroll_counter = 0;
		ax = ax * ddata->scrollx_multiplier / 256;
		ay = ay * ddata->scrolly_multiplier / 256;
		input_report_rel(ddata->input, REL_HWHEEL, ax);
		input_report_rel(ddata->input, REL_WHEEL, ay);
		break;
	case PANDORA_NUB_MODE_MBUTTONS:
		if (!update_pending) {
			release_mbuttons(ddata);
			break;
		}

		pos = NUB_POS_CENTER;
		if      (ax >= ddata->mbutton.threshold_x) pos = NUB_POS_RIGHT;
		else if (ax <= -ddata->mbutton.threshold_x) pos = NUB_POS_LEFT;
		else if (ay >= ddata->mbutton.threshold_y) pos = NUB_POS_UP;
		else if (ay <= -ddata->mbutton.threshold_y) pos = NUB_POS_DOWN;

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
		input_report_abs(ddata->input, ABS_X, ax * 8);
		input_report_abs(ddata->input, ABS_Y, -ay * 8);
		break;
	}
	input_sync(ddata->input);
}

static irqreturn_t pandora_nub_interrupt(int irq, void *dev_id)
{
	struct pandora_nub_drvdata *ddata = dev_id;

	schedule_delayed_work(&ddata->work, 0);

	return IRQ_HANDLED;
}

static int pandora_nub_reset(struct pandora_nub_drvdata *ddata, int val)
{
	int ret;

	dev_dbg(&ddata->client->dev, "pandora_nub_reset: %i\n", val);

	if (ddata->mode != PANDORA_NUB_MODE_ABS)
		release_mbuttons(ddata);

	ret = gpio_direction_output(ddata->reset_gpio, val);
	if (ret < 0) {
		dev_err(&ddata->client->dev, "failed to configure direction "
			"for GPIO %d, error %d\n", ddata->reset_gpio, ret);
	}
	else {
		pandora_nub_reset_state = val;
	}

	return ret;
}

static int pandora_nub_open(struct input_dev *dev)
{
	dev_dbg(&dev->dev, "pandora_nub_open\n");

	/* get out of reset and stay there until user wants to reset it */
	if (pandora_nub_reset_state != 0)
		pandora_nub_reset(input_get_drvdata(dev), 0);

	return 0;
}

static int pandora_nub_input_register(struct pandora_nub_drvdata *ddata, int mode)
{
	struct input_dev *input;
	int ret;

	input = input_allocate_device();
	if (input == NULL)
		return -ENOMEM;

	if (mode != PANDORA_NUB_MODE_ABS) {
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

	input->open = pandora_nub_open;

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

static void pandora_nub_input_unregister(struct pandora_nub_drvdata *ddata)
{
	cancel_delayed_work_sync(&ddata->work);
	input_unregister_device(ddata->input);
}

static int pandora_nub_proc_mode_read(struct seq_file *m, void *p)
{
	struct pandora_nub_drvdata *ddata = m->private;

	switch (ddata->mode) {
	case PANDORA_NUB_MODE_MOUSE:
		seq_printf(m, "mouse\n");
		break;
	case PANDORA_NUB_MODE_SCROLL:
		seq_printf(m, "scroll\n");
		break;
	case PANDORA_NUB_MODE_MBUTTONS:
		seq_printf(m, "mbuttons\n");
		break;
	default:
		seq_printf(m, "absolute\n");
		break;
	}

	return 0;
}

static int pandora_nub_proc_mode_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *offs)
{
	struct pandora_nub_drvdata *ddata =
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
		mode = PANDORA_NUB_MODE_MOUSE;
	else if (strcasecmp(buff, "scroll") == 0)
		mode = PANDORA_NUB_MODE_SCROLL;
	else if (strcasecmp(buff, "mbuttons") == 0)
		mode = PANDORA_NUB_MODE_MBUTTONS;
	else if (strcasecmp(buff, "absolute") == 0)
		mode = PANDORA_NUB_MODE_ABS;
	else {
		dev_err(&ddata->client->dev, "unknown mode: %s\n", buff);
		return -EINVAL;
	}

	if (ddata->mode != PANDORA_NUB_MODE_ABS)
		release_mbuttons(ddata);

	if ((mode == PANDORA_NUB_MODE_ABS && ddata->mode != PANDORA_NUB_MODE_ABS) ||
	    (mode != PANDORA_NUB_MODE_ABS && ddata->mode == PANDORA_NUB_MODE_ABS)) {
		disable_irq(ddata->client->irq);
		pandora_nub_input_unregister(ddata);
		ret = pandora_nub_input_register(ddata, mode);
		if (ret)
			dev_err(&ddata->client->dev, "failed to re-register "
				"input as %d, code %d\n", mode, ret);
		else
			enable_irq(ddata->client->irq);
	}
	ddata->mode = mode;

	return count;
}

static int pandora_nub_proc_int_read(struct seq_file *m, void *p)
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

	ret = kstrtol(buff, 0, &lval);
	if (ret < 0)
		return ret;

	*val = lval;
	return count;
}

static int pandora_nub_proc_int_write(struct file *file, const char __user *buffer,
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

static int pandora_nub_proc_mult_read(struct seq_file *m, void *p)
{
	int *multiplier = m->private;
	int val = *multiplier * 100 / 256;
	return pandora_nub_proc_int_read(m, &val);
}

static int pandora_nub_proc_mult_write(struct file *file, const char __user *buffer,
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

static int pandora_nub_proc_rate_read(struct seq_file *m, void *p)
{
	int *steps = m->private;
	int val = 1000 / PANDORA_NUB_INTERVAL / *steps;
	return pandora_nub_proc_int_read(m, &val);
}

static int pandora_nub_proc_rate_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *offs)
{
	int *steps = ((struct seq_file *)file->private_data)->private;
	int ret, val;

	ret = get_write_value(buffer, count, &val);
	if (ret < 0)
		return ret;
	if (val < 1)
		return -EINVAL;

	*steps = 1000 / PANDORA_NUB_INTERVAL / val;
	if (*steps < 1)
		*steps = 1;
	return ret;
}

static int pandora_nub_proc_treshold_write(struct file *file, const char __user *buffer,
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
pandora_nub_show_reset(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", pandora_nub_reset_state);
}

static ssize_t
pandora_nub_set_reset(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long new_reset;
	struct i2c_client *client;
	struct pandora_nub_drvdata *ddata;
	int ret;

	ret = kstrtol(buf, 10, &new_reset);
	if (ret)
		return -EINVAL;

	client = to_i2c_client(dev);
	ddata = i2c_get_clientdata(client);

	pandora_nub_reset(ddata, new_reset ? 1 : 0);

	return count;
}
static DEVICE_ATTR(reset, S_IRUGO | S_IWUSR,
	pandora_nub_show_reset, pandora_nub_set_reset);

#ifdef CONFIG_OF
static struct pandora_nub_platform_data *
pandora_nub_dt_init(struct i2c_client *client)
{
	// the task is to initialize dynamic pdata from the device tree properties
	struct device_node *np = client->dev.of_node;
	struct pandora_nub_platform_data *pdata;

	pdata = devm_kzalloc(&client->dev,
		 sizeof(struct pandora_nub_platform_data), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->gpio_irq = of_get_gpio(np, 0);
	pdata->gpio_reset = of_get_gpio(np, 1);	// not used

	return pdata;
}

static struct of_device_id pandora_nub_dt_match[] = {
	{
	.compatible = "pandora,pandora-nub",
	},
	{},
};

MODULE_DEVICE_TABLE(of, pandora_nub_dt_match);

#else
static struct pandora_nub_platform_data *
pandora_nub_dt_init(struct i2c_client *client)
{
	return ERR_PTR(-ENODEV);
}

#endif

#define PANDORA_NUB_PE(name, readf, writef) \
static int proc_open_##name(struct inode *inode, struct file *file) \
{ \
	return single_open(file, readf, PDE_DATA(inode)); \
} \
static const struct file_operations pandora_nub_proc_##name = { \
	.open		= proc_open_##name, \
	.read		= seq_read, \
	.llseek		= seq_lseek, \
	.release	= seq_release, \
	.write		= writef, \
}

PANDORA_NUB_PE(mode, pandora_nub_proc_mode_read, pandora_nub_proc_mode_write);
PANDORA_NUB_PE(mouse_sensitivity, pandora_nub_proc_mult_read, pandora_nub_proc_mult_write);
PANDORA_NUB_PE(scrollx_sensitivity, pandora_nub_proc_mult_read, pandora_nub_proc_mult_write);
PANDORA_NUB_PE(scrolly_sensitivity, pandora_nub_proc_mult_read, pandora_nub_proc_mult_write);
PANDORA_NUB_PE(scroll_rate, pandora_nub_proc_rate_read, pandora_nub_proc_rate_write);
PANDORA_NUB_PE(mbutton_threshold, pandora_nub_proc_int_read, pandora_nub_proc_treshold_write);
PANDORA_NUB_PE(mbutton_threshold_y, pandora_nub_proc_int_read, pandora_nub_proc_treshold_write);
PANDORA_NUB_PE(mbutton_delay, pandora_nub_proc_int_read, pandora_nub_proc_int_write);

static int pandora_nub_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct pandora_nub_platform_data *pdata = dev_get_platdata(&client->dev);
	struct pandora_nub_drvdata *ddata;
	char buff[32];
	int ret;

	if (pdata == NULL) {
		pdata = pandora_nub_dt_init(client);
		if (IS_ERR(pdata)) {
			dev_err(&client->dev, "Needs entries in device tree\n");
			return PTR_ERR(pdata);
		}
	}

	if (i2c_check_functionality(client->adapter, I2C_FUNC_I2C) == 0) {
		dev_err(&client->dev, "can't talk I2C?\n");
		return -EIO;
	}

	ddata = kzalloc(sizeof(struct pandora_nub_drvdata), GFP_KERNEL);
	if (ddata == NULL)
		return -ENOMEM;

	idr_preload(GFP_KERNEL);
	mutex_lock(&pandora_nub_mutex);

	ret = idr_alloc(&pandora_nub_proc_id, client, 0, 0, GFP_NOWAIT);
	if (ret >= 0) {
		ddata->proc_id = ret;

		/* hack to get something like a mkdir -p .. */
		if (idr_find(&pandora_nub_proc_id, ddata->proc_id ^ 1) == NULL)
			proc_mkdir("pandora", NULL);
	}

	mutex_unlock(&pandora_nub_mutex);
	idr_preload_end();
	if (ret < 0) {
		dev_err(&client->dev, "failed to alloc idr,"
				" error %d\n", ret);
		goto err_idr;
	}

	mutex_lock(&pandora_nub_mutex);
	if (!pandora_nub_reset_refcount) {
		ret = gpio_request_one(pdata->gpio_reset, GPIOF_OUT_INIT_HIGH,
			"pandora_nub reset");
		if (ret < 0) {
			dev_err(&client->dev, "gpio_request error: %d, %d\n",
				pdata->gpio_reset, ret);
			mutex_unlock(&pandora_nub_mutex);
			goto err_gpio_reset;
		}
	}
	pandora_nub_reset_refcount++;

	mutex_unlock(&pandora_nub_mutex);

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

	snprintf(ddata->dev_name, sizeof(ddata->dev_name),
		 "nub%d", ddata->proc_id);

	INIT_DELAYED_WORK(&ddata->work, pandora_nub_work);
	ddata->mode = PANDORA_NUB_MODE_ABS;
	ddata->client = client;
	ddata->reset_gpio = pdata->gpio_reset;
	ddata->irq_gpio = pdata->gpio_irq;
	ddata->mouse_multiplier = 170 * 256 / 100;
	ddata->scrollx_multiplier =
	ddata->scrolly_multiplier = 8 * 256 / 100;
	ddata->scroll_steps = 1000 / PANDORA_NUB_INTERVAL / 3;
	ddata->mbutton.threshold_x = 20;
	ddata->mbutton.threshold_y = 26;
	ddata->mbutton.delay = 1;
	i2c_set_clientdata(client, ddata);

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

	/* HACK */
	if (pandora_nub_reset_refcount == 2)
		/* resetting drains power, as well as disabling supply,
		 * so keep it powered and out of reset at all times */
		pandora_nub_reset(ddata, 0);

	ret = pandora_nub_input_register(ddata, ddata->mode);
	if (ret) {
		dev_err(&client->dev, "failed to register input device, "
			"error %d\n", ret);
		goto err_input_register;
	}

	if(client->irq) {
	ret = request_threaded_irq(client->irq, NULL,
				     pandora_nub_interrupt,
				     IRQF_ONESHOT |
				     IRQF_TRIGGER_FALLING,
				     "pandora_joystick", ddata);
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
			ddata->proc_root, &pandora_nub_proc_mode,
			ddata);
		proc_create_data("mouse_sensitivity", S_IWUGO | S_IRUGO,
			ddata->proc_root, &pandora_nub_proc_mouse_sensitivity,
			&ddata->mouse_multiplier);
		proc_create_data("scrollx_sensitivity", S_IWUGO | S_IRUGO, ddata->proc_root,
			&pandora_nub_proc_scrollx_sensitivity,
			&ddata->scrollx_multiplier);
		proc_create_data("scrolly_sensitivity", S_IWUGO | S_IRUGO, ddata->proc_root,
			&pandora_nub_proc_scrolly_sensitivity,
			&ddata->scrolly_multiplier);
		proc_create_data("scroll_rate", S_IWUGO | S_IRUGO, ddata->proc_root,
			&pandora_nub_proc_scroll_rate,
			&ddata->scroll_steps);
		proc_create_data("mbutton_threshold", S_IWUGO | S_IRUGO, ddata->proc_root,
			&pandora_nub_proc_mbutton_threshold,
			&ddata->mbutton.threshold_x);
		proc_create_data("mbutton_threshold_y", S_IWUGO | S_IRUGO, ddata->proc_root,
			&pandora_nub_proc_mbutton_threshold_y,
			&ddata->mbutton.threshold_y);
		proc_create_data("mbutton_delay", S_IWUGO | S_IRUGO, ddata->proc_root,
			&pandora_nub_proc_mbutton_delay,
			&ddata->mbutton.delay);
	} else
		dev_err(&client->dev, "can't create proc dir");

	ret = device_create_file(&client->dev, &dev_attr_reset);

	return 0;

err_request_irq:
	pandora_nub_input_unregister(ddata);
err_input_register:
err_regulator_enable:
	regulator_put(ddata->reg);
err_regulator_get:
err_gpio_to_irq:
	gpio_free(pdata->gpio_irq);
err_gpio_irq:
	gpio_free(pdata->gpio_reset);
err_gpio_reset:
	idr_remove(&pandora_nub_proc_id, ddata->proc_id);
err_idr:
	kfree(ddata);
	return ret;
}

static int pandora_nub_remove(struct i2c_client *client)
{
	struct pandora_nub_drvdata *ddata;
	char buff[32];

	dev_dbg(&client->dev, "remove\n");

	ddata = i2c_get_clientdata(client);

	mutex_lock(&pandora_nub_mutex);

	pandora_nub_reset_refcount--;
	if (!pandora_nub_reset_refcount)
		gpio_free(ddata->reset_gpio);

	mutex_unlock(&pandora_nub_mutex);

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
	idr_remove(&pandora_nub_proc_id, ddata->proc_id);

	free_irq(client->irq, ddata);
	pandora_nub_input_unregister(ddata);
	gpio_free(ddata->irq_gpio);
	regulator_put(ddata->reg);
	kfree(ddata);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pandora_nub_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pandora_nub_drvdata *ddata = i2c_get_clientdata(client);

	/* we can't process irqs while i2c is suspended and we can't
	 * ask the device to not generate them, so just disable instead */
	cancel_delayed_work_sync(&ddata->work);
	disable_irq(client->irq);

	return 0;
}

static int pandora_nub_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	enable_irq(client->irq);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(pandora_nub_pm_ops, pandora_nub_i2c_suspend, pandora_nub_i2c_resume);

static const struct i2c_device_id pandora_nub_id[] = {
	{ "pandora_nub", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, pandora_nub_id);

static struct i2c_driver pandora_nub_driver = {
	.driver = {
		.name	= "pandora_nub",
		.owner	= THIS_MODULE,
		.pm	= &pandora_nub_pm_ops,
		.of_match_table = of_match_ptr(pandora_nub_dt_match),
	},
	.probe		= pandora_nub_probe,
	.remove		= pandora_nub_remove,
	.id_table	= pandora_nub_id,
};

module_i2c_driver(pandora_nub_driver);

MODULE_AUTHOR("Grazvydas Ignotas");
MODULE_DESCRIPTION("VSense navigation device driver");
MODULE_LICENSE("GPL");
