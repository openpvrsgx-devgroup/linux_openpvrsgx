/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2010-2013 Texas Instruments Incorporated,
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2010-2013 Texas Instruments Incorporated,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>

#include "omap-aess-priv.h"
#include "aess_gain.h"
#include "aess_mem.h"

/*
 * AESS CONST AREA FOR PARAMETERS TRANSLATION
 */
#define OMAP_AESS_GAIN_MUTED     (0x0001 << 0)
#define OMAP_AESS_GAIN_DISABLED  (0x0001 << 1)

#define OMAP_AESS_GAIN_MIN_MDB	(-12000)
#define OMAP_AESS_GAIN_MAX_MDB	(3000)
#define OMAP_AESS_GAIN_DB2LIN_SIZE \
		(1 + ((OMAP_AESS_GAIN_MAX_MDB - OMAP_AESS_GAIN_MIN_MDB) / 100))

static const u32 aess_db2lin_table[OMAP_AESS_GAIN_DB2LIN_SIZE] = {
	0x00000000,		/* SMEM coding of -120 dB */
	0x00000000,		/* SMEM coding of -119 dB */
	0x00000000,		/* SMEM coding of -118 dB */
	0x00000000,		/* SMEM coding of -117 dB */
	0x00000000,		/* SMEM coding of -116 dB */
	0x00000000,		/* SMEM coding of -115 dB */
	0x00000000,		/* SMEM coding of -114 dB */
	0x00000000,		/* SMEM coding of -113 dB */
	0x00000000,		/* SMEM coding of -112 dB */
	0x00000000,		/* SMEM coding of -111 dB */
	0x00000000,		/* SMEM coding of -110 dB */
	0x00000000,		/* SMEM coding of -109 dB */
	0x00000001,		/* SMEM coding of -108 dB */
	0x00000001,		/* SMEM coding of -107 dB */
	0x00000001,		/* SMEM coding of -106 dB */
	0x00000001,		/* SMEM coding of -105 dB */
	0x00000001,		/* SMEM coding of -104 dB */
	0x00000001,		/* SMEM coding of -103 dB */
	0x00000002,		/* SMEM coding of -102 dB */
	0x00000002,		/* SMEM coding of -101 dB */
	0x00000002,		/* SMEM coding of -100 dB */
	0x00000002,		/* SMEM coding of -99 dB */
	0x00000003,		/* SMEM coding of -98 dB */
	0x00000003,		/* SMEM coding of -97 dB */
	0x00000004,		/* SMEM coding of -96 dB */
	0x00000004,		/* SMEM coding of -95 dB */
	0x00000005,		/* SMEM coding of -94 dB */
	0x00000005,		/* SMEM coding of -93 dB */
	0x00000006,		/* SMEM coding of -92 dB */
	0x00000007,		/* SMEM coding of -91 dB */
	0x00000008,		/* SMEM coding of -90 dB */
	0x00000009,		/* SMEM coding of -89 dB */
	0x0000000A,		/* SMEM coding of -88 dB */
	0x0000000B,		/* SMEM coding of -87 dB */
	0x0000000D,		/* SMEM coding of -86 dB */
	0x0000000E,		/* SMEM coding of -85 dB */
	0x00000010,		/* SMEM coding of -84 dB */
	0x00000012,		/* SMEM coding of -83 dB */
	0x00000014,		/* SMEM coding of -82 dB */
	0x00000017,		/* SMEM coding of -81 dB */
	0x0000001A,		/* SMEM coding of -80 dB */
	0x0000001D,		/* SMEM coding of -79 dB */
	0x00000021,		/* SMEM coding of -78 dB */
	0x00000025,		/* SMEM coding of -77 dB */
	0x00000029,		/* SMEM coding of -76 dB */
	0x0000002E,		/* SMEM coding of -75 dB */
	0x00000034,		/* SMEM coding of -74 dB */
	0x0000003A,		/* SMEM coding of -73 dB */
	0x00000041,		/* SMEM coding of -72 dB */
	0x00000049,		/* SMEM coding of -71 dB */
	0x00000052,		/* SMEM coding of -70 dB */
	0x0000005D,		/* SMEM coding of -69 dB */
	0x00000068,		/* SMEM coding of -68 dB */
	0x00000075,		/* SMEM coding of -67 dB */
	0x00000083,		/* SMEM coding of -66 dB */
	0x00000093,		/* SMEM coding of -65 dB */
	0x000000A5,		/* SMEM coding of -64 dB */
	0x000000B9,		/* SMEM coding of -63 dB */
	0x000000D0,		/* SMEM coding of -62 dB */
	0x000000E9,		/* SMEM coding of -61 dB */
	0x00000106,		/* SMEM coding of -60 dB */
	0x00000126,		/* SMEM coding of -59 dB */
	0x0000014A,		/* SMEM coding of -58 dB */
	0x00000172,		/* SMEM coding of -57 dB */
	0x0000019F,		/* SMEM coding of -56 dB */
	0x000001D2,		/* SMEM coding of -55 dB */
	0x0000020B,		/* SMEM coding of -54 dB */
	0x0000024A,		/* SMEM coding of -53 dB */
	0x00000292,		/* SMEM coding of -52 dB */
	0x000002E2,		/* SMEM coding of -51 dB */
	0x0000033C,		/* SMEM coding of -50 dB */
	0x000003A2,		/* SMEM coding of -49 dB */
	0x00000413,		/* SMEM coding of -48 dB */
	0x00000492,		/* SMEM coding of -47 dB */
	0x00000521,		/* SMEM coding of -46 dB */
	0x000005C2,		/* SMEM coding of -45 dB */
	0x00000676,		/* SMEM coding of -44 dB */
	0x0000073F,		/* SMEM coding of -43 dB */
	0x00000822,		/* SMEM coding of -42 dB */
	0x00000920,		/* SMEM coding of -41 dB */
	0x00000A3D,		/* SMEM coding of -40 dB */
	0x00000B7D,		/* SMEM coding of -39 dB */
	0x00000CE4,		/* SMEM coding of -38 dB */
	0x00000E76,		/* SMEM coding of -37 dB */
	0x0000103A,		/* SMEM coding of -36 dB */
	0x00001235,		/* SMEM coding of -35 dB */
	0x0000146E,		/* SMEM coding of -34 dB */
	0x000016EC,		/* SMEM coding of -33 dB */
	0x000019B8,		/* SMEM coding of -32 dB */
	0x00001CDC,		/* SMEM coding of -31 dB */
	0x00002061,		/* SMEM coding of -30 dB */
	0x00002455,		/* SMEM coding of -29 dB */
	0x000028C4,		/* SMEM coding of -28 dB */
	0x00002DBD,		/* SMEM coding of -27 dB */
	0x00003352,		/* SMEM coding of -26 dB */
	0x00003995,		/* SMEM coding of -25 dB */
	0x0000409C,		/* SMEM coding of -24 dB */
	0x0000487E,		/* SMEM coding of -23 dB */
	0x00005156,		/* SMEM coding of -22 dB */
	0x00005B43,		/* SMEM coding of -21 dB */
	0x00006666,		/* SMEM coding of -20 dB */
	0x000072E5,		/* SMEM coding of -19 dB */
	0x000080E9,		/* SMEM coding of -18 dB */
	0x000090A4,		/* SMEM coding of -17 dB */
	0x0000A24B,		/* SMEM coding of -16 dB */
	0x0000B618,		/* SMEM coding of -15 dB */
	0x0000CC50,		/* SMEM coding of -14 dB */
	0x0000E53E,		/* SMEM coding of -13 dB */
	0x00010137,		/* SMEM coding of -12 dB */
	0x0001209A,		/* SMEM coding of -11 dB */
	0x000143D1,		/* SMEM coding of -10 dB */
	0x00016B54,		/* SMEM coding of -9 dB */
	0x000197A9,		/* SMEM coding of -8 dB */
	0x0001C967,		/* SMEM coding of -7 dB */
	0x00020137,		/* SMEM coding of -6 dB */
	0x00023FD6,		/* SMEM coding of -5 dB */
	0x00028619,		/* SMEM coding of -4 dB */
	0x0002D4EF,		/* SMEM coding of -3 dB */
	0x00032D64,		/* SMEM coding of -2 dB */
	0x000390A4,		/* SMEM coding of -1 dB */
	0x00040000,		/* SMEM coding of 0 dB */
	0x00047CF2,		/* SMEM coding of 1 dB */
	0x00050923,		/* SMEM coding of 2 dB */
	0x0005A670,		/* SMEM coding of 3 dB */
	0x000656EE,		/* SMEM coding of 4 dB */
	0x00071CF5,		/* SMEM coding of 5 dB */
	0x0007FB26,		/* SMEM coding of 6 dB */
	0x0008F473,		/* SMEM coding of 7 dB */
	0x000A0C2B,		/* SMEM coding of 8 dB */
	0x000B4606,		/* SMEM coding of 9 dB */
	0x000CA62C,		/* SMEM coding of 10 dB */
	0x000E314A,		/* SMEM coding of 11 dB */
	0x000FEC9E,		/* SMEM coding of 12 dB */
	0x0011DE0A,		/* SMEM coding of 13 dB */
	0x00140C28,		/* SMEM coding of 14 dB */
	0x00167E60,		/* SMEM coding of 15 dB */
	0x00193D00,		/* SMEM coding of 16 dB */
	0x001C515D,		/* SMEM coding of 17 dB */
	0x001FC5EB,		/* SMEM coding of 18 dB */
	0x0023A668,		/* SMEM coding of 19 dB */
	0x00280000,		/* SMEM coding of 20 dB */
	0x002CE178,		/* SMEM coding of 21 dB */
	0x00325B65,		/* SMEM coding of 22 dB */
	0x00388062,		/* SMEM coding of 23 dB */
	0x003F654E,		/* SMEM coding of 24 dB */
	0x00472194,		/* SMEM coding of 25 dB */
	0x004FCF7C,		/* SMEM coding of 26 dB */
	0x00598C81,		/* SMEM coding of 27 dB */
	0x006479B7,		/* SMEM coding of 28 dB */
	0x0070BC3D,		/* SMEM coding of 29 dB */
	0x007E7DB9,		/* SMEM coding of 30 dB */
};

static const u32 aess_1_alpha_iir[64] = {
	0x040002, 0x040002, 0x040002, 0x040002,	/* 0 */
	0x50E955, 0x48CA65, 0x40E321, 0x72BE78,	/* 1 [ms] */
	0x64BA68, 0x57DF14, 0x4C3D60, 0x41D690,	/* 2 */
	0x38A084, 0x308974, 0x297B00, 0x235C7C,	/* 4 */
	0x1E14B0, 0x198AF0, 0x15A800, 0x125660,	/* 8 */
	0x0F82A0, 0x0D1B5C, 0x0B113C, 0x0956CC,	/* 16 */
	0x07E054, 0x06A3B8, 0x059844, 0x04B680,	/* 32 */
	0x03F80C, 0x035774, 0x02D018, 0x025E0C,	/* 64 */
	0x7F8057, 0x6B482F, 0x5A4297, 0x4BEECB,	/* 128 */
	0x3FE00B, 0x35BAA7, 0x2D3143, 0x2602AF,	/* 256 */
	0x1FF803, 0x1AE2FB, 0x169C9F, 0x13042B,	/* 512 */
	0x0FFE03, 0x0D72E7, 0x0B4F4F, 0x0982CB,	/* 1.024 [s] */
	0x07FF83, 0x06B9CF, 0x05A7E7, 0x04C193,	/* 2.048 */
	0x03FFE3, 0x035CFF, 0x02D403, 0x0260D7,	/* 4.096 */
	0x01FFFB, 0x01AE87, 0x016A07, 0x01306F,	/* 8.192 */
	0x00FFFF, 0x00D743, 0x00B503, 0x009837,
};

static const u32 aess_alpha_iir[64] = {
	0x000000, 0x000000, 0x000000, 0x000000,	/* 0 */
	0x5E2D58, 0x6E6B3C, 0x7E39C0, 0x46A0C5,	/* 1 [ms] */
	0x4DA2CD, 0x541079, 0x59E151, 0x5F14B9,	/* 2 */
	0x63AFC1, 0x67BB45, 0x6B4281, 0x6E51C1,	/* 4 */
	0x70F5A9, 0x733A89, 0x752C01, 0x76D4D1,	/* 8 */
	0x783EB1, 0x797251, 0x7A7761, 0x7B549D,	/* 16 */
	0x7C0FD5, 0x7CAE25, 0x7D33DD, 0x7DA4C1,	/* 32 */
	0x7E03FD, 0x7E5449, 0x7E97F5, 0x7ED0F9,	/* 64 */
	0x7F0101, 0x7F2971, 0x7F4B7D, 0x7F6825,	/* 128 */
	0x7F8041, 0x7F948D, 0x7FA59D, 0x7FB3FD,	/* 256 */
	0x7FC011, 0x7FCA3D, 0x7FD2C9, 0x7FD9F9,	/* 512 */
	0x7FE005, 0x7FE51D, 0x7FE961, 0x7FECFD,	/* 1.024 [s] */
	0x7FF001, 0x7FF28D, 0x7FF4B1, 0x7FF67D,	/* 2.048 */
	0x7FF801, 0x7FF949, 0x7FFA59, 0x7FFB41,	/* 4.096 */
	0x7FFC01, 0x7FFCA5, 0x7FFD2D, 0x7FFDA1,	/* 8.192 */
	0x7FFE01, 0x7FFE51, 0x7FFE95, 0x7FFED1,
};

/**
 * omap_aess_write_gain
 * @aess: Pointer on aess handle
 * @id: gain name or mixer name
 * @f_g: input gain for the mixer
 *
 * Loads the gain coefficients to FW memory. This API can be called when
 * the corresponding MIXER is not activated. After reloading the firmware
 * the default coefficients corresponds to "all input and output mixer's gain
 * in mute state". A mixer is disabled with a network reconfiguration
 * corresponding to an OPP value.
 */
void omap_aess_write_gain(struct omap_aess *aess, u32 id, s32 f_g)
{
	struct omap_aess_gain *gain = &aess->gains[id];
	u32 lin_g, mixer_target;
	s32 gain_index;

	gain_index = (f_g - OMAP_AESS_GAIN_MIN_MDB) / 100;
	gain_index = max(gain_index, 0);
	gain_index = min(gain_index, (s32)OMAP_AESS_GAIN_DB2LIN_SIZE);
	lin_g = aess_db2lin_table[gain_index];
	/* save the input parameters for mute/unmute */
	gain->desired_linear = lin_g;
	gain->desired_decibel = f_g;

	/* SMEM address in bytes */
	mixer_target = aess->fw_info.map[OMAP_AESS_SMEM_GTARGET1_ID].offset;
	mixer_target += (id << 2);

	if (!gain->muted_indicator)
		/* load the S_G_Target SMEM table */
		omap_aess_write(aess, OMAP_AESS_BANK_SMEM, mixer_target, &lin_g,
				sizeof(lin_g));
	else
		/* update muted gain with new value */
		gain->muted_decibel = f_g;
}

/**
 * omap_aess_disable_gain
 * @aess: Pointer on aess handle
 * @id: name of the gain
 *
 * Set gain to silence if not already mute or disable.
 */
void omap_aess_disable_gain(struct omap_aess *aess, u32 id)
{
	struct omap_aess_gain *gain = &aess->gains[id];

	if (!(gain->muted_indicator & OMAP_AESS_GAIN_DISABLED)) {
		/* Check if we are in mute */
		if (!(gain->muted_indicator & OMAP_AESS_GAIN_MUTED)) {
			gain->muted_decibel = gain->desired_decibel;
			/* mute the gain */
			omap_aess_write_gain(aess, id, GAIN_MUTE);
		}
		gain->muted_indicator |= OMAP_AESS_GAIN_DISABLED;
	}
}

/**
 * omap_aess_enable_gain
 * @aess: Pointer on aess handle
 * @id: name of the gain
 *
 * Restore gain if we are in disable mode.
 */
void omap_aess_enable_gain(struct omap_aess *aess, u32 id)
{
	struct omap_aess_gain *gain = &aess->gains[id];

	if ((gain->muted_indicator & OMAP_AESS_GAIN_DISABLED)) {
		/* restore the input parameters for mute/unmute */
		gain->muted_indicator &= ~OMAP_AESS_GAIN_DISABLED;
		/* unmute the gain */
		omap_aess_write_gain(aess, id, gain->muted_decibel);
	}
}

/**
 * omap_aess_mute_gain
 * @aess: Pointer on aess handle
 * @id: name of the gain
 *
 * Set gain to silence if not already mute.
 */
void omap_aess_mute_gain(struct omap_aess *aess, u32 id)
{
	struct omap_aess_gain *gain = &aess->gains[id];

	if (!gain->muted_indicator) {
		gain->muted_decibel = gain->desired_decibel;
		/* mute the gain */
		omap_aess_write_gain(aess, id, GAIN_MUTE);
	}
	gain->muted_indicator |= OMAP_AESS_GAIN_MUTED;
}

/**
 * omap_aess_unmute_gain
 * @aess: Pointer on aess handle
 * @id: name of the gain
 *
 * Restore gain after mute.
 */
void omap_aess_unmute_gain(struct omap_aess *aess, u32 id)
{
	struct omap_aess_gain *gain = &aess->gains[id];

	if ((gain->muted_indicator & OMAP_AESS_GAIN_MUTED)) {
		/* restore the input parameters for mute/unmute */
		u32 f_g = gain->muted_decibel;

		gain->muted_indicator &= ~OMAP_AESS_GAIN_MUTED;
		/* unmute the gain */
		omap_aess_write_gain(aess, id, f_g);
	}
}

static void omap_aess_write_mute_gain(struct omap_aess *aess, u32 id, s32 f_g)
{
	omap_aess_write_gain(aess, id, f_g);
	omap_aess_mute_gain(aess, id);
}

/**
 * omap_aess_write_gain_ramp
 * @aess: Pointer on aess handle
 * @id: gain name or mixer name
 * @ramp: Gaim ramp time
 *
 * Loads the gain ramp for the associated gain.
 */
static void omap_aess_write_gain_ramp(struct omap_aess *aess, u32 id, u32 ramp)
{
	u32 mixer_target;
	u32 alpha, beta;
	u32 ramp_index;

	aess->gains[id].desired_ramp_delay_ms = ramp;

	/* SMEM address in bytes */
	mixer_target = aess->fw_info.map[OMAP_AESS_SMEM_GTARGET1_ID].offset;
	mixer_target += (id << 2);

	ramp = max(min(ramp, (u32)RAMP_MAXLENGTH), (u32)RAMP_MINLENGTH);
	/* ramp data should be interpolated in the table instead */
	ramp_index = 3;
	if ((RAMP_2MS <= ramp) && (ramp < RAMP_5MS))
		ramp_index = 8;
	if ((RAMP_5MS <= ramp) && (ramp < RAMP_50MS))
		ramp_index = 24;
	if ((RAMP_50MS <= ramp) && (ramp < RAMP_500MS))
		ramp_index = 36;
	if (ramp > RAMP_500MS)
		ramp_index = 48;
	beta = aess_alpha_iir[ramp_index];
	alpha = aess_1_alpha_iir[ramp_index];
	/* CMEM bytes address */
	mixer_target = aess->fw_info.map[OMAP_AESS_CMEM_1_ALPHA_ID].offset;
	/* a pair of gains is updated once in the firmware */
	mixer_target += (id >> 1) << 2;
	/* load the ramp delay data */
	omap_aess_write(aess, OMAP_AESS_BANK_CMEM, mixer_target, &alpha,
			sizeof(alpha));
	/* CMEM bytes address */
	mixer_target = aess->fw_info.map[OMAP_AESS_CMEM_ALPHA_ID].offset;
	/* a pair of gains is updated once in the firmware */
	mixer_target += (id >> 1) << 2;
	omap_aess_write(aess, OMAP_AESS_BANK_CMEM, mixer_target, &beta,
			sizeof(beta));
}

/**
 * omap_aess_read_gain
 * @aess: Pointer on aess handle
 * @id: name of the mixer
 * @f_g: pointer on the gain for the mixer
 *
 * Read the gain coefficients in FW memory. This API can be called when
 * the corresponding MIXER is not activated. After reloading the firmware
 * the default coefficients corresponds to "all input and output mixer's
 * gain in mute state". A mixer is disabled with a network reconfiguration
 * corresponding to an OPP value.
 */
int omap_aess_read_gain(struct omap_aess *aess, u32 id, u32 *f_g)
{
	u32 mixer_target, i;

	/* SMEM bytes address */
	mixer_target = aess->fw_info.map[OMAP_AESS_SMEM_GTARGET1_ID].offset;
	mixer_target += (id << 2);
	if (!aess->gains[id].muted_indicator) {
		/* load the S_G_Target SMEM table */
		omap_aess_read(aess, OMAP_AESS_BANK_SMEM, mixer_target, f_g,
			       sizeof(*f_g));
		for (i = 0; i < OMAP_AESS_GAIN_DB2LIN_SIZE; i++) {
				if (aess_db2lin_table[i] == *f_g)
					goto found;
		}
		*f_g = 0;
		return -EINVAL;
found:
		*f_g = (i * 100) + OMAP_AESS_GAIN_MIN_MDB;
	} else {
		/* update muted gain with new value */
		*f_g = aess->gains[id].muted_decibel;
	}
	return 0;
}

/**
 * omap_aess_reset_gain_mixer
 * @aess: Pointer on aess handle
 * @id: name of the mixer
 *
 * restart the working gain value of the mixers when a port is enabled
 */
void omap_aess_reset_gain_mixer(struct omap_aess *aess, u32 id)
{
	u32 lin_g, mixer_target;

	/* SMEM bytes address for the CURRENT gain values */
	mixer_target = aess->fw_info.map[OMAP_AESS_SMEM_GCURRENT_ID].offset;
	mixer_target += (id << 2);
	lin_g = 0;
	/* load the S_G_Target SMEM table */
	omap_aess_write(aess, OMAP_AESS_BANK_SMEM, mixer_target, &lin_g,
			sizeof(lin_g));
}

/**
 * omap_aess_init_gain_ramp
 * @aess: Pointer on aess handle
 *
 * load default gain ramp configuration.
 */
void omap_aess_init_gain_ramp(struct omap_aess *aess)
{
	int i;

	/* Set 2ms ramp for all mixer and gain */
	for (i = 0; i < MAX_NBGAIN_CMEM; i++) {
		omap_aess_write_gain_ramp(aess, i, RAMP_2MS);
	}
}

/**
 * omap_aess_init_gains
 * @aess: Pointer on aess handle
 *
 * load default gain configuration.
 */
void omap_aess_init_gains(struct omap_aess *aess)
{
	/* mixers' configuration */
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DMIC1_LEFT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DMIC1_RIGHT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DMIC2_LEFT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DMIC2_RIGHT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DMIC3_LEFT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DMIC3_RIGHT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_AMIC_LEFT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_AMIC_RIGHT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DL1_LEFT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DL1_RIGHT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DL2_LEFT, GAIN_M7dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_DL2_RIGHT, GAIN_M7dB);
	omap_aess_write_gain(aess, OMAP_AESS_GAIN_SPLIT_LEFT, GAIN_0dB);
	omap_aess_write_gain(aess, OMAP_AESS_GAIN_SPLIT_RIGHT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXDL1_MM_DL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXDL1_MM_UL2, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXDL1_VX_DL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXDL1_TONES, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXDL2_MM_DL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXDL2_MM_UL2, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXDL2_VX_DL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXDL2_TONES, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXECHO_DL1, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXECHO_DL2, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXSDT_UL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXSDT_DL, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXVXREC_MM_DL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXVXREC_TONES, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXVXREC_VX_UL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXVXREC_VX_DL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXAUDUL_MM_DL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXAUDUL_TONES, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXAUDUL_UPLINK, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_MIXAUDUL_VX_DL, GAIN_MUTE);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_BTUL_LEFT, GAIN_0dB);
	omap_aess_write_mute_gain(aess, OMAP_AESS_GAIN_BTUL_RIGHT, GAIN_0dB);
}
