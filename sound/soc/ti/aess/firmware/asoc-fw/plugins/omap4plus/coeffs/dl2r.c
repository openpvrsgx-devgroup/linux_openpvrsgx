/*
 * Copyright (C) 2012 Texas Instruments Inc.
 *
 * Author: Liam Girdwood <lrg@ti.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#define uintptr_t host_uintptr_t
#define mode_t host_mode_t
#define dev_t host_dev_t
#define blkcnt_t host_blkcnt_t
typedef int int32_t;
struct timespec { int seconds; };

/*
 * Default Coefficients for DL2
 */
#include <socfw.h>
#include <aess/omap-aess-priv.h>
#include <aess/aess-fw.h>

/* Flat response with Gain =1 */
static const int32_t dl2r_flat_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0x040002, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};

/* 800Hz cut-off frequency and Gain = 1  */
static const int32_t dl2r_800Hz_0db_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, -7554223,
	708210, -708206, 7554225, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 6802833, -682266, 731554,
};

/* 800Hz cut-off frequency and Gain = 0.25  */
static const int32_t dl2r_800Hz_m12db_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, -3777112,
	5665669, -5665667, 3777112, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 6802833, -682266, 731554,
};

/* 800Hz cut-off frequency and Gain = 0.1  */
static const int32_t dl2r_800Hz_m20db_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, -1510844,
	4532536, -4532536, 1510844, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 6802833, -682266, 731554,
};

/* 450Hz High pass and Gain = 1 */
static const int32_t dl2r_450Hz_0db_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	8046381, -502898, 8046381, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,-7718119, 502466,
};

static const struct snd_soc_fw_coeff_elem elems[] = {
	SND_SOC_FW_COEFF_ELEM("Flat Response", dl2r_flat_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF 0dB", dl2r_800Hz_0db_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF -12dB", dl2r_800Hz_m12db_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF -20dB", dl2r_800Hz_m20db_coeffs),
	SND_SOC_FW_COEFF_ELEM("450Hz HPF 0dB", dl2r_450Hz_0db_coeffs),
};

static const struct snd_soc_fw_coeff dl2r[] = {
	SND_SOC_FW_COEFFICIENT(OMAP_AESS_CMEM_DL2_R_COEFS_ID,
	OMAP_CONTROL_EQU, "DL2 Right Equalizer", elems),
};

const struct snd_soc_fw_plugin plugin = {
	.coeffs 	= dl2r,
	.coeff_count	= ARRAY_SIZE(dl2r),
	.version	= 996000,
};

