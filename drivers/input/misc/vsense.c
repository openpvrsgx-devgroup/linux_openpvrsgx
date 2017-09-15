/*
	vsense.c

	Written by Gražvydas Ignotas <notasas@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.
*/

#include <linux/kernel.h>
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

#define VSENSE_INTERVAL		25

#define VSENSE_MODE_ABS		0
#define VSENSE_MODE_MOUSE	1
#define VSENSE_MODE_SCROLL	2
#define VSENSE_MODE_MBUTTONS	3

static DEFINE_IDR(vsense_proc_id);
static DEFINE_MUTEX(vsense_mutex);

/* Reset state is shared between both nubs, so we keep
 * track of it here.
 */
static int vsense_reset_state;
static int vsense_reset_refcount;

struct vsense_drvdata {
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

static void vsense_work(struct work_struct *work)
{
	struct vsense_drvdata *ddata;
	int ax = 0, ay = 0, rx = 0, ry = 0;
	int update_pending = 0;
	signed char buff[4];
	int ret, pos, l, m, r;

	ddata = container_of(work, struct vsense_drvdata, work.work);

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

	schedule_delayed_work(&ddata->work, msecs_to_jiffies(VSENSE_INTERVAL));
	update_pending = 1;

dosync:
	switch (ddata->mode) {
	case VSENSE_MODE_MOUSE:
		rx = rx * ddata->mouse_multiplier / 256;
		ry = -ry * ddata->mouse_multiplier / 256;
		input_report_rel(ddata->input, REL_X, rx);
		input_report_rel(ddata->input, REL_Y, ry);
		break;
	case VSENSE_MODE_SCROLL:
		if (++(ddata->scroll_counter) < ddata->scroll_steps)
			return;
		ddata->scroll_counter = 0;
		ax = ax * ddata->scrollx_multiplier / 256;
		ay = ay * ddata->scrolly_multiplier / 256;
		input_report_rel(ddata->input, REL_HWHEEL, ax);
		input_report_rel(ddata->input, REL_WHEEL, ay);
		break;
	case VSENSE_MODE_MBUTTONS:
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

static irqreturn_t vsense_isr(int irq, void *dev_id)
{
	struct vsense_drvdata *ddata = dev_id;

	schedule_delayed_work(&ddata->work, 0);

	return IRQ_HANDLED;
}

static int vsense_reset(struct vsense_drvdata *ddata, int val)
{
	int ret;

	dev_dbg(&ddata->client->dev, "vsense_reset: %i\n", val);

	if (ddata->mode != VSENSE_MODE_ABS)
		release_mbuttons(ddata);

	ret = gpio_direction_output(ddata->reset_gpio, val);
	if (ret < 0) {
		dev_err(&ddata->client->dev, "failed to configure direction "
			"for GPIO %d, error %d\n", ddata->reset_gpio, ret);
	}
	else {
		vsense_reset_state = val;
	}

	return ret;
}

static int vsense_open(struct input_dev *dev)
{
	dev_dbg(&dev->dev, "vsense_open\n");

	/* get out of reset and stay there until user wants to reset it */
	if (vsense_reset_state != 0)
		vsense_reset(input_get_drvdata(dev), 0);

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

static int vsense_proc_mode_read(char *page, char **start, off_t off, int count,
		int *eof, void *data)
{
	struct vsense_drvdata *ddata = data;
	char *p = page;
	int len;

	switch (ddata->mode) {
	case VSENSE_MODE_MOUSE:
		len = sprintf(p, "mouse\n");
		break;
	case VSENSE_MODE_SCROLL:
		len = sprintf(p, "scroll\n");
		break;
	case VSENSE_MODE_MBUTTONS:
		len = sprintf(p, "mbuttons\n");
		break;
	default:
		len = sprintf(p, "absolute\n");
		break;
	}

	*eof = 1;
	return len;
}

static int vsense_proc_mode_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	struct vsense_drvdata *ddata = data;
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

static int vsense_proc_int_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int *val = data;
	int len;

	len = sprintf(page, "%d\n", *val);
	*eof = 1;
	return len;
}

static int vsense_proc_int_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	char buff[32];
	long val;
	int ret;
	int *value = data;

	count = strncpy_from_user(buff, buffer,
			count < sizeof(buff) ? count : sizeof(buff) - 1);
	buff[count] = 0;

	ret = strict_strtol(buff, 0, &val);
	if (ret < 0)
		return ret;
	*value = val;
	return count;
}

static int vsense_proc_mult_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int *multiplier = data;
	int val = *multiplier * 100 / 256;
	return vsense_proc_int_read(page, start, off, count, eof, &val);
}

static int vsense_proc_mult_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int *multiplier = data;
	int ret, val, adj;

	ret = vsense_proc_int_write(file, buffer, count, &val);
	if (ret < 0)
		return ret;
	if (val == 0)
		return -EINVAL;

	/* round to higher absolute value */
	adj = val < 0 ? -99 : 99;
	*multiplier = (val * 256 + adj) / 100;

	return ret;
}

static int vsense_proc_rate_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int *steps = data;
	int val = 1000 / VSENSE_INTERVAL / *steps;
	return vsense_proc_int_read(page, start, off, count, eof, &val);
}

static int vsense_proc_rate_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int *steps = data;
	int ret, val;

	ret = vsense_proc_int_write(file, buffer, count, &val);
	if (ret < 0)
		return ret;
	if (val < 1)
		return -EINVAL;

	*steps = 1000 / VSENSE_INTERVAL / val;
	if (*steps < 1)
		*steps = 1;
	return ret;
}

static int vsense_proc_treshold_write(struct file *file, const char __user *buffer,
		unsigned long count, void *data)
{
	int *value = data;
	int ret, val;

	ret = vsense_proc_int_write(file, buffer, count, &val);
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
	return sprintf(buf, "%d\n", vsense_reset_state);
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

	vsense_reset(ddata, new_reset ? 1 : 0);

	return count;
}
static DEVICE_ATTR(reset, S_IRUGO | S_IWUSR,
	vsense_show_reset, vsense_set_reset);

static void vsense_create_proc(struct vsense_drvdata *ddata,
			       void *pdata, const char *name,
			       read_proc_t *read_proc, write_proc_t *write_proc)
{
	struct proc_dir_entry *pret;

	pret = create_proc_entry(name, S_IWUGO | S_IRUGO, ddata->proc_root);
	if (pret == NULL) {
		dev_err(&ddata->client->dev, "failed to create proc file %s\n", name);
		return;
	}

	pret->data = pdata;
	pret->read_proc = read_proc;
	pret->write_proc = write_proc;
}

static int vsense_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct vsense_platform_data *pdata = client->dev.platform_data;
	struct vsense_drvdata *ddata;
	char buff[32];
	int ret;

	if (pdata == NULL) {
		dev_err(&client->dev, "no platform data?\n");
		return -EINVAL;
	}

	if (i2c_check_functionality(client->adapter, I2C_FUNC_I2C) == 0) {
		dev_err(&client->dev, "can't talk I2C?\n");
		return -EIO;
	}

	ddata = kzalloc(sizeof(struct vsense_drvdata), GFP_KERNEL);
	if (ddata == NULL)
		return -ENOMEM;

	ret = idr_pre_get(&vsense_proc_id, GFP_KERNEL);
	if (ret == 0) {
		ret = -ENOMEM;
		goto err_idr;
	}

	mutex_lock(&vsense_mutex);

	ret = idr_get_new(&vsense_proc_id, client, &ddata->proc_id);
	if (ret < 0) {
		mutex_unlock(&vsense_mutex);
		goto err_idr;
	}

	if (!vsense_reset_refcount) {
		ret = gpio_request_one(pdata->gpio_reset, GPIOF_OUT_INIT_HIGH,
			"vsense reset");
		if (ret < 0) {
			dev_err(&client->dev, "gpio_request error: %d, %d\n",
				pdata->gpio_reset, ret);
			mutex_unlock(&vsense_mutex);
			goto err_gpio_reset;
		}
	}
	vsense_reset_refcount++;

	mutex_unlock(&vsense_mutex);

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

	INIT_DELAYED_WORK(&ddata->work, vsense_work);
	ddata->mode = VSENSE_MODE_ABS;
	ddata->client = client;
	ddata->reset_gpio = pdata->gpio_reset;
	ddata->irq_gpio = pdata->gpio_irq;
	ddata->mouse_multiplier = 170 * 256 / 100;
	ddata->scrollx_multiplier =
	ddata->scrolly_multiplier = 8 * 256 / 100;
	ddata->scroll_steps = 1000 / VSENSE_INTERVAL / 3;
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
	if (vsense_reset_refcount == 2)
		/* resetting drains power, as well as disabling supply,
		 * so keep it powered and out of reset at all times */
		vsense_reset(ddata, 0);

	ret = vsense_input_register(ddata, ddata->mode);
	if (ret) {
		dev_err(&client->dev, "failed to register input device, "
			"error %d\n", ret);
		goto err_input_register;
	}

	ret = request_irq(client->irq, vsense_isr,
			IRQF_SAMPLE_RANDOM | IRQF_TRIGGER_FALLING,
			client->name, ddata);
	if (ret) {
		dev_err(&client->dev, "unable to claim irq %d, error %d\n",
			client->irq, ret);
		goto err_request_irq;
	}

	dev_dbg(&client->dev, "probe %02x, gpio %i, irq %i, \"%s\"\n",
		client->addr, pdata->gpio_irq, client->irq, client->name);

	snprintf(buff, sizeof(buff), "pandora/nub%d", ddata->proc_id);
	ddata->proc_root = proc_mkdir(buff, NULL);
	if (ddata->proc_root != NULL) {
		vsense_create_proc(ddata, ddata, "mode",
				vsense_proc_mode_read, vsense_proc_mode_write);
		vsense_create_proc(ddata, &ddata->mouse_multiplier, "mouse_sensitivity",
				vsense_proc_mult_read, vsense_proc_mult_write);
		vsense_create_proc(ddata, &ddata->scrollx_multiplier, "scrollx_sensitivity",
				vsense_proc_mult_read, vsense_proc_mult_write);
		vsense_create_proc(ddata, &ddata->scrolly_multiplier, "scrolly_sensitivity",
				vsense_proc_mult_read, vsense_proc_mult_write);
		vsense_create_proc(ddata, &ddata->scroll_steps, "scroll_rate",
				vsense_proc_rate_read, vsense_proc_rate_write);
		vsense_create_proc(ddata, &ddata->mbutton.threshold_x, "mbutton_threshold",
				vsense_proc_int_read, vsense_proc_treshold_write);
		vsense_create_proc(ddata, &ddata->mbutton.threshold_y, "mbutton_threshold_y",
				vsense_proc_int_read, vsense_proc_treshold_write);
		vsense_create_proc(ddata, &ddata->mbutton.delay, "mbutton_delay",
				vsense_proc_int_read, vsense_proc_int_write);
	} else
		dev_err(&client->dev, "can't create proc dir");

	ret = device_create_file(&client->dev, &dev_attr_reset);

	return 0;

err_request_irq:
	vsense_input_unregister(ddata);
err_input_register:
err_regulator_enable:
	regulator_put(ddata->reg);
err_regulator_get:
err_gpio_to_irq:
	gpio_free(pdata->gpio_irq);
err_gpio_irq:
	gpio_free(pdata->gpio_reset);
err_gpio_reset:
	idr_remove(&vsense_proc_id, ddata->proc_id);
err_idr:
	kfree(ddata);
	return ret;
}

static int __devexit vsense_remove(struct i2c_client *client)
{
	struct vsense_drvdata *ddata;
	char buff[32];

	dev_dbg(&client->dev, "remove\n");

	ddata = i2c_get_clientdata(client);

	mutex_lock(&vsense_mutex);

	vsense_reset_refcount--;
	if (!vsense_reset_refcount)
		gpio_free(ddata->reset_gpio);

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
	idr_remove(&vsense_proc_id, ddata->proc_id);

	free_irq(client->irq, ddata);
	vsense_input_unregister(ddata);
	gpio_free(ddata->irq_gpio);
	regulator_put(ddata->reg);
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
	{ "vsense", 0 },
	{ }
};

static struct i2c_driver vsense_driver = {
	.driver = {
		.name	= "vsense",
		.owner	= THIS_MODULE,
		.pm	= &vsense_pm_ops,
	},
	.probe		= vsense_probe,
	.remove		= __devexit_p(vsense_remove),
	.id_table	= vsense_id,
};

static int __init vsense_init(void)
{
	return i2c_add_driver(&vsense_driver);
}

static void __exit vsense_exit(void)
{
	i2c_del_driver(&vsense_driver);
}


MODULE_AUTHOR("Grazvydas Ignotas");
MODULE_DESCRIPTION("VSense navigation device driver");
MODULE_LICENSE("GPL");

module_init(vsense_init);
module_exit(vsense_exit);
