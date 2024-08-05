/*
 *  psb backlight using HAL
 *
 * Copyright (c) 2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Authors: Eric Knopp
 *
 */

#include <linux/backlight.h>
#include <linux/version.h>
#include "psb_drv.h"
#include "psb_intel_reg.h"
#include "psb_intel_drv.h"
#include "psb_intel_bios.h"
#include "psb_powermgmt.h"

#define MRST_BLC_MAX_PWM_REG_FREQ	    0xFFFF
#define BLC_PWM_PRECISION_FACTOR 100	/* 10000000 */
#define BLC_PWM_FREQ_CALC_CONSTANT 32
#define MHz 1000000
#define BRIGHTNESS_MIN_LEVEL 1
#define BRIGHTNESS_MAX_LEVEL 100
#define BRIGHTNESS_MASK	0xFF
#define BLC_POLARITY_NORMAL 0
#define BLC_POLARITY_INVERSE 1
#define BLC_ADJUSTMENT_MAX 100

#define PSB_BLC_PWM_PRECISION_FACTOR    10
#define PSB_BLC_MAX_PWM_REG_FREQ        0xFFFE
#define PSB_BLC_MIN_PWM_REG_FREQ        0x2

#define PSB_BACKLIGHT_PWM_POLARITY_BIT_CLEAR (0xFFFE)
#define PSB_BACKLIGHT_PWM_CTL_SHIFT	(16)

static struct backlight_device *psb_backlight_device;
u8 blc_pol;
u8 blc_type;


int psb_set_brightness(struct backlight_device *bd)
{
	struct drm_device *dev = (struct drm_device *)bl_get_data(psb_backlight_device);
	int level = bd->props.brightness;

	DRM_DEBUG_DRIVER("backlight level set to %d\n", level);

	if (level > bd->props.max_brightness)
		level = bd->props.max_brightness;

	psb_intel_panel_set_backlight(dev, level);

	return 0;
}

int psb_get_brightness(struct backlight_device *bd)
{
	struct drm_device *dev = (struct drm_device *)bl_get_data(psb_backlight_device);

	/* return locally cached var instead of HW read (due to DPST etc.) */
	return psb_intel_panel_get_backlight(dev);
}

struct backlight_ops psb_ops = {
	.get_brightness = psb_get_brightness,
	.update_status  = psb_set_brightness,
};

static int device_backlight_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = (struct drm_psb_private *) dev->dev_private;

	if (IS_CDV(dev)) {
		dev_priv->blc_adj1 = BLC_ADJUSTMENT_MAX;
		dev_priv->blc_adj2 = BLC_ADJUSTMENT_MAX;
		return 0;
	}

	return 0;
}

int psb_backlight_init(struct drm_device *dev)
{
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	int ret = 0;
	struct backlight_properties props;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.max_brightness = psb_intel_panel_get_max_backlight(dev);
	props.type = BACKLIGHT_RAW;

	psb_backlight_device = backlight_device_register("psb-bl", NULL, (void *)dev, &psb_ops, &props);
	if (IS_ERR(psb_backlight_device))
		return PTR_ERR(psb_backlight_device);

	if ((ret = device_backlight_init(dev)) != 0)
		return ret;

	psb_backlight_device->props.brightness = psb_intel_panel_get_backlight(dev);
	psb_backlight_device->props.max_brightness = props.max_brightness;
	backlight_update_status(psb_backlight_device);
#endif
	return 0;
}

void psb_backlight_exit(void)
{
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	psb_backlight_device->props.brightness = 0;
	backlight_update_status(psb_backlight_device);
	backlight_device_unregister(psb_backlight_device);
#endif
	return;
}

struct backlight_device * psb_get_backlight_device(void)
{
#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	return psb_backlight_device;
#endif
	return NULL;
}
