/*
 * Copyright © 2006-2012 Intel Corporation
 * Copyright (c) 2006 Dave Airlie <airlied@linux.ie>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Eric Anholt <eric@anholt.net>
 *      Dave Airlie <airlied@linux.ie>
 *      Jesse Barnes <jesse.barnes@intel.com>
 *      Chris Wilson <chris@chris-wilson.co.uk>
 */

#include "psb_drv.h"
#include "psb_intel_reg.h"

void
psb_intel_fixed_panel_mode(struct drm_display_mode *fixed_mode,
		       struct drm_display_mode *adjusted_mode)
{
	adjusted_mode->hdisplay = fixed_mode->hdisplay;
	adjusted_mode->hsync_start = fixed_mode->hsync_start;
	adjusted_mode->hsync_end = fixed_mode->hsync_end;
	adjusted_mode->htotal = fixed_mode->htotal;

	adjusted_mode->vdisplay = fixed_mode->vdisplay;
	adjusted_mode->vsync_start = fixed_mode->vsync_start;
	adjusted_mode->vsync_end = fixed_mode->vsync_end;
	adjusted_mode->vtotal = fixed_mode->vtotal;

	adjusted_mode->clock = fixed_mode->clock;

	drm_mode_set_crtcinfo(adjusted_mode, CRTC_INTERLACE_HALVE_V);
}


static int is_backlight_combination_mode(struct drm_device *dev)
{
	return REG_READ(BLC_PWM_CTL2) & PWM_LEGACY_MODE;
}

static u32 psb_intel_get_pwm_ctl(struct drm_device *dev)
{
	u32 val;
	struct drm_psb_private *dev_priv = (struct drm_psb_private *)dev->dev_private;

	val = REG_READ(BLC_PWM_CTL);

	if (dev_priv->saveBLC_PWM_CTL == 0) {
		dev_priv->saveBLC_PWM_CTL = val;
		dev_priv->saveBLC_PWM_CTL2 = REG_READ(BLC_PWM_CTL2);
	} else if (val == 0) {
		REG_WRITE(BLC_PWM_CTL, dev_priv->saveBLC_PWM_CTL);
		REG_WRITE(BLC_PWM_CTL2, dev_priv->saveBLC_PWM_CTL2);
		val = dev_priv->saveBLC_PWM_CTL;
	}
	return val;
}

u32 psb_intel_panel_get_backlight(struct drm_device *dev)
{
	u32 val;

	val = REG_READ(BLC_PWM_CTL) & BACKLIGHT_DUTY_CYCLE_MASK;

	if (is_backlight_combination_mode(dev)) {
		u8 lbpc;

		val &= ~1;
		pci_read_config_byte(dev->pdev, 0xF4, &lbpc);
		val *= lbpc;
	}

	return val;
}

/**
 *  * Returns the maximum level of the backlight duty cycle field.
 *   */
u32 psb_intel_panel_get_max_backlight(struct drm_device *dev)
{
	u32 max;

	max = psb_intel_get_pwm_ctl(dev);

	if (max == 0) {
		DRM_DEBUG_KMS("LVDS Panel PWM value is 0!\n");
	       	/* i915 does this, I believe which means that we should not
		 * smash PWM control as firmware will take control of it. */
		return 1;
	}

	max >>= 16;
	if (is_backlight_combination_mode(dev))
		max *= 0xff;

	return max;
}

static void psb_intel_panel_set_actual_backlight(struct drm_device *dev, int level)
{
	u32 blc_pwm_ctl;

	if (is_backlight_combination_mode(dev)) {
		u32 max = psb_intel_panel_get_max_backlight(dev);
		u8 lbpc;

		lbpc = level * 0xfe / max + 1;
		level /= lbpc;

		pci_write_config_byte(dev->pdev, 0xF4, lbpc);
	}

	blc_pwm_ctl = REG_READ(BLC_PWM_CTL) & ~BACKLIGHT_DUTY_CYCLE_MASK;
	REG_WRITE(BLC_PWM_CTL, (blc_pwm_ctl | (level << BACKLIGHT_DUTY_CYCLE_SHIFT)));
}

void psb_intel_panel_enable_backlight(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;
	
	if (dev_priv->backlight_level == 0)
		dev_priv->backlight_level =
			psb_intel_panel_get_max_backlight(dev);
	
	dev_priv->backlight_enabled = 1;
	psb_intel_panel_set_actual_backlight(dev, dev_priv->backlight_level);	
}

void psb_intel_panel_disable_backlight(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;
	
	dev_priv->backlight_enabled = 0;
	psb_intel_panel_set_actual_backlight(dev, 0);	
}

void psb_intel_panel_set_backlight(struct drm_device *dev, int level)
{
	struct drm_psb_private *dev_priv =
		(struct drm_psb_private *) dev->dev_private;

	dev_priv->backlight_level = level;
	if (dev_priv->backlight_enabled)
		psb_intel_panel_set_actual_backlight(dev, level);
}

static void psb_intel_panel_init_backlight(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	dev_priv->backlight_level = psb_intel_panel_get_backlight(dev);
	dev_priv->backlight_enabled = dev_priv->backlight_level != 0;
}

#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
static int psb_intel_panel_update_status(struct backlight_device *bd)
{
	struct drm_device *dev = bl_get_data(bd);
	psb_intel_panel_set_backlight(dev, bd->props.brightness);
	return 0;
}

static int psb_intel_panel_get_brightness(struct backlight_device *bd)
{
	struct drm_device *dev = bl_get_data(bd);
	return psb_intel_panel_get_backlight(dev);
}

static const struct backlight_ops psb_intel_panel_bl_ops = {
	.update_status = psb_intel_panel_update_status,
	.get_brightness = psb_intel_panel_get_brightness,
};

int psb_intel_panel_setup_backlight(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct backlight_properties props;
	struct drm_connector *connector;

	if (dev_priv->int_lvds_connector)
                connector = dev_priv->int_lvds_connector;
        else if (dev_priv->int_edp_connector)
                connector = dev_priv->int_edp_connector;
        else
                return -ENODEV;

	psb_intel_panel_init_backlight(dev);
	props.type = BACKLIGHT_RAW;
	props.max_brightness = psb_intel_panel_get_max_backlight(dev);
	dev_priv->backlight = backlight_device_register("psb_intel_backlight",
							&connector->kdev, dev,
							&psb_intel_panel_bl_ops, &props);

	if (IS_ERR(dev_priv->backlight)) {
		DRM_ERROR("Failed to register backlight: %ld\n",
				PTR_ERR(dev_priv->backlight));
		dev_priv->backlight = NULL;
		return -ENODEV;
	}
	dev_priv->backlight->props.brightness = psb_intel_panel_get_backlight(dev);
	return 0;
}

void psb_intel_panel_destroy_backlight(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	if (dev_priv->backlight)
		backlight_device_unregister(dev_priv->backlight);
}
#else
int psb_intel_panel_setup_backlight(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	if (!dev_priv->int_lvds_connector && !dev_priv->int_edp_connector)
                return -ENODEV;

	psb_intel_panel_init_backlight(dev);
	return 0;
}

void psb_intel_panel_destroy_backlight(struct drm_device *dev)
{
	return;
}
#endif
