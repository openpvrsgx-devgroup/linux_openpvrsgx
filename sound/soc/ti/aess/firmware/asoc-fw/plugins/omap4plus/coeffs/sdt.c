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

/*
 * Default Coefficients for Sidetone EQ.
 * Please submit new coefficients upstream.
 */

#include <socfw.h>
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
	SND_SOC_FW_COEFF_ELEM("Flat Response", sdt_flat_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF 0dB", sdt_800Hz_0db_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF -12dB", sdt_800Hz_m12db_coeffs),
	SND_SOC_FW_COEFF_ELEM("800Hz HPF -20dB", sdt_800Hz_m20db_coeffs),
};

static const struct snd_soc_fw_coeff sdt[] = {
	SND_SOC_FW_COEFFICIENT(OMAP_AESS_CMEM_SDT_COEFS_ID, 
	 OMAP_CONTROL_EQU, "SDT Equalizer", elems),
};

const struct snd_soc_fw_plugin plugin = {
	.coeffs 	= sdt,
	.coeff_count	= ARRAY_SIZE(sdt),
	.version	= 996000,
};
