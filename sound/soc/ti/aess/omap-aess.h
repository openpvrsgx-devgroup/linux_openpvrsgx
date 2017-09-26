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
 */

#ifndef __OMAP_AESS_H__
#define __OMAP_AESS_H__

/* This must currently match the BE order in DSP */
enum omap_aess_be_id {
	OMAP_AESS_BE_ID_PDM_UL = 0,
	OMAP_AESS_BE_ID_PDM_DL1,
	OMAP_AESS_BE_ID_PDM_DL2,
	OMAP_AESS_BE_ID_BT_VX,
	OMAP_AESS_BE_ID_MM_FM,
	OMAP_AESS_BE_ID_MODEM,
	OMAP_AESS_BE_ID_DMIC0,
	OMAP_AESS_BE_ID_DMIC1,
	OMAP_AESS_BE_ID_DMIC2,
	OMAP_AESS_BE_ID_VXREC,

	OMAP_AESS_BE_ID_NUM,
};

enum omap_aess_port_id {
	/* Logical PORT IDs - Backend */
	OMAP_AESS_BE_PORT_DMIC0 = 0,
	OMAP_AESS_BE_PORT_DMIC1,
	OMAP_AESS_BE_PORT_DMIC2,
	OMAP_AESS_BE_PORT_PDM_DL1,
	OMAP_AESS_BE_PORT_PDM_DL2,
	OMAP_AESS_BE_PORT_MCASP,
	OMAP_AESS_BE_PORT_PDM_UL1,
	OMAP_AESS_BE_PORT_BT_VX_DL,
	OMAP_AESS_BE_PORT_BT_VX_UL,
	OMAP_AESS_BE_PORT_MM_EXT_UL,
	OMAP_AESS_BE_PORT_MM_EXT_DL,

	/* Logical PORT IDs - Frontend */
	OMAP_AESS_FE_PORT_MM_DL1,
	OMAP_AESS_FE_PORT_MM_UL1,
	OMAP_AESS_FE_PORT_MM_UL2,
	OMAP_AESS_FE_PORT_VX_DL,
	OMAP_AESS_FE_PORT_VX_UL,
	OMAP_AESS_FE_PORT_TONES,
	OMAP_AESS_FE_PORT_MM_DL_LP,

	OMAP_AESS_LAST_LOGICAL_PORT_ID,
};

struct omap_aess;

#if IS_ENABLED(CONFIG_SND_OMAP_SOC_AESS)
/* API to get and put aess handle */
struct omap_aess *omap_aess_get_handle(void);
void omap_aess_put_handle(struct omap_aess *aess);

int omap_aess_load_firmware(struct omap_aess *aess, char *fw_name);

int omap_aess_port_open(struct omap_aess *aess, int logical_id);
void omap_aess_port_close(struct omap_aess *aess, int logical_id);
int omap_aess_port_enable(struct omap_aess *aess, int logical_id);
int omap_aess_port_disable(struct omap_aess *aess, int logical_id);
int omap_aess_port_is_enabled(struct omap_aess *aess, int logical_id);

/* Power Management */
void omap_aess_pm_shutdown(struct omap_aess *aess);
void omap_aess_pm_get(struct omap_aess *aess);
void omap_aess_pm_put(struct omap_aess *aess);
void omap_aess_pm_set_mode(struct omap_aess *aess, int mode);

#if 0
/* Operating Point */
int omap_aess_opp_new_request(struct omap_aess *aess, struct device *dev,
			      int opp);
int omap_aess_opp_free_request(struct omap_aess *aess, struct device *dev);
#endif

/* DC Offset */
void omap_aess_dc_set_hs_offset(struct omap_aess *aess, int left, int right,
				int step_mV);
void omap_aess_dc_set_hf_offset(struct omap_aess *aess, int left, int right);
void omap_aess_set_dl1_gains(struct omap_aess *aess, int left, int right);

#else
static inline struct omap_aess *omap_aess_get_handle(void)
{
	return NULL;
}

static inline void omap_aess_put_handle(struct omap_aess *aess)
{
}

static inline int omap_aess_load_firmware(struct omap_aess *aess, char *fw_name)
{
	return -EINVAL;
}

static inline int omap_aess_port_open(struct omap_aess *aess, int logical_id)
{
	return 0;
}

static inline void omap_aess_port_close(struct omap_aess *aess, int logical_id)
{
}

static inline int omap_aess_port_enable(struct omap_aess *aess, int logical_id)
{
	return 0;
}

static inline int omap_aess_port_disable(struct omap_aess *aess, int logical_id)
{
	return 0;
}

static inline int omap_aess_port_is_enabled(struct omap_aess *aess,
					    int logical_id)
{
	return 0;
}

static inline void omap_aess_pm_shutdown(struct omap_aess *aess)
{
}

static inline void omap_aess_pm_get(struct omap_aess *aess)
{
}

static inline void omap_aess_pm_put(struct omap_aess *aess)
{
}

static inline void omap_aess_pm_set_mode(struct omap_aess *aess, int mode)
{
}

static inline void omap_aess_dc_set_hs_offset(struct omap_aess *aess, int left,
					      int right, int step_mV)
{
}

static inline void omap_aess_dc_set_hf_offset(struct omap_aess *aess, int left,
					      int right)
{
}

static inline void omap_aess_set_dl1_gains(struct omap_aess *aess, int left,
					   int right)
{
}

#endif /* IS_ENABLED(CONFIG_SND_OMAP_SOC_AESS) */

#endif	/* End of __OMAP_AESS_H__ */
