/*
 * Copyright (C) 2011-2012 Freescale Semiconductor, Inc.
 *
 * Modified to support the touchscreen on Freescale TWR-LCD-RGB with Qt Embedded
 * Copyright (c) 2012,2015
 * Alexander Potashev, Emcraft Systems, aspotashev@emcraft.com
 * Sergei Miroshnichenko, Emcraft Systems, sergeimir@emcraft.com
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
#include <linux/cdev.h>
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

slide:
	incremental: KEY_I
	decremental: KEY_J
rotate:
	incremental: KEY_K
	decremental: KEY_L

--resistive--
slide:
	down:  	KEY_H;
	up:   	KEY_G;
	left: 	KEY_F;
	right:	KEY_E;
*/

/*gestures*/
#define ZOOM_IN 		1
#define ZOOM_OUT		2
#define ROTATE_CLK 		7
#define ROTATE_COUNTER_CLK 	8
#define SINGLE_TOUCH		3

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

/*private struct crtouch*/
struct crtouch_data {
	int x1;
	int x2;
	int y1;
	int y2;
	int x1_old;
	int y1_old;
	struct workqueue_struct *workqueue;
	struct work_struct 	work;
	struct input_dev 	*input_dev;
	struct i2c_client 	*client;
	int is_multitouch;
	int is_gestures;
	int is_capacitive;
	int wake_gpio;
	int pending_gpio;
	int polling_period;
	int have_pressure_event;
};

int xmax = 0;
int ymax = 0;

s32 data_configuration = 0;

struct crtouch_data *crtouch;
struct i2c_client *client_public;
struct class *crtouch_class;
dev_t dev_number;
static struct cdev crtouch_cdev;
bool status_pressed = 0;

s32 rotate_angle = 0;
int rotate_state = 0;
int last_angle = 0;

int sin_data[SIZE_OF_SIN_COS] = 	{
	1143,  2287,  3429,  4571,  5711,  6850,  7986,  9120,  10252, 11380,
	12504, 13625, 14742, 15854, 16961, 18064, 19160, 20251, 21336, 22414,
	23486, 24550, 25606, 26655, 27696, 28729, 29752, 30767, 31772, 32768,
	33753, 34728, 35693, 36647, 37589, 38521, 39440, 40347, 41243, 42125,
	42995, 43852, 44695, 45525, 46340, 47142, 47929, 48702, 49460, 50203,
	50931, 51643, 52339, 53019, 53683, 54331, 54963, 55577, 56175, 56755,
	57319, 57864, 58393, 58903, 59395, 59870, 60326, 60763, 61183, 61583,
	61965, 62328, 62672, 62997, 63302, 63589, 63856, 64103, 64331, 64540,
	64729, 64898, 65047, 65176, 65286, 65376, 65446, 65496, 65526, 65535,
	65526, 65496, 65446, 65376, 65286, 65176, 65047, 64898, 64729, 64540,
	64331, 64103, 63856, 63589, 63302, 62997, 62672, 62328, 61965, 61583,
	61183, 60763, 60326, 59870, 59395, 58903, 58393, 57864, 57319, 56755,
	56175, 55577, 54963, 54331, 53683, 53019, 52339, 51643, 50931, 50203,
	49460, 48702, 47929, 47142, 46340, 45525, 44695, 43852, 42995, 42125,
	41243, 40347, 39440, 38521, 37589, 36647, 35693, 34728, 33753, 32768,
	31772, 30767, 29752, 28798, 27696, 26655, 25606, 24550, 23486, 22414,
	21336, 20251, 19160, 18064, 16961, 15854, 14742, 13625, 12504, 11380,
	10252, 9120,  7986,  6850,  5711,  4571,  3429,  2287,  1143,  65,
};

int cos_data[SIZE_OF_SIN_COS] = 	{
	65526, 65496, 65446, 65376, 65286, 65176, 65047, 64898, 64729, 64540,
	64331, 64103, 63856, 63589, 63302, 62997, 62672, 62328, 61965, 61583,
	61183, 60763, 60326, 59870, 59395, 58903, 58393, 57864, 57319, 56755,
	56175, 55577, 54963, 54331, 53683, 53019, 52339, 51643, 50931, 50203,
	49460, 48702, 47929, 47142, 46340, 45525, 44695, 43852, 42995, 42125,
	41243, 40347, 39440, 38521, 37589, 36647, 35693, 34728, 33753, 32768,
	31772, 30767, 29752, 28798, 27696, 26655, 25606, 24550, 23486, 22414,
	21336, 20251, 19160, 18064, 16961, 15854, 14742, 13625, 12504, 11380,
	10252, 9120,  7986,  6850,  5711,  4571,  3429,  2287,  1143,  65,
	-1143, -2287, -3429, -4571, -5711, -6850, -7986, -9120, -10252, -11380,
	-12504, -13625, -14742, -15854, -16961, -18064, -19160, -20251, -21336, -22414,
	-23486, -24550, -25606, -26655, -27696, -28729, -29752, -30767, -31772, -32768,
	-33753, -34728, -35693, -36647, -37589, -38521, -39440, -40347, -41243, -42125,
	-42995, -43852, -44695, -45525, -46340, -47142, -47929, -48702, -49460, -50203,
	-50931, -51643, -52339, -53019, -53683, -54331, -54963, -55577, -56175, -56755,
	-57319, -57864, -58393, -58903, -59395, -59870, -60326, -60763, -61183, -61583,
	-61965, -62328, -62672, -62997, -63302, -63589, -63856, -64103, -64331, -64540,
	-64729, -64898, -65047, -65176, -65286, -65376, -65446, -65496, -65526, -65535,
};

void report_single_touch(int multitouch, int pressure_event)
{
	if (multitouch) {
		input_report_abs(crtouch->input_dev, ABS_MT_POSITION_X, crtouch->x1);
		input_report_abs(crtouch->input_dev, ABS_MT_POSITION_Y, crtouch->y1);
		input_report_abs(crtouch->input_dev, ABS_MT_TOUCH_MAJOR, 1);
		input_mt_sync(crtouch->input_dev);
	}

	input_event(crtouch->input_dev, EV_ABS, ABS_X, crtouch->x1);
	input_event(crtouch->input_dev, EV_ABS, ABS_Y, crtouch->y1);
	input_event(crtouch->input_dev, EV_KEY, BTN_TOUCH, 1);

	if (pressure_event)
		input_report_abs(crtouch->input_dev, ABS_PRESSURE, 1);

	input_sync(crtouch->input_dev);

	status_pressed = CRICS_TOUCHED;
}

void report_multi_touch(void)
{
	input_report_key(crtouch->input_dev, ABS_MT_TRACKING_ID, 0);
	input_report_abs(crtouch->input_dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(crtouch->input_dev, ABS_MT_POSITION_X, crtouch->x1);
	input_report_abs(crtouch->input_dev, ABS_MT_POSITION_Y, crtouch->y1);
	input_mt_sync(crtouch->input_dev);

	input_report_key(crtouch->input_dev, ABS_MT_TRACKING_ID, 1);
	input_report_abs(crtouch->input_dev, ABS_MT_TOUCH_MAJOR, 1);
	input_report_abs(crtouch->input_dev, ABS_MT_POSITION_X, crtouch->x2);
	input_report_abs(crtouch->input_dev, ABS_MT_POSITION_Y, crtouch->y2);
	input_mt_sync(crtouch->input_dev);

	input_sync(crtouch->input_dev);

	status_pressed = CRICS_TOUCHED;
}

void free_touch(int multitouch, int have_pressure_event)
{
	if (multitouch) {
		input_event(crtouch->input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(crtouch->input_dev);
	}

	input_event(crtouch->input_dev, EV_KEY, BTN_TOUCH, 0);

	if (have_pressure_event)
		input_report_abs(crtouch->input_dev, ABS_PRESSURE, 0);

	input_sync(crtouch->input_dev);

	status_pressed = CRICS_RELEASED;
}

void free_two_touch(void){

	input_event(crtouch->input_dev, EV_ABS, ABS_MT_TOUCH_MAJOR, 0);
	input_mt_sync(crtouch->input_dev);
	input_sync(crtouch->input_dev);

}

int read_resolution(void)
{
	char resolution[LEN_RESOLUTION_BYTES];
	int horizontal, vertical, ret;

	ret = i2c_smbus_read_i2c_block_data(client_public,
		HORIZONTAL_RESOLUTION_MBS, LEN_RESOLUTION_BYTES, resolution);

	if (ret < 0)
		return ret;

	horizontal = resolution[1];
	horizontal |= resolution[0] << 8;
	vertical = resolution[3];
	vertical |= resolution[2] << 8;

	xmax = horizontal;
	ymax = vertical;

	return 0;
}

static void report_MT(struct work_struct *work)
{
	struct crtouch_data *crtouch = container_of(work, struct crtouch_data, work);
	struct i2c_client *client = crtouch->client;

	int result;
	char xy[LEN_XY];
	s32 status_register_1 = 0;

	s32 status_register_2 = 0;
	s32 rotate_angle_help = 0;
	int command = 0;
	int degrees = 0;
	int zoom_value_moved = 0;

	s32 dynamic_status = 0;
	s32 fifo_capacitive = 0;

	status_register_1 = i2c_smbus_read_byte_data(client, STATUS_REGISTER_1);

	if (crtouch->is_gestures) {
		if ((status_register_1 & MASK_EVENTS_ZOOM_R) == MASK_EVENTS_ZOOM_R && (status_register_1 & TWO_TOUCH)) {
			status_register_2 = i2c_smbus_read_byte_data(client, STATUS_REGISTER_2);

			if ((status_register_2 & MASK_ZOOM_DIRECTION) == MASK_ZOOM_DIRECTION)
				command = ZOOM_OUT;
			else
				command = ZOOM_IN;
		} else if ((status_register_1 & MASK_EVENTS_ROTATE_R) == MASK_EVENTS_ROTATE_R  && (status_register_1 & TWO_TOUCH)) {
			status_register_2 = i2c_smbus_read_byte_data(client, STATUS_REGISTER_2);
			rotate_angle = i2c_smbus_read_byte_data(client, ROTATE_ANGLE);
			rotate_angle_help = rotate_angle;

			if ((status_register_2 & MASK_ROTATE_DIRECTION) == MASK_ROTATE_DIRECTION) {
				command = ROTATE_COUNTER_CLK;

				if (rotate_state == ROTATE_CLK)
					last_angle = 0;

				rotate_state = ROTATE_COUNTER_CLK;
				rotate_angle -= last_angle;
				last_angle += rotate_angle;
			} else {
				command = ROTATE_CLK;

				if (rotate_state == ROTATE_COUNTER_CLK)
					last_angle = 0;

				rotate_state = ROTATE_CLK;
				rotate_angle -= last_angle;
				last_angle += rotate_angle;
			}
		}

		if ((status_register_1 & MASK_EVENTS_SLIDE_R) == MASK_EVENTS_SLIDE_R) {
			status_register_2 = i2c_smbus_read_byte_data(client, STATUS_REGISTER_2);

			if ((status_register_2 & MASK_SLIDE_DOWN) == MASK_SLIDE_DOWN) {
				input_report_key(crtouch->input_dev, KEY_H, 1);
				input_report_key(crtouch->input_dev, KEY_H, 0);
				input_sync(crtouch->input_dev);
			} else if ((status_register_2 & MASK_SLIDE_UP) == MASK_SLIDE_UP) {
				input_report_key(crtouch->input_dev, KEY_G, 1);
				input_report_key(crtouch->input_dev, KEY_G, 0);
				input_sync(crtouch->input_dev);
			} else if ((status_register_2 & MASK_SLIDE_LEFT) == MASK_SLIDE_LEFT) {
				input_report_key(crtouch->input_dev, KEY_F, 1);
				input_report_key(crtouch->input_dev, KEY_F, 0);
				input_sync(crtouch->input_dev);
			} else if ((status_register_2 & MASK_SLIDE_RIGHT) == MASK_SLIDE_RIGHT) {
				input_report_key(crtouch->input_dev, KEY_E, 1);
				input_report_key(crtouch->input_dev, KEY_E, 0);
				input_sync(crtouch->input_dev);
			}
		}
	} /* is_gestures */

	if (crtouch->is_capacitive && ((status_register_1 & MASK_EVENTS_CAPACITIVE) == MASK_EVENTS_CAPACITIVE)) {
		if ((data_configuration & MASK_KEYPAD_CONF) == MASK_KEYPAD_CONF) {
			while ((fifo_capacitive = i2c_smbus_read_byte_data(client, CAPACITIVE_ELECTRODES_FIFO)) < 0xFF) {
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
		} else if ((data_configuration & MASK_SLIDE_CONF) == MASK_SLIDE_CONF) {
			dynamic_status = i2c_smbus_read_byte_data(client, DYNAMIC_STATUS);

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
		} else if ((data_configuration & MASK_ROTARY_CONF) == MASK_ROTARY_CONF) {
			dynamic_status = i2c_smbus_read_byte_data(client, DYNAMIC_STATUS);

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
	} /* is_capacitive */

	if ((status_register_1 & MASK_RESISTIVE_SAMPLE) == MASK_RESISTIVE_SAMPLE
	    && (!crtouch->is_multitouch || !(status_register_1 & TWO_TOUCH))) {
		/*clean zoom data when release 2touch*/
		last_angle = 0;
		rotate_state = 0;

		if (!(status_register_1 & MASK_PRESSED)) {
			if (status_pressed)
				free_touch(crtouch->is_multitouch, crtouch->have_pressure_event);
		} else {
			result = i2c_smbus_read_i2c_block_data(client, X_COORDINATE_MSB, LEN_XY, xy);

			if (result >= 0) {
				crtouch->x1 = xy[1];
				crtouch->x1 |= xy[0] << 8;
				crtouch->y1 = xy[3];
				crtouch->y1 |= xy[2] << 8;

				if (crtouch->x1_old != crtouch->x1 || crtouch->y1_old != crtouch->y1) {
					crtouch->x1_old = crtouch->x1;
					crtouch->y1_old = crtouch->y1;
					report_single_touch(crtouch->is_multitouch, crtouch->have_pressure_event);
				}
			}
		}
	}

	if (crtouch->is_gestures && command) {
		if (command == ZOOM_IN || command == ZOOM_OUT) {
			free_two_touch();

			switch (command) {
			case ZOOM_IN:
				msleep(1);
				/*simulate initial points*/
				crtouch->x1 = (xmax/2) - (xmax/20);
				crtouch->x2 = (xmax/2) + (xmax/20);
				crtouch->y1 = ymax/2;
				crtouch->y2 = ymax/2;

				report_multi_touch();

				zoom_value_moved = (xmax/100);

				if (zoom_value_moved > 0) {
					msleep(1);
					crtouch->x1 -= zoom_value_moved;
					crtouch->x2 += zoom_value_moved;
					report_multi_touch();
					msleep(1);
					crtouch->x1 -= zoom_value_moved;
					crtouch->x2 += zoom_value_moved;
					report_multi_touch();

				} else {
					msleep(1);
					crtouch->x1 -= 5;
					crtouch->x2 += 5;
					report_multi_touch();
					msleep(1);
					crtouch->x1 -= 5;
					crtouch->x2 += 5;
					report_multi_touch();
				}
				break;

			case ZOOM_OUT:
				msleep(1);
				crtouch->x1 = ((xmax * 43) / 100);
				crtouch->x2 = ((xmax * 57) / 100);
				crtouch->y1 = ymax/2;
				crtouch->y2 = ymax/2;
				report_multi_touch();

				zoom_value_moved = (xmax / 100);

				if (zoom_value_moved > 0) {
					msleep(1);
					crtouch->x1 += zoom_value_moved;
					crtouch->x2 -= zoom_value_moved;
					report_multi_touch();
					msleep(1);
					crtouch->x1 += zoom_value_moved;
					crtouch->x2 -= zoom_value_moved;
					report_multi_touch();
				} else {
					msleep(1);
					crtouch->x1 += 5;
					crtouch->x2 -= 5;
					report_multi_touch();
					msleep(1);
					crtouch->x1 += 5;
					crtouch->x2 -= 5;
					report_multi_touch();
				}
				break;
			}
		} else if (command == ROTATE_CLK || command == ROTATE_COUNTER_CLK) {
			free_two_touch();

			/*get radians from crtouch and change them to degrees*/
			degrees = ((rotate_angle * 360) / (64 * 3));

			if (degrees < 0) {
				/*fix if negative values appear, because of time sincronization*/
				degrees = rotate_angle_help;
				last_angle = rotate_angle_help;
			}

			/*simulate initial points*/
			crtouch->x1 = xmax/2;
			crtouch->y1 = ymax/2;
			crtouch->x2 = (xmax/2) + (xmax);
			crtouch->y2 = (ymax/2);
			report_multi_touch();

			if (command == ROTATE_CLK) {
				if (degrees <= 180) {
					crtouch->x2 = (xmax/2) + (((cos_data[degrees-1]*(xmax))/65536));
					crtouch->y2 = (ymax/2) + (((sin_data[degrees-1]*(ymax))/65536));
				} else if (degrees <= 360) {
					crtouch->x2 = (xmax/2) + (((~cos_data[(degrees-181)])*(xmax))/65536);
					crtouch->y2 = (ymax/2) + ((((~sin_data[(degrees-181)])*(ymax))/65536));
				} else if (degrees <= 540) {
					crtouch->x2 = (xmax/2) + (((cos_data[degrees-361]*(xmax))/65536));
					crtouch->y2 = (ymax/2) + (((sin_data[degrees-361]*(ymax))/65536));
				}
				report_multi_touch();
			} else if (command == ROTATE_COUNTER_CLK) {
				if (degrees <= 180) {
					crtouch->x2 = (xmax/2) - (((cos_data[180 - degrees]*(xmax))/65536));
					crtouch->y2 = (ymax/2) - (((sin_data[180 - degrees]*(ymax))/65536));
				} else if (degrees <= 360) {
					crtouch->x2 = (xmax/2) - (((~cos_data[360 - degrees]*(xmax))/65536));
					crtouch->y2 = (ymax/2) - (((~sin_data[360 - degrees]*(ymax))/65536));
				} else if (degrees <= 540) {
					crtouch->x2 = (xmax/2) - (((cos_data[540 - degrees]*(xmax))/65536));
					crtouch->y2 = (ymax/2) - (((sin_data[540 - degrees]*(ymax))/65536));
				}
				report_multi_touch();
			}
		}

		/*clean command*/
		command = 0;
	} /* is_gestures */
}

int crtouch_open(struct inode *inode, struct file *filp)
{
	/*Do nothing*/

	return 0;
}

/*read crtouch registers*/
static ssize_t crtouch_read(struct file *filep,
			char *buf, size_t count, loff_t *fpos)
{
	s32 data_to_read;

	if (buf != NULL) {
		switch (*buf) {

		case STATUS_ERROR:
		case STATUS_REGISTER_1:
		case STATUS_REGISTER_2:
		case X_COORDINATE_MSB:
		case X_COORDINATE_LSB:
		case Y_COORDINATE_MSB:
		case Y_COORDINATE_LSB:
		case PRESSURE_VALUE_MSB:
		case PRESSURE_VALUE_LSB:
		case FIFO_STATUS:
		case FIFO_X_COORDINATE_MSB:
		case FIFO_X_COORDINATE_LSB:
		case FIFO_Y_COORDINATE_MSB:
		case FIFO_Y_COORDINATE_LSB:
		case FIFO_PRESSURE_VALUE_COORDINATE_MSB:
		case FIFO_PRESSURE_VALUE_COORDINATE_LSB:
		case UART_BAUDRATE_MSB:
		case UART_BAUDRATE_MID:
		case UART_BAUDRATE_LSB:
		case DEVICE_IDENTIFIER_REGISTER:
		case SLIDE_DISPLACEMENT:
		case ROTATE_ANGLE:
		case ZOOM_SIZE:
		case ELECTRODE_STATUS:
		case FAULTS_NOTE1:
		case E0_BASELINE_MSB:
		case E0_BASELINE_LSB:
		case E1_BASELINE_MSB:
		case E1_BASELINE_LSB:
		case E2_BASELINE_MSB:
		case E2_BASELINE_LSB:
		case E3_BASELINE_MSB:
		case E3_BASELINE_LSB:
		case E0_INSTANT_DELTA:
		case E1_INSTANT_DELTA:
		case E2_INSTANT_DELTA:
		case E3_INSTANT_DELTA:
		case DYNAMIC_STATUS:
		case STATIC_STATUS:
		case CONFIGURATION:
		case TRIGGER_EVENTS:
		case FIFO_SETUP:
		case SAMPLING_X_Y:
		case X_SETTLING_TIME_MBS:
		case X_SETTLING_TIME_LBS:
		case Y_SETTLING_TIME_MBS:
		case Y_SETTLING_TIME_LBS:
		case Z_SETTLING_TIME_MBS:
		case Z_SETTLING_TIME_LBS:
		case HORIZONTAL_RESOLUTION_MBS:
		case HORIZONTAL_RESOLUTION_LBS:
		case VERTICAL_RESOLUTION_MBS:
		case VERTICAL_RESOLUTION_LBS:
		case SLIDE_STEPS:
		case SYSTEM_CONFIG_NOTE2:
		case DC_TRACKER_RATE:
		case RESPONSE_TIME:
		case STUCK_KEY_TIMEOUT:
		case E0_SENSITIVITY:
		case E1_SENSITIVITY:
		case E2_SENSITIVITY:
		case E3_SENSITIVITY:
		case ELECTRODE_ENABLERS:
		case LOW_POWER_SCAN_PERIOD:
		case LOW_POWER_ELECTRODE:
		case LOW_POWER_ELECTRODE_SENSITIVITY:
		case CONTROL_CONFIG:
		case EVENTS:
		case AUTO_REPEAT_RATE:
		case AUTO_REPEAT_START:
		case MAX_TOUCHES:
			data_to_read = i2c_smbus_read_byte_data(client_public, *buf);

			if (data_to_read >= 0 && copy_to_user(buf, &data_to_read, 1))
				printk(KERN_DEBUG "error reading from userspace\n");
			break;

		default:
			printk(KERN_DEBUG "invalid address to read\n");
			return -ENXIO;
		}
	}

	return 1;
}

/*write crtouch register to configure*/
static ssize_t crtouch_write(struct file *filep, const char __user *buf, size_t size, loff_t *fpos)
{
	const unsigned char *data_to_write = NULL;

	data_to_write = buf;

	if (data_to_write == NULL)
		return -ENOMEM;
	if ((data_to_write + 1) == NULL)
		return -EINVAL;

	/*update driver variable*/
	if (*data_to_write == CONFIGURATION) {
		if ((*(data_to_write + 1) >= 0x00) && (*(data_to_write + 1) <= 0xFF))
			data_configuration = *(data_to_write + 1);
	}

	switch (*data_to_write) {
	case CONFIGURATION:
	case X_SETTLING_TIME_MBS:
	case X_SETTLING_TIME_LBS:
	case Y_SETTLING_TIME_MBS:
	case Y_SETTLING_TIME_LBS:
	case Z_SETTLING_TIME_MBS:
	case Z_SETTLING_TIME_LBS:
	case HORIZONTAL_RESOLUTION_MBS:
	case HORIZONTAL_RESOLUTION_LBS:
	case VERTICAL_RESOLUTION_MBS:
	case VERTICAL_RESOLUTION_LBS:
	case SLIDE_STEPS:
	case SYSTEM_CONFIG_NOTE2:
	case DC_TRACKER_RATE:
	case STUCK_KEY_TIMEOUT:
	case LOW_POWER_ELECTRODE_SENSITIVITY:
	case EVENTS:
	case CONTROL_CONFIG:
	case AUTO_REPEAT_RATE:
	case AUTO_REPEAT_START:
	case TRIGGER_EVENTS:
		if ((*(data_to_write + 1) >= 0x00) && (*(data_to_write + 1) <= 0xFF))
			i2c_smbus_write_byte_data(client_public,
						  *data_to_write, *(data_to_write + 1));
		else
			printk(KERN_DEBUG "invalid range of data\n");
		break;

	case RESPONSE_TIME:
		if ((*(data_to_write + 1) >= 0x00) && (*(data_to_write + 1) <= 0x3F))
			i2c_smbus_write_byte_data(client_public,
						  *data_to_write, *(data_to_write+1));
		else
			printk(KERN_DEBUG "invalid range of data\n");
		break;

	case FIFO_SETUP:
		if ((*(data_to_write + 1) >= 0x00) && (*(data_to_write + 1) <= 0x1F))
			i2c_smbus_write_byte_data(client_public,
						  *data_to_write, *(data_to_write+1));
		else
			printk(KERN_DEBUG "invalid range of data\n");
		break;

	case SAMPLING_X_Y:
		if ((*(data_to_write + 1) >= 0x05) && (*(data_to_write + 1) <= 0x64))
			i2c_smbus_write_byte_data(client_public,
						  *data_to_write, *(data_to_write+1));
		else
			printk(KERN_DEBUG "invalid range of data\n");
		break;

	case E0_SENSITIVITY:
	case E1_SENSITIVITY:
	case E2_SENSITIVITY:
	case E3_SENSITIVITY:
		if ((*(data_to_write+1) >= 0x02) && (*(data_to_write+1) <= 0x7F))
			i2c_smbus_write_byte_data(client_public,
						  *data_to_write, *(data_to_write+1));
		else
			printk(KERN_DEBUG "invalid range of data\n");
		break;

	case ELECTRODE_ENABLERS:
	case LOW_POWER_SCAN_PERIOD:
	case LOW_POWER_ELECTRODE:
	case MAX_TOUCHES:
		if ((*(data_to_write+1) >= 0x00) && (*(data_to_write+1) <= 0x0F))
			i2c_smbus_write_byte_data(client_public,
						  *data_to_write, *(data_to_write+1));
		else
			printk(KERN_DEBUG "invalid range of data\n");
		break;

	default:
		printk(KERN_DEBUG "invalid address to write\n");
		return -ENXIO;
	}

	return 1;
}

struct file_operations file_ops_crtouch = {
	open:	crtouch_open,
	write:	crtouch_write,
	read:	crtouch_read,
};


static irqreturn_t crtouch_irq(int irq, void *dev_id)
{
	queue_work(crtouch->workqueue, &crtouch->work);
	return IRQ_HANDLED;
}
static struct timer_list tsc_poll_timer;

static void tsc_poll_handler(unsigned long arg)
{
	queue_work(crtouch->workqueue, &crtouch->work);

	tsc_poll_timer.expires += crtouch->polling_period;
	add_timer(&tsc_poll_timer);
}

static int crtouch_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int result = 0;
	struct input_dev *input_dev;
	int error = 0;
	s32 mask_trigger = 0;
	struct device_node *node = client->dev.of_node;

	/*to be able to communicate by i2c with crtouch (dev)*/
	client_public = client;

	crtouch = kzalloc(sizeof(struct crtouch_data), GFP_KERNEL);

	if (!crtouch)
		return -ENOMEM;

	input_dev = input_allocate_device();
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

	crtouch->is_multitouch = of_property_read_bool(node, "is-multitouch");
	crtouch->is_gestures = of_property_read_bool(node, "is-gestures");
	crtouch->is_capacitive = of_property_read_bool(node, "is-capacitive");
	crtouch->have_pressure_event = of_property_read_bool(node, "pressure-event");

	error = read_resolution();
	if (error < 0) {
		dev_err(&client->dev, "failed to read size of screen\n");
		result = -EIO;
		goto err_free_wq;
	}

	data_configuration = i2c_smbus_read_byte_data(client, CONFIGURATION);
	if (data_configuration < 0) {
		dev_err(&client->dev, "failed to read CRTOUCH configuration\n");
		goto err_free_wq;
	}
	data_configuration &= CLEAN_SLIDE_EVENTS;
	if (data_configuration & SLEEP_MODE) {
		dev_info(&client->dev, "Sleep Mode is not supported, disabling\n");
		data_configuration &= ~SLEEP_MODE;
	}
	if (crtouch->is_multitouch)
		data_configuration |= SET_MULTITOUCH;
	else
		data_configuration &= ~SET_MULTITOUCH;
	i2c_smbus_write_byte_data(client, CONFIGURATION, data_configuration);

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

	input_set_abs_params(crtouch->input_dev, ABS_X, XMIN, xmax, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_Y, YMIN, ymax, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_MT_POSITION_X,
				XMIN, xmax, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_MT_POSITION_Y,
				YMIN, ymax, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_MT_TOUCH_MAJOR, 0, 1, 0, 0);
	input_set_abs_params(crtouch->input_dev, ABS_MT_TRACKING_ID, 0, 1, 0, 0);

	input_set_abs_params(crtouch->input_dev, ABS_PRESSURE, 0, 1, 0, 0);
	input_set_drvdata(crtouch->input_dev, crtouch);
	dev_info(&client->dev, "CR-TOUCH max values X: %d Y: %d\n", xmax, ymax);

	result = input_register_device(crtouch->input_dev);
	if (result)
		goto err_free_wq;

	if (alloc_chrdev_region(&dev_number, 0, 2, DEV_NAME) < 0) {
		dev_err(&client->dev, "failed to allocate cr-touch device\n");
		goto err_unr_dev;
	}

	cdev_init(&crtouch_cdev , &file_ops_crtouch);

	if (cdev_add(&crtouch_cdev, dev_number, 1)) {
		dev_err(&client->dev, "failed to register cr-touch device\n");
		goto err_unr_chrdev;
	}

	crtouch_class = class_create(THIS_MODULE, DEV_NAME);

	if (crtouch_class == NULL) {
		dev_err(&client->dev, "failed to create a class\n");
		goto err_unr_cdev;
	}

	if (device_create(crtouch_class, NULL, dev_number,
				NULL, DEV_NAME) == NULL) {
		dev_err(&client->dev, "failed to create a device\n");
		goto err_unr_class;
	}

	crtouch->wake_gpio = of_get_named_gpio(node, "wake-gpios", 0);
	if (gpio_is_valid(crtouch->wake_gpio)) {
		result = devm_gpio_request_one(&client->dev, crtouch->wake_gpio,
					       GPIOF_OUT_INIT_HIGH, "GPIO_WAKE_CRTOUCH");
		if (result != 0) {
			dev_err(&client->dev, "failed to request GPIO\n");
			goto err_unr_createdev;
		}

	}

	crtouch->pending_gpio = of_get_named_gpio(node, "pending-gpios", 0);
	if (gpio_is_valid(crtouch->pending_gpio)) {
		result = devm_gpio_request_one(&client->dev, crtouch->pending_gpio,
					       GPIOF_IN, "GPIO_WAKE_CRTOUCH");
		if (result != 0) {
			dev_err(&client->dev, "failed to request GPIO for IRQ\n");
			goto err_free_pin;
		}

		result = request_irq(gpio_to_irq(crtouch->pending_gpio), crtouch_irq,
				     IRQF_TRIGGER_FALLING, IRQ_NAME, crtouch_irq);

		if (result < 0) {
			dev_err(&client->dev, "failed to request IRQ\n");
			goto err_free_pinIrq;
		}

		device_init_wakeup(&client->dev, 1);
	} else {
		if (of_property_read_u32(node, "polling-period-ms", &crtouch->polling_period))
			crtouch->polling_period = DEFAULT_POLLING_PERIOD;
		crtouch->polling_period = msecs_to_jiffies(crtouch->polling_period);
		init_timer(&tsc_poll_timer);
		tsc_poll_timer.function = tsc_poll_handler;
		tsc_poll_timer.expires = jiffies + crtouch->polling_period;
		add_timer(&tsc_poll_timer);
		dev_dbg(&client->dev, "Configured to poll every %d ms\n", crtouch->polling_period);
	}

	/*clean interrupt pin*/
	i2c_smbus_read_byte_data(client, STATUS_REGISTER_1);

	return 0;

err_free_pinIrq:
	if (gpio_is_valid(crtouch->pending_gpio))
		gpio_free(crtouch->pending_gpio);
err_free_pin:
	if (gpio_is_valid(crtouch->wake_gpio))
		gpio_free(crtouch->wake_gpio);
err_unr_createdev:
	device_destroy(crtouch_class, dev_number);
err_unr_class:
	class_destroy(crtouch_class);
err_unr_cdev:
	cdev_del(&crtouch_cdev);
err_unr_chrdev:
	unregister_chrdev_region(dev_number, 1);
err_unr_dev:
	input_unregister_device(crtouch->input_dev);
err_free_wq:
	destroy_workqueue(crtouch->workqueue);
err_wqueue:
	input_free_device(crtouch->input_dev);
err_free_mem:
	kfree(crtouch);
	return result;
}

#ifdef CONFIG_PM
static int crtouch_resume(struct device *dev)
{
	struct crtouch_data *crtouch = dev_get_drvdata(dev);
	struct i2c_client *client = crtouch->client;

	if (gpio_is_valid(crtouch->wake_gpio)) {
		gpio_set_value(crtouch->wake_gpio, 0);
		udelay(10);
		gpio_set_value(crtouch->wake_gpio, 1);
	}

	if (gpio_is_valid(crtouch->pending_gpio)) {
		int pend_irq = gpio_to_irq(crtouch->pending_gpio);

		if (device_may_wakeup(&client->dev))
			disable_irq_wake(pend_irq);
		else
			enable_irq(pend_irq);
	}

	return 0;
}

static int crtouch_suspend(struct device *dev)
{
	struct crtouch_data *crtouch = dev_get_drvdata(dev);
	struct i2c_client *client = crtouch->client;
	s32 data_to_read;

	data_to_read = i2c_smbus_read_byte_data(client_public, CONFIGURATION);
	data_to_read |= SHUTDOWN_CRICS;
	i2c_smbus_write_byte_data(client, CONFIGURATION , data_to_read);

	if (gpio_is_valid(crtouch->pending_gpio)) {
		int pend_irq = gpio_to_irq(crtouch->pending_gpio);

		if (device_may_wakeup(&client->dev))
			enable_irq_wake(pend_irq);
		else
			disable_irq(pend_irq);
	}

	return 0;
}

static const struct dev_pm_ops crtouch_pm_ops = {
	.suspend = crtouch_suspend,
	.resume = crtouch_resume,
};
#endif /* CONFIG_PM */

static const struct i2c_device_id crtouch_idtable[] = {
	{ DEVICE_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, crtouch_idtable);

#ifdef CONFIG_OF
static const struct of_device_id crtouch_of_match[] = {
	{ .compatible = "fsl,crtouch" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, crtouch_of_match);
#endif

static struct i2c_driver crtouch_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= DEVICE_NAME,
		.pm = &crtouch_pm_ops,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(crtouch_of_match),
#endif
	},
	.id_table	= crtouch_idtable,
	.probe		= crtouch_probe,
};
module_i2c_driver(crtouch_i2c_driver);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("CR-TOUCH multitouch driver");
MODULE_LICENSE("GPL");
