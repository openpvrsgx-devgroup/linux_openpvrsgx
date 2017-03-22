/*
 * txs02612.c
 * Driver for controlling the txs02612 sd level shifter and switch.
 *
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mmc/host.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#undef pr_debug
#define pr_debug printk

struct txs_data {
	struct gpio_desc *control_gpio;	/* the control gpio */
	struct mmc_host *host;		/* host side */
	struct mutex lock;
	struct mmc_host *mmc[2];	/* client side */
};

/* we might remove the /sys access to the switch lever */

static ssize_t set_switch(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct txs_data *data = dev_get_drvdata(dev);
	unsigned long val;
// FIXME: we could decode the state as string (eMMC vs uSD)
	int err = kstrtoul(buf, 10, &val);

	pr_debug("%s() to %ld\n", __func__, val);

	if (err) {
		if (*buf == 'e')
			val = 0;
		else if (*buf == 'u')
			val = 1;
		else
			return err;
	}
	if (val > 1)
		return -EINVAL;

	gpiod_set_value_cansleep(data->control_gpio, val);

// maybe: mmc_detect_change(host, delay);

	return count;
}

static ssize_t show_switch(struct device *dev,
			struct device_attribute *attr, char *buf)
	{
	struct txs_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", gpiod_get_value_cansleep(data->control_gpio)?"uSD":"eMMC");
	}

static DEVICE_ATTR(switch, S_IWUSR | S_IRUGO,
		show_switch, set_switch);

static struct attribute *txs_attributes[] = {
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group txs_attr_group = {
	.attrs = txs_attributes,
};

static void txs02612_request(struct mmc_host *client, struct mmc_request *req) {
	// FIXME: get platform data
#if 0
printk("%s: client=%px req=%px\n", __func__, client, req);
printk("%s: parent=%px\n", __func__, client->parent);
#endif

	struct platform_device *dev = to_platform_device(client->parent);
	struct txs_data *data = platform_get_drvdata(dev);
	int client_port, current_port;

#if 0
printk("%s: data=%px\n", __func__, data);
#endif

	/* find out how the switch is positioned */
	current_port = gpiod_get_value_cansleep(data->control_gpio);	// this eats time but can not get out of sync

	// we could also check for client != data->mmc[current_port]
	// before we do a search

	/* find out which client port did initiate this request */
	for (client_port = 0; client_port < 2; client_port++) {
		if (client == data->mmc[client_port])
			break;
	}

#if 1
printk("%s: current=%d client=%d\n", __func__, current_port, client_port);
#endif

	if (current_port != client_port) {
		mutex_lock(&data->lock);
		gpiod_set_value_cansleep(data->control_gpio, client_port);

		/*
		 * make host_mmc switch clock rates, bus width, power etc.
		 */
	}

	/* forward the request to the host controller */

// FIXME: does this call just trigger the request?
// if yes, we need to install a done completion function pointer
// for the request and release the lock by that!

	if (!IS_ERR_OR_NULL(data->host)) {
		data->host->ops->request(data->host, req);
	}

	if (current_port != client_port) {
		mutex_unlock(&data->lock);
	}
}

static int txs02612_start_signal_voltage_switch(struct mmc_host *client, struct mmc_ios *ios)
{
	// CHECKME: do we have to forward this after throwing the switch as well?

	return 0;
}

static void txs02612_set_ios(struct mmc_host *client, struct mmc_ios *ios)
{
	// CHECKME: do we have to forward this after throwing the switch as well?

	return;
}

static const struct mmc_host_ops txs02612_ops = {
/* required ops */
	.request = txs02612_request,
	.start_signal_voltage_switch = txs02612_start_signal_voltage_switch,
	.set_ios = txs02612_set_ios,
/* potential extensions:
	.get_ro = handle separate write protect gpios
	.get_cd = handle separate card detect gpios
*/
};

/* this helper should be part of drivers/base/core.c */

struct helper {
	struct device_node *node;
	struct device *child;
};

static int find_by_node(struct device *dev, void *data)
{
	struct helper *r = (struct helper *) data;

#if 0
	// check if phandle is matching dev->of_node

	printk("%s: dev=%px r->child=%px r->node=%px of=%px - %pOFfp\n", __func__,
		    dev,
		    r->child, r->node,
						dev->of_node, dev->of_node);	// child of node we may be looking for...
#endif
	if (dev->of_node == r->node) {
		r->child = dev;
#if 1
		printk("%s: found :)\n", __func__);
#endif
		return 1;	// found
	}

	return device_for_each_child(dev, data, find_by_node);
}

static struct device *device_find_by_node(struct device *parent, struct device_node *node)
{
	struct helper r = {
		.node = node,
		.child = NULL
	};

#if 1
	printk("%s: parent=%px of=%px - %pOFfp\n", __func__, parent, node, node);
#endif

	device_for_each_child(parent, (void *) &r, find_by_node);

	return r.child;
}

/* do something similar to devm_usb_get_phy_by_phandle() */

static struct mmc_host *devm_mmc_get_mmc_by_phandle(struct device *dev,
	const char *phandle, u8 index)
	{
	struct device_node *node = of_parse_phandle(dev->of_node, phandle, index);
	struct device *mmcdev;

	if (!node) {
		dev_err(dev, "failed to get %s phandle in %pOF node\n", phandle,
			dev->of_node);
		return ERR_PTR(-ENODEV);
	}

#if 0
	printk("%s: dev = %px\n", __func__, dev);
	printk("%s: dev parent = %px\n", __func__, dev->parent);
	printk("%s: dev of_node = %pOFfp\n", __func__, dev->of_node);
	printk("%s: looking for = %pOFfp\n", __func__, node);
#endif

	mmcdev = device_find_by_node(dev->parent, node);
	of_node_put(node);

	printk("%s: node %pOFfp -> dev = %px\n", __func__, node, mmcdev);
	if (!mmcdev)
		return NULL; // error handling

	// somehow call device_put(result) if no longer needed, i.e. make the result a devres object

	/*
	 * Room for improvement:
	 * after identifying and claiming the host_mmc port
	 * we should disable it from user-space
	 * so that partprobe etc. doesn't see it
	 */

	// is this correct?
	// while not sure:
		return NULL;

	return container_of(mmcdev, struct mmc_host, class_dev);
}

static int txs_probe(struct platform_device *dev)
{
	struct txs_data *data;
	int port;
	int err;

	dev_err(&dev->dev, "probe\n");

	data = devm_kzalloc(&dev->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		err = -ENOMEM;
		goto out;
	}

	data->control_gpio = devm_gpiod_get_index(&dev->dev,
						"select", 0,
						GPIOD_OUT_HIGH);
	if (IS_ERR(data->control_gpio)) {
		/* defer until we have the gpio */
		if (PTR_ERR(data->control_gpio) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		dev_err(&dev->dev, "could not get the enable-gpio\n");
		return PTR_ERR(data->control_gpio);
	}

	platform_set_drvdata(dev, data);

	/* identify our host port */
	data->host = devm_mmc_get_mmc_by_phandle(&dev->dev, "mmc", 0);
	// FIXME: need to handle EPROBE_DEFER?
	if (IS_ERR_OR_NULL(data->host)) {
		dev_err(&dev->dev, "could not get access to host port, using manual mode\n");
		// return -EPROBE_DEFER;
	}

	/* Register sysfs hooks */
	err = sysfs_create_group(&dev->dev.kobj, &txs_attr_group);
	if (err)
		goto out;

	mutex_init(&data->lock);

	if (1 || !IS_ERR_OR_NULL(data->host)) {
		/* setup our client ports */

		// alternatively loop over OF children

		for (port = 0; port < 2; port++) {
			data->mmc[port] = mmc_alloc_host(sizeof(struct mmc_host), &dev->dev);
			// error check?
			data->mmc[port]->ops = &txs02612_ops;

			// do other setup here

			/* note: first requests may be done before this function returns */
			err = mmc_add_host(data->mmc[port]);
			if (err < 0) {
				dev_err(&dev->dev, "could not create port %d err=%d\n", port, err);
				mmc_free_host(data->mmc[port]);
				goto mmc_error;
			}
		}
	}

	dev_err(&dev->dev, "probed\n");

	return 0;

mmc_error:
	mutex_destroy(&data->lock);

	/* error handling - remove and free other port(s) */
	while (--port >= 0) {
		mmc_remove_host(data->mmc[port]);
		mmc_free_host(data->mmc[port]);
	}
out:
	pr_debug("%s() error %d\n", __func__, err);

	return err;
}

static int txs_remove(struct platform_device *dev)
{
	struct txs_data *data = platform_get_drvdata(dev);
	int port;

	for (port = 0; port < 2; port++) {
		mmc_remove_host(data->mmc[port]);
		mmc_free_host(data->mmc[port]);
	}

	sysfs_remove_group(&dev->dev.kobj, &txs_attr_group);

	mutex_destroy(&data->lock);

	return 0;
}

static int txs_suspend(struct device *dev)
{
	struct txs_data *data = dev_get_drvdata(dev);

	return 0;
}

static int txs_resume(struct device *dev)
{
	struct txs_data *data = dev_get_drvdata(dev);

	return 0;
}

static const struct of_device_id txs_of_match[] = {
	{ .compatible = "ti,txs02612" },
	{},
};
MODULE_DEVICE_TABLE(of, txs_of_match);

static SIMPLE_DEV_PM_OPS(txs_pm_ops, txs_suspend, txs_resume);

static struct platform_driver txs_driver = {
	.probe		= txs_probe,
	.remove		= txs_remove,
	.driver = {
		.name	= "txs02612",
		.owner	= THIS_MODULE,
		.pm	= &txs_pm_ops,
		.of_match_table = of_match_ptr(txs_of_match)
	},
};

module_platform_driver(txs_driver);

MODULE_ALIAS("platform:txs02612");

MODULE_AUTHOR("Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("txs02612 SD level shifter and switch");
MODULE_LICENSE("GPL v2");
