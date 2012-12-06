/*
 * linux/sound/socfw.h -- ALSA SoC Layer
 *
 * Copyright:	2012 Texas Instruments Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Simple file API to load FW, dynamic mixers, coefficients, DAPM graphs,
 * algorithms, equalisers, etc.
 */

#ifndef __SOC_FW_H
#define __SOC_FW_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <sound/asound.h>
#include <sound/asoc.h>
#include <sound/tlv.h>

/* kernel typedefs */
typedef	uint32_t u32;
typedef	int32_t s32;
typedef	uint16_t u16;
typedef	int16_t s16;
typedef	uint8_t u8;
typedef	int8_t s8;

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))

/*
 * Generic Coefficients with Kcontrols
 */

#define SND_SOC_FW_COEFF_ELEM(text, c) \
	{.description = text, \
	.coeffs = (const void*)c, \
	.size = ARRAY_SIZE(c) * sizeof(c[0])}

#define SND_SOC_FW_COEFFICIENT(i, idx, text, e) \
	{.description = text, .index = idx, \
	.elems = e, .id = i,\
	.count = ARRAY_SIZE(e)}

struct snd_soc_fw_coeff_elem {
	const void *coeffs;		/* coefficient data */
	u32 size;			/* coefficient data size bytes */
	const char *description;	/* description e.g. "4kHz LPF 0dB" */
};

struct snd_soc_fw_coeff {
	u32 id;
	u32 index;
	const char *description;	/* description e.g. "EQ1" */
	int count;			/* number of coefficient elements */
	const struct snd_soc_fw_coeff_elem *elems;	/* elements */
};

/*
 * PLUGIN API.
 */

struct snd_soc_fw_plugin {
	const struct snd_soc_dapm_route *graph;
	int graph_count;

	const struct snd_soc_dapm_widget *widgets;	
	int widget_count;

	const struct snd_kcontrol_new *kcontrols;
	int kcontrol_count;

	const struct snd_soc_fw_coeff *coeffs;
	int coeff_count;

	int version;
};

extern __attribute__((const, noreturn))
int ____ilog2_NaN(void);

/**
 * ilog2 - log of base 2 of 32-bit or a 64-bit unsigned value
 * @n - parameter
 *
 * constant-capable log of base 2 calculation
 * - this can be used to initialise global variables from constant data, hence
 *   the massive ternary operator construction
 *
 * selects the appropriately-sized optimised version depending on sizeof(n)
 */
#define ilog2(n)				\
(						\
		(n) < 1 ? ____ilog2_NaN() :	\
		(n) & (1ULL << 63) ? 63 :	\
		(n) & (1ULL << 62) ? 62 :	\
		(n) & (1ULL << 61) ? 61 :	\
		(n) & (1ULL << 60) ? 60 :	\
		(n) & (1ULL << 59) ? 59 :	\
		(n) & (1ULL << 58) ? 58 :	\
		(n) & (1ULL << 57) ? 57 :	\
		(n) & (1ULL << 56) ? 56 :	\
		(n) & (1ULL << 55) ? 55 :	\
		(n) & (1ULL << 54) ? 54 :	\
		(n) & (1ULL << 53) ? 53 :	\
		(n) & (1ULL << 52) ? 52 :	\
		(n) & (1ULL << 51) ? 51 :	\
		(n) & (1ULL << 50) ? 50 :	\
		(n) & (1ULL << 49) ? 49 :	\
		(n) & (1ULL << 48) ? 48 :	\
		(n) & (1ULL << 47) ? 47 :	\
		(n) & (1ULL << 46) ? 46 :	\
		(n) & (1ULL << 45) ? 45 :	\
		(n) & (1ULL << 44) ? 44 :	\
		(n) & (1ULL << 43) ? 43 :	\
		(n) & (1ULL << 42) ? 42 :	\
		(n) & (1ULL << 41) ? 41 :	\
		(n) & (1ULL << 40) ? 40 :	\
		(n) & (1ULL << 39) ? 39 :	\
		(n) & (1ULL << 38) ? 38 :	\
		(n) & (1ULL << 37) ? 37 :	\
		(n) & (1ULL << 36) ? 36 :	\
		(n) & (1ULL << 35) ? 35 :	\
		(n) & (1ULL << 34) ? 34 :	\
		(n) & (1ULL << 33) ? 33 :	\
		(n) & (1ULL << 32) ? 32 :	\
		(n) & (1ULL << 31) ? 31 :	\
		(n) & (1ULL << 30) ? 30 :	\
		(n) & (1ULL << 29) ? 29 :	\
		(n) & (1ULL << 28) ? 28 :	\
		(n) & (1ULL << 27) ? 27 :	\
		(n) & (1ULL << 26) ? 26 :	\
		(n) & (1ULL << 25) ? 25 :	\
		(n) & (1ULL << 24) ? 24 :	\
		(n) & (1ULL << 23) ? 23 :	\
		(n) & (1ULL << 22) ? 22 :	\
		(n) & (1ULL << 21) ? 21 :	\
		(n) & (1ULL << 20) ? 20 :	\
		(n) & (1ULL << 19) ? 19 :	\
		(n) & (1ULL << 18) ? 18 :	\
		(n) & (1ULL << 17) ? 17 :	\
		(n) & (1ULL << 16) ? 16 :	\
		(n) & (1ULL << 15) ? 15 :	\
		(n) & (1ULL << 14) ? 14 :	\
		(n) & (1ULL << 13) ? 13 :	\
		(n) & (1ULL << 12) ? 12 :	\
		(n) & (1ULL << 11) ? 11 :	\
		(n) & (1ULL << 10) ? 10 :	\
		(n) & (1ULL <<  9) ?  9 :	\
		(n) & (1ULL <<  8) ?  8 :	\
		(n) & (1ULL <<  7) ?  7 :	\
		(n) & (1ULL <<  6) ?  6 :	\
		(n) & (1ULL <<  5) ?  5 :	\
		(n) & (1ULL <<  4) ?  4 :	\
		(n) & (1ULL <<  3) ?  3 :	\
		(n) & (1ULL <<  2) ?  2 :	\
		(n) & (1ULL <<  1) ?  1 :	\
		(n) & (1ULL <<  0) ?  0 :	\
		____ilog2_NaN()			\
 )

/**
 * roundup_pow_of_two - round the given value up to nearest power of two
 * @n - parameter
 *
 * round the given value up to the nearest power of two
 * - the result is undefined when n == 0
 * - this can be used to initialise global variables from constant data
 */
#define roundup_pow_of_two(n)			\
(						\
		(n == 1) ? 1 :			\
		(1UL << (ilog2((n) - 1) + 1))	\
 )

/*
 * Kcontrol and DAPM get/put/info types.
 */
#define snd_soc_info_enum_ext		SOC_CONTROL_TYPE_ENUM_EXT
#define snd_soc_info_enum_double	SOC_CONTROL_TYPE_ENUM
#define snd_soc_get_enum_double		SOC_CONTROL_TYPE_ENUM
#define snd_soc_put_enum_double		SOC_CONTROL_TYPE_ENUM
#define snd_soc_get_value_enum_double	SOC_CONTROL_TYPE_ENUM_VALUE
#define snd_soc_put_value_enum_double	SOC_CONTROL_TYPE_ENUM_VALUE
#define snd_soc_info_volsw		SOC_CONTROL_TYPE_VOLSW
#define snd_soc_info_volsw_ext		SOC_CONTROL_TYPE_VOLSW_EXT
#define snd_soc_info_bool_ext		SOC_CONTROL_TYPE_BOOL
#define snd_soc_get_volsw		SOC_CONTROL_TYPE_VOLSW
#define snd_soc_put_volsw		SOC_CONTROL_TYPE_VOLSW
#define snd_soc_get_volsw_2r 		snd_soc_get_volsw
#define snd_soc_put_volsw_2r 		snd_soc_put_volsw
#define snd_soc_get_volsw_sx		SOC_CONTROL_TYPE_VOLSW_SX
#define snd_soc_put_volsw_sx		SOC_CONTROL_TYPE_VOLSW_SX
#define snd_soc_info_volsw_s8		SOC_CONTROL_TYPE_VOLSW_S8
#define snd_soc_get_volsw_s8		SOC_CONTROL_TYPE_VOLSW_S8
#define snd_soc_put_volsw_s8		SOC_CONTROL_TYPE_VOLSW_S8
#define snd_soc_info_volsw_range	SOC_CONTROL_TYPE_RANGE
#define snd_soc_put_volsw_range		SOC_CONTROL_TYPE_RANGE
#define snd_soc_get_volsw_range		SOC_CONTROL_TYPE_RANGE
#define snd_soc_bytes_info		SOC_CONTROL_TYPE_BYTES
#define snd_soc_bytes_get		SOC_CONTROL_TYPE_BYTES
#define snd_soc_bytes_put		SOC_CONTROL_TYPE_BYTES
#define snd_soc_info_xr_sx		SOC_CONTROL_TYPE_XR_SX
#define snd_soc_get_xr_sx		SOC_CONTROL_TYPE_XR_SX
#define snd_soc_put_xr_sx		SOC_CONTROL_TYPE_XR_SX
#define snd_soc_get_strobe		SOC_CONTROL_TYPE_STROBE
#define snd_soc_put_strobe		SOC_CONTROL_TYPE_STROBE

#define snd_soc_dapm_put_volsw		SOC_DAPM_TYPE_VOLSW
#define snd_soc_dapm_get_volsw		SOC_DAPM_TYPE_VOLSW
#define snd_soc_dapm_get_enum_double	SOC_DAPM_TYPE_ENUM_DOUBLE
#define snd_soc_dapm_put_enum_double	SOC_DAPM_TYPE_ENUM_DOUBLE
#define snd_soc_dapm_get_enum_virt	SOC_DAPM_TYPE_ENUM_VIRT
#define snd_soc_dapm_put_enum_virt	SOC_DAPM_TYPE_ENUM_VIRT
#define snd_soc_dapm_get_value_enum_double	SOC_DAPM_TYPE_ENUM_VALUE
#define snd_soc_dapm_put_value_enum_double	SOC_DAPM_TYPE_ENUM_VALUE
#define snd_soc_dapm_info_pin_switch	SOC_DAPM_TYPE_PIN
#define snd_soc_dapm_get_pin_switch	SOC_DAPM_TYPE_PIN
#define snd_soc_dapm_put_pin_switch	SOC_DAPM_TYPE_PIN

/*
 * Kernel Kcontrol and DAPM types compressed below for userspace control and
 * widget building. i.e. missing any status and kernel types. These types are
 * NOT written to file as FW but are used for familiarity and easy reuse of
 * code for defining widgets and kcontrols. 
 */

/* mixer control */
struct soc_mixer_control {
	int min, max, platform_max;
	unsigned int reg, rreg, shift, rshift, invert;
};

struct soc_bytes {
	int base;
	int num_regs;
	u32 mask;
};

/* multi register control */
struct soc_mreg_control {
	long min, max;
	unsigned int regbase, regcount, nbits, invert;
};

/* enumerated kcontrol */
struct soc_enum {
	unsigned short reg;
	unsigned short reg2;
	unsigned char shift_l;
	unsigned char shift_r;
	unsigned int max;
	unsigned int mask;
	const char * const *texts;
	const unsigned int *values;
};

struct snd_soc_dapm_route {
	const char *sink;
	const char *control;
	const char *source;
};

struct snd_kcontrol;

struct snd_kcontrol_new {
	int iface;	/* interface identifier */
	int get;
	int put;
	int info;
	int index;
	int access;
	const unsigned char *name;	/* ASCII name of item */
	unsigned int count;		/* count of same elements */
	union {
		const unsigned int *p;
	} tlv;
	unsigned long private_value;
};

/* dapm widget */
struct snd_soc_dapm_widget {
	enum snd_soc_dapm_type id;
	const char *name;		/* widget name */
	const char *sname;	/* stream name */

	/* dapm control */
	int reg;				/* negative reg = no direct dapm */
	unsigned char shift;			/* bits to shift */
	unsigned int saved_value;		/* widget saved value */
	unsigned int value;				/* widget current value */
	unsigned int mask;			/* non-shifted mask */
	unsigned char invert:1;			/* invert the power bit */
	unsigned char ignore_suspend:1;         /* kept enabled over suspend */
	unsigned char denum:1;		/* dynamic enum control */
	unsigned char dmixer:1;		/* dynamic mixer control*/

	int num_kcontrols;
	const struct snd_kcontrol_new *kcontrol_news;
};

#endif
