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

#include <sys/types.h>
#include <stdint.h>

#include <sound/asound.h>

/* kernel typedefs */
typedef	uint32_t u32;
typedef	int32_t s32;
typedef	uint16_t u16;
typedef	int16_t s16;
typedef	uint8_t u8;
typedef	int8_t s8;

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))

#include <soc-fw.h>

/*
 * Generic Coefficients with Kcontrols
 */

#define SND_SOC_FW_COEFF_ELEM(text, c) \
	{.description = text, \
	.coeffs = (const void*)c, \
	.size = ARRAY_SIZE(c) * sizeof(c[0])}

#define SND_SOC_FW_COEFFICIENT(i, text, e) \
	{.description = text, \
	.elems = e, .id = i,\
	.count = ARRAY_SIZE(e)}

struct snd_soc_fw_coeff_elem {
	const void *coeffs;		/* coefficient data */
	u32 size;			/* coefficient data size bytes */
	const char *description;	/* description e.g. "4kHz LPF 0dB" */
};

struct snd_soc_fw_coeff {
	u32 id;
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

/*
 * ASoC C types to support copy and paste from Kernel Source files.
 */

/*
 * Kernel headers
 */

#include <soc.h>
#include <soc-dapm.h>
#include <tlv.h>

/*
 * Kcontrol and DAPM get/put/info types.
 */
#define snd_soc_info_enum_ext		SOC_MIXER_IO_ENUM_EXT
#define snd_soc_info_enum_double	SOC_MIXER_IO_ENUM
#define snd_soc_get_enum_double		SOC_MIXER_IO_ENUM
#define snd_soc_put_enum_double		SOC_MIXER_IO_ENUM
#define snd_soc_get_value_enum_double	SOC_MIXER_IO_ENUM_VALUE
#define snd_soc_put_value_enum_double	SOC_MIXER_IO_ENUM_VALUE
#define snd_soc_info_volsw		SOC_MIXER_IO_VOLSW
#define snd_soc_info_volsw_ext		SOC_MIXER_IO_VOLSW_EXT
#define snd_soc_info_bool_ext		SOC_MIXER_IO_BOOL
#define snd_soc_get_volsw		SOC_MIXER_IO_VOLSW
#define snd_soc_put_volsw		SOC_MIXER_IO_VOLSW
#define snd_soc_get_volsw_2r 		snd_soc_get_volsw
#define snd_soc_put_volsw_2r 		snd_soc_put_volsw
#define snd_soc_get_volsw_sx		SOC_MIXER_IO_VOLSW_SX
#define snd_soc_put_volsw_sx		SOC_MIXER_IO_VOLSW_SX
#define snd_soc_info_volsw_s8		SOC_MIXER_IO_VOLSW_S8
#define snd_soc_get_volsw_s8		SOC_MIXER_IO_VOLSW_S8
#define snd_soc_put_volsw_s8		SOC_MIXER_IO_VOLSW_S8
#define snd_soc_info_volsw_range	SOC_MIXER_IO_RANGE
#define snd_soc_put_volsw_range		SOC_MIXER_IO_RANGE
#define snd_soc_get_volsw_range		SOC_MIXER_IO_RANGE
#define snd_soc_bytes_info		SOC_MIXER_IO_BYTES
#define snd_soc_bytes_get		SOC_MIXER_IO_BYTES
#define snd_soc_bytes_put		SOC_MIXER_IO_BYTES
#define snd_soc_info_xr_sx		SOC_MIXER_IO_XR_SX
#define snd_soc_get_xr_sx		SOC_MIXER_IO_XR_SX
#define snd_soc_put_xr_sx		SOC_MIXER_IO_XR_SX
#define snd_soc_get_strobe		SOC_MIXER_IO_STROBE
#define snd_soc_put_strobe		SOC_MIXER_IO_STROBE

#define snd_soc_dapm_put_volsw		SOC_DAPM_IO_VOLSW
#define snd_soc_dapm_get_volsw		SOC_DAPM_IO_VOLSW
#define snd_soc_dapm_get_enum_double	SOC_DAPM_IO_ENUM_DOUBLE
#define snd_soc_dapm_put_enum_double	SOC_DAPM_IO_ENUM_DOUBLE
#define snd_soc_dapm_get_enum_virt	SOC_DAPM_IO_ENUM_VIRT
#define snd_soc_dapm_put_enum_virt	SOC_DAPM_IO_ENUM_VIRT
#define snd_soc_dapm_get_value_enum_double	SOC_DAPM_IO_ENUM_VALUE
#define snd_soc_dapm_put_value_enum_double	SOC_DAPM_IO_ENUM_VALUE
#define snd_soc_dapm_info_pin_switch	SOC_DAPM_IO_PIN
#define snd_soc_dapm_get_pin_switch	SOC_DAPM_IO_PIN
#define snd_soc_dapm_put_pin_switch	SOC_DAPM_IO_PIN

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
