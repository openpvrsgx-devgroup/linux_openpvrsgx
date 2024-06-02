/*
 * w2scbw003.c
 * Driver for power controlling the w2cbw003 WiFi/Bluetooth chip.
 *
 * powers on the chip if the tty port associated/connected
 * to the bluetooth subsystem is opened (e.g. hciattach /dev/ttyBT0)
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/serdev.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>

// #undef pr_debug
// #define pr_debug printk

struct w2cbw_data {
	struct		regulator *vdd_regulator;
	struct		serdev_device *uart;	/* the uart connected to the chip */
#if !IS_ENABLED(CONFIG_W2CBW003_HCI)
	struct		tty_driver *tty_drv;	/* this is the user space tty */
	struct		device *dev;	/* returned by tty_port_register_device() */
	struct		tty_port port;
	int		open_count;	/* how often we were opened */
#endif
};

static struct w2cbw_data *w2cbw_by_minor[1];

static void w2cbw_set_power(void *pdata, int val)
{
	struct w2cbw_data *data = (struct w2cbw_data *) pdata;

	pr_debug("%s(...,%x)\n", __func__, val);

	if (val != 0)
		WARN_ON(regulator_enable(data->vdd_regulator));
	else
		regulator_disable(data->vdd_regulator);
}

/* called each time data is received by the UART (i.e. sent by the w2cbw003) */

static int w2cbw_uart_receive_buf(struct serdev_device *serdev, const unsigned char *rxdata,
				size_t count)
{
	struct w2cbw_data *data = (struct w2cbw_data *) serdev_device_get_drvdata(serdev);

//	pr_debug("%s() characters\n", __func__, count);

#if !IS_ENABLED(CONFIG_W2CBW003_HCI)
	if (data->open_count > 0) {
		int n;

		pr_debug("w2cbw003: uart=>tty %d chars\n", count);
		n = tty_insert_flip_string(&data->port, rxdata, count);	/* pass to user-space */
		if (n != count)
			pr_debug("w2cbw003: did loose %d characters\n", count - n);
		tty_flip_buffer_push(&data->port);
		return n;
	}

	/* nobody is listenig - ignore incoming data */
	return count;
#endif
}

static struct serdev_device_ops serdev_ops = {
	.receive_buf = w2cbw_uart_receive_buf,
#if 0
	.write_wakeup = w2cbw_uart_wakeup,
#endif
};

#if !IS_ENABLED(CONFIG_W2CBW003_HCI)

static struct w2cbw_data *w2cbw_get_by_minor(unsigned int minor)
{
	return w2cbw_by_minor[minor];
}

static int w2cbw_tty_install(struct tty_driver *driver, struct tty_struct *tty)
{
	struct w2cbw_data *data;
	int retval;

	pr_debug("%s() tty = %p\n", __func__, tty);

	data = w2cbw_get_by_minor(tty->index);
	pr_debug("%s() data = %p\n", __func__, data);

	if (!data)
		return -ENODEV;

	retval = tty_standard_install(driver, tty);
	if (retval)
		goto error_init_termios;

	tty->driver_data = data;

	return 0;

error_init_termios:
	tty_port_put(&data->port);
	return retval;
}

static int w2cbw_tty_open(struct tty_struct *tty, struct file *file)
{
	struct w2cbw_data *data = tty->driver_data;

	pr_debug("%s() data = %p open_count = ++%d\n", __func__, data, data->open_count);
//	val = (val & TIOCM_DTR) != 0;	/* DTR controls power on/off */

	w2cbw_set_power(data, ++data->open_count > 0);

// we could/should return -Esomething if already open...

	return tty_port_open(&data->port, tty, file);
}

static void w2cbw_tty_close(struct tty_struct *tty, struct file *file)
{
	struct w2cbw_data *data = tty->driver_data;

	pr_debug("%s()\n", __func__);
//	val = (val & TIOCM_DTR) != 0;	/* DTR controls power on/off */
	w2cbw_set_power(data, --data->open_count > 0);

	tty_port_close(&data->port, tty, file);
}

static int w2cbw_tty_write(struct tty_struct *tty,
		const unsigned char *buffer, int count)
{
	struct w2cbw_data *data = tty->driver_data;

	pr_debug("w2cbw003: tty=>uart %d chars\n", count);

	return serdev_device_write_buf(data->uart, buffer, count);	/* simply pass down to UART */
}

static int w2cbw_tty_write_room(struct tty_struct *tty)
{
	struct w2cbw_data *data = tty->driver_data;

	return serdev_device_write_room(data->uart);	/* simply pass from UART */
}

static void w2cbw_tty_set_termios(struct tty_struct *tty,
		struct ktermios *old)
{ /* copy from tty settings */
	struct w2cbw_data *data = tty->driver_data;

pr_debug("%s() flow = %d\n", __func__, tty_port_cts_enabled(tty->port));
pr_debug("%s() baud rate = %d\n", __func__, tty_termios_baud_rate(&tty->termios));

	serdev_device_set_flow_control(data->uart, tty_port_cts_enabled(tty->port));
	serdev_device_set_baudrate(data->uart, tty_termios_baud_rate(&tty->termios));
/* this may trigger
[  295.607781]
[  295.609349] =============================================
[  295.615024] [ INFO: possible recursive locking detected ]
[  295.620707] 4.11.0-rc8-letux+ #1026 Tainted: G        W
[  295.626745] ---------------------------------------------
[  295.632421] hciattach/2633 is trying to acquire lock:
[  295.637737]  (&tty->termios_rwsem){++++.+}, at: [<c04bb400>] tty_set_termios+0x40/0x1b0
[  295.646190]
[  295.646190] but task is already holding lock:
[  295.652329]  (&tty->termios_rwsem){++++.+}, at: [<c04bb400>] tty_set_termios+0x40/0x1b0
[  295.660780]
[  295.660780] other info that might help us debug this:
[  295.667646]  Possible unsafe locking scenario:
[  295.667646]
[  295.673873]        CPU0
[  295.676444]        ----
[  295.679017]   lock(&tty->termios_rwsem);
[  295.683151]   lock(&tty->termios_rwsem);
[  295.687288]
[  295.687288]  *** DEADLOCK ***
[  295.687288]
[  295.693517]  May be due to missing lock nesting notation
[  295.693517]
[  295.700666] 2 locks held by hciattach/2633:
[  295.705063]  #0:  (&tty->ldisc_sem){++++++}, at: [<c04bc790>] tty_ldisc_ref_wait+0x18/0x34
[  295.713789]  #1:  (&tty->termios_rwsem){++++.+}, at: [<c04bb400>] tty_set_termios+0x40/0x1b0
[  295.722691]
[  295.722691] stack backtrace:
[  295.727285] CPU: 1 PID: 2633 Comm: hciattach Tainted: G        W       4.11.0-rc8-letux+ #1026
[  295.736342] Hardware name: Generic OMAP5 (Flattened Device Tree)
[  295.742679] [<c010f378>] (unwind_backtrace) from [<c010b8fc>] (show_stack+0x10/0x14)
[  295.750841] [<c010b8fc>] (show_stack) from [<c043e4a0>] (dump_stack+0x98/0xd0)
[  295.758450] [<c043e4a0>] (dump_stack) from [<c0186ff0>] (validate_chain+0x760/0x1010)
[  295.766694] [<c0186ff0>] (validate_chain) from [<c0188e58>] (__lock_acquire+0x690/0x760)
[  295.775214] [<c0188e58>] (__lock_acquire) from [<c0189428>] (lock_acquire+0x1d0/0x29c)
[  295.783558] [<c0189428>] (lock_acquire) from [<c0754a88>] (down_write+0x28/0x78)
[  295.791354] [<c0754a88>] (down_write) from [<c04bb400>] (tty_set_termios+0x40/0x1b0)
[  295.799508] [<c04bb400>] (tty_set_termios) from [<c04d50dc>] (ttyport_set_flow_control+0x58/0x60)
[  295.808856] [<c04d50dc>] (ttyport_set_flow_control) from [<bf0a4080>] (w2cbw_tty_set_termios+0x58/0x80 [w2cbw003_bluetooth])
[  295.820675] [<bf0a4080>] (w2cbw_tty_set_termios [w2cbw003_bluetooth]) from [<c04bb514>] (tty_set_termios+0x154/0x1b0)
[  295.831859] [<c04bb514>] (tty_set_termios) from [<c04bbd18>] (set_termios.part.1+0x3b4/0x42c)
[  295.840841] [<c04bbd18>] (set_termios.part.1) from [<c04bbff4>] (tty_mode_ioctl+0x1e8/0x598)
[  295.849721] [<c04bbff4>] (tty_mode_ioctl) from [<c04b7080>] (tty_ioctl+0xe84/0xedc)
[  295.857784] [<c04b7080>] (tty_ioctl) from [<c0282e80>] (vfs_ioctl+0x20/0x34)
[  295.865215] [<c0282e80>] (vfs_ioctl) from [<c0283844>] (do_vfs_ioctl+0x82c/0x93c)
[  295.873098] [<c0283844>] (do_vfs_ioctl) from [<c02839a0>] (SyS_ioctl+0x4c/0x74)
[  295.880801] [<c02839a0>] (SyS_ioctl) from [<c0107040>] (ret_fast_syscall+0x0/0x1c)
*/
}

static const struct tty_operations w2cbw_serial_ops = {
	.install = w2cbw_tty_install,
	.open = w2cbw_tty_open,
	.close = w2cbw_tty_close,
	.write = w2cbw_tty_write,
	.write_room = w2cbw_tty_write_room,
	.set_termios = w2cbw_tty_set_termios,
#if 0
	.cleanup = w2cbw_tty_cleanup,
	.ioctl = w2cbw_tty_ioctl,
	.chars_in_buffer = w2cbw_tty_chars_in_buffer,
	.tiocmget = w2cbw_tty_tiocmget,
	.tiocmset = w2cbw_tty_tiocmset,
	.get_icount = w2cbw_tty_get_count,
	.unthrottle = w2cbw_tty_unthrottle
#endif
};

static const struct tty_port_operations w2cbw_port_ops = {
};
#endif

static int w2cbw_probe(struct serdev_device *serdev)
{
	struct device *dev = &serdev->dev;
	struct w2cbw_data *data;
	int err;
	int minor;

	pr_debug("%s()\n", __func__);

	minor = 0;
	if (w2cbw_by_minor[minor]) {
		pr_err("w2cbw minor is already in use!\n");
		return -ENODEV;
	}

	if (!serdev->dev.of_node) {
		dev_err(&serdev->dev, "No device tree data\n");
		return -EINVAL;
	}

	data = devm_kzalloc(&serdev->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	data->vdd_regulator = devm_regulator_get_optional(dev, "vdd");

	if (IS_ERR(data->vdd_regulator)) {
		if (PTR_ERR(data->vdd_regulator) == -EPROBE_DEFER)
			return -EPROBE_DEFER;	/* we can't probe yet */
		data->uart = NULL;	/* no regulator yet */
	}

	pr_debug("%s() vdd_regulator = %p\n", __func__, data->vdd_regulator);

	w2cbw_by_minor[minor] = data;

	serdev_device_set_drvdata(serdev, data);

	pr_debug("w2cbw003 probed\n");

	data->uart = serdev;

	serdev_device_set_client_ops(data->uart, &serdev_ops);
	serdev_device_open(data->uart);

	serdev_device_set_baudrate(data->uart, 9600);
	serdev_device_set_flow_control(data->uart, false);

#if IS_ENABLED(CONFIG_W2CBW003_HCI)

	hci_uart_register_device(tbd);

#else // IS_ENABLED(CONFIG_W2CBW003_HCI)

#if 1
	pr_debug("w2cbw alloc_tty_driver\n");
#endif
	/* allocate the tty driver */
	data->tty_drv = alloc_tty_driver(1);
	if (!data->tty_drv)
		return -ENOMEM;

	/* initialize the tty driver */
	data->tty_drv->owner = THIS_MODULE;
	data->tty_drv->driver_name = "w2cbw003";
	data->tty_drv->name = "ttyBT";
	data->tty_drv->major = 0;
	data->tty_drv->minor_start = 0;
	data->tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	data->tty_drv->subtype = SERIAL_TYPE_NORMAL;
	data->tty_drv->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	data->tty_drv->init_termios = tty_std_termios;
	data->tty_drv->init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	/*
	 * tty_termios_encode_baud_rate(&data->tty_drv->init_termios, 115200, 115200);
	 * w2cbw_tty_termios(&data->tty_drv->init_termios);
	 */
	tty_set_operations(data->tty_drv, &w2cbw_serial_ops);

#if 1
	pr_debug("w2cbw tty_register_driver\n");
#endif
	/* register the tty driver */
	err = tty_register_driver(data->tty_drv);
	if (err) {
		pr_err("%s - tty_register_driver failed(%d)\n",
			__func__, err);
		put_tty_driver(data->tty_drv);
		goto err;
	}

#if 1
	pr_debug("w2cbw call tty_port_init\n");
#endif
	tty_port_init(&data->port);
	data->port.ops = &w2cbw_port_ops;

#if 1
	pr_debug("w2cbw call tty_port_register_device\n");
#endif
/*
 * FIXME: this appears to reenter this probe() function a second time
 * which only fails because the minor number is already assigned
 */

	data->dev = tty_port_register_device(&data->port,
			data->tty_drv, minor, &serdev->dev);

#if 1
	pr_debug("w2cbw tty_port_register_device -> %p\n", data->dev);
	pr_debug("w2cbw port.tty = %p\n", data->port.tty);
#endif
//	data->port.tty->driver_data = data;	/* make us known in tty_struct */

#endif // IS_ENABLED(CONFIG_W2CBW003_HCI)

	/* keep off until user space requests the device */
	if (regulator_is_enabled(data->vdd_regulator))
		w2cbw_set_power(data, false);

	pr_debug("w2cbw probed\n");

	return 0;

err:
	serdev_device_close(data->uart);
#if 0
	if (err == -EBUSY)
		err = -EPROBE_DEFER;
#endif
#if 1
	pr_debug("w2cbw error %d\n", err);
#endif
	return err;
}

static void w2cbw_remove(struct serdev_device *serdev)
{
	struct w2cbw_data *data = serdev_device_get_drvdata(serdev);
	int minor;

	/* what is the right sequence to avoid problems? */
	serdev_device_close(data->uart);

#if !IS_ENABLED(CONFIG_W2CBW003_HCI)
	// should get minor from searching for data == w2cbw_by_minor[minor]
	minor = 0;
	tty_unregister_device(data->tty_drv, minor);

	tty_unregister_driver(data->tty_drv);
#endif
}

// ??? add suspend/resume handler by regulator

static const struct of_device_id w2cbw_of_match[] = {
	{ .compatible = "wi2wi,w2cbw003-bluetooth" },
	{},
};
MODULE_DEVICE_TABLE(of, w2cbw_of_match);

static struct serdev_device_driver w2cbw_driver = {
	.probe		= w2cbw_probe,
	.remove		= w2cbw_remove,
	.driver = {
		.name	= "w2cbw003",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(w2cbw_of_match)
	},
};

module_serdev_device_driver(w2cbw_driver);

MODULE_ALIAS("w2cbw003-bluetooth");

MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("w2cbw003 power management driver");
MODULE_LICENSE("GPL v2");
