/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * This header provides constants for Good Display epaper displays
 *
 * Copyright 2019 Jan Sebastian Goette
 */

enum gdepaper_controller_res {
	GDEP_CTRL_RES_320X300 = 0,
	GDEP_CTRL_RES_300X200 = 1,
	GDEP_CTRL_RES_296X160 = 2,
	GDEP_CTRL_RES_296X128 = 3,
};

enum gdepaper_color_type {
	GDEPAPER_COL_BW = 0,
	GDEPAPER_COL_BW_RED,
	GDEPAPER_COL_BW_YELLOW,
	GDEPAPER_COL_END
};

enum gdepaper_vghl_lv {
	GDEP_PWR_VGHL_16V = 0,
	GDEP_PWR_VGHL_15V = 1,
	GDEP_PWR_VGHL_14V = 2,
	GDEP_PWR_VGHL_13V = 3,
};
