/*
 *
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

/*
 * Default Coefficients for DL1EQ.
 * Please submit new coefficients upstream.
 */

#include <socfw.h>
#include <omap-abe-priv.h>
#include <aess/aess-fw.h>

/* Flat response with Gain =1 */
static const int32_t dl1_flat_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0x040002, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};

/* 800Hz cut-off frequency and Gain = 1  */
static const int32_t dl1_800Hz_0db_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, -7554223,
	708210, -708206, 7554225, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 6802833, -682266, 731554,
};

/* 800Hz cut-off frequency and Gain = 0.25  */
static const int32_t dl1_800Hz_m12db_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, -3777112,
	5665669, -5665667, 3777112, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 6802833, -682266, 731554,
};

/* 800Hz cut-off frequency and Gain = 0.1  */
static const int32_t dl1_800Hz_m20db_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, -1510844,
	4532536, -4532536, 1510844, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 6802833, -682266, 731554,
};

/* 4000Hz low pass and Gain = 1 */
static const int32_t dl1_4000Hz_0db_coeffs[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0002e630,
	0x0008b290, 0x0008b290, 0x0002e630, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0x0058ae58, 0xfffa666a, 0x0007da1e,
};

static const struct snd_soc_fw_coeff_elem elems[] = {
	SND_SOC_FW_COEFF_ELEM("Flat Response", dl1_flat_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF 0dB", dl1_800Hz_0db_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF -12dB", dl1_800Hz_m12db_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF -20dB", dl1_800Hz_m20db_coeffs),
	SND_SOC_FW_COEFF_ELEM("4000Hz LPF 0dB", dl1_4000Hz_0db_coeffs),
};

static const struct snd_soc_fw_coeff dl1[] = {
	SND_SOC_FW_COEFFICIENT(OMAP_AESS_CMEM_DL1_COEFS_ID,  OMAP_CONTROL_EQU, 
	"DL1 Equalizer", elems),
};

const struct snd_soc_fw_plugin plugin = {
	.coeffs 	= dl1,
	.coeff_count	= ARRAY_SIZE(dl1),
	.version	= 996000,
};
