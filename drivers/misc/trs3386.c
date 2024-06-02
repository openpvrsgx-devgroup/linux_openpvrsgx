/*
 * trs3386.c
 * Driver for controlling modem status of trs3386 level shifter
 * chip through gpio(s).
 *
 * forwards the mctrl status changes to gpios that drive the
 * TRS3386 chip.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/serial_core.h>
#include <linux/of_gpio.h>

/* this defines the indexes in the gpio list and their
   mapping to mctrl bits */

enum {
	GPIO_DTR = 0,
	GPIO_RTS,
	GPIO_DSR,
	GPIO_RI,
	GPIO_OUT1,
	GPIO_OUT2,
	NUMBER_OF_GPIOS
};

static int tiocm_mask[] = {
	[GPIO_DTR] = TIOCM_DTR,
	[GPIO_RTS] = TIOCM_RTS,
	[GPIO_DSR] = TIOCM_DSR,
	[GPIO_RI] = TIOCM_RI,
	[GPIO_OUT1] = TIOCM_OUT1,
	[GPIO_OUT2] = TIOCM_OUT2,
};

struct trs_data {
	int gpios[NUMBER_OF_GPIOS];
	int gpioflags[NUMBER_OF_GPIOS];
	struct uart_port *uart;	/* the drvdata of the uart or tty */
};

/* called by uart modem control line changes (e.g. DTR) */

static void trs_mctrl(void *pdata, int val)
{
	int i;
	struct trs_data *data = (struct trs_data *) pdata;

	pr_debug("%s(...,%x)\n", __func__, val);

	/* change gpio status */

	for (i=0; i<NUMBER_OF_GPIOS; i++)
		if(gpio_is_valid(data->gpios[i])) {
			bool on = (val & tiocm_mask[i]) != 0;

			if (data->gpioflags[i] & GPIOF_ACTIVE_LOW)
				on = !on;
			gpio_set_value_cansleep(data->gpios[i], on);
		}
}

static int trs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct trs_data *data;
	int i;

	pr_debug("%s()\n", __func__);

	if (!pdev->dev.of_node) {
		dev_err(&pdev->dev, "No device tree data\n");
		return EINVAL;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	for (i=0; i<NUMBER_OF_GPIOS; i++) {
		enum of_gpio_flags flags;

		data->gpios[i] = of_get_named_gpio_flags(dev->of_node,
					"gpios", i,
					&flags);

		if (data->gpios[i] == -EPROBE_DEFER)
			return -EPROBE_DEFER;	/* defer until we have all gpios */
		data->gpioflags[i] = flags;
	}

	data->uart = devm_serial_get_uart_by_phandle(&pdev->dev, "uart", 0);

	if (!data->uart) {
		dev_err(&pdev->dev, "No UART link\n");
		return -EINVAL;
	}

	if (IS_ERR(data->uart)) {
		if (PTR_ERR(data->uart) == -EPROBE_DEFER)
			return -EPROBE_DEFER;	/* we can't probe yet */
		data->uart = NULL;	/* no UART */
	}


	for (i=0; i<NUMBER_OF_GPIOS; i++)
		if(gpio_is_valid(data->gpios[i]))
			devm_gpio_request(&pdev->dev, data->gpios[i], "mctrl-gpio");

	uart_register_slave(data->uart, data);
	uart_register_mctrl_notification(data->uart, trs_mctrl);

	platform_set_drvdata(pdev, data);

	pr_debug("trs3386 probed\n");

	return 0;
}

static int trs_remove(struct platform_device *pdev)
{
	struct trs_data *data = platform_get_drvdata(pdev);

	uart_register_slave(data->uart, NULL);

	return 0;
}

static const struct of_device_id trs_of_match[] = {
	{ .compatible = "ti,trs3386" },
	{},
};
MODULE_DEVICE_TABLE(of, trs_of_match);

static struct platform_driver trs_driver = {
	.probe		= trs_probe,
	.remove		= trs_remove,
	.driver = {
		.name	= "trs3386",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(trs_of_match)
	},
};

module_platform_driver(trs_driver);

MODULE_ALIAS("trs3386");

MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("trs3386 modem status line management driver");
MODULE_LICENSE("GPL v2");
