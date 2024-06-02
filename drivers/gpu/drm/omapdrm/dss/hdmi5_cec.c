// SPDX-License-Identifier: GPL-2.0-only
/*
 * HDMI CEC
 *
 * Copyright 2019 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 */
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/clk.h>

#include "dss.h"
#include "hdmi.h"
#include "hdmi5_core.h"
#include "hdmi5_cec.h"

static int hdmi5_cec_log_addr(struct cec_adapter *adap, u8 logical_addr)
{
	struct hdmi_core_data *core = cec_get_drvdata(adap);
	u8 v;

	if (logical_addr == CEC_LOG_ADDR_INVALID) {
		hdmi_write_reg(core->base, HDMI_CORE_CEC_ADDR_L, 0);
		hdmi_write_reg(core->base, HDMI_CORE_CEC_ADDR_H, 0);
		return 0;
	}

	if (logical_addr <= 7) {
		v = hdmi_read_reg(core->base, HDMI_CORE_CEC_ADDR_L);
		v |= 1 << logical_addr;
		hdmi_write_reg(core->base, HDMI_CORE_CEC_ADDR_L, v);
		v = hdmi_read_reg(core->base, HDMI_CORE_CEC_ADDR_H);
		v |= 1 << 7;
		hdmi_write_reg(core->base, HDMI_CORE_CEC_ADDR_H, v);
	} else {
		v = hdmi_read_reg(core->base, HDMI_CORE_CEC_ADDR_H);
		v |= 1 << (logical_addr - 8);
		v |= 1 << 7;
		hdmi_write_reg(core->base, HDMI_CORE_CEC_ADDR_H, v);
	}

	return 0;
}

static int hdmi5_cec_transmit(struct cec_adapter *adap, u8 attempts,
			      u32 signal_free_time, struct cec_msg *msg)
{
	struct hdmi_core_data *core = cec_get_drvdata(adap);
	unsigned int i, ctrl;

	switch (signal_free_time) {
	case CEC_SIGNAL_FREE_TIME_RETRY:
		ctrl = CEC_CTRL_RETRY;
		break;
	case CEC_SIGNAL_FREE_TIME_NEW_INITIATOR:
	default:
		ctrl = CEC_CTRL_NORMAL;
		break;
	case CEC_SIGNAL_FREE_TIME_NEXT_XFER:
		ctrl = CEC_CTRL_IMMED;
		break;
	}

	for (i = 0; i < msg->len; i++)
		hdmi_write_reg(core->base,
			       HDMI_CORE_CEC_TX_DATA0 + i * 4, msg->msg[i]);

	hdmi_write_reg(core->base, HDMI_CORE_CEC_TX_CNT, msg->len);
	hdmi_write_reg(core->base, HDMI_CORE_CEC_CTRL,
		       ctrl | CEC_CTRL_START);

	return 0;
}

void hdmi5_cec_irq(struct hdmi_core_data *core)
{
	struct cec_adapter *adap = core->adap;
	unsigned int stat = hdmi_read_reg(core->base, HDMI_CORE_IH_CEC_STAT0);

	if (stat == 0)
		return;

	hdmi_write_reg(core->base, HDMI_CORE_IH_CEC_STAT0, stat);

	if (stat & CEC_STAT_ERROR_INIT)
		cec_transmit_attempt_done(adap, CEC_TX_STATUS_ERROR);
	else if (stat & CEC_STAT_DONE)
		cec_transmit_attempt_done(adap, CEC_TX_STATUS_OK);
	else if (stat & CEC_STAT_NACK)
		cec_transmit_attempt_done(adap, CEC_TX_STATUS_NACK);

	if (stat & CEC_STAT_EOM) {
		struct cec_msg msg = {};
		unsigned int len, i;

		len = hdmi_read_reg(core->base, HDMI_CORE_CEC_RX_CNT);
		if (len > sizeof(msg.msg))
			len = sizeof(msg.msg);

		for (i = 0; i < len; i++)
			msg.msg[i] =
				hdmi_read_reg(core->base,
					      HDMI_CORE_CEC_RX_DATA0 + i * 4);

		hdmi_write_reg(core->base, HDMI_CORE_CEC_LOCK, 0);

		msg.len = len;
		cec_received_msg(adap, &msg);
	}
}

static int hdmi5_cec_enable(struct cec_adapter *adap, bool enable)
{
	struct hdmi_core_data *core = cec_get_drvdata(adap);
	struct omap_hdmi *hdmi = container_of(core, struct omap_hdmi, core);
	unsigned int irqs;
	int err;

	if (!enable) {
		hdmi_write_reg(core->base, HDMI_CORE_CEC_MASK, ~0);
		hdmi_write_reg(core->base, HDMI_CORE_IH_MUTE_CEC_STAT0, ~0);
		hdmi_wp_clear_irqenable(core->wp, HDMI_IRQ_CORE);
		hdmi_wp_set_irqstatus(core->wp, HDMI_IRQ_CORE);
		REG_FLD_MOD(core->base, HDMI_CORE_MC_CLKDIS, 0x01, 5, 5);
		hdmi5_core_disable(hdmi);
		return 0;
	}
	err = hdmi5_core_enable(hdmi);
	if (err)
		return err;

	REG_FLD_MOD(core->base, HDMI_CORE_MC_CLKDIS, 0x00, 5, 5);
	hdmi_write_reg(core->base, HDMI_CORE_IH_I2CM_STAT0, ~0);
	hdmi_write_reg(core->base, HDMI_CORE_IH_MUTE_I2CM_STAT0, ~0);
	hdmi_write_reg(core->base, HDMI_CORE_CEC_CTRL, 0);
	hdmi_write_reg(core->base, HDMI_CORE_IH_CEC_STAT0, ~0);
	hdmi_write_reg(core->base, HDMI_CORE_CEC_LOCK, 0);
	hdmi_write_reg(core->base, HDMI_CORE_CEC_TX_CNT, 0);

	hdmi5_cec_log_addr(adap, CEC_LOG_ADDR_INVALID);

	/* Enable HDMI core interrupts */
	hdmi_wp_set_irqenable(core->wp, HDMI_IRQ_CORE);

	irqs = CEC_STAT_ERROR_INIT | CEC_STAT_NACK | CEC_STAT_EOM |
	       CEC_STAT_DONE;
	hdmi_write_reg(core->base, HDMI_CORE_CEC_MASK, ~irqs);
	hdmi_write_reg(core->base, HDMI_CORE_IH_MUTE_CEC_STAT0, ~irqs);
	return 0;
}

static const struct cec_adap_ops hdmi5_cec_ops = {
	.adap_enable = hdmi5_cec_enable,
	.adap_log_addr = hdmi5_cec_log_addr,
	.adap_transmit = hdmi5_cec_transmit,
};

void hdmi5_cec_set_phys_addr(struct hdmi_core_data *core, struct edid *edid)
{
	cec_s_phys_addr_from_edid(core->adap, edid);
}

int hdmi5_cec_init(struct platform_device *pdev, struct hdmi_core_data *core,
		   struct hdmi_wp_data *wp, struct drm_connector *conn)
{
	const u32 caps = CEC_CAP_DEFAULTS | CEC_CAP_CONNECTOR_INFO;
	struct cec_connector_info conn_info;
	unsigned int ret;

	core->cec_clk = devm_clk_get(&pdev->dev, "cec");
	if (IS_ERR(core->cec_clk))
		return PTR_ERR(core->cec_clk);
	ret = clk_prepare_enable(core->cec_clk);
	if (ret)
		return ret;

	core->adap = cec_allocate_adapter(&hdmi5_cec_ops, core,
					  "omap5", caps, CEC_MAX_LOG_ADDRS);
	ret = PTR_ERR_OR_ZERO(core->adap);
	if (ret < 0)
		return ret;
	cec_fill_conn_info_from_drm(&conn_info, conn);
	cec_s_conn_info(core->adap, &conn_info);
	core->wp = wp;

	ret = cec_register_adapter(core->adap, &pdev->dev);
	if (ret < 0) {
		cec_delete_adapter(core->adap);
		return ret;
	}
	return 0;
}

void hdmi5_cec_uninit(struct hdmi_core_data *core)
{
	clk_disable_unprepare(core->cec_clk);
	cec_unregister_adapter(core->adap);
}

