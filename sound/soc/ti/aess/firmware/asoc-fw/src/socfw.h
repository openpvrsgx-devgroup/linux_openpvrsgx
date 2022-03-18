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

#define SNDRV_CTL_ELEM_ID_NAME_MAXLEN 44	// from include/uapi/sound/asound.h

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
#define snd_soc_dapm_info_pin_switch	SOC_DAPM_TYPE_PIN
#define snd_soc_dapm_get_pin_switch	SOC_DAPM_TYPE_PIN
#define snd_soc_dapm_put_pin_switch	SOC_DAPM_TYPE_PIN

#include <sound/soc-dapm.h>

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))

/*
 * Kernel Kcontrol and DAPM types compressed below for userspace control and
 * widget building. i.e. missing any status and kernel types. These types are
 * NOT written to file as FW but are used for familiarity and easy reuse of
 * code for defining widgets and kcontrols. 
 */

/* mixer control */

#if 0 /* disable redefinition of these structs */
struct soc_mixer_control {
	int min, max, platform_max;
	int reg, rreg;
	unsigned int shift, rshift;
	unsigned int sign_bit;
	unsigned int invert:1;
	unsigned int autodisable:1;
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
	unsigned char shift_l;
	unsigned char shift_r;
	unsigned int items;
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
	unsigned int on_val;			/* on state value */
	unsigned int off_val;			/* off state value */
	unsigned char ignore_suspend:1;         /* kept enabled over suspend */
	unsigned char denum:1;		/* dynamic enum control */
	unsigned char dmixer:1;		/* dynamic mixer control*/

	int num_kcontrols;
	const struct snd_kcontrol_new *kcontrol_news;
};

#endif
#endif

// taken from https://git.ti.com/cgit/lcpd-agross/omapdrm/plain/include/uapi/sound/asoc.h?id=c15a2d5a683de12f6768d0efb8df7ecd6aa9b3ed
// we should not append here but add to include/uapi/sound/asoc.h in a way to the Letux kernel sources so that we can eventually upstream it as a patch

/*
 * linux/uapi/sound/asoc.h -- ALSA SoC Firmware Controls and DAPM
 *
 * Author:		Liam Girdwood
 * Created:		Aug 11th 2005
 * Copyright:	Wolfson Microelectronics. PLC.
 *              2012 Texas Instruments Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Simple file API to load FW that includes mixers, coefficients, DAPM graphs,
 * algorithms, equalisers, DAIs, widgets etc.
 */

#ifndef __LINUX_UAPI_SND_ASOC_SOCFW_H
#define __LINUX_UAPI_SND_ASOC_SOCFW_H

// #include <linux/types.h>

/*
 * Convenience kcontrol builders
 */
#define SOC_DOUBLE_VALUE(xreg, shift_left, shift_right, xmax, xinvert) 	((unsigned long)&(struct soc_mixer_control) 	{.reg = xreg, .rreg = xreg, .shift = shift_left, 	.rshift = shift_right, .max = xmax, .platform_max = xmax, 	.invert = xinvert})
#define SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert) 	SOC_DOUBLE_VALUE(xreg, xshift, xshift, xmax, xinvert)
#define SOC_SINGLE_VALUE_EXT(xreg, xmax, xinvert) 	((unsigned long)&(struct soc_mixer_control) 	{.reg = xreg, .max = xmax, .platform_max = xmax, .invert = xinvert})
#define SOC_DOUBLE_R_VALUE(xlreg, xrreg, xshift, xmax, xinvert) 	((unsigned long)&(struct soc_mixer_control) 	{.reg = xlreg, .rreg = xrreg, .shift = xshift, .rshift = xshift, 	.max = xmax, .platform_max = xmax, .invert = xinvert})
#define SOC_DOUBLE_R_RANGE_VALUE(xlreg, xrreg, xshift, xmin, xmax, xinvert) 	((unsigned long)&(struct soc_mixer_control) 	{.reg = xlreg, .rreg = xrreg, .shift = xshift, .rshift = xshift, 	.min = xmin, .max = xmax, .platform_max = xmax, .invert = xinvert})
#define SOC_SINGLE(xname, reg, shift, max, invert) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_volsw, .get = snd_soc_get_volsw,	.put = snd_soc_put_volsw, .index = SOC_CONTROL_IO_VOLSW, 	.private_value =  SOC_SINGLE_VALUE(reg, shift, max, invert) }
#define SOC_SINGLE_RANGE(xname, xreg, xshift, xmin, xmax, xinvert) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),	.info = snd_soc_info_volsw_range, .get = snd_soc_get_volsw_range, 	.put = snd_soc_put_volsw_range, .index = SOC_CONTROL_IO_RANGE, 	.private_value = (unsigned long)&(struct soc_mixer_control) 		{.reg = xreg, .shift = xshift, .min = xmin,		 .max = xmax, .platform_max = xmax, .invert = xinvert} }
#define SOC_SINGLE_TLV(xname, reg, shift, max, invert, tlv_array) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |		 SNDRV_CTL_ELEM_ACCESS_READWRITE,	.tlv.p = (tlv_array), 	.info = snd_soc_info_volsw, .get = snd_soc_get_volsw,	.put = snd_soc_put_volsw, .index = SOC_CONTROL_IO_VOLSW, 	.private_value =  SOC_SINGLE_VALUE(reg, shift, max, invert) }
#define SOC_SINGLE_SX_TLV(xname, xreg, xshift, xmin, xmax, tlv_array) {       .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | 	SNDRV_CTL_ELEM_ACCESS_READWRITE, 	.tlv.p  = (tlv_array),	.info = snd_soc_info_volsw, 	.get = snd_soc_get_volsw_sx,	.put = snd_soc_put_volsw_sx, 	.index = SOC_CONTROL_IO_VOLSW_SX, 	.private_value = (unsigned long)&(struct soc_mixer_control) 		{.reg = xreg, .rreg = xreg, 		.shift = xshift, .rshift = xshift, 		.max = xmax, .min = xmin} }
#define SOC_SINGLE_RANGE_TLV(xname, xreg, xshift, xmin, xmax, xinvert, tlv_array) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |		 SNDRV_CTL_ELEM_ACCESS_READWRITE,	.tlv.p = (tlv_array), 	.info = snd_soc_info_volsw_range, .index = SOC_CONTROL_IO_RANGE, 	.get = snd_soc_get_volsw_range, .put = snd_soc_put_volsw_range, 	.private_value = (unsigned long)&(struct soc_mixer_control) 		{.reg = xreg, .shift = xshift, .min = xmin,		 .max = xmax, .platform_max = xmax, .invert = xinvert} }
#define SOC_DOUBLE(xname, reg, shift_left, shift_right, max, invert) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),	.info = snd_soc_info_volsw, .get = snd_soc_get_volsw, 	.put = snd_soc_put_volsw, .index = SOC_CONTROL_IO_VOLSW, 	.private_value = SOC_DOUBLE_VALUE(reg, shift_left, shift_right, 					  max, invert) }
#define SOC_DOUBLE_R(xname, reg_left, reg_right, xshift, xmax, xinvert) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), 	.info = snd_soc_info_volsw, .index = SOC_CONTROL_IO_VOLSW, 	.get = snd_soc_get_volsw, .put = snd_soc_put_volsw, 	.private_value = SOC_DOUBLE_R_VALUE(reg_left, reg_right, xshift, 					    xmax, xinvert) }
#define SOC_DOUBLE_R_RANGE(xname, reg_left, reg_right, xshift, xmin, 			   xmax, xinvert)		{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),	.info = snd_soc_info_volsw_range, .index = SOC_CONTROL_IO_RANGE, 	.get = snd_soc_get_volsw_range, .put = snd_soc_put_volsw_range, 	.private_value = SOC_DOUBLE_R_RANGE_VALUE(reg_left, reg_right, 					    xshift, xmin, xmax, xinvert) }
#define SOC_DOUBLE_TLV(xname, reg, shift_left, shift_right, max, invert, tlv_array) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |		 SNDRV_CTL_ELEM_ACCESS_READWRITE,	.tlv.p = (tlv_array), 	.info = snd_soc_info_volsw, .get = snd_soc_get_volsw, 	.put = snd_soc_put_volsw, .index = SOC_CONTROL_IO_VOLSW, 	.private_value = SOC_DOUBLE_VALUE(reg, shift_left, shift_right, 					  max, invert) }
#define SOC_DOUBLE_R_TLV(xname, reg_left, reg_right, xshift, xmax, xinvert, tlv_array) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |		 SNDRV_CTL_ELEM_ACCESS_READWRITE,	.tlv.p = (tlv_array), 	.info = snd_soc_info_volsw, .index = SOC_CONTROL_IO_VOLSW, 	.get = snd_soc_get_volsw, .put = snd_soc_put_volsw, 	.private_value = SOC_DOUBLE_R_VALUE(reg_left, reg_right, xshift, 					    xmax, xinvert) }
#define SOC_DOUBLE_R_RANGE_TLV(xname, reg_left, reg_right, xshift, xmin, 			       xmax, xinvert, tlv_array)		{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |		 SNDRV_CTL_ELEM_ACCESS_READWRITE,	.tlv.p = (tlv_array), 	.info = snd_soc_info_volsw_range, .index = SOC_CONTROL_IO_RANGE, 	.get = snd_soc_get_volsw_range, .put = snd_soc_put_volsw_range, 	.private_value = SOC_DOUBLE_R_RANGE_VALUE(reg_left, reg_right, 					    xshift, xmin, xmax, xinvert) }
#define SOC_DOUBLE_R_SX_TLV(xname, xreg, xrreg, xshift, xmin, xmax, tlv_array) {       .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), 	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | 	SNDRV_CTL_ELEM_ACCESS_READWRITE, 	.tlv.p  = (tlv_array), 	.info = snd_soc_info_volsw, 	.get = snd_soc_get_volsw_sx, 	.put = snd_soc_put_volsw_sx, 	.index = SOC_CONTROL_IO_VOLSW, 	.private_value = (unsigned long)&(struct soc_mixer_control) 		{.reg = xreg, .rreg = xrreg, 		.shift = xshift, .rshift = xshift, 		.max = xmax, .min = xmin} }
#define SOC_DOUBLE_S8_TLV(xname, xreg, xmin, xmax, tlv_array) {	.iface  = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), 	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | 		  SNDRV_CTL_ELEM_ACCESS_READWRITE, 	.tlv.p  = (tlv_array), 	.info   = snd_soc_info_volsw_s8, .get = snd_soc_get_volsw_s8, 	.put    = snd_soc_put_volsw_s8, .index = SOC_CONTROL_IO_VOLSW_S8, 	.private_value = (unsigned long)&(struct soc_mixer_control) 		{.reg = xreg, .min = xmin, .max = xmax, 		 .platform_max = xmax} }
#define SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmax, xtexts) {	.reg = xreg, .shift_l = xshift_l, .shift_r = xshift_r, 	.max = xmax, .texts = xtexts, 	.mask = xmax ? roundup_pow_of_two(xmax) - 1 : 0}
#define SOC_ENUM_SINGLE(xreg, xshift, xmax, xtexts) 	SOC_ENUM_DOUBLE(xreg, xshift, xshift, xmax, xtexts)
#define SOC_ENUM_SINGLE_EXT(xmax, xtexts) {	.max = xmax, .texts = xtexts }
#define SOC_VALUE_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmask, xmax, xtexts, xvalues) {	.reg = xreg, .shift_l = xshift_l, .shift_r = xshift_r, 	.mask = xmask, .max = xmax, .texts = xtexts, .values = xvalues}
#define SOC_VALUE_ENUM_SINGLE(xreg, xshift, xmask, xmax, xtexts, xvalues) 	SOC_VALUE_ENUM_DOUBLE(xreg, xshift, xshift, xmask, xmax, xtexts, xvalues)
#define SOC_ENUM(xname, xenum) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,	.info = snd_soc_info_enum_double, .index = SOC_CONTROL_IO_ENUM, 	.get = snd_soc_get_enum_double, .put = snd_soc_put_enum_double, 	.private_value = (unsigned long)&xenum }
#define SOC_VALUE_ENUM(xname, xenum) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,	.info = snd_soc_info_enum_double, 	.get = snd_soc_get_value_enum_double, 	.put = snd_soc_put_value_enum_double, 	.index = SOC_CONTROL_IO_ENUM_VALUE, 	.private_value = (unsigned long)&xenum }
#define SOC_SINGLE_EXT(xname, xreg, xshift, xmax, xinvert,	 xhandler_get, xhandler_put) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_volsw, .index = SOC_CONTROL_IO_EXT, 	.get = xhandler_get, .put = xhandler_put, 	.private_value = SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert) }
#define SOC_DOUBLE_EXT(xname, reg, shift_left, shift_right, max, invert,	 xhandler_get, xhandler_put) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),	.info = snd_soc_info_volsw, .index = SOC_CONTROL_IO_EXT, 	.get = xhandler_get, .put = xhandler_put, 	.private_value = 		SOC_DOUBLE_VALUE(reg, shift_left, shift_right, max, invert) }
#define SOC_SINGLE_EXT_TLV(xname, xreg, xshift, xmax, xinvert,	 xhandler_get, xhandler_put, tlv_array) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |		 SNDRV_CTL_ELEM_ACCESS_READWRITE,	.tlv.p = (tlv_array), 	.info = snd_soc_info_volsw, .index = SOC_CONTROL_IO_EXT, 	.get = xhandler_get, .put = xhandler_put, 	.private_value = SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert) }
#define SOC_DOUBLE_EXT_TLV(xname, xreg, shift_left, shift_right, xmax, xinvert,	 xhandler_get, xhandler_put, tlv_array) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), 	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | 		 SNDRV_CTL_ELEM_ACCESS_READWRITE, 	.tlv.p = (tlv_array), 	.info = snd_soc_info_volsw, .index = SOC_CONTROL_IO_EXT,	.get = xhandler_get, .put = xhandler_put, 	.private_value = SOC_DOUBLE_VALUE(xreg, shift_left, shift_right, 					  xmax, xinvert) }
#define SOC_DOUBLE_R_EXT_TLV(xname, reg_left, reg_right, xshift, xmax, xinvert,	 xhandler_get, xhandler_put, tlv_array) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), 	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | 		 SNDRV_CTL_ELEM_ACCESS_READWRITE, 	.tlv.p = (tlv_array), 	.info = snd_soc_info_volsw, .index = SOC_CONTROL_IO_EXT, 	.get = xhandler_get, .put = xhandler_put, 	.private_value = SOC_DOUBLE_R_VALUE(reg_left, reg_right, xshift, 					    xmax, xinvert) }
#define SOC_SINGLE_BOOL_EXT(xname, xdata, xhandler_get, xhandler_put) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_bool_ext, .index = SOC_CONTROL_IO_BOOL_EXT, 	.get = xhandler_get, .put = xhandler_put, 	.private_value = xdata }
#define SOC_ENUM_EXT(xname, xenum, xhandler_get, xhandler_put) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_enum_ext, .index = SOC_CONTROL_IO_ENUM_EXT, 	.get = xhandler_get, .put = xhandler_put, 	.private_value = (unsigned long)&xenum }

#define SND_SOC_BYTES(xname, xbase, xregs)		      {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,   	.info = snd_soc_bytes_info, .get = snd_soc_bytes_get, 	.index = SOC_CONTROL_IO_BYTES, 	.put = snd_soc_bytes_put, .private_value =	      		((unsigned long)&(struct soc_bytes)           		{.base = xbase, .num_regs = xregs }) }

#define SND_SOC_BYTES_MASK(xname, xbase, xregs, xmask)	      {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,   	.info = snd_soc_bytes_info, .get = snd_soc_bytes_get, 	.index = SOC_CONTROL_IO_BYTES, 	.put = snd_soc_bytes_put, .private_value =	      		((unsigned long)&(struct soc_bytes)           		{.base = xbase, .num_regs = xregs,	      		 .mask = xmask }) }

#define SOC_SINGLE_XR_SX(xname, xregbase, xregcount, xnbits, 		xmin, xmax, xinvert) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), 	.info = snd_soc_info_xr_sx, .get = snd_soc_get_xr_sx, 	.put = snd_soc_put_xr_sx, 	.index = SOC_CONTROL_IO_VOLSW_XR_SX, 	.private_value = (unsigned long)&(struct soc_mreg_control) 		{.regbase = xregbase, .regcount = xregcount, .nbits = xnbits, 		.invert = xinvert, .min = xmin, .max = xmax} }

#define SOC_SINGLE_STROBE(xname, xreg, xshift, xinvert) 	SOC_SINGLE_EXT(xname, xreg, xshift, 1, xinvert, 		snd_soc_get_strobe, snd_soc_put_strobe)

/*
 * Simplified versions of above macros, declaring a struct and calculating
 * ARRAY_SIZE internally
 */
#define SOC_ENUM_DOUBLE_DECL(name, xreg, xshift_l, xshift_r, xtexts) 	struct soc_enum name = SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, 						ARRAY_SIZE(xtexts), xtexts)
#define SOC_ENUM_SINGLE_DECL(name, xreg, xshift, xtexts) 	SOC_ENUM_DOUBLE_DECL(name, xreg, xshift, xshift, xtexts)
#define SOC_ENUM_SINGLE_EXT_DECL(name, xtexts) 	struct soc_enum name = SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(xtexts), xtexts)
#define SOC_VALUE_ENUM_DOUBLE_DECL(name, xreg, xshift_l, xshift_r, xmask, xtexts, xvalues) 	struct soc_enum name = SOC_VALUE_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmask, 							ARRAY_SIZE(xtexts), xtexts, xvalues)
#define SOC_VALUE_ENUM_SINGLE_DECL(name, xreg, xshift, xmask, xtexts, xvalues) 	SOC_VALUE_ENUM_DOUBLE_DECL(name, xreg, xshift, xshift, xmask, xtexts, xvalues)


/*
 * Numeric IDs for stock mixer types that are used to enumerate FW based mixers.
 */

// we add a cast to int because p and g are function pointers (strangely)
#define SOC_CONTROL_ID_PUT(p)	((((int)p) & 0xff) << 16)
#define SOC_CONTROL_ID_GET(g)	((((int)g) & 0xff) << 8)
#define SOC_CONTROL_ID_INFO(i)	((i & 0xff) << 0)
#define SOC_CONTROL_ID(g, p, i)		(SOC_CONTROL_ID_PUT(p) | SOC_CONTROL_ID_GET(g) |	SOC_CONTROL_ID_INFO(i))

#define SOC_CONTROL_GET_ID_PUT(id)	((id & 0xff0000) >> 16)
#define SOC_CONTROL_GET_ID_GET(id)	((id & 0x00ff00) >> 8)
#define SOC_CONTROL_GET_ID_INFO(id)	((id & 0x0000ff) >> 0)

/* individual kcontrol info types - can be mixed with other types */
#define SOC_CONTROL_TYPE_EXT		0	/* driver defined */
#define SOC_CONTROL_TYPE_VOLSW		1
#define SOC_CONTROL_TYPE_VOLSW_SX	2
#define SOC_CONTROL_TYPE_VOLSW_S8	3
#define SOC_CONTROL_TYPE_VOLSW_XR_SX	4
#define SOC_CONTROL_TYPE_ENUM		6
#define SOC_CONTROL_TYPE_ENUM_EXT	7
#define SOC_CONTROL_TYPE_BYTES		8
#define SOC_CONTROL_TYPE_BOOL_EXT	9
#define SOC_CONTROL_TYPE_ENUM_VALUE	10
#define SOC_CONTROL_TYPE_RANGE		11
#define SOC_CONTROL_TYPE_STROBE		12

/* compound control IDs */
#define SOC_CONTROL_IO_VOLSW 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_VOLSW, 		SOC_CONTROL_TYPE_VOLSW, 		SOC_CONTROL_TYPE_VOLSW)
#define SOC_CONTROL_IO_VOLSW_SX 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_VOLSW_SX, 		SOC_CONTROL_TYPE_VOLSW_SX, 		SOC_CONTROL_TYPE_VOLSW)
#define SOC_CONTROL_IO_VOLSW_S8 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_VOLSW_S8, 		SOC_CONTROL_TYPE_VOLSW_S8, 		SOC_CONTROL_TYPE_VOLSW_S8)
#define SOC_CONTROL_IO_VOLSW_XR_SX 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_VOLSW_XR_SX, 		SOC_CONTROL_TYPE_VOLSW_XR_SX, 		SOC_CONTROL_TYPE_VOLSW_XR_SX)
#define SOC_CONTROL_IO_EXT 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_EXT, 		SOC_CONTROL_TYPE_EXT, 		SOC_CONTROL_TYPE_VOLSW)
#define SOC_CONTROL_IO_ENUM 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_ENUM, 		SOC_CONTROL_TYPE_ENUM, 		SOC_CONTROL_TYPE_ENUM)
#define SOC_CONTROL_IO_ENUM_EXT 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_EXT, 		SOC_CONTROL_TYPE_EXT, 		SOC_CONTROL_TYPE_ENUM_EXT)
#define SOC_CONTROL_IO_BYTES 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_BYTES, 		SOC_CONTROL_TYPE_BYTES, 		SOC_CONTROL_TYPE_BYTES)
#define SOC_CONTROL_IO_BOOL_EXT 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_EXT, 		SOC_CONTROL_TYPE_EXT, 		SOC_CONTROL_TYPE_BOOL_EXT)
#define SOC_CONTROL_IO_ENUM_VALUE 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_ENUM_VALUE, 		SOC_CONTROL_TYPE_ENUM_VALUE, 		SOC_CONTROL_TYPE_ENUM)
#define SOC_CONTROL_IO_RANGE 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_RANGE, 		SOC_CONTROL_TYPE_RANGE, 		SOC_CONTROL_TYPE_RANGE)
#define SOC_CONTROL_IO_STROBE 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_STROBE, 		SOC_CONTROL_TYPE_STROBE, 		SOC_CONTROL_TYPE_STROBE)

#if 0	// already defined in soc-dapm.h

/* widget has no PM register bit */
#define SND_SOC_NOPM	-1

/*
 * SoC dynamic audio power management
 *
 * We can have up to 4 power domains
 *  1. Codec domain - VREF, VMID
 *     Usually controlled at codec probe/remove, although can be set
 *     at stream time if power is not needed for sidetone, etc.
 *  2. Platform/Machine domain - physically connected inputs and outputs
 *     Is platform/machine and user action specific, is set in the machine
 *     driver and by userspace e.g when HP are inserted
 *  3. Path domain - Internal codec path mixers
 *     Are automatically set when mixer and mux settings are
 *     changed by the user.
 *  4. Stream domain - DAC's and ADC's.
 *     Enabled when stream playback/capture is started.
 */

/* codec domain */
#define SND_SOC_DAPM_VMID(wname) {	.id = snd_soc_dapm_vmid, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0}

/* platform domain */
#define SND_SOC_DAPM_SIGGEN(wname) {	.id = snd_soc_dapm_siggen, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM }
#define SND_SOC_DAPM_INPUT(wname) {	.id = snd_soc_dapm_input, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM }
#define SND_SOC_DAPM_OUTPUT(wname) {	.id = snd_soc_dapm_output, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM }
#define SND_SOC_DAPM_MIC(wname, wevent) {	.id = snd_soc_dapm_mic, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, 	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD}
#define SND_SOC_DAPM_HP(wname, wevent) {	.id = snd_soc_dapm_hp, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, 	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_SPK(wname, wevent) {	.id = snd_soc_dapm_spk, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, 	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_LINE(wname, wevent) {	.id = snd_soc_dapm_line, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, 	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD}

/* path domain */
#define SND_SOC_DAPM_PGA(wname, wreg, wshift, winvert,	 wcontrols, wncontrols) {	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
#define SND_SOC_DAPM_OUT_DRV(wname, wreg, wshift, winvert,	 wcontrols, wncontrols) {	.id = snd_soc_dapm_out_drv, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
#define SND_SOC_DAPM_MIXER(wname, wreg, wshift, winvert, 	 wcontrols, wncontrols){	.id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
#define SND_SOC_DAPM_MIXER_NAMED_CTL(wname, wreg, wshift, winvert, 	 wcontrols, wncontrols){       .id = snd_soc_dapm_mixer_named_ctl, .name = wname, .reg = wreg, 	.shift = wshift, .invert = winvert, .kcontrol_news = wcontrols, 	.num_kcontrols = wncontrols}
#define SND_SOC_DAPM_MICBIAS(wname, wreg, wshift, winvert) {	.id = snd_soc_dapm_micbias, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = NULL, .num_kcontrols = 0}
#define SND_SOC_DAPM_SWITCH(wname, wreg, wshift, winvert, wcontrols) {	.id = snd_soc_dapm_switch, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1}
#define SND_SOC_DAPM_MUX(wname, wreg, wshift, winvert, wcontrols) {	.id = snd_soc_dapm_mux, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1}
#define SND_SOC_DAPM_VIRT_MUX(wname, wreg, wshift, winvert, wcontrols) {	.id = snd_soc_dapm_virt_mux, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1}
#define SND_SOC_DAPM_VALUE_MUX(wname, wreg, wshift, winvert, wcontrols) {	.id = snd_soc_dapm_value_mux, .name = wname, .reg = wreg, 	.shift = wshift, .invert = winvert, .kcontrol_news = wcontrols, 	.num_kcontrols = 1}

/* Simplified versions of above macros, assuming wncontrols = ARRAY_SIZE(wcontrols) */
#define SOC_PGA_ARRAY(wname, wreg, wshift, winvert,	 wcontrols) {	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = ARRAY_SIZE(wcontrols)}
#define SOC_MIXER_ARRAY(wname, wreg, wshift, winvert, 	 wcontrols){	.id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = ARRAY_SIZE(wcontrols)}
#define SOC_MIXER_NAMED_CTL_ARRAY(wname, wreg, wshift, winvert, 	 wcontrols){       .id = snd_soc_dapm_mixer_named_ctl, .name = wname, .reg = wreg, 	.shift = wshift, .invert = winvert, .kcontrol_news = wcontrols, 	.num_kcontrols = ARRAY_SIZE(wcontrols)}

/* path domain with event - event handler must return 0 for success */
#define SND_SOC_DAPM_PGA_E(wname, wreg, wshift, winvert, wcontrols, 	wncontrols, wevent, wflags) {	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols, 	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_OUT_DRV_E(wname, wreg, wshift, winvert, wcontrols, 	wncontrols, wevent, wflags) {	.id = snd_soc_dapm_out_drv, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols, 	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_MIXER_E(wname, wreg, wshift, winvert, wcontrols, 	wncontrols, wevent, wflags) {	.id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols, 	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_MIXER_NAMED_CTL_E(wname, wreg, wshift, winvert, 	wcontrols, wncontrols, wevent, wflags) {       .id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, 	.num_kcontrols = wncontrols, .event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_SWITCH_E(wname, wreg, wshift, winvert, wcontrols, 	wevent, wflags) {	.id = snd_soc_dapm_switch, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1, 	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_MUX_E(wname, wreg, wshift, winvert, wcontrols, 	wevent, wflags) {	.id = snd_soc_dapm_mux, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1, 	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_VIRT_MUX_E(wname, wreg, wshift, winvert, wcontrols, 	wevent, wflags) {	.id = snd_soc_dapm_virt_mux, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1, 	.event = wevent, .event_flags = wflags}

/* additional sequencing control within an event type */
#define SND_SOC_DAPM_PGA_S(wname, wsubseq, wreg, wshift, winvert, 	wevent, wflags) {	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .event = wevent, .event_flags = wflags, 	.subseq = wsubseq}
#define SND_SOC_DAPM_SUPPLY_S(wname, wsubseq, wreg, wshift, winvert, wevent, 	wflags)	{	.id = snd_soc_dapm_supply, .name = wname, .reg = wreg,		.shift = wshift, .invert = winvert, .event = wevent, 	.event_flags = wflags, .subseq = wsubseq}

/* Simplified versions of above macros, assuming wncontrols = ARRAY_SIZE(wcontrols) */
#define SOC_PGA_E_ARRAY(wname, wreg, wshift, winvert, wcontrols, 	wevent, wflags) {	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = ARRAY_SIZE(wcontrols), 	.event = wevent, .event_flags = wflags}
#define SOC_MIXER_E_ARRAY(wname, wreg, wshift, winvert, wcontrols, 	wevent, wflags) {	.id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = ARRAY_SIZE(wcontrols), 	.event = wevent, .event_flags = wflags}
#define SOC_MIXER_NAMED_CTL_E_ARRAY(wname, wreg, wshift, winvert, 	wcontrols, wevent, wflags) {       .id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, 	.invert = winvert, .kcontrol_news = wcontrols, 	.num_kcontrols = ARRAY_SIZE(wcontrols), .event = wevent, .event_flags = wflags}

/* events that are pre and post DAPM */
#define SND_SOC_DAPM_PRE(wname, wevent) {	.id = snd_soc_dapm_pre, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, 	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_POST(wname, wevent) {	.id = snd_soc_dapm_post, .name = wname, .kcontrol_news = NULL, 	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, 	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD}

/* stream domain */
#define SND_SOC_DAPM_AIF_IN(wname, stname, wslot, wreg, wshift, winvert) {	.id = snd_soc_dapm_aif_in, .name = wname, .sname = stname, 	.reg = wreg, .shift = wshift, .invert = winvert }
#define SND_SOC_DAPM_AIF_IN_E(wname, stname, wslot, wreg, wshift, winvert, 			      wevent, wflags)				{	.id = snd_soc_dapm_aif_in, .name = wname, .sname = stname, 	.reg = wreg, .shift = wshift, .invert = winvert, 	.event = wevent, .event_flags = wflags }
#define SND_SOC_DAPM_AIF_OUT(wname, stname, wslot, wreg, wshift, winvert) {	.id = snd_soc_dapm_aif_out, .name = wname, .sname = stname, 	.reg = wreg, .shift = wshift, .invert = winvert }
#define SND_SOC_DAPM_AIF_OUT_E(wname, stname, wslot, wreg, wshift, winvert, 			     wevent, wflags)				{	.id = snd_soc_dapm_aif_out, .name = wname, .sname = stname, 	.reg = wreg, .shift = wshift, .invert = winvert, 	.event = wevent, .event_flags = wflags }
#define SND_SOC_DAPM_DAC(wname, stname, wreg, wshift, winvert) {	.id = snd_soc_dapm_dac, .name = wname, .sname = stname, .reg = wreg, 	.shift = wshift, .invert = winvert}
#define SND_SOC_DAPM_DAC_E(wname, stname, wreg, wshift, winvert, 			   wevent, wflags)				{	.id = snd_soc_dapm_dac, .name = wname, .sname = stname, .reg = wreg, 	.shift = wshift, .invert = winvert, 	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_ADC(wname, stname, wreg, wshift, winvert) {	.id = snd_soc_dapm_adc, .name = wname, .sname = stname, .reg = wreg, 	.shift = wshift, .invert = winvert}
#define SND_SOC_DAPM_ADC_E(wname, stname, wreg, wshift, winvert, 			   wevent, wflags)				{	.id = snd_soc_dapm_adc, .name = wname, .sname = stname, .reg = wreg, 	.shift = wshift, .invert = winvert, 	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_CLOCK_SUPPLY(wname) {	.id = snd_soc_dapm_clock_supply, .name = wname, 	.reg = SND_SOC_NOPM, .event = dapm_clock_event, 	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD }

/* generic widgets */
#define SND_SOC_DAPM_REG(wid, wname, wreg, wshift, wmask, won_val, woff_val) {	.id = wid, .name = wname, .kcontrol_news = NULL, .num_kcontrols = 0, 	.reg = -((wreg) + 1), .shift = wshift, .mask = wmask, 	.on_val = won_val, .off_val = woff_val, .event = dapm_reg_event, 	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD}
#define SND_SOC_DAPM_SUPPLY(wname, wreg, wshift, winvert, wevent, wflags) {	.id = snd_soc_dapm_supply, .name = wname, .reg = wreg,		.shift = wshift, .invert = winvert, .event = wevent, 	.event_flags = wflags}
#define SND_SOC_DAPM_REGULATOR_SUPPLY(wname, wdelay, wflags)	    {	.id = snd_soc_dapm_regulator_supply, .name = wname, 	.reg = SND_SOC_NOPM, .shift = wdelay, .event = dapm_regulator_event, 	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD, 	.invert = wflags}


/* dapm kcontrol types */
#define SOC_DAPM_SINGLE(xname, reg, shift, max, invert) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_volsw, .index = SOC_DAPM_IO_VOLSW, 	.get = snd_soc_dapm_get_volsw, .put = snd_soc_dapm_put_volsw, 	.private_value =  SOC_SINGLE_VALUE(reg, shift, max, invert) }
#define SOC_DAPM_SINGLE_TLV(xname, reg, shift, max, invert, tlv_array) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_volsw, .index = SOC_DAPM_IO_VOLSW, 	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE,	.tlv.p = (tlv_array), 	.get = snd_soc_dapm_get_volsw, .put = snd_soc_dapm_put_volsw, 	.private_value =  SOC_SINGLE_VALUE(reg, shift, max, invert) }
#define SOC_DAPM_ENUM(xname, xenum) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_enum_double, 	.get = snd_soc_dapm_get_enum_double, 	.put = snd_soc_dapm_put_enum_double, 	.index = SOC_DAPM_IO_ENUM_DOUBLE, 	.private_value = (unsigned long)&xenum }
#define SOC_DAPM_ENUM_VIRT(xname, xenum)		    {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_enum_double, 	.get = snd_soc_dapm_get_enum_virt, 	.put = snd_soc_dapm_put_enum_virt, 	.index = SOC_DAPM_IO_ENUM_VIRT, 	.private_value = (unsigned long)&xenum }
#define SOC_DAPM_ENUM_EXT(xname, xenum, xget, xput) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_enum_double, 	.index = SOC_DAPM_IO_ENUM_EXT, 	.get = xget, 	.put = xput, 	.private_value = (unsigned long)&xenum }
#define SOC_DAPM_VALUE_ENUM(xname, xenum) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, 	.info = snd_soc_info_enum_double, 	.get = snd_soc_dapm_get_value_enum_double, 	.put = snd_soc_dapm_put_value_enum_double, 	.index = SOC_DAPM_IO_ENUM_VALUE, 	.private_value = (unsigned long)&xenum }
#define SOC_DAPM_PIN_SWITCH(xname) {	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname " Switch", 	.info = snd_soc_dapm_info_pin_switch, 	.get = snd_soc_dapm_get_pin_switch, 	.put = snd_soc_dapm_put_pin_switch, 	.index = SOC_DAPM_IO_PIN, 	.private_value = (unsigned long)xname }
#endif // already defined in soc-dapm.h

#define SOC_DAPM_TYPE_VOLSW		64
#define SOC_DAPM_TYPE_ENUM_DOUBLE	65
#define SOC_DAPM_TYPE_ENUM_VIRT		66
#define SOC_DAPM_TYPE_ENUM_VALUE	67
#define SOC_DAPM_TYPE_PIN		68
#define SOC_DAPM_TYPE_ENUM_EXT		69

#define SOC_DAPM_IO_VOLSW 	SOC_CONTROL_ID(SOC_DAPM_TYPE_VOLSW, 		SOC_DAPM_TYPE_VOLSW, 		SOC_DAPM_TYPE_VOLSW)
#define SOC_DAPM_IO_ENUM_DOUBLE 	SOC_CONTROL_ID(SOC_DAPM_TYPE_ENUM_DOUBLE, 		SOC_DAPM_TYPE_ENUM_DOUBLE, 		SOC_CONTROL_TYPE_ENUM)
#define SOC_DAPM_IO_ENUM_VIRT 	SOC_CONTROL_ID(SOC_DAPM_TYPE_ENUM_VIRT, 		SOC_DAPM_TYPE_ENUM_VIRT, 		SOC_CONTROL_TYPE_ENUM)
#define SOC_DAPM_IO_ENUM_VALUE 	SOC_CONTROL_ID(SOC_DAPM_TYPE_ENUM_VALUE, 		SOC_DAPM_TYPE_ENUM_VALUE, 		SOC_CONTROL_TYPE_ENUM)
#define SOC_DAPM_IO_PIN 	SOC_CONTROL_ID(SOC_DAPM_TYPE_PIN, 		SOC_DAPM_TYPE_PIN, 		SOC_DAPM_TYPE_PIN)
#define SOC_DAPM_IO_ENUM_EXT 	SOC_CONTROL_ID(SOC_CONTROL_TYPE_EXT, 		SOC_CONTROL_TYPE_EXT, 		SOC_CONTROL_TYPE_ENUM)

#if 0	// already defined in soc-dapm.h
/* dapm widget types */
enum snd_soc_dapm_type {
	snd_soc_dapm_input = 0,		/* input pin */
	snd_soc_dapm_output,		/* output pin */
	snd_soc_dapm_mux,			/* selects 1 analog signal from many inputs */
	snd_soc_dapm_virt_mux,			/* virtual version of snd_soc_dapm_mux */
	snd_soc_dapm_value_mux,			/* selects 1 analog signal from many inputs */
	snd_soc_dapm_mixer,			/* mixes several analog signals together */
	snd_soc_dapm_mixer_named_ctl,		/* mixer with named controls */
	snd_soc_dapm_pga,			/* programmable gain/attenuation (volume) */
	snd_soc_dapm_out_drv,			/* output driver */
	snd_soc_dapm_adc,			/* analog to digital converter */
	snd_soc_dapm_dac,			/* digital to analog converter */
	snd_soc_dapm_micbias,		/* microphone bias (power) */
	snd_soc_dapm_mic,			/* microphone */
	snd_soc_dapm_hp,			/* headphones */
	snd_soc_dapm_spk,			/* speaker */
	snd_soc_dapm_line,			/* line input/output */
	snd_soc_dapm_switch,		/* analog switch */
	snd_soc_dapm_vmid,			/* codec bias/vmid - to minimise pops */
	snd_soc_dapm_pre,			/* machine specific pre widget - exec first */
	snd_soc_dapm_post,			/* machine specific post widget - exec last */
	snd_soc_dapm_supply,		/* power/clock supply */
	snd_soc_dapm_regulator_supply,	/* external regulator */
	snd_soc_dapm_clock_supply,	/* external clock */
	snd_soc_dapm_aif_in,		/* audio interface input */
	snd_soc_dapm_aif_out,		/* audio interface output */
	snd_soc_dapm_siggen,		/* signal generator */
	snd_soc_dapm_dai,		/* link to DAI structure */
	snd_soc_dapm_dai_link,		/* link between two DAI structures */
};
#endif	// already defined in soc-dapm.h

/* Header magic number and string sizes */
#define SND_SOC_FW_MAGIC	0x41536F43 /* ASoC */

/* string sizes */
#define SND_SOC_FW_TEXT_SIZE	32
#define SND_SOC_FW_NUM_TEXTS	16

/* ABI version */
#define SND_SOC_FW_ABI_VERSION		0x1

/*
 * File and Block header data types.
 * Add new generic and vendor types to end of list.
 * Generic types are handled by the core whilst vendors types are passed
 * to the component drivers for handling.
 */
#define SND_SOC_FW_MIXER		1
#define SND_SOC_FW_DAPM_GRAPH		2
#define SND_SOC_FW_DAPM_WIDGET		3
#define SND_SOC_FW_DAI_LINK		4
#define SND_SOC_FW_COEFF		5

#define SND_SOC_FW_VENDOR_FW		1000
#define SND_SOC_FW_VENDOR_CONFIG	1001
#define SND_SOC_FW_VENDOR_COEFF	1002
#define SND_SOC_FW_VENDOR_CODEC	1003

/*
 * File and Block Header
 */
struct snd_soc_fw_hdr {
	__le32 magic;
	__le32 abi;		/* ABI version */
	__le32 type;
	__le32 vendor_type;	/* optional vendor specific type info */
	__le32 version;		/* optional vendor specific version details */
	__le32 size;		/* data bytes, excluding this header */
} __attribute__((packed));


struct snd_soc_fw_ctl_tlv {
	__le32 numid;	/* control element numeric identification */
	__le32 length;	/* in bytes aligned to 4 */
	/* tlv data starts here */
} __attribute__((packed));

struct snd_soc_fw_control_hdr {
	char name[SND_SOC_FW_TEXT_SIZE];
	__le32 index;
	__le32 access;
	__le32 tlv_size;
} __attribute__((packed));

/*
 * Mixer kcontrol.
 */
struct snd_soc_fw_mixer_control {
	struct snd_soc_fw_control_hdr hdr;
	__s32 min;
	__s32 max;
	__s32 platform_max;
	__le32 reg;
	__le32 rreg;
	__le32 shift;
	__le32 rshift;
	__le32 invert;
} __attribute__((packed));

/*
 * Enumerated kcontrol
 */
struct snd_soc_fw_enum_control {
	struct snd_soc_fw_control_hdr hdr;
	__le32 reg;
	__le32 reg2;
	__le32 shift_l;
	__le32 shift_r;
	__le32 max;
	__le32 mask;
	__le32 count;
	char texts[SND_SOC_FW_NUM_TEXTS][SND_SOC_FW_TEXT_SIZE];
	__le32 values[SND_SOC_FW_NUM_TEXTS * SND_SOC_FW_TEXT_SIZE / 4];
} __attribute__((packed));

/*
 * kcontrol Header
 */
struct snd_soc_fw_kcontrol {
	__le32 count; /* in kcontrols (based on type) */
	/* kcontrols here */
} __attribute__((packed));

/*
 * DAPM Graph Element
 */
struct snd_soc_fw_dapm_graph_elem {
	char sink[SND_SOC_FW_TEXT_SIZE];
	char control[SND_SOC_FW_TEXT_SIZE];
	char source[SND_SOC_FW_TEXT_SIZE];
} __attribute__((packed));


/*
 * DAPM Widget.
 */
struct snd_soc_fw_dapm_widget {
	__le32 id;		/* snd_soc_dapm_type */
	char name[SND_SOC_FW_TEXT_SIZE];
	char sname[SND_SOC_FW_TEXT_SIZE];

	__s32 reg;		/* negative reg = no direct dapm */
	__le32 shift;		/* bits to shift */
	__le32 mask;		/* non-shifted mask */
	__u8 invert;		/* invert the power bit */
	__u8 ignore_suspend;	/* kept enabled over suspend */
	__u8 padding[2];

	/* kcontrols that relate to this widget */
	struct snd_soc_fw_kcontrol kcontrol;
	/* controls follow here */
} __attribute__((packed));

/*
 * DAPM Graph and Pins.
 */
struct snd_soc_fw_dapm_elems {
	__le32 count; /* in elements */
	/* elements here */
} __attribute__((packed));

/*
 * Coeffcient File Data.
 */
struct snd_soc_file_coeff_data {
	__le32 count; /* in elems */
	__le32 size;	/* total data size */
	__le32 id; /* associated mixer ID */
	/* data here */
} __attribute__((packed));

#endif	// __LINUX_UAPI_SND_ASOC_SOCFW_H

// we should #include <sound/soc.h> for
// struct soc_mixer_control
// struct soc_enum
// but this is NOT uapi!
// hence we add a copy for the moment
// beware: this must be synced to the kernel (but is quite unlikely to change often)

#ifndef __LINUX_SND_SOC_H
#define __LINUX_SND_SOC_H

/* mixer control */
struct soc_mixer_control {
	int min, max, platform_max;
	int reg, rreg;
	unsigned int shift, rshift;
	unsigned int sign_bit;
	unsigned int invert:1;
	unsigned int autodisable:1;
#ifdef CONFIG_SND_SOC_TOPOLOGY
	struct snd_soc_dobj dobj;
#endif
};

/* enumerated kcontrol */
struct soc_enum {
	int reg;
	unsigned char shift_l;
	unsigned char shift_r;
	unsigned int items;
	unsigned int mask;
	const char * const *texts;
	const unsigned int *values;
	unsigned int autodisable:1;
#ifdef CONFIG_SND_SOC_TOPOLOGY
	struct snd_soc_dobj dobj;
#endif
};

#endif // __LINUX_SND_SOC_H

