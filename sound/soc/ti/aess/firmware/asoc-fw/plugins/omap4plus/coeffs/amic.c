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
 * Default Coefficients for amicEQ.
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

/* 20kHz cut-off frequency and Gain = 1 */
static const int32_t amic_20kHz_0dB_coeffs[] = {
	-4119413, -192384, -341428, -348088,
	-151380, 151380, 348088, 341428, 192384,
	4119419, 1938156, -6935719, 775202,
	-1801934, 2997698, -3692214, 3406822,
	-2280190, 1042982
};

/* 20kHz cut-off frequency and Gain = 0.25 */
static const int32_t amic_20kHz_m12db_coeffs[] = {
	-1029873, -3078121, -5462817, -5569389,
	-2422069, 2422071, 5569391, 5462819,
	3078123, 1029875, 1938188, -6935811,
	775210, -1801950, 2997722, -3692238,
	3406838, -2280198, 1042982,
};

/* 20kHz cut-off frequency and Gain = 0.125 */
static const int32_t amic_20kHz_m15db_coeffs[] = {
	-514937, -1539061, -2731409, -2784693,
	-1211033, 1211035, 2784695, 2731411,
	1539063, 514939, 1938188, -6935811,
	775210, -1801950, 2997722, -3692238,
	3406838, -2280198, 1042982,
};

static const struct snd_soc_fw_coeff_elem elems[] = {
	SND_SOC_FW_COEFF_ELEM(amic_20kHz_0dB_coeffs),
	SND_SOC_FW_COEFF_ELEM(amic_20kHz_m12db_coeffs),
	SND_SOC_FW_COEFF_ELEM(amic_20kHz_m15db_coeffs),
};

static const char * const texts[] = {
	"20kHz LPF 0dB",
	"20kHz LPF -12dB",
	"20kHz LPF -15dB",
};

static const struct snd_soc_fw_coeff amic[] = {
	SND_SOC_FW_COEFFICIENT(OMAP_AESS_CMEM_96_48_AMIC_COEFS_ID, OMAP_CONTROL_EQU,
	"Amic Equalizer", elems),
};

static const struct snd_kcontrol_new controls[] = {
	/* Amic equalizer control */
	SND_SOC_ENUM_COEFFS("Amic Equalizer", OMAP_AESS_CMEM_96_48_AMIC_COEFS_ID,
		OMAP_AESS_MIXER_EQU, OMAP_AESS_MIXER_EQU, texts),
};

const struct snd_soc_fw_plugin plugin = {
	.kcontrols 	= controls,
	.kcontrol_count	= ARRAY_SIZE(controls),
	.coeffs 	= amic,
	.coeff_count	= ARRAY_SIZE(amic),
	.version	= 996000,
};
