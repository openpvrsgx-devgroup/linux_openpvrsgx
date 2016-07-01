/*
 * Driver for Austria Microsystems joysticks AS5011
 *
 * Original work by Gražvydas Ignotas <notasas@gmail.com>
 * Patched by
 * H. Nikolaus Schaller <hns@goldelico.com>
 * Andrey Utkin <andrey_utkin@fastmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/idr.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#define POLL_INTERVAL 25 /* msecs */

struct as5013_drvdata {
	char dev_name[8];
	struct input_dev *input;
	struct i2c_client *client;
	int irq_gpio;
	/* For polling state without hardware interrupt */
	struct delayed_work work;
};

static int as5013_i2c_write(struct i2c_client *client, uint8_t aregaddr,
			    uint8_t avalue)
{
	uint8_t data[2] = { aregaddr, avalue };
	struct i2c_msg msg = { client->addr, 0, 2, data };
	int ret;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		dev_err(&client->dev, "write 0x%x failed: ret %d\n",
			aregaddr, ret);
		return ret;
	}

	return 0;
}

static int as5013_i2c_read(struct i2c_client *client,
			   uint8_t aregaddr, uint8_t *value)
{
	struct i2c_msg msg_set[2] = {
		{ client->addr, 0, 1, &aregaddr },
		{ client->addr, I2C_M_RD, 1, value }
	};
	int ret;

	ret = i2c_transfer(client->adapter, msg_set, ARRAY_SIZE(msg_set));
	if (ret < 0) {
		dev_err(&client->dev, "read 0x%x failed: ret %d\n",
			aregaddr, ret);
		return ret;
	}

	return 0;
}

static irqreturn_t as5013_axis_isr(int irq, void *dev_id)
{
	struct as5013_drvdata *ddata = dev_id;
	struct i2c_client *client = ddata->client;
	int ax;
	int ay;
	uint8_t value;
	int ret;

	/* TODO Read two regs in a single act? */
	ret = as5013_i2c_read(client, 0x10, &value);
	if (ret < 0)
		return IRQ_HANDLED;
	ax = -(int8_t)value;

	ret = as5013_i2c_read(client, 0x11, &value);
	if (ret < 0)
		return IRQ_HANDLED;
	ay = -(int8_t)value;

	input_report_abs(ddata->input, ABS_X, ax);
	input_report_abs(ddata->input, ABS_Y, ay);
	input_sync(ddata->input);

	return IRQ_HANDLED;
}

/* Repeated device polling for case of no hardware interrupt */
static void as5013_work(struct work_struct *work)
{
	struct as5013_drvdata *ddata;
	struct i2c_client *client;
	uint8_t value;
	int ret;

	ddata = container_of(work, struct as5013_drvdata, work.work);
	client = ddata->client;

	ret = as5013_i2c_read(client, 0x0f, &value);
	if (ret < 0)
		return;

	if (value & 1)
		as5013_axis_isr(0, ddata);

	schedule_delayed_work(&ddata->work, msecs_to_jiffies(POLL_INTERVAL));
}

static int as5013_low_power_enter(struct as5013_drvdata *ddata)
{
	if (ddata->client->irq) {
		/*
		 * Set to Low Power mode
		 * with largest internal wakeup time interval
		 */
		as5013_i2c_write(ddata->client, 0xf, 0x7c);
		disable_irq(ddata->client->irq);
		return 0;
	}

	/* In case of no hardware interrupt, manage polling */
	return !cancel_delayed_work_sync(&ddata->work);
}

static int as5013_low_power_leave(struct as5013_drvdata *ddata)
{
	if (ddata->client->irq) {
		/* Set to default mode */
		as5013_i2c_write(ddata->client, 0xf, 0x80);
		enable_irq(ddata->client->irq);
		return 0;
	}

	/* In case of no hardware interrupt, manage polling */
	return !schedule_delayed_work(&ddata->work,
				      msecs_to_jiffies(POLL_INTERVAL));
}

static int as5013_open(struct input_dev *dev)
{
	struct as5013_drvdata *ddata = input_get_drvdata(dev);

	return as5013_low_power_leave(ddata);
}

static void as5013_close(struct input_dev *dev)
{
	struct as5013_drvdata *ddata = input_get_drvdata(dev);

	as5013_low_power_enter(ddata);
}

static int as5013_input_register(struct as5013_drvdata *ddata)
{
	struct input_dev *input = input_allocate_device();
	int ret;

	if (!input)
		return -ENOMEM;

	set_bit(EV_ABS, input->evbit);
	input_set_abs_params(input, ABS_X, -128, 127, 0, 0);
	input_set_abs_params(input, ABS_Y, -128, 127, 0, 0);

	input->name = ddata->dev_name;
	input->dev.parent = &ddata->client->dev;
	input->id.bustype = BUS_I2C;
	input->open = as5013_open;
	input->close = as5013_close;
	ddata->input = input;
	input_set_drvdata(input, ddata);

	ret = input_register_device(input);
	if (ret)
		input_free_device(input);

	return ret;
}

static void as5013_input_unregister(struct as5013_drvdata *ddata)
{
	cancel_delayed_work_sync(&ddata->work);
	input_unregister_device(ddata->input);
}

#ifdef CONFIG_OF
static int as5013_dt_init(struct as5013_drvdata *ddata)
{
	struct device_node *np = ddata->client->dev.of_node;

	ddata->irq_gpio = of_get_gpio(np, 0);

	return 0;
}

static const struct of_device_id as5013_dt_match[] = {
	{
	.compatible = "ams,as5013",
	},
	{},
};

MODULE_DEVICE_TABLE(of, as5013_dt_match);

#else
static int as5013_dt_init(struct as5013_drvdata *ddata)
{
	return -ENODEV;
}

#endif

static int as5013_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct as5013_drvdata *ddata;
	uint8_t value = 0;
	int ret;
	int i;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "can't talk I2C?\n");
		return -EIO;
	}

	ddata = kzalloc(sizeof(struct as5013_drvdata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	/*
	 * 0x40 and 0x41 are only i2c addresses available in non-custom as5013
	 * samples. So make nicer names for these.
	 */
	if (client->addr == 0x40 || client->addr == 0x41) {
		snprintf(ddata->dev_name, sizeof(ddata->dev_name), "nub%d",
			 client->addr - 0x40);
	} else {
		snprintf(ddata->dev_name, sizeof(ddata->dev_name), "nub0x%02x",
			 client->addr);
	}

	ddata->client = client;
	i2c_set_clientdata(client, ddata);

	ret = as5013_dt_init(ddata);
	if (ret) {
		dev_err(&client->dev, "Needs entries in device tree\n");
		goto free_ddata;
	}

	/* TODO Read 0xc - 0xf regs in a single act? */
	ret = as5013_i2c_read(client, 0x0c, &value);
	if (ret < 0)
		goto free_ddata;
	dev_info(&client->dev, "ID code: %02x\n", value);

	ret = as5013_i2c_read(client, 0x0d, &value);
	if (ret < 0)
		goto free_ddata;
	dev_info(&client->dev, "ID version: %02x\n", value);

	ret = as5013_i2c_read(client, 0x0e, &value);
	if (ret < 0)
		goto free_ddata;
	dev_info(&client->dev, "silicon revision: %02x\n", value);

	/* TODO Sanitize. A single read with delay is needed? */
	for (i = 0; i < 10; i++) {
		ret = as5013_i2c_read(client, 0x0f, &value);
		if (ret < 0)
			goto free_ddata;
		dev_info(&client->dev, "control: %02x\n", value);
		if ((value & 0xfe) == 0xf0)
			break;
	}

	if ((value & 0xfe) != 0xf0) {
		dev_err(&client->dev, "bad control: %02x\n", value);
		ret = -ENODEV;
		goto free_ddata;
	}

	if (gpio_is_valid(ddata->irq_gpio)) {
		ret = gpio_request_one(ddata->irq_gpio, GPIOF_IN, client->name);
		if (ret < 0) {
			dev_err(&client->dev,
				"failed to request GPIO %d, error %d\n",
				ddata->irq_gpio, ret);
			goto free_ddata;
		}

		ret = gpio_to_irq(ddata->irq_gpio);
		if (ret < 0) {
			dev_err(&client->dev,
				"unable to get irq number for GPIO %d, error %d\n",
				ddata->irq_gpio, ret);
			goto free_gpio;
		}
		client->irq = ret;

		ret = request_threaded_irq(client->irq, NULL, as5013_axis_isr,
					   IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
					   "as5013", ddata);
		if (ret) {
			dev_err(&client->dev, "unable to claim irq %d, error %d\n",
				client->irq, ret);
			goto free_gpio;
		}
	} else {
		ddata->irq_gpio = 0;
		client->irq = 0;
		INIT_DELAYED_WORK(&ddata->work, as5013_work);
	}

	ret = as5013_input_register(ddata);
	if (ret) {
		dev_err(&client->dev,
			"Failed to register input device %s, error %d\n",
			ddata->dev_name, ret);
		goto free_irq;
	}

	dev_dbg(&client->dev, "probe %02x, gpio %i, irq %i, \"%s\"\n",
		client->addr, ddata->irq_gpio, client->irq, client->name);

	return 0;

free_irq:
	if (client->irq)
		free_irq(client->irq, ddata);
free_gpio:
	gpio_free(ddata->irq_gpio);
free_ddata:
	kfree(ddata);
	return ret;
}

static int as5013_remove(struct i2c_client *client)
{
	struct as5013_drvdata *ddata = i2c_get_clientdata(client);

	as5013_input_unregister(ddata);

	if (client->irq) {
		free_irq(client->irq, ddata);
		gpio_free(ddata->irq_gpio);
	} else {
		cancel_delayed_work_sync(&ddata->work);
	}
	kfree(ddata);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int as5013_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct as5013_drvdata *ddata = i2c_get_clientdata(client);

	return as5013_low_power_enter(ddata);
}

static int as5013_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct as5013_drvdata *ddata = i2c_get_clientdata(client);

	return as5013_low_power_leave(ddata);
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(as5013_pm_ops, as5013_suspend, as5013_resume);

static const struct i2c_device_id as5013_id[] = {
	{ "as5013", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, as5013_id);

static struct i2c_driver as5013_driver = {
	.driver = {
		.name	= "as5013",
		.owner	= THIS_MODULE,
		.pm	= &as5013_pm_ops,
		.of_match_table = of_match_ptr(as5013_dt_match),
	},
	.probe		= as5013_probe,
	.remove		= as5013_remove,
	.id_table	= as5013_id,
};

module_i2c_driver(as5013_driver);

MODULE_AUTHOR("Grazvydas Ignotas");
MODULE_AUTHOR("Andrey Utkin <andrey_utkin@fastmail.com>");
MODULE_DESCRIPTION("Driver for Austria Microsystems AS5013 joystick");
MODULE_LICENSE("GPL");
