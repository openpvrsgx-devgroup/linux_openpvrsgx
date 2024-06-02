/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HDMI header definition for OMAP5 HDMI CEC IP
 *
 * Copyright 2019 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 */

#ifndef _HDMI5_CEC_H_
#define _HDMI5_CEC_H_

/* HDMI CEC funcs */
#ifdef CONFIG_OMAP5_DSS_HDMI_CEC
void hdmi5_cec_set_phys_addr(struct hdmi_core_data *core,
			     struct edid *edid);
void hdmi5_cec_irq(struct hdmi_core_data *core);
int hdmi5_cec_init(struct platform_device *pdev, struct hdmi_core_data *core,
		   struct hdmi_wp_data *wp, struct drm_connector *conn);
void hdmi5_cec_uninit(struct hdmi_core_data *core);
#else
static inline void hdmi5_cec_set_phys_addr(struct hdmi_core_data *core,
					   struct edid *edid)
{
}

static inline void hdmi5_cec_irq(struct hdmi_core_data *core)
{
}

static inline int hdmi5_cec_init(struct platform_device *pdev,
				 struct hdmi_core_data *core,
				 struct hdmi_wp_data *wp,
				 struct drm_connector *conn)
{
	return 0;
}

static inline void hdmi5_cec_uninit(struct hdmi_core_data *core)
{
}
#endif

#endif
