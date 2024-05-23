/*
 * Copyright (c) 2011 Intel Corporation
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
 * Authers: Jesse Barnes <jesse.barnes@intel.com>
 */

#include <linux/i2c.h>
#include <linux/fb.h>
#include <drm/drmP.h>
#include "psb_drv.h"
#include "psb_intel_drv.h"
/**
 * psb_intel_ddc_probe
 *
 */
bool psb_intel_ddc_probe(struct psb_intel_output *psb_intel_output)
{
	u8 out_buf[] = { 0x0, 0x0 };
	u8 buf[2];
	int ret;
	struct i2c_msg msgs[] = {
		{
		 .addr = 0x50,
		 .flags = 0,
		 .len = 1,
		 .buf = out_buf,
		 },
		{
		 .addr = 0x50,
		 .flags = I2C_M_RD,
		 .len = 1,
		 .buf = buf,
		 }
	};

	ret = i2c_transfer(&psb_intel_output->ddc_bus->adapter, msgs, 2);
	if (ret == 2)
		return true;

	return false;
}

/**
 * psb_intel_ddc_get_modes - get modelist from monitor
 * @connector: DRM connector device to use
 *
 * Fetch the EDID information from @connector using the DDC bus.
 */
int psb_intel_ddc_get_modes(struct psb_intel_output *psb_intel_output)
{
	struct edid *edid;
	int ret = 0;

	edid =
	    drm_get_edid(&psb_intel_output->base,
			 &psb_intel_output->ddc_bus->adapter);
	if (edid) {
		drm_mode_connector_update_edid_property(&psb_intel_output->
							base, edid);
		ret = drm_add_edid_modes(&psb_intel_output->base, edid);
		kfree(edid);
	}
	return ret;
}


static const char *force_audio_names[] = {
	"off",
	"auto",
	"on",
};

void psb_intel_attach_force_audio_property(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_property *prop;
	int i;

	prop = dev_priv->force_audio_property;
	if (prop == NULL) {
		prop = drm_property_create(dev, DRM_MODE_PROP_ENUM,
					   "audio",
					   ARRAY_SIZE(force_audio_names));
		if (prop == NULL)
			return;

		for (i = 0; i < ARRAY_SIZE(force_audio_names); i++)
			drm_property_add_enum(prop, i, i-1, force_audio_names[i]);

		dev_priv->force_audio_property = prop;
	}
	drm_connector_attach_property(connector, prop, 0);
}


static const char *broadcast_rgb_names[] = {
	"Full",
	"Limited 16:235",
};

void
psb_intel_attach_broadcast_rgb_property(struct drm_connector *connector)
{
	struct drm_device *dev = connector->dev;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_property *prop;
	int i;

	prop = dev_priv->broadcast_rgb_property;
	if (prop == NULL) {
		prop = drm_property_create(dev, DRM_MODE_PROP_ENUM,
					   "Broadcast RGB",
					   ARRAY_SIZE(broadcast_rgb_names));
		if (prop == NULL)
			return;

		for (i = 0; i < ARRAY_SIZE(broadcast_rgb_names); i++)
			drm_property_add_enum(prop, i, i, broadcast_rgb_names[i]);

		dev_priv->broadcast_rgb_property = prop;
	}

	drm_connector_attach_property(connector, prop, 0);
}
