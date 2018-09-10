/*
 * ALSA SoC driver for OMAP4/5 AESS (Audio Engine Sub-System)
 *
 * Copyright (C) 2010-2013 Texas Instruments
 *
 * Authors: Liam Girdwood <lrg@ti.com>
 *          Misael Lopez Cruz <misael.lopez@ti.com>
 *          Sebastien Guiriec <s-guiriec@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * Contact: Misael Lopez Cruz <misael.lopez@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 */

#include <linux/export.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/soc-topology.h>

#include "omap-aess-priv.h"
#include "aess_mem.h"
#include "aess_gain.h"
#include "aess_port.h"
#include "aess_utils.h"

/* Firmware coefficients and equalizers */
#define OMAP_AESS_MAX_FW_SIZE		(1024 * 128)

/* Gain value conversion */
#define OMAP_AESS_MAX_GAIN		12000	/* 120dB */
#define OMAP_AESS_GAIN_SCALE		100	/* 1dB */
#define aess_gain_to_val(gain) \
	((val + OMAP_AESS_MAX_GAIN) / OMAP_AESS_GAIN_SCALE)
#define aess_val_to_gain(val) \
	(-OMAP_AESS_MAX_GAIN + (val * OMAP_AESS_GAIN_SCALE))

struct omap_aess_filter {
	/* type of filter */
	u32 equ_type;
	/* filter length */
	u32 equ_length;
	union {
		/* parameters are the direct and recursive coefficients in */
		/* Q6.26 integer fixed-point format. */
		s32 type1[NBEQ1];
		struct {
			/* center frequency of the band [Hz] */
			s32 freq[NBEQ2];
			/* gain of each band. [dB] */
			s32 gain[NBEQ2];
			/* Q factor of this band [dB] */
			s32 q[NBEQ2];
		} type2;
	} coef;
	s32 equ_param3;
};

/* TODO: we have to use the shift value atm to represent register
 * id due to current HAL ID MACROS not being unique.
 */
static int aess_put_mixer(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm = snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_platform *platform = snd_soc_dapm_kcontrol_platform(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_dapm_update update;
	long value = ucontrol->value.integer.value[0];

	update.kcontrol = kcontrol;
	update.reg = mc->reg;
	update.mask = 0x1 << mc->shift;
	if (value) {
		update.val = value << mc->shift;;
		aess->opp.widget[mc->reg] |= value << mc->shift;
		snd_soc_dapm_mixer_update_power(dapm, kcontrol, 1, &update);
		omap_aess_enable_gain(aess, mc->reg);
	} else {
		update.val = 0;
		aess->opp.widget[mc->reg] &= ~(0x1 << mc->shift);
		snd_soc_dapm_mixer_update_power(dapm, kcontrol, 0, &update);
		omap_aess_disable_gain(aess, mc->reg);
	}

	return 1;
}

static int aess_get_mixer(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_soc_dapm_kcontrol_platform(kcontrol);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);

	ucontrol->value.integer.value[0] =
				(aess->opp.widget[mc->reg] >> mc->shift) & 0x1;

	return 0;
}

int aess_mixer_enable_mono(struct omap_aess *aess, int id, int enable)
{
	int mixer;

	switch (id) {
	case OMAP_AESS_MIX_DL1_MONO:
		mixer = MIXDL1;
		break;
	case OMAP_AESS_MIX_DL2_MONO:
		mixer = MIXDL2;
		break;
	case OMAP_AESS_MIX_AUDUL_MONO:
		mixer = MIXAUDUL;
		break;
	default:
		return -EINVAL;
	}

	omap_aess_mono_mixer(aess, mixer, enable);

	return 0;
}

static int aess_put_mono_mixer(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int id = mc->shift - OMAP_AESS_MIX_DL1_MONO;

	aess->mixer.mono[id] = ucontrol->value.integer.value[0];
	aess_mixer_enable_mono(aess, mc->shift, aess->mixer.mono[id]);

	return 1;
}

static int aess_get_mono_mixer(struct snd_kcontrol *kcontrol,
			       struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	int id = mc->shift - OMAP_AESS_MIX_DL1_MONO;

	ucontrol->value.integer.value[0] = aess->mixer.mono[id];
	return 0;
}

/* router IDs that match our mixer strings */
static const int router[] = {
	OMAP_AESS_BUFFER_ZERO_ID, /* strangely this is not 0 */
	OMAP_AESS_BUFFER_DMIC1_L_ID,
	OMAP_AESS_BUFFER_DMIC1_R_ID,
	OMAP_AESS_BUFFER_DMIC2_L_ID,
	OMAP_AESS_BUFFER_DMIC2_R_ID,
	OMAP_AESS_BUFFER_DMIC3_L_ID,
	OMAP_AESS_BUFFER_DMIC3_R_ID,
	OMAP_AESS_BUFFER_BT_UL_L_ID,
	OMAP_AESS_BUFFER_BT_UL_R_ID,
	OMAP_AESS_BUFFER_MM_EXT_IN_L_ID,
	OMAP_AESS_BUFFER_MM_EXT_IN_R_ID,
	OMAP_AESS_BUFFER_AMIC_L_ID,
	OMAP_AESS_BUFFER_AMIC_R_ID,
	OMAP_AESS_BUFFER_VX_REC_L_ID,
	OMAP_AESS_BUFFER_VX_REC_R_ID,
};

static int aess_ul_mux_put_route(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm = snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_platform *platform = snd_soc_dapm_kcontrol_platform(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	struct snd_soc_dapm_update update;
	int mux = ucontrol->value.enumerated.item[0];
	int reg = e->reg - OMAP_AESS_MUX(0);
	u16 data;

	if (mux > OMAP_AESS_ROUTES_UL) {
		pr_err("inavlid mux %d\n", mux);
		return 0;
	}

	data = omap_aess_get_label_data(aess, router[mux]);

	/* TODO: remove the gap */
	if (reg < 6) {
		/* 0  .. 5   = MM_UL */
		aess->mixer.route_ul[reg] = data;
	} else if (reg < 10) {
		/* 10 .. 11  = MM_UL2 */
		/* 12 .. 13  = VX_UL */
		aess->mixer.route_ul[reg + 4] = data;
	}

	omap_aess_set_router_configuration(aess, (u32 *)aess->mixer.route_ul);

	if (router[mux] != OMAP_AESS_BUFFER_ZERO_ID)
		aess->opp.widget[e->reg] = e->shift_l;
	else
		aess->opp.widget[e->reg] = 0;

	update.kcontrol = kcontrol;
	update.reg = e->reg;
	update.mask = e->shift_l;
	update.val = aess->opp.widget[e->reg];
	snd_soc_dapm_mux_update_power(dapm, kcontrol, mux, e, &update);

	return 1;
}

static int aess_ul_mux_get_route(struct snd_kcontrol *kcontrol,
				 struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_soc_dapm_kcontrol_platform(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int reg = e->reg - OMAP_AESS_MUX(0), i, rval = 0;

	/* TODO: remove the gap */
	if (reg < 6) {
		/* 0  .. 5   = MM_UL */
		rval = aess->mixer.route_ul[reg];
	} else if (reg < 10) {
		/* 10 .. 11  = MM_UL2 */
		/* 12 .. 13  = VX_UL */
		rval = aess->mixer.route_ul[reg + 4];
	}

	for (i = 0; i < ARRAY_SIZE(router); i++) {
		if (omap_aess_get_label_data(aess, router[i]) == rval) {
			ucontrol->value.integer.value[0] = i;
			return 0;
		}
	}

	return 1;
}

static int aess_put_switch(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_dapm_context *dapm = snd_soc_dapm_kcontrol_dapm(kcontrol);
	struct snd_soc_platform *platform = snd_soc_dapm_kcontrol_platform(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_dapm_update update;
	long value = ucontrol->value.integer.value[0];

	update.kcontrol = kcontrol;
	update.reg = mc->reg;
	update.mask = 0x1 << mc->shift;
	if (value) {
		update.val = value << mc->shift;;
		aess->opp.widget[mc->reg] |= value << mc->shift;
		snd_soc_dapm_mixer_update_power(dapm, kcontrol, 1, &update);
	} else {
		update.val = 0;
		aess->opp.widget[mc->reg] &= ~(0x1 << mc->shift);
		snd_soc_dapm_mixer_update_power(dapm, kcontrol, 0, &update);
	}

	return 1;
}

static int aess_vol_put_mixer(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	long value = ucontrol->value.integer.value[0];

	omap_aess_write_mixer(aess, mc->reg, aess_val_to_gain(value));

	return 1;
}

static int aess_vol_put_gain(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;

	omap_aess_write_gain(aess, mc->shift,
		       aess_val_to_gain(ucontrol->value.integer.value[0]));
	omap_aess_write_gain(aess, mc->rshift,
		       aess_val_to_gain(ucontrol->value.integer.value[1]));

	return 1;
}

static int aess_vol_get_mixer(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	u32 val;

	omap_aess_read_mixer(aess, mc->reg, &val);
	ucontrol->value.integer.value[0] = aess_gain_to_val(val);

	return 0;
}

static int aess_vol_get_gain(struct snd_kcontrol *kcontrol,
			     struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	u32 val;

	omap_aess_read_gain(aess, mc->shift, &val);
	ucontrol->value.integer.value[0] = aess_gain_to_val(val);
	omap_aess_read_gain(aess, mc->rshift, &val);
	ucontrol->value.integer.value[1] = aess_gain_to_val(val);

	return 0;
}

/**
 * omap_aess_write_filter
 * @aess: Pointer on aess handle
 * @id: name of the equalizer
 * @param: equalizer coefficients
 *
 * Load the coefficients in CMEM.
 */
static int omap_aess_write_filter(struct omap_aess *aess, u32 id,
				  struct omap_aess_filter *param)
{
	struct omap_aess_addr equ_addr;
	u32 length, *src;

	switch (id) {
	case OMAP_AESS_CMEM_DL1_COEFS_ID:
		memcpy(&equ_addr,
		       &aess->fw_info.map[OMAP_AESS_SMEM_DL1_M_EQ_DATA_ID],
		       sizeof(struct omap_aess_addr));
		break;
	case OMAP_AESS_CMEM_DL2_L_COEFS_ID:
	case OMAP_AESS_CMEM_DL2_R_COEFS_ID:
		memcpy(&equ_addr,
		       &aess->fw_info.map[OMAP_AESS_SMEM_DL2_M_LR_EQ_DATA_ID],
		       sizeof(struct omap_aess_addr));
		break;
	case OMAP_AESS_CMEM_SDT_COEFS_ID:
		memcpy(&equ_addr,
		       &aess->fw_info.map[OMAP_AESS_SMEM_SDT_F_DATA_ID],
		       sizeof(struct omap_aess_addr));
		break;
	case OMAP_AESS_CMEM_96_48_AMIC_COEFS_ID:
		memcpy(&equ_addr,
		       &aess->fw_info.map[OMAP_AESS_SMEM_AMIC_96_48_DATA_ID],
		       sizeof(struct omap_aess_addr));
		break;
	case OMAP_AESS_CMEM_96_48_DMIC_COEFS_ID:
		memcpy(&equ_addr,
		       &aess->fw_info.map[OMAP_AESS_SMEM_DMIC0_96_48_DATA_ID],
		       sizeof(struct omap_aess_addr));
		/* three DMIC are clear at the same time DMIC0 DMIC1 DMIC2 */
		equ_addr.bytes *= 3;
		break;
	default:
		return -EINVAL;
	}

	/* reset SMEM buffers before the coefficients are loaded */
	omap_aess_mem_reset(aess, equ_addr);

	length = param->equ_length;
	src = (u32 *)((param->coef).type1);
	omap_aess_write_map(aess, id, src);

	/* reset SMEM buffers after the coefficients are loaded */
	omap_aess_mem_reset(aess, equ_addr);
	return 0;
}

int aess_mixer_set_equ_profile(struct omap_aess *aess, unsigned int id,
			      unsigned int profile)
{
	struct omap_aess_filter params;
	void *src_coeff;

	switch (id) {
	case OMAP_AESS_CMEM_DL1_COEFS_ID:
		aess->equ.dl1.profile = profile;
		params.equ_length = aess->equ.dl1.profile_size;
		src_coeff = aess->equ.dl1.coeff_data;
		break;
	case OMAP_AESS_CMEM_DL2_L_COEFS_ID:
		aess->equ.dl2l.profile = profile;
		params.equ_length = aess->equ.dl2l.profile_size;
		src_coeff = aess->equ.dl2l.coeff_data;
		break;
	case OMAP_AESS_CMEM_DL2_R_COEFS_ID:
		aess->equ.dl2r.profile = profile;
		params.equ_length = aess->equ.dl2r.profile_size;
		src_coeff = aess->equ.dl2r.coeff_data;
		break;
	case OMAP_AESS_CMEM_SDT_COEFS_ID:
		aess->equ.sdt.profile = profile;
		params.equ_length = aess->equ.sdt.profile_size;
		src_coeff = aess->equ.sdt.coeff_data;
		break;
	case OMAP_AESS_CMEM_96_48_AMIC_COEFS_ID:
		aess->equ.amic.profile = profile;
		params.equ_length = aess->equ.amic.profile_size;
		src_coeff = aess->equ.amic.coeff_data;
		break;
	case OMAP_AESS_CMEM_96_48_DMIC_COEFS_ID:
		aess->equ.dmic.profile = profile;
		params.equ_length = aess->equ.dmic.profile_size;
		src_coeff = aess->equ.dmic.coeff_data;
		break;
	default:
		return -EINVAL;
	}

	src_coeff += profile * params.equ_length;
	memcpy(params.coef.type1, src_coeff, params.equ_length);

	omap_aess_write_filter(aess, id, &params);

	return 0;
}

static int aess_get_equalizer(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_enum *eqc = (struct soc_enum *)kcontrol->private_value;

	switch (eqc->reg) {
	case OMAP_AESS_CMEM_DL1_COEFS_ID:
		ucontrol->value.integer.value[0] = aess->equ.dl1.profile;
		break;
	case OMAP_AESS_CMEM_DL2_L_COEFS_ID:
		ucontrol->value.integer.value[0] = aess->equ.dl2l.profile;
		break;
	case OMAP_AESS_CMEM_DL2_R_COEFS_ID:
		ucontrol->value.integer.value[0] = aess->equ.dl2r.profile;
		break;
	case OMAP_AESS_CMEM_SDT_COEFS_ID:
		ucontrol->value.integer.value[0] = aess->equ.sdt.profile;
		break;
	case OMAP_AESS_CMEM_96_48_AMIC_COEFS_ID:
		ucontrol->value.integer.value[0] = aess->equ.amic.profile;
		break;
	case OMAP_AESS_CMEM_96_48_DMIC_COEFS_ID:
		ucontrol->value.integer.value[0] = aess->equ.dmic.profile;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int aess_put_equalizer(struct snd_kcontrol *kcontrol,
			      struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_platform *platform = snd_kcontrol_chip(kcontrol);
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	struct soc_enum *eqc = (struct soc_enum *)kcontrol->private_value;
	u16 val = ucontrol->value.enumerated.item[0];
	int ret;

	ret = aess_mixer_set_equ_profile(aess, eqc->reg, val);
	if (ret < 0)
		return ret;

	return 1;
}

static const struct snd_soc_tplg_kcontrol_ops omap_aess_fw_ops[] = {
{OMAP_CONTROL_DEFAULT,	aess_get_mixer, aess_put_mixer, NULL},
{OMAP_CONTROL_VOLUME,	aess_vol_get_mixer, aess_vol_put_mixer, NULL},
{OMAP_CONTROL_ROUTER,	aess_ul_mux_get_route, aess_ul_mux_put_route, NULL},
{OMAP_CONTROL_EQU,	aess_get_equalizer, aess_put_equalizer, NULL},
{OMAP_CONTROL_GAIN,	aess_vol_get_gain, aess_vol_put_gain, NULL},
{OMAP_CONTROL_MONO,	aess_get_mono_mixer, aess_put_mono_mixer, NULL},
{OMAP_CONTROL_SWITCH,	NULL, aess_put_switch, NULL},
};

static int aess_load_coeffs(struct snd_soc_platform *platform,
			    struct snd_soc_tplg_hdr *hdr)
{
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	const struct snd_soc_file_coeff_data *cd = snd_soc_tplg_get_data(hdr);
	const void *coeff_data = cd + 1;

	dev_dbg(platform->dev,"coeff %d size 0x%x with %d elems\n",
		cd->id, cd->size, cd->count);

	switch (cd->id) {
	case OMAP_AESS_CMEM_DL1_COEFS_ID:
		aess->equ.dl1.profile_size = cd->size / cd->count;
		aess->equ.dl1.num_profiles = cd->count;
		aess->equ.dl1.coeff_data = kmalloc(cd->size, GFP_KERNEL);
		if (aess->equ.dl1.coeff_data == NULL)
			return -ENOMEM;
		memcpy(aess->equ.dl1.coeff_data, coeff_data, cd->size);
		break;
	case OMAP_AESS_CMEM_DL2_L_COEFS_ID:
		aess->equ.dl2l.profile_size = cd->size / cd->count;
		aess->equ.dl2l.num_profiles = cd->count;
		aess->equ.dl2l.coeff_data = kmalloc(cd->size, GFP_KERNEL);
		if (aess->equ.dl2l.coeff_data == NULL)
			return -ENOMEM;
		memcpy(aess->equ.dl2l.coeff_data, coeff_data, cd->size);
		break;
	case OMAP_AESS_CMEM_DL2_R_COEFS_ID:
		aess->equ.dl2r.profile_size = cd->size / cd->count;
		aess->equ.dl2r.num_profiles = cd->count;
		aess->equ.dl2r.coeff_data = kmalloc(cd->size, GFP_KERNEL);
		if (aess->equ.dl2r.coeff_data == NULL)
			return -ENOMEM;
		memcpy(aess->equ.dl2r.coeff_data, coeff_data, cd->size);
		break;
	case OMAP_AESS_CMEM_SDT_COEFS_ID:
		aess->equ.sdt.profile_size = cd->size / cd->count;
		aess->equ.sdt.num_profiles = cd->count;
		aess->equ.sdt.coeff_data = kmalloc(cd->size, GFP_KERNEL);
		if (aess->equ.sdt.coeff_data == NULL)
			return -ENOMEM;
		memcpy(aess->equ.sdt.coeff_data, coeff_data, cd->size);
		break;
	case OMAP_AESS_CMEM_96_48_AMIC_COEFS_ID:
		aess->equ.amic.profile_size = cd->size / cd->count;
		aess->equ.amic.num_profiles = cd->count;
		aess->equ.amic.coeff_data = kmalloc(cd->size, GFP_KERNEL);
		if (aess->equ.amic.coeff_data == NULL)
			return -ENOMEM;
		memcpy(aess->equ.amic.coeff_data, coeff_data, cd->size);
		break;
	case OMAP_AESS_CMEM_96_48_DMIC_COEFS_ID:
		aess->equ.dmic.profile_size = cd->size / cd->count;
		aess->equ.dmic.num_profiles = cd->count;
		aess->equ.dmic.coeff_data = kmalloc(cd->size, GFP_KERNEL);
		if (aess->equ.dmic.coeff_data == NULL)
			return -ENOMEM;
		memcpy(aess->equ.dmic.coeff_data, coeff_data, cd->size);
		break;
	default:
		dev_err(platform->dev, "invalid coefficient ID %d\n", cd->id);
		return -EINVAL;
	}

	return 0;
}

static int aess_load_fw(struct snd_soc_platform *platform,
			struct snd_soc_tplg_hdr *hdr)
{
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	const void *fw_data = snd_soc_tplg_get_data(hdr);

	/* get firmware and coefficients header info */
	memcpy(&aess->hdr, fw_data, sizeof(struct fw_header));
	if (hdr->size > OMAP_AESS_MAX_FW_SIZE) {
		dev_err(aess->dev, "Firmware too large (%d bytes)\n",
			hdr->size);
		return -ENOMEM;
	}
	dev_info(aess->dev, "AESS firmware size %d bytes\n", hdr->size);
	dev_info(aess->dev, "AESS mem P %d C %d D %d S %d bytes\n",
		 aess->hdr.pmem_size, aess->hdr.cmem_size,
		 aess->hdr.dmem_size, aess->hdr.smem_size);

	dev_info(aess->dev, "AESS Firmware version %x\n", aess->hdr.version);

	/* store AESS firmware for later context restore */
	aess->fw_data = fw_data;

	return 0;
}

static int aess_load_config(struct snd_soc_platform *platform,
			    struct snd_soc_tplg_hdr *hdr)
{
	struct omap_aess *aess = snd_soc_platform_get_drvdata(platform);
	const void *fw_data = snd_soc_tplg_get_data(hdr);

	/* store AESS config for later context restore */
	dev_info(aess->dev, "AESS Config size %d bytes\n", hdr->size);

	aess->fw_config = fw_data;

	return 0;
}

/* callback to handle vendor data */
static int aess_vendor_load(struct snd_soc_component *component,
			    int index,
			    struct snd_soc_tplg_hdr *hdr)
{
	struct snd_soc_platform *platform = snd_soc_component_to_platform(component);
	switch (hdr->type) {
	case SND_SOC_TPLG_TYPE_VENDOR_FW:
		return aess_load_fw(platform, hdr);
	case SND_SOC_TPLG_TYPE_VENDOR_CONFIG:
		return aess_load_config(platform, hdr);
	case SND_SOC_TPLG_TYPE_VENDOR_COEFF:
		return aess_load_coeffs(platform, hdr);
	case SND_SOC_TPLG_TYPEVENDOR_CODEC:
	default:
		dev_err(platform->dev, "vendor type %d:%d not supported\n",
			hdr->type, hdr->vendor_type);
		return 0;
	}
	return 0;
}

struct snd_soc_tplg_ops soc_tplg_ops = {
	.vendor_load	= aess_vendor_load,
	.io_ops		= omap_aess_fw_ops,
	.io_ops_count	= ARRAY_SIZE(omap_aess_fw_ops),
};
