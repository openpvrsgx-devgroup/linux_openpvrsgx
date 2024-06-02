/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (C) 2020 Andreas Kemnade */
void mxc_epdc_init_sequence(struct mxc_epdc *priv, struct drm_display_mode *m);
int mxc_epdc_init_hw(struct mxc_epdc *priv);

void mxc_epdc_powerup(struct mxc_epdc *priv);
void mxc_epdc_powerdown(struct mxc_epdc *priv);

