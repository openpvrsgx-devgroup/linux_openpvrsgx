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
#define intptr_t host_intptr_t
#define mode_t host_mode_t
#define dev_t host_dev_t
#define blkcnt_t host_blkcnt_t
typedef int int32_t;
struct timespec { int seconds; };

/*
 * Default Coefficients for Sidetone EQ.
 * Please submit new coefficients upstream.
 */

#define __OMAP_AESS_PRIV_H__	// do not define struct snd_soc_file_coeff_data
#include <socfw.h>
#undef __OMAP_AESS_PRIV_H__	// we want to include it later as it defines struct snd_soc_file_coeff_data
struct snd_soc_dai_driver {
	/* DAI description */
	const char *name;
};

#include <aess/omap-aess-priv.h>
#include <aess/aess-fw.h>

/* Flat response with Gain =1 */
static const int32_t sdt_flat_coeffs[] = {
	0, 0, 0, 0, 0x040002, 0, 0, 0, 0,
};

/* 800Hz cut-off frequency and Gain = 1  */
static const int32_t sdt_800Hz_0db_coeffs[] = {
	0, -7554223, 708210, -708206, 7554225,
	0, 6802833, -682266, 731554,
};

/* 800Hz cut-off frequency and Gain = 0.25  */
static const int32_t sdt_800Hz_m12db_coeffs[] = {
	0, -3777112, 5665669, -5665667, 3777112,
	0, 6802833, -682266, 731554,
};

/* 800Hz cut-off frequency and Gain = 0.1  */
static const int32_t sdt_800Hz_m20db_coeffs[] = {
	0, -1510844, 4532536, -4532536, 1510844,
	0, 6802833, -682266, 731554,
};

static const struct snd_soc_fw_coeff_elem elems[] = {
	SND_SOC_FW_COEFF_ELEM(sdt_flat_coeffs),
	SND_SOC_FW_COEFF_ELEM(sdt_800Hz_0db_coeffs),
	SND_SOC_FW_COEFF_ELEM(sdt_800Hz_m12db_coeffs),
	SND_SOC_FW_COEFF_ELEM(sdt_800Hz_m20db_coeffs),
};

static const char * const texts[] = {
	"Flat Response",
	"800Hz HPF 0dB",
	"800Hz HPF -12dB",
	"800Hz HPF -20dB",
};

static const struct snd_soc_fw_coeff sdt[] = {
	SND_SOC_FW_COEFFICIENT(OMAP_AESS_CMEM_SDT_COEFS_ID, OMAP_CONTROL_EQU,
	"SDT Equalizer", elems),
};

static const struct snd_kcontrol_new controls[] = {
	/* DL2 left equalizer control */
	SND_SOC_ENUM_COEFFS("SDT Equalizer", OMAP_AESS_CMEM_SDT_COEFS_ID,
		OMAP_AESS_MIXER_EQU, OMAP_AESS_MIXER_EQU, texts),
};

const struct snd_soc_fw_plugin plugin = {
	.kcontrols 	= controls,
	.kcontrol_count	= ARRAY_SIZE(controls),
	.coeffs 	= sdt,
	.coeff_count	= ARRAY_SIZE(sdt),
	.version	= 996000,
};
