/*
 * Driver for Freescale CR Touch I2C touch controller
 *
 * Copyright (C) 2011-2012 Freescale Semiconductor, Inc.
 * Author: Alison Wang <alison.wang@freescale.com>
 *
 * Copyright (C) 2012,2015 Emcraft Systems
 * Added Qt Embedded support - Alexander Potashev <aspotashev@emcraft.com>
 * Port to latest kernel - Sergei Miroshnichenko <sergeimir@emcraft.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>
#include <linux/syscalls.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#define DEVICE_NAME	"crtouch_drv"

/*keys reported to input device
--capacitive--
keypad:
	crtouch_btn_0 = KEY_A
	crtouch_btn_1 = KEY_B
	crtouch_btn_2 = KEY_C
	crtouch_btn_3 = KEY_D

--resistive--
slide:
	down:  	KEY_H;
	up:   	KEY_G;
	left: 	KEY_F;
	right:	KEY_E;
*/

/*status registers*/
#define STATUS_ERROR				0x00
#define STATUS_REGISTER_1 			0x01
#define STATUS_REGISTER_2 			0x02
#define X_COORDINATE_MSB 			0x03
#define X_COORDINATE_LSB 			0x04
#define Y_COORDINATE_MSB 			0x05
#define Y_COORDINATE_LSB 			0x06
#define PRESSURE_VALUE_MSB 			0x07
#define PRESSURE_VALUE_LSB 			0x08
#define FIFO_STATUS	 			0x09
#define FIFO_X_COORDINATE_MSB 			0x0A
#define FIFO_X_COORDINATE_LSB 			0x0B
#define FIFO_Y_COORDINATE_MSB 			0x0C
#define FIFO_Y_COORDINATE_LSB 			0x0D
#define FIFO_PRESSURE_VALUE_COORDINATE_MSB 	0x0E
#define FIFO_PRESSURE_VALUE_COORDINATE_LSB 	0x0F
#define UART_BAUDRATE_MSB 			0x10
#define UART_BAUDRATE_MID 			0x11
#define UART_BAUDRATE_LSB 			0x12
#define DEVICE_IDENTIFIER_REGISTER 		0x13
#define SLIDE_DISPLACEMENT 			0x14
#define ROTATE_ANGLE	 			0x15
#define ZOOM_SIZE	 			0x16
#define ELECTRODE_STATUS 			0x20
#define FAULTS_NOTE1	 			0x21
#define E0_BASELINE_MSB 			0x22
#define E0_BASELINE_LSB 			0x23
#define E1_BASELINE_MSB 			0x24
#define E1_BASELINE_LSB 			0x25
#define E2_BASELINE_MSB 			0x26
#define E2_BASELINE_LSB 			0x27
#define E3_BASELINE_MSB 			0x28
#define E3_BASELINE_LSB 			0x29
#define E0_INSTANT_DELTA 			0x2A
#define E1_INSTANT_DELTA 			0x2B
#define E2_INSTANT_DELTA 			0x2C
#define E3_INSTANT_DELTA 			0x2D
#define DYNAMIC_STATUS				0x2E
#define STATIC_STATUS				0x2F

/*configuration registers*/
#define CAPACITIVE_ELECTRODES_FIFO		0x30
#define CONFIGURATION 				0x40
#define TRIGGER_EVENTS				0x41
#define FIFO_SETUP				0x42
#define SAMPLING_X_Y				0x43
#define X_SETTLING_TIME_MBS			0x44
#define X_SETTLING_TIME_LBS			0x45
#define Y_SETTLING_TIME_MBS			0x46
#define Y_SETTLING_TIME_LBS			0x47
#define Z_SETTLING_TIME_MBS			0x48
#define Z_SETTLING_TIME_LBS			0x49
#define HORIZONTAL_RESOLUTION_MBS		0x4A
#define HORIZONTAL_RESOLUTION_LBS		0x4B
#define VERTICAL_RESOLUTION_MBS			0x4C
#define VERTICAL_RESOLUTION_LBS			0x4D
#define SLIDE_STEPS				0x4E
#define SYSTEM_CONFIG_NOTE2			0x60
#define DC_TRACKER_RATE				0x61
#define RESPONSE_TIME				0x62
#define STUCK_KEY_TIMEOUT			0x63
#define E0_SENSITIVITY				0x64
#define E1_SENSITIVITY				0x65
#define E2_SENSITIVITY				0x66
#define E3_SENSITIVITY				0x67
#define ELECTRODE_ENABLERS			0x68
#define LOW_POWER_SCAN_PERIOD			0x69
#define LOW_POWER_ELECTRODE			0x6A
#define LOW_POWER_ELECTRODE_SENSITIVITY		0x6B
#define CONTROL_CONFIG				0x6C
#define EVENTS					0x6D
#define AUTO_REPEAT_RATE			0x6E
#define AUTO_REPEAT_START			0x6F
#define MAX_TOUCHES				0x70

/*mask status registers*/
#define MASK_EVENTS				0x7F
#define MASK_PRESSED				0x80
#define MASK_PRESSED_OR_CAPACITIVE_EVT		0x82
#define TWO_TOUCH				0x40
#define MASK_ZOOM_DIRECTION			0x20
#define MASK_ROTATE_DIRECTION			0x10
#define MASK_SLIDE_LEFT				0x04
#define MASK_SLIDE_RIGHT			0x00
#define MASK_SLIDE_DOWN				0x0C
#define MASK_SLIDE_UP				0x08

/*mask resistive*/
#define MASK_RESISTIVE_SAMPLE			0x01
#define MASK_EVENTS_ZOOM_R			0x21
#define MASK_EVENTS_ROTATE_R			0x11
#define MASK_EVENTS_SLIDE_R			0x09

/*mask capacitive*/
#define MASK_EVENTS_CAPACITIVE			0x02
#define MASK_KEYPAD_CONF			0x60
#define MASK_SLIDE_CONF				0x20
#define MASK_ROTARY_CONF			0x10

/*dynamic registers mask*/
#define MASK_DYNAMIC_FLAG			0x80
#define MASK_DYNAMIC_DIRECTION			0x40
#define MASK_DYNAMIC_DISPLACEMENT		0x03
#define MASK_DYNAMIC_DISPLACEMENT_BTN0		0x00
#define MASK_DYNAMIC_DISPLACEMENT_BTN1		0x01
#define MASK_DYNAMIC_DISPLACEMENT_BTN2		0x02
#define MASK_DYNAMIC_DISPLACEMENT_BTN3		0x03

#define SHUTDOWN_CRICS 				0x01
#define START_CALIBRATION 			0x40
#define READ_AND_WRITE 				0666
#define KEY_NUMBER 				0x03
#define EV_TYPE					0x80
#define LEN_RESOLUTION_BYTES			0x04
#define LEN_XY					0x04
#define MAX_ZOOM_CRICS				0xFF
#define SIZE_OF_SIN_COS				0xB4
#define TRIGGUER_TOUCH_RELEASE			0x83

#define CRICS_TOUCHED				0x01
#define SLEEP_MODE				0x02
#define CRICS_RELEASED				0x00
#define SET_TRIGGER_RESISTIVE			0x81
#define CLEAN_SLIDE_EVENTS			0xEF
#define SET_MULTITOUCH				0x0C

#define IRQ_NAME	"CRTOUCH_IRQ"
#define DEV_NAME 	"crtouch_dev"

#define XMIN	0
#define YMIN	0

#define DEFAULT_POLLING_PERIOD	20

struct crtouch_data {
	int x;
	int y;
	struct workqueue_struct *workqueue;
	struct work_struct 	work;
	struct input_dev 	*input_dev;
	struct i2c_client 	*client;
	int is_capacitive;
	int polling_period;
	struct timer_list tsc_poll_timer;
	s32 configuration;
	int xmax;
	int ymax;
	bool status_pressed;
};

void report_single_touch(struct crtouch_data *crtouch, int x, int y)
{
	input_event(crtouch->input_dev, EV_ABS, ABS_X, x);
	input_event(crtouch->input_dev, EV_ABS, ABS_Y, y);
	input_event(crtouch->input_dev, EV_KEY, BTN_TOUCH, 1);
	input_report_abs(crtouch->input_dev, ABS_PRESSURE, 1);
	input_sync(crtouch->input_dev);

	crtouch->status_pressed = CRICS_TOUCHED;
}

void free_touch(struct crtouch_data *crtouch)
{
	input_event(crtouch->input_dev, EV_KEY, BTN_TOUCH, 0);
	input_report_abs(crtouch->input_dev, ABS_PRESSURE, 0);
	input_sync(crtouch->input_dev);

	crtouch->status_pressed = CRICS_RELEASED;
}

static void report_MT(struct work_struct *work)
{
	struct crtouch_data *crtouch = container_of(work, struct crtouch_data, work);
	struct i2c_client *client = crtouch->client;
	s32 status = i2c_smbus_read_byte_data(client, STATUS_REGISTER_1);
	s32 dynamic_status = i2c_smbus_read_byte_data(client, DYNAMIC_STATUS);

	bool capacitive_event = (status & MASK_EVENTS_CAPACITIVE) == MASK_EVENTS_CAPACITIVE;
	bool resistive_event = (status & MASK_RESISTIVE_SAMPLE) == MASK_RESISTIVE_SAMPLE;

	if (crtouch->is_capacitive && capacitive_event) {
		if ((crtouch->configuration & MASK_KEYPAD_CONF) == MASK_KEYPAD_CONF) {
			s32 fifo_capacitive;
			while ((fifo_capacitive =
				i2c_smbus_read_byte_data(client, CAPACITIVE_ELECTRODES_FIFO)) < 0xFF) {
				if ((fifo_capacitive & KEY_NUMBER) == MASK_DYNAMIC_DISPLACEMENT_BTN3) {
					if ((fifo_capacitive & EV_TYPE) == 0) {
						input_report_key(crtouch->input_dev, KEY_D, 1);
						input_sync(crtouch->input_dev);
					} else {
						input_report_key(crtouch->input_dev, KEY_D, 0);
						input_sync(crtouch->input_dev);
					}
				} else if ((fifo_capacitive & KEY_NUMBER) == MASK_DYNAMIC_DISPLACEMENT_BTN2) {
					if ((fifo_capacitive & EV_TYPE) == 0) {
						input_report_key(crtouch->input_dev, KEY_C, 1);
						input_sync(crtouch->input_dev);
					} else {
						input_report_key(crtouch->input_dev, KEY_C, 0);
						input_sync(crtouch->input_dev);
					}
				} else if ((fifo_capacitive & KEY_NUMBER) == MASK_DYNAMIC_DISPLACEMENT_BTN1) {
					if ((fifo_capacitive & EV_TYPE) == 0) {
						input_report_key(crtouch->input_dev, KEY_B, 1);
						input_sync(crtouch->input_dev);
					} else {
						input_report_key(crtouch->input_dev, KEY_B, 0);
						input_sync(crtouch->input_dev);
					}
				} else if ((fifo_capacitive & KEY_NUMBER) == MASK_DYNAMIC_DISPLACEMENT_BTN0) {
					if ((fifo_capacitive & EV_TYPE) == 0) {
						input_report_key(crtouch->input_dev, KEY_A, 1);
						input_sync(crtouch->input_dev);
					} else {
						input_report_key(crtouch->input_dev, KEY_A, 0);
						input_sync(crtouch->input_dev);
					}
				}
			}
		} else if ((crtouch->configuration & MASK_SLIDE_CONF) == MASK_SLIDE_CONF) {
			if (dynamic_status & MASK_DYNAMIC_FLAG) {
				if (dynamic_status & MASK_DYNAMIC_DIRECTION) {
					input_report_key(crtouch->input_dev, KEY_I, 1);
					input_report_key(crtouch->input_dev, KEY_I, 0);
					input_sync(crtouch->input_dev);
				} else {
					input_report_key(crtouch->input_dev, KEY_J, 1);
					input_report_key(crtouch->input_dev, KEY_J, 0);
					input_sync(crtouch->input_dev);
				}
			}
		} else if ((crtouch->configuration & MASK_ROTARY_CONF) == MASK_ROTARY_CONF) {
			if (dynamic_status & MASK_DYNAMIC_FLAG) {
				if (dynamic_status & MASK_DYNAMIC_DIRECTION) {
					input_report_key(crtouch->input_dev, KEY_K, 1);
					input_report_key(crtouch->input_dev, KEY_K, 0);
					input_sync(crtouch->input_dev);
				} else {
					input_report_key(crtouch->input_dev, KEY_L, 1);
					input_report_key(crtouch->input_dev, KEY_L, 0);
					input_sync(crtouch->input_dev);
				}
			}
		}
	} /* capacitive_event */

	if (resistive_event) {
		if (!(status & MASK_PRESSED) && crtouch->status_pressed) {
			free_touch(crtouch);
		} else {
			char xy[LEN_XY];
			int result = i2c_smbus_read_i2c_block_data(client, X_COORDINATE_MSB, LEN_XY, xy);

			if (result >= 0) {
				int x = xy[1] | (xy[0] << 8);
				int y = xy[3] | (xy[2] << 8);

				if (crtouch->x != x || crtouch->y != y) {
					crtouch->x = x;
					crtouch->y = y;
					report_single_touch(crtouch, x, y);
				}
			}
		}
	}
}

static irqreturn_t crtouch_irq(int irq, void *dev)
{
	struct crtouch_data *crtouch = dev_get_drvdata(dev);
	queue_work(crtouch->workqueue, &crtouch->work);
	return IRQ_HANDLED;
}

static void tsc_poll_handler(struct timer_list *arg)
{
/* FIXME: how do we pass the private data through timer_list? */
	struct crtouch_data *crtouch = (struct crtouch_data *)arg;

	queue_work(crtouch->workqueue, &crtouch->work);

	crtouch->tsc_poll_timer.expires += crtouch->polling_period;
	add_timer(&crtouch->tsc_poll_timer);
}

int read_resolution(struct crtouch_data *crtouch)
{
	char resolution[LEN_RESOLUTION_BYTES];
	int horizontal, vertical, ret;

	ret = i2c_smbus_read_i2c_block_data(crtouch->client,
		HORIZONTAL_RESOLUTION_MBS, LEN_RESOLUTION_BYTES, resolution);

	if (ret < 0)
		return ret;

	horizontal = resolution[1];
	horizontal |= resolution[0] << 8;
	vertical = resolution[3];
	vertical |= resolution[2] << 8;

	crtouch->xmax = horizontal;
	crtouch->ymax = vertical;

	return 0;
}

static int crtouch_probe(struct i2c_client *client)
{
	int result = 0;
	struct input_dev *input_dev;
	int error = 0;
	s32 mask_trigger = 0;
	struct device_node *node = client->dev.of_node;
	struct crtouch_data *crtouch;

	crtouch = devm_kzalloc(&client->dev, sizeof(*crtouch), GFP_KERNEL);

	if (!crtouch)
		return -ENOMEM;

	input_dev = devm_input_allocate_device(&client->dev);
	if (!input_dev) {
		result = -ENOMEM;
		goto err_free_mem;
	}

	crtouch->input_dev = input_dev;
	crtouch->client = client;
	i2c_set_clientdata(client, crtouch);
	crtouch->workqueue = create_singlethread_workqueue("crtouch");
	INIT_WORK(&crtouch->work, report_MT);

	if (crtouch->workqueue == NULL) {
		dev_err(&client->dev, "failed to create workqueue\n");
		result = -ENOMEM;
		goto err_wqueue;
	}

	crtouch->is_capacitive = of_property_read_bool(node, "is-capacitive");

	error = read_resolution(crtouch);
	if (error < 0) {
		dev_err(&client->dev, "failed to read size of screen\n");
		result = -EIO;
		goto err_free_wq;
	}

	crtouch->configuration = i2c_smbus_read_byte_data(client, CONFIGURATION);
	if (crtouch->configuration < 0) {
		dev_err(&client->dev, "failed to read CRTOUCH configuration\n");
		goto err_free_wq;
	}
	crtouch->configuration &= CLEAN_SLIDE_EVENTS;
	if (crtouch->configuration & SLEEP_MODE) {
		dev_info(&client->dev, "Sleep Mode is not supported, disabling\n");
		crtouch->configuration &= ~SLEEP_MODE;
	}
	crtouch->configuration &= ~SET_MULTITOUCH;
	i2c_smbus_write_byte_data(client, CONFIGURATION, crtouch->configuration);

	mask_trigger = i2c_smbus_read_byte_data(client, TRIGGER_EVENTS);
	mask_trigger |= SET_TRIGGER_RESISTIVE;
	i2c_smbus_write_byte_data(client, TRIGGER_EVENTS, mask_trigger);

	crtouch->input_dev->name = "CRTOUCH Input Device";
	crtouch->input_dev->id.bustype = BUS_I2C;

	__set_bit(EV_ABS, crtouch->input_dev->evbit);
	__set_bit(EV_KEY, crtouch->input_dev->evbit);
	__set_bit(BTN_TOUCH, crtouch->input_dev->keybit);
	__set_bit(ABS_X, crtouch->input_dev->absbit);
	__set_bit(ABS_Y, crtouch->input_dev->absbit);
	__set_bit(ABS_PRESSURE, crtouch->input_dev->absbit);
	__set_bit(EV_SYN, crtouch->input_dev->evbit);

	/*register keys that will be reported by crtouch*/
	__set_bit(KEY_A, crtouch->input_dev->keybit);
	__set_bit(KEY_B, crtouch->input_dev->keybit);
	__set_bit(KEY_C, crtouch->input_dev->keybit);
	__set_bit(KEY_D, crtouch->input_dev->keybit);
	__set_bit(KEY_E, crtouch->input_dev->keybit);
	__set_bit(KEY_F, crtouch->input_dev->keybit);
	__set_bit(KEY_G, crtouch->input_dev->keybit);
	__set_bit(KEY_H, crtouch->input_dev->keybit);
	__set_bit(KEY_I, crtouch->input_dev->keybit);
	__set_bit(KEY_J, crtouch->input_dev->keybit);
	__set_bit(KEY_K, crtouch->input_dev->keybit);
	__set_bit(KEY_L, crtouch->input_dev->keybit);

	input_set_abs_params(crtouch->input_dev, ABS_X, XMIN, crtouch->xmax, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_Y, YMIN, crtouch->ymax, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_MT_POSITION_X,
				XMIN, crtouch->xmax, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_MT_POSITION_Y,
				YMIN, crtouch->ymax, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_MT_TOUCH_MAJOR, 0, 1, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_MT_TRACKING_ID, 0, 1, 0, 0);

	input_set_abs_params(crtouch->input_dev, ABS_PRESSURE, 0, 1, 0, 0);
	input_set_drvdata(crtouch->input_dev, crtouch);
	dev_info(&client->dev, "CR-TOUCH max values X: %d Y: %d\n", crtouch->xmax, crtouch->ymax);

	result = input_register_device(crtouch->input_dev);
	if (result)
		goto err_unr_dev;

	if (client->irq) {
		result = devm_request_irq(&client->dev, client->irq, crtouch_irq,
					  IRQF_TRIGGER_FALLING, IRQ_NAME, &client->dev);

		device_init_wakeup(&client->dev, 1);
	}

	if (!client->irq || result) {
		if (of_property_read_u32(node, "polling-period-ms", &crtouch->polling_period))
			crtouch->polling_period = DEFAULT_POLLING_PERIOD;
		crtouch->polling_period = msecs_to_jiffies(crtouch->polling_period);
		timer_setup(&crtouch->tsc_poll_timer, &tsc_poll_handler, 0);
		crtouch->tsc_poll_timer.function	= tsc_poll_handler;
		crtouch->tsc_poll_timer.expires	= jiffies + crtouch->polling_period;
/* FIXME:		crtouch->tsc_poll_timer.data		= (unsigned long)crtouch; */
		add_timer(&crtouch->tsc_poll_timer);
		dev_dbg(&client->dev, "Configured to poll every %d ms\n", crtouch->polling_period);
	}

	/*clean interrupt pin*/
	i2c_smbus_read_byte_data(client, STATUS_REGISTER_1);

	return 0;

err_unr_dev:
	input_unregister_device(crtouch->input_dev);
err_free_wq:
	destroy_workqueue(crtouch->workqueue);
err_wqueue:
	input_free_device(crtouch->input_dev);
err_free_mem:
	devm_kfree(&client->dev, crtouch);
	return result;
}

#ifdef CONFIG_PM_SLEEP
static int crtouch_resume(struct device *dev)
{
	struct crtouch_data *crtouch = dev_get_drvdata(dev);
	struct i2c_client *client = crtouch->client;

	if (client->irq) {
		if (device_may_wakeup(&client->dev))
			disable_irq_wake(client->irq);
		else
			enable_irq(client->irq);
	}

	return 0;
}

static int crtouch_suspend(struct device *dev)
{
	struct crtouch_data *crtouch = dev_get_drvdata(dev);
	struct i2c_client *client = crtouch->client;
	s32 data_to_read;

	data_to_read = i2c_smbus_read_byte_data(client, CONFIGURATION);
	data_to_read |= SHUTDOWN_CRICS;
	i2c_smbus_write_byte_data(client, CONFIGURATION , data_to_read);

	if (client->irq) {
		if (device_may_wakeup(&client->dev))
			enable_irq_wake(client->irq);
		else
			disable_irq(client->irq);
	}

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static const struct dev_pm_ops crtouch_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(crtouch_suspend, crtouch_resume)
};

static const struct i2c_device_id crtouch_idtable[] = {
	{ DEVICE_NAME, 0},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, crtouch_idtable);

static const struct of_device_id crtouch_of_match[] = {
	{ .compatible = "fsl,crtouch" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, crtouch_of_match);

static struct i2c_driver crtouch_i2c_driver = {
	.driver = {
		.owner		= THIS_MODULE,
		.name		= DEVICE_NAME,
		.pm		= &crtouch_pm_ops,
		.of_match_table	= crtouch_of_match,
	},
	.id_table	= crtouch_idtable,
	.probe		= crtouch_probe,
};
module_i2c_driver(crtouch_i2c_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("CR-TOUCH multitouch driver");
MODULE_LICENSE("GPL");
