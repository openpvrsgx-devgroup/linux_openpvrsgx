// SPDX-License-Identifier: GPL-2.0+
// Copyright (C) 2020 Andreas Kemnade
//

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include "mxc_epdc.h"

#define DEFAULT_TEMP_INDEX      0
#define DEFAULT_TEMP            20 /* room temp in deg Celsius */

struct waveform_data_header {
	unsigned int wi0;
	unsigned int wi1;
	unsigned int wi2;
	unsigned int wi3;
	unsigned int wi4;
	unsigned int wi5;
	unsigned int wi6;
	unsigned int xwia:24;
	unsigned int cs1:8;
	unsigned int wmta:24;
	unsigned int fvsn:8;
	unsigned int luts:8;
	unsigned int mc:8;
	unsigned int trc:8;
	unsigned int reserved0_0:8;
	unsigned int eb:8;
	unsigned int sb:8;
	unsigned int reserved0_1:8;
	unsigned int reserved0_2:8;
	unsigned int reserved0_3:8;
	unsigned int reserved0_4:8;
	unsigned int reserved0_5:8;
	unsigned int cs2:8;
};

struct mxcfb_waveform_data_file {
	struct waveform_data_header wdh;
	u32 *data;      /* Temperature Range Table + Waveform Data */
};

void mxc_epdc_set_update_waveform(struct mxc_epdc *priv,
				  struct mxcfb_waveform_modes *wv_modes)
{
	u32 val;

	/* Configure the auto-waveform look-up table based on waveform modes */

	/* Entry 1 = DU, 2 = GC4, 3 = GC8, etc. */
	val = (wv_modes->mode_du << EPDC_AUTOWV_LUT_DATA_OFFSET) |
		(0 << EPDC_AUTOWV_LUT_ADDR_OFFSET);
	epdc_write(priv, EPDC_AUTOWV_LUT, val);
	val = (wv_modes->mode_du << EPDC_AUTOWV_LUT_DATA_OFFSET) |
		(1 << EPDC_AUTOWV_LUT_ADDR_OFFSET);
	epdc_write(priv, EPDC_AUTOWV_LUT, val);
	val = (wv_modes->mode_gc4 << EPDC_AUTOWV_LUT_DATA_OFFSET) |
		(2 << EPDC_AUTOWV_LUT_ADDR_OFFSET);
	epdc_write(priv, EPDC_AUTOWV_LUT, val);
	val = (wv_modes->mode_gc8 << EPDC_AUTOWV_LUT_DATA_OFFSET) |
		(3 << EPDC_AUTOWV_LUT_ADDR_OFFSET);
	epdc_write(priv, EPDC_AUTOWV_LUT, val);
	val = (wv_modes->mode_gc16 << EPDC_AUTOWV_LUT_DATA_OFFSET) |
		(4 << EPDC_AUTOWV_LUT_ADDR_OFFSET);
	epdc_write(priv, EPDC_AUTOWV_LUT, val);
	val = (wv_modes->mode_gc32 << EPDC_AUTOWV_LUT_DATA_OFFSET) |
		(5 << EPDC_AUTOWV_LUT_ADDR_OFFSET);
	epdc_write(priv, EPDC_AUTOWV_LUT, val);
}

int mxc_epdc_fb_get_temp_index(struct mxc_epdc *priv, int temp)
{
	int i;
	int index = -1;

	if (temp == TEMP_USE_AMBIENT) {
		if (priv->thermal) {
			if (thermal_zone_get_temp(priv->thermal, &temp)) {
				dev_err(priv->drm.dev,
					"reading temperature failed");
				return DEFAULT_TEMP_INDEX;
			}
			temp /= 1000;
		} else
			temp = DEFAULT_TEMP;
	}

	if (priv->trt_entries == 0) {
		dev_err(priv->drm.dev,
			"No TRT exists...using default temp index\n");
		return DEFAULT_TEMP_INDEX;
	}

	/* Search temperature ranges for a match */
	for (i = 0; i < priv->trt_entries - 1; i++) {
		if ((temp >= priv->temp_range_bounds[i])
			&& (temp < priv->temp_range_bounds[i+1])) {
			index = i;
			break;
		}
	}

	if (index < 0) {
		dev_err(priv->drm.dev,
			"No TRT index match...using lowest/highest\n");
		if (temp < priv->temp_range_bounds[0]) {
			dev_dbg(priv->drm.dev, "temperature < minimum range\n");
			return 0;
		}

		if (temp >= priv->temp_range_bounds[priv->trt_entries-1]) {
			dev_dbg(priv->drm.dev, "temperature >= maximum range\n");
			return priv->trt_entries-1;
		}

		return DEFAULT_TEMP_INDEX;
	}

	dev_dbg(priv->drm.dev, "Using temperature index %d\n", index);

	return index;
}



int mxc_epdc_prepare_waveform(struct mxc_epdc *priv,
			      const u8 *data, size_t size)
{
	int ret;
	const struct mxcfb_waveform_data_file *wv_file;
	int wv_data_offs;
	int i;

	priv->wv_modes.mode_init = 0;
	priv->wv_modes.mode_du = 1;
	priv->wv_modes.mode_gc4 = 3;
	priv->wv_modes.mode_gc8 = 2;
	priv->wv_modes.mode_gc16 = 2;
	priv->wv_modes.mode_gc32 = 2;
	priv->wv_modes_update = true;

	wv_file = (struct mxcfb_waveform_data_file *)data;

	/* Get size and allocate temperature range table */
	priv->trt_entries = wv_file->wdh.trc + 1;
	priv->temp_range_bounds = devm_kzalloc(priv->drm.dev, priv->trt_entries, GFP_KERNEL);

	for (i = 0; i < priv->trt_entries; i++)
		dev_dbg(priv->drm.dev, "trt entry #%d = 0x%x\n", i, *((u8 *)&wv_file->data + i));

	/* Copy TRT data */
	memcpy(priv->temp_range_bounds, &wv_file->data, priv->trt_entries);

	/* Set default temperature index using TRT and room temp */
	priv->temp_index = mxc_epdc_fb_get_temp_index(priv, DEFAULT_TEMP);

	/* Get offset and size for waveform data */
	wv_data_offs = sizeof(wv_file->wdh) + priv->trt_entries + 1;
	priv->waveform_buffer_size = size - wv_data_offs;

	/* Allocate memory for waveform data */
	priv->waveform_buffer_virt = dmam_alloc_coherent(priv->drm.dev,
						priv->waveform_buffer_size,
						&priv->waveform_buffer_phys,
						GFP_DMA | GFP_KERNEL);
	if (priv->waveform_buffer_virt == NULL) {
		dev_err(priv->drm.dev, "Can't allocate mem for waveform!\n");
		return -ENOMEM;
	}

	memcpy(priv->waveform_buffer_virt, (u8 *)(data) + wv_data_offs,
		priv->waveform_buffer_size);

	/* Read field to determine if 4-bit or 5-bit mode */
	if ((wv_file->wdh.luts & 0xC) == 0x4)
		priv->buf_pix_fmt = EPDC_FORMAT_BUF_PIXEL_FORMAT_P5N;
	else
		priv->buf_pix_fmt = EPDC_FORMAT_BUF_PIXEL_FORMAT_P4N;

	dev_info(priv->drm.dev, "EPDC pix format: %x\n",
		 priv->buf_pix_fmt);

	return 0;
}
