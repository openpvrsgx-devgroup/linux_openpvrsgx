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

#ifndef __OMAP_AESS_PRIV_H__
#define __OMAP_AESS_PRIV_H__

#include "aess-fw.h"

#ifdef __KERNEL__

#include <sound/soc.h>
#include <sound/soc-fw.h>

/* AESS registers */
#define OMAP_AESS_REVISION			0x00
#define OMAP_AESS_MCU_IRQSTATUS_RAW		0x24
#define OMAP_AESS_MCU_IRQSTATUS			0x28
#define OMAP_AESS_MCU_IRQENABLE_SET		0x3C
#define OMAP_AESS_MCU_IRQENABLE_CLR		0x40
#define OMAP_AESS_DSP_IRQSTATUS_RAW		0x4C
#define OMAP_AESS_DMAENABLE_SET			0x60
#define OMAP_AESS_DMAENABLE_CLR			0x64
#define OMAP_AESS_EVENT_GENERATOR_COUNTER	0x68
#define OMAP_AESS_EVENT_GENERATOR_START		0x6C
#define OMAP_AESS_EVENT_SOURCE_SELECTION	0x70
#define OMAP_AESS_AUDIO_ENGINE_SCHEDULER	0x74
#define OMAP_AESS_AUTO_GATING_ENABLE		0x7C
#define OMAP_AESS_DMASTATUS_RAW			0x84

#endif /* __KERNEL__ */

enum omap_aess_dai_id {
	OMAP_AESS_DAI_MM1 = 0,
	OMAP_AESS_DAI_MM2_CAPTURE,
	OMAP_AESS_DAI_VOICE,
	OMAP_AESS_DAI_TONES,
	OMAP_AESS_DAI_MODEM,
	OMAP_AESS_DAI_MM1_LP,

	OMAP_AESS_DAI_NUM,
};

#define OMAP_AESS_MIXER_BASE	(0)
#define OMAP_AESS_MIXER(x)	(OMAP_AESS_MIXER_BASE + x)

#define OMAP_AESS_MIX_SWITCH_PDM_DL	OMAP_AESS_MIXER(1)
#define OMAP_AESS_MIX_SWITCH_BT_VX_DL	OMAP_AESS_MIXER(2)
#define OMAP_AESS_MIX_SWITCH_MM_EXT_DL	OMAP_AESS_MIXER(3)
#define OMAP_AESS_MIX_DL1_MONO		OMAP_AESS_MIXER(4)
#define OMAP_AESS_MIX_DL2_MONO		OMAP_AESS_MIXER(5)
#define OMAP_AESS_MIX_AUDUL_MONO	OMAP_AESS_MIXER(6)

#define OMAP_AESS_VIRTUAL_SWITCH	36

#define OMAP_AESS_NUM_MONO_MIXERS	(OMAP_AESS_MIX_AUDUL_MONO - OMAP_AESS_MIX_DL1_MONO + 1)
#define OMAP_AESS_NUM_MIXERS		(OMAP_AESS_MIX_AUDUL_MONO + 1)

#define OMAP_AESS_MUX_BASE	(37)
#define OMAP_AESS_MUX(x)	(OMAP_AESS_MUX_BASE + x)

#define OMAP_AESS_MUX_MM_UL10		OMAP_AESS_MUX(0)
#define OMAP_AESS_MUX_MM_UL11		OMAP_AESS_MUX(1)
#define OMAP_AESS_MUX_MM_UL12		OMAP_AESS_MUX(2)
#define OMAP_AESS_MUX_MM_UL13		OMAP_AESS_MUX(3)
#define OMAP_AESS_MUX_MM_UL14		OMAP_AESS_MUX(4)
#define OMAP_AESS_MUX_MM_UL15		OMAP_AESS_MUX(5)
#define OMAP_AESS_MUX_MM_UL20		OMAP_AESS_MUX(6)
#define OMAP_AESS_MUX_MM_UL21		OMAP_AESS_MUX(7)
#define OMAP_AESS_MUX_VX_UL0		OMAP_AESS_MUX(8)
#define OMAP_AESS_MUX_VX_UL1		OMAP_AESS_MUX(9)

#define OMAP_AESS_NUM_MUXES		(OMAP_AESS_MUX_VX_UL1 - OMAP_AESS_MUX_MM_UL10)

#define OMAP_AESS_WIDGET_BASE	(OMAP_AESS_MUX_VX_UL1 + 1)
#define OMAP_AESS_WIDGET(x)	(OMAP_AESS_WIDGET_BASE + x)

/* AESS AIF Frontend Widgets */
#define OMAP_AESS_AIF_TONES_DL		OMAP_AESS_WIDGET(0)
#define OMAP_AESS_AIF_VX_DL		OMAP_AESS_WIDGET(1)
#define OMAP_AESS_AIF_VX_UL		OMAP_AESS_WIDGET(2)
#define OMAP_AESS_AIF_MM_UL1		OMAP_AESS_WIDGET(3)
#define OMAP_AESS_AIF_MM_UL2		OMAP_AESS_WIDGET(4)
#define OMAP_AESS_AIF_MM_DL		OMAP_AESS_WIDGET(5)
#define OMAP_AESS_AIF_MM_DL_LP		OMAP_AESS_AIF_MM_DL
#define OMAP_AESS_AIF_MODEM_DL		OMAP_AESS_WIDGET(6)
#define OMAP_AESS_AIF_MODEM_UL		OMAP_AESS_WIDGET(7)

/* AESS AIF Backend Widgets */
#define OMAP_AESS_AIF_PDM_UL1		OMAP_AESS_WIDGET(8)
#define OMAP_AESS_AIF_PDM_DL1		OMAP_AESS_WIDGET(9)
#define OMAP_AESS_AIF_PDM_DL2		OMAP_AESS_WIDGET(10)
#define OMAP_AESS_AIF_BT_VX_UL		OMAP_AESS_WIDGET(11)
#define OMAP_AESS_AIF_BT_VX_DL		OMAP_AESS_WIDGET(12)
#define OMAP_AESS_AIF_MM_EXT_UL		OMAP_AESS_WIDGET(13)
#define OMAP_AESS_AIF_MM_EXT_DL		OMAP_AESS_WIDGET(14)
#define OMAP_AESS_AIF_DMIC0		OMAP_AESS_WIDGET(15)
#define OMAP_AESS_AIF_DMIC1		OMAP_AESS_WIDGET(16)
#define OMAP_AESS_AIF_DMIC2		OMAP_AESS_WIDGET(17)

/* AESS ROUTE_UL MUX Widgets */
#define OMAP_AESS_MUX_UL00		OMAP_AESS_WIDGET(18)
#define OMAP_AESS_MUX_UL01		OMAP_AESS_WIDGET(19)
#define OMAP_AESS_MUX_UL02		OMAP_AESS_WIDGET(20)
#define OMAP_AESS_MUX_UL03		OMAP_AESS_WIDGET(21)
#define OMAP_AESS_MUX_UL04		OMAP_AESS_WIDGET(22)
#define OMAP_AESS_MUX_UL05		OMAP_AESS_WIDGET(23)
#define OMAP_AESS_MUX_UL10		OMAP_AESS_WIDGET(24)
#define OMAP_AESS_MUX_UL11		OMAP_AESS_WIDGET(25)
#define OMAP_AESS_MUX_VX00		OMAP_AESS_WIDGET(26)
#define OMAP_AESS_MUX_VX01		OMAP_AESS_WIDGET(27)

/* AESS Volume and Mixer Widgets */
#define OMAP_AESS_MIXER_DL1		OMAP_AESS_WIDGET(28)
#define OMAP_AESS_MIXER_DL2		OMAP_AESS_WIDGET(29)
#define OMAP_AESS_VOLUME_DL1		OMAP_AESS_WIDGET(30)
#define OMAP_AESS_MIXER_AUDIO_UL	OMAP_AESS_WIDGET(31)
#define OMAP_AESS_MIXER_VX_REC		OMAP_AESS_WIDGET(32)
#define OMAP_AESS_MIXER_SDT		OMAP_AESS_WIDGET(33)
#define OMAP_AESS_VSWITCH_DL1_PDM	OMAP_AESS_WIDGET(34)
#define OMAP_AESS_VSWITCH_DL1_BT_VX	OMAP_AESS_WIDGET(35)
#define OMAP_AESS_VSWITCH_DL1_MM_EXT	OMAP_AESS_WIDGET(36)
#define OMAP_AESS_AIF_VXREC		OMAP_AESS_WIDGET(37)

#define OMAP_AESS_NUM_WIDGETS	(OMAP_AESS_AIF_VXREC - OMAP_AESS_AIF_TONES_DL)

#define OMAP_AESS_NUM_DAPM_REG	(OMAP_AESS_AIF_VXREC + 1)

#define OMAP_AESS_ROUTES_UL		14

/* TODO: combine / sort */
#define OMAP_AESS_MIXER_DEFAULT	128
#define OMAP_AESS_MIXER_MONO	129
#define OMAP_AESS_MIXER_ROUTER	130
#define OMAP_AESS_MIXER_EQU	131
#define OMAP_AESS_MIXER_SWITCH	132
#define OMAP_AESS_MIXER_GAIN	133
#define OMAP_AESS_MIXER_VOLUME	134

#define OMAP_CONTROL_DEFAULT \
	SOC_CONTROL_ID(OMAP_AESS_MIXER_DEFAULT, \
		OMAP_AESS_MIXER_DEFAULT, \
		SOC_CONTROL_TYPE_EXT)
#define OMAP_CONTROL_MONO \
	SOC_CONTROL_ID(OMAP_AESS_MIXER_MONO, \
		OMAP_AESS_MIXER_MONO, \
		SOC_CONTROL_TYPE_VOLSW)
#define OMAP_CONTROL_ROUTER \
	SOC_CONTROL_ID(OMAP_AESS_MIXER_ROUTER, \
		OMAP_AESS_MIXER_ROUTER, \
		SOC_CONTROL_TYPE_ENUM)
#define OMAP_CONTROL_EQU \
	SOC_CONTROL_ID(OMAP_AESS_MIXER_EQU, \
		OMAP_AESS_MIXER_EQU, \
		SOC_CONTROL_TYPE_ENUM)
#define OMAP_CONTROL_SWITCH \
	SOC_CONTROL_ID(OMAP_AESS_MIXER_DEFAULT, \
		OMAP_AESS_MIXER_SWITCH, \
		SOC_CONTROL_TYPE_VOLSW)
#define OMAP_CONTROL_GAIN \
	SOC_CONTROL_ID(OMAP_AESS_MIXER_GAIN, \
		OMAP_AESS_MIXER_GAIN, \
		SOC_CONTROL_TYPE_VOLSW)
#define OMAP_CONTROL_VOLUME \
	SOC_CONTROL_ID(OMAP_AESS_MIXER_VOLUME, \
		OMAP_AESS_MIXER_VOLUME, \
		SOC_CONTROL_TYPE_VOLSW)

enum opp_level {
	OMAP_AESS_OPP_25 = 0,
	OMAP_AESS_OPP_50,
	OMAP_AESS_OPP_100,
	OMAP_AESS_OPP_COUNT,
};

#ifdef __KERNEL__

#define OMAP_AESS_DMA_RESOURCES	8
#define OMAP_AESS_IO_RESOURCES	5

/*
 * AESS Firmware Header.
 * The AESS firmware blob has a header that describes each data section. This
 * way we can store coefficients etc in the firmware.
 */
struct fw_header {
	u32 version;	/* min version of AESS firmware required */
	u32 pmem_size;
	u32 cmem_size;
	u32 dmem_size;
	u32 smem_size;
};

struct omap_aess_dc_offset {
	/* DC offset cancellation */
	int power_mode;
	u32 hsl;
	u32 hsr;
	u32 hfl;
	u32 hfr;
};

struct omap_aess_opp {
	struct mutex mutex;
	struct mutex req_mutex;
	int level;
	unsigned long freqs[OMAP_AESS_OPP_COUNT];
	u32 widget[OMAP_AESS_NUM_DAPM_REG + 1];
	struct list_head req;
	int req_count;
};

struct omap_aess_modem {
	struct snd_pcm_substream *substream[2];
	struct snd_soc_dai *dai;
};

struct omap_aess_coeff {
	int profile; /* current enabled profile */
	int num_profiles;
	int profile_size;
	void *coeff_data;
};

struct omap_aess_equ {
	struct omap_aess_coeff dl1;
	struct omap_aess_coeff dl2l;
	struct omap_aess_coeff dl2r;
	struct omap_aess_coeff sdt;
	struct omap_aess_coeff amic;
	struct omap_aess_coeff dmic;
};

struct omap_aess_dai {
	int num_active;
	int num_suspended;
};

struct omap_aess_mixer {
	int mono[OMAP_AESS_NUM_MONO_MIXERS];
	u16 route_ul[16];
};

struct omap_aess_mapping {
	struct omap_aess_addr *map;
	u32 *fct_id;
	u32 *label_id;
	int nb_init_task;
	struct omap_aess_task *init_table;
	struct omap_aess_port *port;
	struct omap_aess_port *ping_pong;
	struct omap_aess_task *dl1_mono_mixer;
	struct omap_aess_task *dl2_mono_mixer;
	struct omap_aess_task *audul_mono_mixer;
	int *asrc;
};

struct omap_aess_pingppong {
	int buf_id;
	int buf_id_next;
	int buf_addr[4];
	int first_irq;

	/* size of each ping/pong buffers */
	u32 size;
	/* base addresses of the ping pong buffers in bytes addresses */
	u32 base_address[MAX_PINGPONG_BUFFERS];
};

struct omap_aess_gain {
	u8  muted_indicator;
	u32 desired_decibel;
	u32 muted_decibel;
	u32 desired_linear;
	u32 desired_ramp_delay_ms;
};

struct omap_aess_debug;

/*
 * AESS private data.
 */
struct omap_aess {
	struct device *dev;

	struct clk *clk;
	void __iomem *io_base[OMAP_AESS_IO_RESOURCES];
	u32 dmem_l4;
	u32 dmem_l3;
	u32 aess_config_l3;
	int irq;
	int active;
	int nr_users;	/* Number of external users of omap_aess struct */
	struct mutex mutex;
	int (*get_context_lost_count)(struct device *dev);
	int (*device_scale)(struct device *req_dev,
			    struct device *target_dev,
			    unsigned long rate);
	u32 context_lost;

	struct omap_aess_opp opp;
	struct omap_aess_dc_offset dc_offset;
	struct omap_aess_modem modem;
	struct omap_aess_equ equ;
	struct omap_aess_dai dai;
	struct omap_aess_mixer mixer;

	/* firmware */
	struct fw_header hdr;
	const void *fw_config;
	const void *fw_data;
	const struct firmware *fw;
	bool fw_loaded;

	/* from former omap_aess struct */
	u32 firmware_version_number;
	u16 MultiFrame[25][8];

	/* Housekeeping for gains */
	struct omap_aess_gain gains[MAX_NBGAIN_CMEM];

	/* Ping-Pong mode */
	struct omap_aess_pingppong pingpong;

	u32 irq_dbg_read_ptr;
	struct omap_aess_mapping fw_info;

	/* List of open AESS logical ports */
	struct list_head ports;

	/* spinlock */
	spinlock_t lock;

#ifdef CONFIG_DEBUG_FS
	struct omap_aess_debug *debug;
	struct dentry *debugfs_root;
	struct dentry *debugfs_ports_root;
#endif
};

/* Extern variables, structures, functions */
extern struct snd_soc_fw_platform_ops soc_fw_ops;
extern struct snd_soc_platform_driver omap_aess_platform;
extern struct snd_soc_dai_driver omap_aess_dai[6];

int aess_mixer_enable_mono(struct omap_aess *aess, int id, int enable);
int aess_mixer_set_equ_profile(struct omap_aess *aess, unsigned int id,
				     unsigned int profile);

void aess_init_debugfs(struct omap_aess *aess);
void aess_cleanup_debugfs(struct omap_aess *aess);

#endif  /* __kernel__ */
#endif	/* End of __OMAP_AESS_PRIV_H__ */
