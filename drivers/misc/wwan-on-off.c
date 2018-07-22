/*
 * wwan_on_off
 * driver for controlling power states of some WWAN modules
 * like the GTM601 or the PHS8 which are independently powered
 * from the APU so that they can continue to run during suspend
 * and potentially during power-off.
 *
 * Such modules usually have some ON_KEY or IGNITE input
 * that can be triggered to turn the modem power on or off
 * by giving a sufficiently long (200ms) impulse.
 *
 * Some modules have a power-is-on feedback that can be fed
 * into another GPIO so that the driver knows the real state.
 *
 * If this is not available we can monitor some USB PHY
 * port which becomes active if the modem is powered on.
 *
 * The driver is based on the w2sg0004 driver developed
 * by Neil Brown.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/rfkill.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/usb/phy.h>
#include <linux/workqueue.h>

#define DEBUG

struct wwan_on_off {
	struct regulator	*vcc_regulator;
	struct rfkill	*rf_kill;
	struct gpio_desc	*on_off_gpio;	/* may be invalid */
	struct gpio_desc	*feedback_gpio;	/* may be invalid */
	struct usb_phy	*usb_phy;	/* USB PHY to monitor for modem activity */
	bool		is_power_on;	/* current state */
	bool		can_turnoff;	/* can also turn off by impulse */
};

static bool wwan_on_off_is_powered_on(struct wwan_on_off *wwan)
{ /* check with physical interfaces if possible */
	if (!IS_ERR_OR_NULL(wwan->feedback_gpio)) {
#ifdef DEBUG
printk("%s: gpio value = %d\n", __func__, gpiod_get_value_cansleep(wwan->feedback_gpio));
printk("%s: return '%s'\n", __func__, gpiod_get_value_cansleep(wwan->feedback_gpio)?"true":"false");
		return gpiod_get_value_cansleep(wwan->feedback_gpio);	/* read gpio */
#endif
}
	if (!IS_ERR_OR_NULL(wwan->usb_phy)) {
		printk("%s: USB phy event %d\n", __func__, wwan->usb_phy->last_event);
		/* check with PHY if available */
	}
	if (IS_ERR_OR_NULL(wwan->feedback_gpio)) {
#ifdef DEBUG
printk("%s: in-off invalid\n", __func__);
printk("%s: return 'true'\n", __func__);
#endif
		return true;	/* we can't even control power, assume it is on */
}
#ifdef DEBUG
printk("%s: we assume %d\n", __func__, wwan->is_power_on);
printk("%s: return '%s'\n", __func__, wwan->is_power_on?"true":"false");
#endif
	return wwan->is_power_on;	/* assume that we know the correct state */
}

static void wwan_on_off_set_power(struct wwan_on_off *wwan, bool on)
{
	int state;
#ifdef DEBUG
	printk("%s:on = %d\n", __func__, on);
#endif
	if (IS_ERR_OR_NULL(wwan->on_off_gpio))
		return;	/* we can't control power */

	state = wwan_on_off_is_powered_on(wwan);

#ifdef DEBUG
	printk("%s: state %d\n", __func__, state);
	if (!IS_ERR_OR_NULL(wwan->vcc_regulator))
		printk("%s: regulator %d\n", __func__, regulator_is_enabled(wwan->vcc_regulator));
#endif

	if(state != on) {
		if (on && !IS_ERR_OR_NULL(wwan->vcc_regulator) && !regulator_is_enabled(wwan->vcc_regulator)) {
			int ret = regulator_enable(wwan->vcc_regulator);	/* turn on regulator */
			mdelay(2000);
		}
		if (!on && !wwan->can_turnoff) {
			printk("%s: can't turn off by impulse\n", __func__);
			if (!IS_ERR_OR_NULL(wwan->vcc_regulator) && regulator_is_enabled(wwan->vcc_regulator))
				regulator_disable(wwan->vcc_regulator);
			return;
		}
#ifdef DEBUG
		printk("%s: send impulse\n", __func__);
#endif
		// use gpiolib to generate impulse
		gpiod_set_value_cansleep(wwan->on_off_gpio, 1);
		// FIXME: check feedback_gpio for early end of impulse
		msleep(200);	/* wait 200 ms */
		gpiod_set_value_cansleep(wwan->on_off_gpio, 0);
		msleep(500);	/* wait 500 ms */
		wwan->is_power_on = on;
		if (wwan_on_off_is_powered_on(wwan) != on) {
			printk("%s: failed to change modem state\n", __func__);	/* warning only! using USB feedback might not be immediate */
			if (!IS_ERR_OR_NULL(wwan->vcc_regulator) && regulator_is_enabled(wwan->vcc_regulator))
				regulator_disable(wwan->vcc_regulator);
		}
	}

#ifdef DEBUG
	printk("%s: done\n", __func__);
#endif
}

static int wwan_on_off_rfkill_set_block(void *data, bool blocked)
{
	struct wwan_on_off *wwan = data;
	int ret = 0;
#ifdef DEBUG
	printk("%s: blocked: %d\n", __func__, blocked);
#endif
	pr_debug("%s: blocked: %d\n", __func__, blocked);
	if (IS_ERR_OR_NULL(wwan->on_off_gpio))
		return -EIO;	/* can't block if we have no control */

	wwan_on_off_set_power(wwan, !blocked);
	return ret;
}

static struct rfkill_ops wwan_on_off_rfkill_ops = {
	// get status to read feedback gpio as HARD block (?)
	.set_block = wwan_on_off_rfkill_set_block,
};

static int wwan_on_off_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct wwan_on_off *wwan;
	struct rfkill *rf_kill;
	int err;
#ifdef DEBUG
	printk("%s: wwan_on_off_probe()\n", __func__);
#endif

	if (!pdev->dev.of_node)
		return -EINVAL;

	wwan = devm_kzalloc(dev, sizeof(*wwan), GFP_KERNEL);
	if (wwan == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, wwan);

	wwan->on_off_gpio = devm_gpiod_get_index(&pdev->dev,
						   "on-off", 0,
						   GPIOD_OUT_HIGH);
	if (IS_ERR_OR_NULL(wwan->on_off_gpio)) {
		/* defer until we have the gpio */
		if (PTR_ERR(wwan->on_off_gpio) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
	}

	wwan->feedback_gpio = devm_gpiod_get_index(&pdev->dev,
						   "on-indicator", 0,
						   GPIOD_IN);
	if (IS_ERR_OR_NULL(wwan->feedback_gpio)) {
		/* defer until we have the gpio */
		if (PTR_ERR(wwan->feedback_gpio) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
	}

	wwan->vcc_regulator = devm_regulator_get_optional(&pdev->dev,
							"modem");
	if (IS_ERR_OR_NULL(wwan->vcc_regulator)) {
		/* defer until we can get the regulator */
		if (PTR_ERR(wwan->vcc_regulator) == -EPROBE_DEFER)
			return -EPROBE_DEFER;

		wwan->vcc_regulator = NULL;	/* ignore other errors */
	}

	wwan->usb_phy = devm_usb_get_phy_by_phandle(dev, "usb-port", 0);
	printk("%s: onoff = %p indicator = %p %d usb_phy = %ld\n", __func__, wwan->on_off_gpio, wwan->feedback_gpio, PTR_ERR(wwan->usb_phy));
	// get optional reference to USB PHY (through "usb-port")
#ifdef DEBUG
	printk("%s: wwan_on_off_probe() wwan=%p\n", __func__, wwan);
#endif

	// FIXME: read from of_device_id table
	wwan->can_turnoff = of_device_is_compatible(dev->of_node, "option,gtm601-power");
	wwan->is_power_on = false;	/* assume initial power is off */

	rf_kill = rfkill_alloc("WWAN", &pdev->dev,
				RFKILL_TYPE_WWAN,
				&wwan_on_off_rfkill_ops, wwan);
	if (rf_kill == NULL) {
		return -ENOMEM;
	}

	rfkill_init_sw_state(rf_kill, !wwan_on_off_is_powered_on(wwan));	/* set initial state */

	err = rfkill_register(rf_kill);
	if (err) {
		dev_err(&pdev->dev, "Cannot register rfkill device\n");
		goto err;
	}

	wwan->rf_kill = rf_kill;

#ifdef DEBUG
	printk("%s: successfully probed\n", __func__);
#endif
	return 0;

err:
	rfkill_destroy(rf_kill);
#ifdef DEBUG
	printk("%s: probe failed %d\n", __func__, err);
#endif
	return err;
}

static int wwan_on_off_remove(struct platform_device *pdev)
{
	struct wwan_on_off *wwan = platform_get_drvdata(pdev);
	return 0;
}

/* we only suspend the driver (i.e. set the gpio in a state
 * that it does not harm)
 * the reason is that the modem must continue to be powered
 * on to receive SMS and incoming calls that wake up the CPU
 * through a wakeup GPIO
 */

static int wwan_on_off_suspend(struct device *dev)
{
#ifdef DEBUG
	struct wwan_on_off *wwan = dev_get_drvdata(dev);
	printk("%s: WWAN suspend\n", __func__);
#endif
	/* set gpio to harmless mode */
	return 0;
}

static int wwan_on_off_resume(struct device *dev)
{
#ifdef DEBUG
	struct wwan_on_off *wwan = dev_get_drvdata(dev);
	printk("%s: WWAN resume\n", __func__);
#endif
	/* restore gpio */
	return 0;
}

/* on system power off we must turn off the
 * modem (which has a separate connection to
 * the battery).
 */

static int wwan_on_off_poweroff(struct device *dev)
{
	struct wwan_on_off *wwan = dev_get_drvdata(dev);
#ifdef DEBUG
	printk("%s: WWAN poweroff\n", __func__);
#endif
	wwan_on_off_set_power(wwan, 0);	/* turn off modem */
	printk("%s: WWAN powered off\n", __func__);
	return 0;
}

static const struct of_device_id wwan_of_match[] = {
	{ .compatible = "option,gtm601-power" },
	{ .compatible = "gemalto,phs8-power" },
	{ .compatible = "gemalto,pls8-power" },
	{},
};
MODULE_DEVICE_TABLE(of, wwan_of_match);

const struct dev_pm_ops wwan_on_off_pm_ops = {
	.suspend = wwan_on_off_suspend,
	.resume = wwan_on_off_resume,
	.freeze = wwan_on_off_suspend,
	.thaw = wwan_on_off_resume,
	.poweroff = wwan_on_off_poweroff,
	.restore = wwan_on_off_resume,
	};

static struct platform_driver wwan_on_off_driver = {
	.driver.name	= "wwan-on-off",
	.driver.owner	= THIS_MODULE,
	.driver.pm	= &wwan_on_off_pm_ops,
	.driver.of_match_table = of_match_ptr(wwan_of_match),
	.probe		= wwan_on_off_probe,
	.remove		= wwan_on_off_remove,
};

static int __init wwan_on_off_init(void)
{
	printk("%s: wwan_on_off_init\n", __func__);
	return platform_driver_register(&wwan_on_off_driver);
}
module_init(wwan_on_off_init);

static void __exit wwan_on_off_exit(void)
{
	platform_driver_unregister(&wwan_on_off_driver);
}
module_exit(wwan_on_off_exit);


MODULE_ALIAS("wwan_on_off");

MODULE_AUTHOR("Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("3G Modem rfkill and virtual GPIO driver");
MODULE_LICENSE("GPL v2");
