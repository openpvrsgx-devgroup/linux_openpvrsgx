/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (C) 2020 Andreas Kemnade */

#include <drm/drm_drv.h>
#include <drm/drm_connector.h>
#include <drm/drm_simple_kms_helper.h>

struct clk;
struct regulator;
struct mxc_epdc {
	struct drm_device drm;
	struct drm_simple_display_pipe pipe;
	struct drm_connector connector;
	struct drm_panel *panel;
};

