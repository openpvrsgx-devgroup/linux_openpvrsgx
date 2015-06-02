// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for power controlling the w2sg0004 GPS receiver.
 *
 * Copyright (C) 2013 Neil Brown <neilb@suse.de>
 * Copyright (C) 2015-2018 H. Nikolaus Schaller <hns@goldelico.com>,
 *						Golden Delicious Computers
 *
 * This receiver has an ON/OFF pin which must be toggled to
 * turn the device 'on' or 'off'.  A high->low->high toggle
 * will switch the device on if it is off, and off if it is on.
 *
 * Contrary to newer Sirf based modules is not possible to directly
 * detect the power state of the w2sg0004.
 *
 * However, when it is on it will send characters on a UART line
 * regularly.
 *
 * To detect that the power state is out of sync (e.g. if GPS
 * was enabled before a reboot), we monitor the serdev data stream
 * and compare with what the driver thinks about the state.
 *
 * In addition we register as a rfkill client so that we can
 * control the LNA power.
 *
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gnss.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/rfkill.h>
#include <linux/serdev.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

/*
 * There seems to be restrictions on how quickly we can toggle the
 * on/off line.  data sheets says "two rtc ticks", whatever that means.
 * If we do it too soon it doesn't work.
 * So we have a state machine which uses the common work queue to ensure
 * clean transitions.
 * When a change is requested we record that request and only act on it
 * once the previous change has completed.
 * A change involves a 10ms low pulse, and a 990ms raised level, so only
 * one change per second.
 */

enum w2sg_state {
	W2SG_IDLE,	/* is not changing state */
	W2SG_PULSE,	/* activate on/off impulse */
	W2SG_NOPULSE	/* deactivate on/off impulse */
};

struct w2sg_data {
	struct		gnss_device *gdev;
	struct		regulator *lna_regulator;
	struct		gpio_desc *on_off_gpio;	/* the on-off gpio */
	struct		rfkill *rf_kill;
	struct		serdev_device *uart;	/* uart connected to the chip */
	bool		lna_blocked;	/* rfkill block gps is active */
	bool		lna_is_off;	/* LNA is currently off */
	bool		is_on;		/* current power state (0/1) */
	unsigned long	last_toggle;
	unsigned long	backoff;	/* time to wait since last_toggle */
	enum		w2sg_state state;	/* state engine state */
	bool		requested;	/* requested power state (0/1) */
	bool		suspended;
	struct		delayed_work work;
	int		discard_count;
};

static int w2sg_set_lna_power(struct w2sg_data *data)
{
	int ret = 0;
	bool off = data->suspended || !data->requested || data->lna_blocked;

	if (off != data->lna_is_off) {
		data->lna_is_off = off;
		if (!IS_ERR_OR_NULL(data->lna_regulator)) {
			if (off)
				regulator_disable(data->lna_regulator);
			else
				ret = regulator_enable(data->lna_regulator);
		}
	}

	return ret;
}

static void w2sg_set_power(struct w2sg_data *data, bool val)
{
	if (val && !data->requested) {
		data->requested = true;
	} else if (!val && data->requested) {
		data->backoff = HZ;
		data->requested = false;
	} else
		return;

	if (!data->suspended)
		schedule_delayed_work(&data->work, 0);
}

/* called each time data is received by the UART (i.e. sent by the w2sg0004) */
static int w2sg_uart_receive_buf(struct serdev_device *serdev,
				const unsigned char *rxdata,
				size_t count)
{
	struct w2sg_data *data =
		(struct w2sg_data *) serdev_device_get_drvdata(serdev);

	if (!data->requested && !data->is_on) {
		/*
		 * we have received characters while the w2sg
		 * should have been be turned off
		 */
		data->discard_count += count;
		if ((data->state == W2SG_IDLE) &&
		    time_after(jiffies,
		    data->last_toggle + data->backoff)) {
			/* Should be off by now, time to toggle again */
			dev_dbg(&serdev->dev, "w2sg00x4 has sent %d characters data although it should be off!\n",
				data->discard_count);

			data->discard_count = 0;

			data->is_on = true;
			data->backoff *= 2;
			if (!data->suspended)
				schedule_delayed_work(&data->work, 0);
		}
		return count;
	}

	/*
	 * pass to user-space
	 */

	if (data->requested)
		return gnss_insert_raw(data->gdev, rxdata, count);

	data->discard_count += count;

	return count;
}

/* try to toggle the power state by sending a pulse to the on-off GPIO */
static void toggle_work(struct work_struct *work)
{
	struct w2sg_data *data = container_of(work, struct w2sg_data,
					      work.work);

	w2sg_set_lna_power(data);	/* update LNA power state */

	switch (data->state) {
	case W2SG_IDLE:
		if (data->requested == data->is_on)
			return;

		gpiod_set_value_cansleep(data->on_off_gpio, 1);
		data->state = W2SG_PULSE;

		schedule_delayed_work(&data->work,
				      msecs_to_jiffies(10));
		break;

	case W2SG_PULSE:
		gpiod_set_value_cansleep(data->on_off_gpio, 0);
		data->last_toggle = jiffies;
		data->state = W2SG_NOPULSE;
		data->is_on = !data->is_on;

		schedule_delayed_work(&data->work,
				      msecs_to_jiffies(10));
		break;

	case W2SG_NOPULSE:
		data->state = W2SG_IDLE;

		break;

	}
}

static int w2sg_rfkill_set_block(void *pdata, bool blocked)
{
	struct w2sg_data *data = pdata;

	data->lna_blocked = blocked;

	if (!data->suspended)
		schedule_delayed_work(&data->work, 0);

	return 0;
}

static struct rfkill_ops w2sg0004_rfkill_ops = {
	.set_block = w2sg_rfkill_set_block,
};

static struct serdev_device_ops serdev_ops = {
	.receive_buf = w2sg_uart_receive_buf,
};

static int w2sg_gps_open(struct gnss_device *gdev)
{ /* user-space has opened our interface */
	struct w2sg_data *data = gnss_get_drvdata(gdev);

	w2sg_set_power(data, true);

	return 0;
}

static void w2sg_gps_close(struct gnss_device *gdev)
{ /* user-space has finally closed our interface */
	struct w2sg_data *data = gnss_get_drvdata(gdev);

	w2sg_set_power(data, false);
}

static int w2sg_gps_send(struct gnss_device *gdev,
		const unsigned char *buffer, size_t count)
{ /* raw data coming from user space */
	struct w2sg_data *data = gnss_get_drvdata(gdev);

	/* simply pass down to UART */
	return serdev_device_write_buf(data->uart, buffer, count);
}

static const struct gnss_operations w2sg_gnss_ops = {
	.open		= w2sg_gps_open,
	.close		= w2sg_gps_close,
	.write_raw	= w2sg_gps_send,
};

static int w2sg_probe(struct serdev_device *serdev)
{
	struct w2sg_data *data;
	struct rfkill *rf_kill;
	struct gnss_device *gdev;
	int err;

	data = devm_kzalloc(&serdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->on_off_gpio = devm_gpiod_get_index(&serdev->dev,
						   "enable", 0,
						   GPIOD_OUT_LOW);
	if (IS_ERR(data->on_off_gpio)) {
		/* defer until we have the gpio */
		if (PTR_ERR(data->on_off_gpio) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		dev_err(&serdev->dev, "could not get the enable-gpio");
		return PTR_ERR(data->on_off_gpio);
	}

	gpiod_direction_output(data->on_off_gpio, false);

	data->lna_regulator = devm_regulator_get_optional(&serdev->dev,
							"lna");
	if (IS_ERR(data->lna_regulator)) {
		/* defer until we can get the regulator */
		if (PTR_ERR(data->lna_regulator) == -EPROBE_DEFER)
			return -EPROBE_DEFER;

		data->lna_regulator = NULL;	/* ignore other errors */
	}

	gdev = gnss_allocate_device(&serdev->dev);
	if (!gdev)
		return -ENOMEM;

	gdev->ops = &w2sg_gnss_ops;
	gnss_set_drvdata(gdev, data);

	data->gdev = gdev;
	data->uart = serdev;

	data->lna_blocked = true;
	data->lna_is_off = true;

	data->is_on = false;
	data->requested = false;
	data->state = W2SG_IDLE;
	data->last_toggle = jiffies;
	data->backoff = HZ;

	INIT_DELAYED_WORK(&data->work, toggle_work);

	serdev_device_set_drvdata(serdev, data);
	serdev_device_set_client_ops(data->uart, &serdev_ops);

	err = serdev_device_open(data->uart);
	if (err < 0)
		goto err_put_gnss;

	err = serdev_device_set_baudrate(data->uart, 9600);
	if (err < 0)
		goto err_close_serdev;

	serdev_device_set_flow_control(data->uart, false);

	err = gnss_register_device(gdev);
	if (err < 0)
		goto err_close_serdev;

	rf_kill = rfkill_alloc("GPS", &serdev->dev, RFKILL_TYPE_GPS,
				&w2sg0004_rfkill_ops, data);
	if (!rf_kill) {
		err = -ENOMEM;
		goto err_deregister_gnss;
	}

	err = rfkill_register(rf_kill);
	if (err) {
		dev_err(&serdev->dev, "Cannot register rfkill device\n");
		goto err_destroy_rfkill;
	}

	data->rf_kill = rf_kill;

	/* keep off until user space requests the device */
	w2sg_set_power(data, false);

	return 0;

err_destroy_rfkill:
	rfkill_destroy(data->rf_kill);

err_deregister_gnss:
	gnss_deregister_device(data->gdev);

err_close_serdev:
	serdev_device_close(data->uart);

err_put_gnss:
	gnss_put_device(data->gdev);

	return err;
}

static void w2sg_remove(struct serdev_device *serdev)
{
	struct w2sg_data *data = serdev_device_get_drvdata(serdev);

	rfkill_destroy(data->rf_kill);

	gnss_deregister_device(data->gdev);

	cancel_delayed_work_sync(&data->work);

	serdev_device_close(data->uart);

	gnss_put_device(data->gdev);
}

static int __maybe_unused w2sg_suspend(struct device *dev)
{
	struct w2sg_data *data = dev_get_drvdata(dev);

	data->suspended = true;

	cancel_delayed_work_sync(&data->work);

	w2sg_set_lna_power(data);	/* shuts down if needed */

	if (data->state == W2SG_PULSE) {
		msleep(10);
		gpiod_set_value_cansleep(data->on_off_gpio, 0);
		data->last_toggle = jiffies;
		data->is_on = !data->is_on;
		data->state = W2SG_NOPULSE;
	}

	if (data->state == W2SG_NOPULSE) {
		msleep(10);
		data->state = W2SG_IDLE;
	}

	if (data->is_on) {
		gpiod_set_value_cansleep(data->on_off_gpio, 1);
		msleep(10);
		gpiod_set_value_cansleep(data->on_off_gpio, 0);
		data->is_on = false;
	}

	return 0;
}

static int __maybe_unused w2sg_resume(struct device *dev)
{
	struct w2sg_data *data = dev_get_drvdata(dev);

	data->suspended = false;

	schedule_delayed_work(&data->work, 0);	/* enables LNA if needed */

	return 0;
}

static const struct of_device_id w2sg0004_of_match[] = {
	{ .compatible = "wi2wi,w2sg0004" },
	{},
};
MODULE_DEVICE_TABLE(of, w2sg0004_of_match);

SIMPLE_DEV_PM_OPS(w2sg_pm_ops, w2sg_suspend, w2sg_resume);

static struct serdev_device_driver w2sg_driver = {
	.probe		= w2sg_probe,
	.remove		= w2sg_remove,
	.driver = {
		.name	= "w2sg0004",
		.owner	= THIS_MODULE,
		.pm	= &w2sg_pm_ops,
		.of_match_table = of_match_ptr(w2sg0004_of_match)
	},
};

module_serdev_device_driver(w2sg_driver);

MODULE_AUTHOR("NeilBrown <neilb@suse.de>");
MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("w2sg0004 GPS power management driver");
MODULE_LICENSE("GPL v2");
