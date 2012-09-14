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

typedef	uint32_t u32;
typedef	int32_t s32;
typedef	uint16_t u16;
typedef	int16_t s16;
typedef	uint8_t u8;
typedef	int8_t s8;

#define SND_SOC_FILE_MAGIC	0x41536F43 /* ASoC */
#define SND_SOC_FILE_TEXT_SIZE	32
#define SND_SOC_FILE_NUM_TEXTS	16

/* 
 * File andBlock header data types.
 * Add new generic and vendor types to end of list.
 * Generic types are handled by the core whilst vendors types are passed
 * to the component drivers for handling.
 */
#define SND_SOC_FILE_MIXER		1
#define SND_SOC_FILE_DAPM_GRAPH		2
#define SND_SOC_FILE_DAPM_PINS		3
#define SND_SOC_FILE_DAPM_WIDGET	4
#define SND_SOC_FILE_DAI_LINK		5
#define SND_SOC_FILE_COEFFS		6

#define SND_SOC_FILE_VENDOR_FW		1000
#define SND_SOC_FILE_VENDOR_CONFIG	1001
#define SND_SOC_FILE_VENDOR_COEFF	1002
#define SND_SOC_FILE_VENDOR_CODEC	1003

/*
 * File and Block Header
 */
struct snd_soc_file_hdr {
	u32 magic;
	u32 type;
	u32 vendor_type; /* optional vendor specific type info */
	u32 version; /* optional vendor specific version details */
	u32 size; /* data bytes, excluding this header */
	/* file data contents start here */
};

/*
 * Mixer KControl Types.
 * Simple ones atm, add new ones to end.
 */
#define SND_SOC_FILE_MIXER_SINGLE_VALUE		0
#define SND_SOC_FILE_MIXER_DOUBLE_VALUE		1
#define SND_SOC_FILE_MIXER_SINGLE_VALUE_EXT	2
#define SND_SOC_FILE_MIXER_DOUBLE_VALUE_EXT	3

struct snd_soc_file_control_hdr {
	char name[SND_SOC_FILE_TEXT_SIZE];
	u32 type;
};

/* 
 * Mixer KControl.
 */
struct snd_soc_file_mixer_control {
	struct snd_soc_file_control_hdr hdr;
	s32 min;
	s32 max;
	s32 platform_max;
	u32 reg;
	u32 rreg;
	u32 shift;
	u32 rshift;
	u32 invert;
};

/*
 * Enum KControl Types.
 * Simple ones atm, add new ones to end.
 */
#define SND_SOC_FILE_ENUM_SINGLE_T	0	/* text */
#define SND_SOC_FILE_ENUM_DOUBLE_T	1
#define SND_SOC_FILE_ENUM_SINGLE_T_EXT	2
#define SND_SOC_FILE_ENUM_DOUBLE_T_EXT	3
#define SND_SOC_FILE_ENUM_SINGLE_V	4	/* value */
#define SND_SOC_FILE_ENUM_DOUBLE_V	5
#define SND_SOC_FILE_ENUM_SINGLE_V_EXT	6
#define SND_SOC_FILE_ENUM_DOUBLE_V_EXT	7

/* 
 * Enumerated KControl
 */
struct snd_soc_file_enum_control {
	struct snd_soc_file_control_hdr hdr;
	u32 reg;
	u32 reg2;
	u32 shift_l;
	u32 shift_r;
	u32 max;
	u32 mask;
	union {	/* both texts and values are the same size */
		char texts[SND_SOC_FILE_NUM_TEXTS][SND_SOC_FILE_TEXT_SIZE];
		u32 values[SND_SOC_FILE_NUM_TEXTS * SND_SOC_FILE_TEXT_SIZE / 4];
	};
};

/*
 * Kcontrol Header Types.
 */
#define SND_SOC_FILE_MIXER_VALUE	0
#define SND_SOC_FILE_MIXER_ENUM		1

/*
 * Kcontrol Header
 */
struct snd_soc_file_kcontrol {
	u32 count; /* in kcontrols (based on type) */
	/* kcontrols here */
};

/*
 * DAPM Graph Element
 */
struct snd_soc_file_dapm_graph_elem {
	char sink[SND_SOC_FILE_TEXT_SIZE];
	char control[SND_SOC_FILE_TEXT_SIZE];
	char source[SND_SOC_FILE_TEXT_SIZE];
};

/*
 * DAPM Pin Element.
 */
struct snd_soc_file_dapm_pin_elem {
	char name[SND_SOC_FILE_TEXT_SIZE];
	u32 disconnect:1;
	u32 ignore_suspend:1;
};


/*
 * DAPM Widget.
 */
struct snd_soc_file_dapm_widget {
	u32 id;		/* snd_soc_dapm_type */
	char name[SND_SOC_FILE_TEXT_SIZE];
	char sname[SND_SOC_FILE_TEXT_SIZE];
	
	s32 reg;		/* negative reg = no direct dapm */
	u32 shift;		/* bits to shift */
	u32 mask;		/* non-shifted mask */
	u32 invert:1;		/* invert the power bit */
	u32 ignore_suspend:1;	/* kept enabled over suspend */

	/* kcontrols that relate to this widget */
	struct snd_soc_file_kcontrol kcontrol;
	/* controls follow here */
};

/*
 * DAPM Graph and Pins.
 */
struct snd_soc_file_dapm_elems {
	u32 count; /* in elements */
	/* elements here */
};

/*
 * Coeffcient File Data.
 */
struct snd_soc_file_coeff_data {
	u32 count; /* in bytes */
	/* data here */
};

/*
 * Generic Coefficients with Kcontrols
 */

#define SND_SOC_FILE_COEFF_ELEM(text, c) \
	{.description = text, \
	.coeffs = (const void*)c, \
	.size = ARRAY_SIZE(c) * sizeof(c[0])}

#define SND_SOC_FILE_COEFF(i, text, e) \
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

/* get/put/info types - TODO: move this into kernel header */
#define SOC_MIXER_IO_VOLSW		0
#define SOC_MIXER_IO_VOLSW_SX		1
#define SOC_MIXER_IO_VOLSW_S8		2
#define SOC_MIXER_IO_VOLSW_XR_SX	3
#define SOC_MIXER_IO_VOLSW_EXT		4
#define SOC_MIXER_IO_ENUM		5
#define SOC_MIXER_IO_ENUM_EXT		6
#define SOC_MIXER_IO_BYTES		7
#define SOC_MIXER_IO_BOOL		8

/*
 * Convenience kcontrol builders
 */
#define SOC_DOUBLE_VALUE(xreg, shift_left, shift_right, xmax, xinvert) \
	((unsigned long)&(struct soc_mixer_control) \
	{.reg = xreg, .rreg = xreg, .shift = shift_left, \
	.rshift = shift_right, .max = xmax, .platform_max = xmax, \
	.invert = xinvert})

#define SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert) \
	SOC_DOUBLE_VALUE(xreg, xshift, xshift, xmax, xinvert)

#define SOC_SINGLE_VALUE_EXT(xreg, xmax, xinvert) \
	((unsigned long)&(struct soc_mixer_control) \
	{.reg = xreg, .max = xmax, .platform_max = xmax, .invert = xinvert})

#define SOC_DOUBLE_R_VALUE(xlreg, xrreg, xshift, xmax, xinvert) \
	((unsigned long)&(struct soc_mixer_control) \
	{.reg = xlreg, .rreg = xrreg, .shift = xshift, .rshift = xshift, \
	.max = xmax, .platform_max = xmax, .invert = xinvert})

#define SOC_SINGLE(xname, reg, shift, max, invert) \
{	.iface = SOC_MIXER_IO_VOLSW, .name = xname, \
	.private_value =  SOC_SINGLE_VALUE(reg, shift, max, invert) }

#define SOC_SINGLE_TLV(xname, reg, shift, max, invert, tlv_array) \
{	.iface = SOC_MIXER_IO_VOLSW, .name = xname, \
	.tlv.p = (tlv_array), \
	.private_value =  SOC_SINGLE_VALUE(reg, shift, max, invert) }

#define SOC_SINGLE_SX_TLV(xname, xreg, xshift, xmin, xmax, tlv_array) \
{       .iface = SOC_MIXER_IO_VOLSW_SX, .name = xname, \
	.tlv.p  = (tlv_array),\
	.private_value = (unsigned long)&(struct soc_mixer_control) \
		{.reg = xreg, .rreg = xreg, \
		.shift = xshift, .rshift = xshift, \
		.max = xmax, .min = xmin} }

#define SOC_DOUBLE(xname, reg, shift_left, shift_right, max, invert) \
{	.iface = SOC_MIXER_IO_VOLSW, .name = (xname),\
	.private_value = SOC_DOUBLE_VALUE(reg, shift_left, shift_right, \
					  max, invert) }

#define SOC_DOUBLE_R(xname, reg_left, reg_right, xshift, xmax, xinvert) \
{	.iface = SOC_MIXER_IO_VOLSW, .name = (xname), \
	.private_value = \
		SOC_DOUBLE_R_VALUE(reg_left, reg_right, xshift,xmax, xinvert) }

#define SOC_DOUBLE_TLV(xname, reg, shift_left, shift_right, max, invert, tlv_array) \
{	.iface = SOC_MIXER_IO_VOLSW, .name = (xname),\
	.tlv.p = (tlv_array), .private_value =\
		SOC_DOUBLE_VALUE(reg, shift_left, shift_right, max, invert) }

#define SOC_DOUBLE_R_TLV(xname, reg_left, reg_right, xshift, xmax, xinvert, tlv_array) \
{	.iface = SOC_MIXER_IO_VOLSW_SX, .name = (xname),\
	.tlv.p = (tlv_array), \
	.private_value =\
		SOC_DOUBLE_R_VALUE(reg_left, reg_right, xshift, xmax, xinvert) }

#define SOC_DOUBLE_R_SX_TLV(xname, xreg, xrreg, xshift, xmin, xmax, tlv_array) \
{       .iface = SOC_MIXER_IO_VOLSW_SX, .name = (xname), \
	.tlv.p  = (tlv_array), \
	.private_value = (unsigned long)&(struct soc_mixer_control) \
		{.reg = xreg, .rreg = xrreg, \
		.shift = xshift, .rshift = xshift, \
		.max = xmax, .min = xmin} }

#define SOC_DOUBLE_S8_TLV(xname, xreg, xmin, xmax, tlv_array) \
{	.iface  = SOC_MIXER_IO_VOLSW_S8, .name = (xname), \
	.tlv.p  = (tlv_array), \
	.private_value = (unsigned long)&(struct soc_mixer_control) \
		{.reg = xreg, .min = xmin, .max = xmax, \
		 .platform_max = xmax} }

#define SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmax, xtexts) \
{	.reg = xreg, .shift_l = xshift_l, .shift_r = xshift_r, \
	.max = xmax, .texts = xtexts }
#define SOC_ENUM_SINGLE(xreg, xshift, xmax, xtexts) \
	SOC_ENUM_DOUBLE(xreg, xshift, xshift, xmax, xtexts)
#define SOC_ENUM_SINGLE_EXT(xmax, xtexts) \
{	.max = xmax, .texts = xtexts }
#define SOC_VALUE_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmask, xmax, xtexts, xvalues) \
{	.reg = xreg, .shift_l = xshift_l, .shift_r = xshift_r, \
	.mask = xmask, .max = xmax, .texts = xtexts, .values = xvalues}
#define SOC_VALUE_ENUM_SINGLE(xreg, xshift, xmask, xmax, xtexts, xvalues) \
	SOC_VALUE_ENUM_DOUBLE(xreg, xshift, xshift, xmask, xmax, xtexts, xvalues)

#define SOC_ENUM(xname, xenum) \
{	.iface = SOC_MIXER_IO_ENUM, .name = xname,\
	.private_value = (unsigned long)&xenum }

#define SOC_VALUE_ENUM(xname, xenum) \
{	.iface = SOC_MIXER_IO_ENUM, .name = xname,\
	.private_value = (unsigned long)&xenum }

#define SOC_SINGLE_EXT(xname, xreg, xshift, xmax, xinvert,\
	 xhandler_get, xhandler_put) \
{	.iface = SOC_MIXER_IO_VOLSW_EXT, .name = xname, \
	.private_value = SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert) }

#define SOC_DOUBLE_EXT(xname, reg, shift_left, shift_right, max, invert,\
	 xhandler_get, xhandler_put) \
{	.iface = SOC_MIXER_IO_VOLSW_EXT, .name = (xname),\
	.private_value = \
		SOC_DOUBLE_VALUE(reg, shift_left, shift_right, max, invert) }

#define SOC_SINGLE_EXT_TLV(xname, xreg, xshift, xmax, xinvert,\
	 xhandler_get, xhandler_put, tlv_array) \
{	.iface = SOC_MIXER_IO_VOLSW_EXT, .name = xname, \
	.tlv.p = (tlv_array), \
	.private_value = SOC_SINGLE_VALUE(xreg, xshift, xmax, xinvert) }

#define SOC_DOUBLE_EXT_TLV(xname, xreg, shift_left, shift_right, xmax, xinvert,\
	 xhandler_get, xhandler_put, tlv_array) \
{	.iface = SOC_MIXER_IO_VOLSW_EXT, .name = (xname), \
	.tlv.p = (tlv_array), \
	.private_value = SOC_DOUBLE_VALUE(xreg, shift_left, shift_right, \
					  xmax, xinvert) }

#define SOC_DOUBLE_R_EXT_TLV(xname, reg_left, reg_right, xshift, xmax, xinvert,\
	 xhandler_get, xhandler_put, tlv_array) \
{	.iface = SOC_MIXER_IO_VOLSW_EXT, .name = (xname), \
	.tlv.p = (tlv_array), \
	.private_value = SOC_DOUBLE_R_VALUE(reg_left, reg_right, xshift, \
					    xmax, xinvert) }

#define SOC_SINGLE_BOOL_EXT(xname, xdata, xhandler_get, xhandler_put) \
{	.iface = SOC_MIXER_IO_BOOL_EXT, .name = xname, \
	.private_value = xdata }

#define SOC_ENUM_EXT(xname, xenum, xhandler_get, xhandler_put) \
{	.iface = SOC_MIXER_IO_ENUM_EXT, .name = xname, \
	.private_value = (unsigned long)&xenum }

#define SND_SOC_BYTES(xname, xbase, xregs)		      \
{	.iface = SOC_MIXER_IO_BYTES, .name = xname,   \
	.private_value =	      \
		((unsigned long)&(struct soc_bytes)           \
		{.base = xbase, .num_regs = xregs }) }

#define SND_SOC_BYTES_MASK(xname, xbase, xregs, xmask)	      \
{	.iface = SOC_MIXER_IO_BYTES, .name = xname,   \
	.private_value =	      \
		((unsigned long)&(struct soc_bytes)           \
		{.base = xbase, .num_regs = xregs,	      \
		 .mask = xmask }) }

#define SOC_SINGLE_XR_SX(xname, xregbase, xregcount, xnbits, \
		xmin, xmax, xinvert) \
{	.iface = SOC_MIXER_IO_VOLSW_XR_SX, .name = (xname), \
	.private_value = (unsigned long)&(struct soc_mreg_control) \
		{.regbase = xregbase, .regcount = xregcount, .nbits = xnbits, \
		.invert = xinvert, .min = xmin, .max = xmax} }

#define SOC_SINGLE_STROBE(xname, xreg, xshift, xinvert) \
	SOC_SINGLE_EXT(xname, xreg, xshift, 1, xinvert, \
		snd_soc_get_strobe, snd_soc_put_strobe)

/*
 * Simplified versions of above macros, declaring a struct and calculating
 * ARRAY_SIZE internally
 */
#define SOC_ENUM_DOUBLE_DECL(name, xreg, xshift_l, xshift_r, xtexts) \
	struct soc_enum name = SOC_ENUM_DOUBLE(xreg, xshift_l, xshift_r, \
						ARRAY_SIZE(xtexts), xtexts)
#define SOC_ENUM_SINGLE_DECL(name, xreg, xshift, xtexts) \
	SOC_ENUM_DOUBLE_DECL(name, xreg, xshift, xshift, xtexts)
#define SOC_ENUM_SINGLE_EXT_DECL(name, xtexts) \
	struct soc_enum name = SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(xtexts), xtexts)
#define SOC_VALUE_ENUM_DOUBLE_DECL(name, xreg, xshift_l, xshift_r, xmask, xtexts, xvalues) \
	struct soc_enum name = SOC_VALUE_ENUM_DOUBLE(xreg, xshift_l, xshift_r, xmask, \
							ARRAY_SIZE(xtexts), xtexts, xvalues)
#define SOC_VALUE_ENUM_SINGLE_DECL(name, xreg, xshift, xmask, xtexts, xvalues) \
	SOC_VALUE_ENUM_DOUBLE_DECL(name, xreg, xshift, xshift, xmask, xtexts, xvalues)


/* codec domain */
#define SND_SOC_DAPM_VMID(wname) \
{	.id = snd_soc_dapm_vmid, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0}

/* platform domain */
#define SND_SOC_DAPM_SIGGEN(wname) \
{	.id = snd_soc_dapm_siggen, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM }
#define SND_SOC_DAPM_INPUT(wname) \
{	.id = snd_soc_dapm_input, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM }
#define SND_SOC_DAPM_OUTPUT(wname) \
{	.id = snd_soc_dapm_output, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM }
#define SND_SOC_DAPM_MIC(wname, wevent) \
{	.id = snd_soc_dapm_mic, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD}
#define SND_SOC_DAPM_HP(wname, wevent) \
{	.id = snd_soc_dapm_hp, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_SPK(wname, wevent) \
{	.id = snd_soc_dapm_spk, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_LINE(wname, wevent) \
{	.id = snd_soc_dapm_line, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD}

/* path domain */
#define SND_SOC_DAPM_PGA(wname, wreg, wshift, winvert,\
	 wcontrols, wncontrols) \
{	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
#define SND_SOC_DAPM_OUT_DRV(wname, wreg, wshift, winvert,\
	 wcontrols, wncontrols) \
{	.id = snd_soc_dapm_out_drv, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
#define SND_SOC_DAPM_MIXER(wname, wreg, wshift, winvert, \
	 wcontrols, wncontrols)\
{	.id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
#define SND_SOC_DAPM_MIXER_NAMED_CTL(wname, wreg, wshift, winvert, \
	 wcontrols, wncontrols)\
{       .id = snd_soc_dapm_mixer_named_ctl, .name = wname, .reg = wreg, \
	.shift = wshift, .invert = winvert, .kcontrol_news = wcontrols, \
	.num_kcontrols = wncontrols}
#define SND_SOC_DAPM_MICBIAS(wname, wreg, wshift, winvert) \
{	.id = snd_soc_dapm_micbias, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = NULL, .num_kcontrols = 0}
#define SND_SOC_DAPM_SWITCH(wname, wreg, wshift, winvert, wcontrols) \
{	.id = snd_soc_dapm_switch, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1}
#define SND_SOC_DAPM_MUX(wname, wreg, wshift, winvert, wcontrols) \
{	.id = snd_soc_dapm_mux, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1}
#define SND_SOC_DAPM_VIRT_MUX(wname, wreg, wshift, winvert, wcontrols) \
{	.id = snd_soc_dapm_virt_mux, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1}
#define SND_SOC_DAPM_VALUE_MUX(wname, wreg, wshift, winvert, wcontrols) \
{	.id = snd_soc_dapm_value_mux, .name = wname, .reg = wreg, \
	.shift = wshift, .invert = winvert, .kcontrol_news = wcontrols, \
	.num_kcontrols = 1}

/* Simplified versions of above macros, assuming wncontrols = ARRAY_SIZE(wcontrols) */
#define SOC_PGA_ARRAY(wname, wreg, wshift, winvert,\
	 wcontrols) \
{	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = ARRAY_SIZE(wcontrols)}
#define SOC_MIXER_ARRAY(wname, wreg, wshift, winvert, \
	 wcontrols)\
{	.id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = ARRAY_SIZE(wcontrols)}
#define SOC_MIXER_NAMED_CTL_ARRAY(wname, wreg, wshift, winvert, \
	 wcontrols)\
{       .id = snd_soc_dapm_mixer_named_ctl, .name = wname, .reg = wreg, \
	.shift = wshift, .invert = winvert, .kcontrol_news = wcontrols, \
	.num_kcontrols = ARRAY_SIZE(wcontrols)}

/* path domain with event - event handler must return 0 for success */
#define SND_SOC_DAPM_PGA_E(wname, wreg, wshift, winvert, wcontrols, \
	wncontrols, wevent, wflags) \
{	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols, \
	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_OUT_DRV_E(wname, wreg, wshift, winvert, wcontrols, \
	wncontrols, wevent, wflags) \
{	.id = snd_soc_dapm_out_drv, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols, \
	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_MIXER_E(wname, wreg, wshift, winvert, wcontrols, \
	wncontrols, wevent, wflags) \
{	.id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = wncontrols, \
	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_MIXER_NAMED_CTL_E(wname, wreg, wshift, winvert, \
	wcontrols, wncontrols, wevent, wflags) \
{       .id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, \
	.num_kcontrols = wncontrols, .event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_SWITCH_E(wname, wreg, wshift, winvert, wcontrols, \
	wevent, wflags) \
{	.id = snd_soc_dapm_switch, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1, \
	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_MUX_E(wname, wreg, wshift, winvert, wcontrols, \
	wevent, wflags) \
{	.id = snd_soc_dapm_mux, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1, \
	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_VIRT_MUX_E(wname, wreg, wshift, winvert, wcontrols, \
	wevent, wflags) \
{	.id = snd_soc_dapm_virt_mux, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = 1, \
	.event = wevent, .event_flags = wflags}

/* additional sequencing control within an event type */
#define SND_SOC_DAPM_PGA_S(wname, wsubseq, wreg, wshift, winvert, \
	wevent, wflags) \
{	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .event = wevent, .event_flags = wflags, \
	.subseq = wsubseq}
#define SND_SOC_DAPM_SUPPLY_S(wname, wsubseq, wreg, wshift, winvert, wevent, \
	wflags)	\
{	.id = snd_soc_dapm_supply, .name = wname, .reg = wreg,	\
	.shift = wshift, .invert = winvert, .event = wevent, \
	.event_flags = wflags, .subseq = wsubseq}

/* Simplified versions of above macros, assuming wncontrols = ARRAY_SIZE(wcontrols) */
#define SOC_PGA_E_ARRAY(wname, wreg, wshift, winvert, wcontrols, \
	wevent, wflags) \
{	.id = snd_soc_dapm_pga, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = ARRAY_SIZE(wcontrols), \
	.event = wevent, .event_flags = wflags}
#define SOC_MIXER_E_ARRAY(wname, wreg, wshift, winvert, wcontrols, \
	wevent, wflags) \
{	.id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, .num_kcontrols = ARRAY_SIZE(wcontrols), \
	.event = wevent, .event_flags = wflags}
#define SOC_MIXER_NAMED_CTL_E_ARRAY(wname, wreg, wshift, winvert, \
	wcontrols, wevent, wflags) \
{       .id = snd_soc_dapm_mixer, .name = wname, .reg = wreg, .shift = wshift, \
	.invert = winvert, .kcontrol_news = wcontrols, \
	.num_kcontrols = ARRAY_SIZE(wcontrols), .event = wevent, .event_flags = wflags}

/* events that are pre and post DAPM */
#define SND_SOC_DAPM_PRE(wname, wevent) \
{	.id = snd_soc_dapm_pre, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_POST(wname, wevent) \
{	.id = snd_soc_dapm_post, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_POST_PMD}

/* stream domain */
#define SND_SOC_DAPM_AIF_IN(wname, stname, wslot, wreg, wshift, winvert) \
{	.id = snd_soc_dapm_aif_in, .name = wname, .sname = stname, \
	.reg = wreg, .shift = wshift, .invert = winvert }
#define SND_SOC_DAPM_AIF_IN_E(wname, stname, wslot, wreg, wshift, winvert, \
			      wevent, wflags)				\
{	.id = snd_soc_dapm_aif_in, .name = wname, .sname = stname, \
	.reg = wreg, .shift = wshift, .invert = winvert, \
	.event = wevent, .event_flags = wflags }
#define SND_SOC_DAPM_AIF_OUT(wname, stname, wslot, wreg, wshift, winvert) \
{	.id = snd_soc_dapm_aif_out, .name = wname, .sname = stname, \
	.reg = wreg, .shift = wshift, .invert = winvert }
#define SND_SOC_DAPM_AIF_OUT_E(wname, stname, wslot, wreg, wshift, winvert, \
			     wevent, wflags)				\
{	.id = snd_soc_dapm_aif_out, .name = wname, .sname = stname, \
	.reg = wreg, .shift = wshift, .invert = winvert, \
	.event = wevent, .event_flags = wflags }
#define SND_SOC_DAPM_DAC(wname, stname, wreg, wshift, winvert) \
{	.id = snd_soc_dapm_dac, .name = wname, .sname = stname, .reg = wreg, \
	.shift = wshift, .invert = winvert}
#define SND_SOC_DAPM_DAC_E(wname, stname, wreg, wshift, winvert, \
			   wevent, wflags)				\
{	.id = snd_soc_dapm_dac, .name = wname, .sname = stname, .reg = wreg, \
	.shift = wshift, .invert = winvert, \
	.event = wevent, .event_flags = wflags}
#define SND_SOC_DAPM_ADC(wname, stname, wreg, wshift, winvert) \
{	.id = snd_soc_dapm_adc, .name = wname, .sname = stname, .reg = wreg, \
	.shift = wshift, .invert = winvert}
#define SND_SOC_DAPM_ADC_E(wname, stname, wreg, wshift, winvert, \
			   wevent, wflags)				\
{	.id = snd_soc_dapm_adc, .name = wname, .sname = stname, .reg = wreg, \
	.shift = wshift, .invert = winvert, \
	.event = wevent, .event_flags = wflags}

/* generic widgets */
#define SND_SOC_DAPM_REG(wid, wname, wreg, wshift, wmask, won_val, woff_val) \
{	.id = wid, .name = wname, .kcontrol_news = NULL, .num_kcontrols = 0, \
	.reg = -((wreg) + 1), .shift = wshift, .mask = wmask, \
	.on_val = won_val, .off_val = woff_val, .event = dapm_reg_event, \
	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD}
#define SND_SOC_DAPM_SUPPLY(wname, wreg, wshift, winvert, wevent, wflags) \
{	.id = snd_soc_dapm_supply, .name = wname, .reg = wreg,	\
	.shift = wshift, .invert = winvert, .event = wevent, \
	.event_flags = wflags}
#define SND_SOC_DAPM_REGULATOR_SUPPLY(wname, wdelay) \
{	.id = snd_soc_dapm_regulator_supply, .name = wname, \
	.reg = SND_SOC_NOPM, .shift = wdelay, .event = dapm_regulator_event, \
	.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD }

/* dapm kcontrol types */
#define SOC_DAPM_SINGLE(xname, reg, shift, max, invert) \
{	.iface = SOC_MIXER_IO_VOLSW, .name = xname, \
	.private_value =  SOC_SINGLE_VALUE(reg, shift, max, invert) }

#define SOC_DAPM_SINGLE_TLV(xname, reg, shift, max, invert, tlv_array) \
{	.iface = SOC_MIXER_IO_VOLSW, .name = xname, \
	.tlv.p = (tlv_array), \
	.private_value =  SOC_SINGLE_VALUE(reg, shift, max, invert) }

#define SOC_DAPM_ENUM(xname, xenum) \
{	.iface = SOC_MIXER_IO_ENUM, .name = xname, \
  	.private_value = (unsigned long)&xenum }

#define SOC_DAPM_ENUM_VIRT(xname, xenum)		    \
{	.iface = SOC_MIXER_IO_ENUM, .name = xname, \
	.private_value = (unsigned long)&xenum }

#define SOC_DAPM_ENUM_EXT(xname, xenum, xget, xput) \
{	.iface = SOC_MIXER_IO_ENUM_EXT, .name = xname, \
	.private_value = (unsigned long)&xenum }

#define SOC_DAPM_VALUE_ENUM(xname, xenum) \
{	.iface = SOC_MIXER_IO_ENUM_VALUE, .name = xname, \
	.private_value = (unsigned long)&xenum }

#define SOC_DAPM_PIN_SWITCH(xname) \
{	.iface = SOC_MIXER_IO_PIN, .name = xname " Switch", \
	.private_value = (unsigned long)xname }

#define SND_SOC_NOPM	-1

/*
 * Declare IO mechanisms as NULL for ease of control import.
 */
#define snd_soc_info_enum_ext		NULL
#define snd_soc_info_enum_double	NULL
#define snd_soc_get_enum_double		NULL
#define snd_soc_put_enum_double		NULL
#define snd_soc_get_value_enum_double	NULL
#define snd_soc_put_value_enum_double	NULL
#define snd_soc_info_volsw		NULL
#define snd_soc_info_volsw_ext		NULL
#define snd_soc_info_bool_ext		NULL
#define snd_soc_get_volsw		NULL
#define snd_soc_put_volsw		NULL
#define snd_soc_get_volsw_2r 		snd_soc_get_volsw
#define snd_soc_put_volsw_2r 		snd_soc_put_volsw
#define snd_soc_get_volsw_sx		NULL
#define snd_soc_put_volsw_sx		NULL
#define snd_soc_info_volsw_s8		NULL
#define snd_soc_get_volsw_s8		NULL
#define snd_soc_put_volsw_s8		NULL
#define snd_soc_dapm_put_volsw		NULL
#define snd_soc_dapm_get_volsw		NULL
#define snd_soc_dapm_get_enum_double	NULL
#define snd_soc_dapm_put_enum_double	NULL
#define snd_soc_dapm_get_enum_virt	NULL
#define snd_soc_dapm_put_enum_virt	NULL
#define snd_soc_dapm_get_value_enum_double	NULL
#define snd_soc_dapm_put_value_enum_double	NULL
#define snd_soc_dapm_info_pin_switch	NULL
#define snd_soc_dapm_get_pin_switch	NULL
#define snd_soc_dapm_put_pin_switch	NULL

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))

#define SNDRV_CTL_TLVT_CONTAINER 0	/* one level down - group of TLVs */
#define SNDRV_CTL_TLVT_DB_SCALE	1       /* dB scale */
#define SNDRV_CTL_TLVT_DB_LINEAR 2	/* linear volume */
#define SNDRV_CTL_TLVT_DB_RANGE 3	/* dB range container */
#define SNDRV_CTL_TLVT_DB_MINMAX 4	/* dB scale with min/max */
#define SNDRV_CTL_TLVT_DB_MINMAX_MUTE 5	/* dB scale with min/max with mute */

#define TLV_DB_SCALE_MASK	0xffff
#define TLV_DB_SCALE_MUTE	0x10000
#define TLV_DB_SCALE_ITEM(min, step, mute)			\
	SNDRV_CTL_TLVT_DB_SCALE, 2 * sizeof(unsigned int),	\
	(min), ((step) & TLV_DB_SCALE_MASK) | ((mute) ? TLV_DB_SCALE_MUTE : 0)
#define DECLARE_TLV_DB_SCALE(name, min, step, mute) \
	unsigned int name[] = { TLV_DB_SCALE_ITEM(min, step, mute) }

/* dB scale specified with min/max values instead of step */
#define TLV_DB_MINMAX_ITEM(min_dB, max_dB)			\
	SNDRV_CTL_TLVT_DB_MINMAX, 2 * sizeof(unsigned int),	\
	(min_dB), (max_dB)
#define TLV_DB_MINMAX_MUTE_ITEM(min_dB, max_dB)			\
	SNDRV_CTL_TLVT_DB_MINMAX_MUTE, 2 * sizeof(unsigned int),	\
	(min_dB), (max_dB)
#define DECLARE_TLV_DB_MINMAX(name, min_dB, max_dB) \
	unsigned int name[] = { TLV_DB_MINMAX_ITEM(min_dB, max_dB) }
#define DECLARE_TLV_DB_MINMAX_MUTE(name, min_dB, max_dB) \
	unsigned int name[] = { TLV_DB_MINMAX_MUTE_ITEM(min_dB, max_dB) }

/* linear volume between min_dB and max_dB (.01dB unit) */
#define TLV_DB_LINEAR_ITEM(min_dB, max_dB)		    \
	SNDRV_CTL_TLVT_DB_LINEAR, 2 * sizeof(unsigned int), \
	(min_dB), (max_dB)
#define DECLARE_TLV_DB_LINEAR(name, min_dB, max_dB)	\
	unsigned int name[] = { TLV_DB_LINEAR_ITEM(min_dB, max_dB) }

/* dB range container */
/* Each item is: <min> <max> <TLV> */
/* The below assumes that each item TLV is 4 words like DB_SCALE or LINEAR */
#define TLV_DB_RANGE_HEAD(num)			\
	SNDRV_CTL_TLVT_DB_RANGE, 6 * (num) * sizeof(unsigned int)

#define TLV_DB_GAIN_MUTE	-9999999

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
	const unsigned char *name;	/* ASCII name of item */
	unsigned int count;		/* count of same elements */
	union {
		const unsigned int *p;
	} tlv;
	unsigned long private_value;
};

enum snd_soc_dapm_type {
	snd_soc_dapm_input = 0,		/* input pin */
	snd_soc_dapm_output = 1,		/* output pin */
	snd_soc_dapm_mux = 2,			/* selects 1 analog signal from many inputs */
	snd_soc_dapm_virt_mux = 3,			/* virtual version of snd_soc_dapm_mux */
	snd_soc_dapm_value_mux = 4,			/* selects 1 analog signal from many inputs */
	snd_soc_dapm_mixer = 5,			/* mixes several analog signals together */
	snd_soc_dapm_mixer_named_ctl = 6,		/* mixer with named controls */
	snd_soc_dapm_pga = 7,			/* programmable gain/attenuation (volume) */
	snd_soc_dapm_out_drv = 8,			/* output driver */
	snd_soc_dapm_adc = 9,			/* analog to digital converter */
	snd_soc_dapm_dac = 10,			/* digital to analog converter */
	snd_soc_dapm_micbias = 11,		/* microphone bias (power) */
	snd_soc_dapm_mic = 12,			/* microphone */
	snd_soc_dapm_hp = 13,			/* headphones */
	snd_soc_dapm_spk = 14,			/* speaker */
	snd_soc_dapm_line = 15,			/* line input/output */
	snd_soc_dapm_switch = 16,		/* analog switch */
	snd_soc_dapm_vmid = 17,			/* codec bias/vmid - to minimise pops */
	snd_soc_dapm_pre =18,			/* machine specific pre widget - exec first */
	snd_soc_dapm_post = 19,			/* machine specific post widget - exec last */
	snd_soc_dapm_supply = 20,		/* power/clock supply */
	snd_soc_dapm_regulator_supply = 21,	/* external regulator */
	snd_soc_dapm_clock_supply = 22,	/* external clock */
	snd_soc_dapm_aif_in = 23,		/* audio interface input */
	snd_soc_dapm_aif_out = 24,		/* audio interface output */
	snd_soc_dapm_siggen = 25,		/* signal generator */
	snd_soc_dapm_dai = 26,		/* link to DAI structure */
	snd_soc_dapm_dai_link = 27,		/* link between two DAI structures */
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

struct soc_fw_priv;

struct soc_fw_priv *socfw_new(const char *name, int verbose);
void socfw_free(struct soc_fw_priv *soc_fw);
int socfw_import_plugin(struct soc_fw_priv *soc_fw, const char *name);
int socfw_import_vendor(struct soc_fw_priv *soc_fw, const char *name, int type);
int socfw_import_dapm_graph(struct soc_fw_priv *soc_fw,
	const struct snd_soc_dapm_route *graph, int graph_count);
int socfw_import_dapm_widgets(struct soc_fw_priv *soc_fw,
	const struct snd_soc_dapm_widget *widgets, int widget_count);
int socfw_import_controls(struct soc_fw_priv *soc_fw,
	const struct snd_kcontrol_new *kcontrols, int kcontrol_count);
#endif
