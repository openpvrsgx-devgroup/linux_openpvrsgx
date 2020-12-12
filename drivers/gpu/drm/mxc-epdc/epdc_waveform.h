/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (C) 2020 Andreas Kemnade */
int mxc_epdc_fb_get_temp_index(struct mxc_epdc *priv, int temp);
int mxc_epdc_prepare_waveform(struct mxc_epdc *priv,
			      const u8 *waveform, size_t size);
void mxc_epdc_set_update_waveform(struct mxc_epdc *priv,
				  struct mxcfb_waveform_modes *wv_modes);
