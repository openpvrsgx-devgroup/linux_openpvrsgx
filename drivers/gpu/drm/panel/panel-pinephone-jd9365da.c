// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 UBports
 * Author: Marius Gripsgard <marius@ubports.com>
 */

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>

#include <linux/backlight.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#define PINEPHONE_INIT_CMD_LEN	2

struct pinephone {
	struct drm_panel	panel;
	struct mipi_dsi_device	*dsi;

	struct backlight_device	*backlight;
	struct regulator	*dvdd;
	struct regulator	*avdd;
	struct regulator	*cvdd;
	struct gpio_desc	*reset;
};

static inline struct pinephone *panel_to_pinephone(struct drm_panel *panel)
{
	return container_of(panel, struct pinephone, panel);
}

struct pinephone_init_cmd {
	u8 data[PINEPHONE_INIT_CMD_LEN];
};

static const struct pinephone_init_cmd pinephone_init_cmds[] = {
	{ .data = { 0xE0, 0x00 } },

	{ .data = { 0xE1, 0x93} },
	{ .data = { 0xE2, 0x65 } },
  { .data = { 0xE3, 0xF8 } },
  { .data = { 0x80, 0x03 } },// 02-3lane, 03-4lane

  { .data = { 0x70, 0x10 } },
  { .data = { 0x71, 0x13 } },
  { .data = { 0x72, 0x06 } },
  { .data = { 0x75, 0x03 } },

  { .data = { 0xE0, 0x01 } },

  { .data = { 0x00, 0x00 } },
  { .data = { 0x01, 0x25 } },//0x1D
  { .data = { 0x03, 0x00 } },
  { .data = { 0x04, 0x25 } },

  { .data = { 0x17, 0x00 } },
  { .data = { 0x18, 0xD7 } },
  { .data = { 0x19, 0x01 } },
  { .data = { 0x1A, 0x00 } },
  { .data = { 0x1B, 0xD7 } },
  { .data = { 0x1C, 0x01 } },

  { .data = { 0x1F, 0x6A } },
  { .data = { 0x20, 0x28 } },
  { .data = { 0x21, 0x28 } },
  { .data = { 0x22, 0x4F } },
  { .data = { 0x24, 0xFe } },
  { .data = { 0x26, 0xF1 } },

  { .data = { 0x37, 0x09 } },

  { .data = { 0x38, 0x04 } },
  { .data = { 0x39, 0x08 } },
  { .data = { 0x3A, 0x12 } },
  { .data = { 0x3C, 0x6A } },
  { .data = { 0x3D, 0xFF } },
  { .data = { 0x3E, 0xFF } },
  { .data = { 0x3F, 0x7F } },

  { .data = { 0x40, 0x04 } },
  { .data = { 0x41, 0xB4 } },
  { .data = { 0x43, 0x14 } },
  { .data = { 0x44, 0x0F } },
  { .data = { 0x45, 0x46 } },

  { .data = { 0x55, 0x01 } },
  { .data = { 0x56, 0x01 } },
  { .data = { 0x57, 0x6D } },
  { .data = { 0x58, 0x0A } },
  { .data = { 0x59, 0x1A } },
  { .data = { 0x5A, 0x29 } },
  { .data = { 0x5B, 0x14 } },
  { .data = { 0x5C, 0x15 } },

  { .data = { 0x5D, 0x70 } },
  { .data = { 0x5E, 0x4F } },
  { .data = { 0x5F, 0x3A } },
  { .data = { 0x60, 0x2B } },
  { .data = { 0x61, 0x23 } },
  { .data = { 0x62, 0x12 } },
  { .data = { 0x63, 0x16 } },
  { .data = { 0x64, 0x01 } },
  { .data = { 0x65, 0x1D } },
  { .data = { 0x66, 0x1F } },
  { .data = { 0x67, 0x22 } },
  { .data = { 0x68, 0x41 } },
  { .data = { 0x69, 0x30 } },
  { .data = { 0x6A, 0x38 } },
  { .data = { 0x6B, 0x2B } },
  { .data = { 0x6C, 0x28 } },
  { .data = { 0x6D, 0x1D } },
  { .data = { 0x6E, 0x0E } },
  { .data = { 0x6F, 0x02 } },
  { .data = { 0x70, 0x70 } },
  { .data = { 0x71, 0x4F } },
  { .data = { 0x72, 0x3A } },
  { .data = { 0x73, 0x2B } },
  { .data = { 0x74, 0x23 } },
  { .data = { 0x75, 0x12 } },
  { .data = { 0x76, 0x16 } },
  { .data = { 0x77, 0x01 } },
  { .data = { 0x78, 0x1D } },
  { .data = { 0x79, 0x1F } },
  { .data = { 0x7A, 0x22 } },
  { .data = { 0x7B, 0x41 } },
  { .data = { 0x7C, 0x30 } },
  { .data = { 0x7D, 0x38 } },
  { .data = { 0x7E, 0x2B } },
  { .data = { 0x7F, 0x28 } },
  { .data = { 0x80, 0x1D } },
  { .data = { 0x81, 0x0E } },
  { .data = { 0x82, 0x02 } },

  { .data = { 0xE0, 0x02 } },

  { .data = { 0x00, 0x53 } },
  { .data = { 0x01, 0x51 } },
  { .data = { 0x02, 0x4B } },
  { .data = { 0x03, 0x49 } },
  { .data = { 0x04, 0x47 } },
  { .data = { 0x05, 0x45 } },
  { .data = { 0x06, 0x5F } },
  { .data = { 0x07, 0x5F } },
  { .data = { 0x08, 0x5F } },
  { .data = { 0x09, 0x5F } },
  { .data = { 0x0A, 0x5F } },
  { .data = { 0x0B, 0x5F } },
  { .data = { 0x0C, 0x5F } },
  { .data = { 0x0D, 0x5F } },
  { .data = { 0x0E, 0x5F } },
  { .data = { 0x0F, 0x5F } },
  { .data = { 0x10, 0x5F } },
  { .data = { 0x11, 0x5F } },
  { .data = { 0x12, 0x41 } },
  { .data = { 0x13, 0x43 } },
  { .data = { 0x14, 0x5F } },
  { .data = { 0x15, 0x5F } },

  { .data = { 0x16, 0x52 } },
  { .data = { 0x17, 0x50 } },
  { .data = { 0x18, 0x4A } },
  { .data = { 0x19, 0x48 } },
  { .data = { 0x1A, 0x46 } },
  { .data = { 0x1B, 0x44 } },
  { .data = { 0x1C, 0x5F } },
  { .data = { 0x1D, 0x5F } },
  { .data = { 0x1E, 0x5F } },
  { .data = { 0x1F, 0x5F } },
  { .data = { 0x20, 0x5F } },
  { .data = { 0x21, 0x5F } },
  { .data = { 0x22, 0x5F } },
  { .data = { 0x23, 0x5F } },
  { .data = { 0x24, 0x5F } },
  { .data = { 0x25, 0x5F } },
  { .data = { 0x26, 0x5F } },
  { .data = { 0x27, 0x5F } },
  { .data = { 0x28, 0x40 } },
  { .data = { 0x29, 0x42 } },
  { .data = { 0x2A, 0x5F } },
  { .data = { 0x2B, 0x5F } },

  { .data = { 0x2C, 0x00 } },
  { .data = { 0x2D, 0x02 } },
  { .data = { 0x2E, 0x08 } },
  { .data = { 0x2F, 0x0A } },
  { .data = { 0x30, 0x04 } },
  { .data = { 0x31, 0x06 } },
  { .data = { 0x32, 0x1F } },
  { .data = { 0x33, 0x1F } },
  { .data = { 0x34, 0x1F } },
  { .data = { 0x35, 0x1F } },
  { .data = { 0x36, 0x1F } },
  { .data = { 0x37, 0x1F } },
  { .data = { 0x38, 0x1F } },
  { .data = { 0x39, 0x1F } },
  { .data = { 0x3A, 0x1F } },
  { .data = { 0x3B, 0x1F } },
  { .data = { 0x3C, 0x1F } },
  { .data = { 0x3D, 0x1F } },
  { .data = { 0x3E, 0x12 } },
  { .data = { 0x3F, 0x10 } },
  { .data = { 0x40, 0x1F } },
  { .data = { 0x41, 0x1F } },

  { .data = { 0x42, 0x01 } },
  { .data = { 0x43, 0x03 } },
  { .data = { 0x44, 0x09 } },
  { .data = { 0x45, 0x0B } },
  { .data = { 0x46, 0x05 } },
  { .data = { 0x47, 0x07 } },
  { .data = { 0x48, 0x1F } },
  { .data = { 0x49, 0x1F } },
  { .data = { 0x4A, 0x1F } },
  { .data = { 0x4B, 0x1F } },
  { .data = { 0x4C, 0x1F } },
  { .data = { 0x4D, 0x1F } },
  { .data = { 0x4E, 0x1F } },
  { .data = { 0x4F, 0x1F } },
  { .data = { 0x50, 0x1F } },
  { .data = { 0x51, 0x1F } },
  { .data = { 0x52, 0x1F } },
  { .data = { 0x53, 0x1F } },
  { .data = { 0x54, 0x13 } },
  { .data = { 0x55, 0x11 } },
  { .data = { 0x56, 0x1F } },
  { .data = { 0x57, 0x1F } },

  { .data = { 0x58, 0x40 } },
  { .data = { 0x59, 0x00 } },
  { .data = { 0x5A, 0x00 } },
  { .data = { 0x5B, 0x30 } },
  { .data = { 0x5C, 0x08 } },
  { .data = { 0x5D, 0x40 } },
  { .data = { 0x5E, 0x01 } },
  { .data = { 0x5F, 0x02 } },
  { .data = { 0x60, 0x40 } },
  { .data = { 0x61, 0x01 } },
  { .data = { 0x62, 0x02 } },
  { .data = { 0x63, 0x58 } },
  { .data = { 0x64, 0x58 } },
  { .data = { 0x65, 0x75 } },
  { .data = { 0x66, 0xAC } },
  { .data = { 0x67, 0x73 } },
  { .data = { 0x68, 0x0B } },
  { .data = { 0x69, 0x58 } },
  { .data = { 0x6A, 0x58 } },
  { .data = { 0x6B, 0x08 } },
  { .data = { 0x6C, 0x00 } },
  { .data = { 0x6D, 0x00 } },
  { .data = { 0x6E, 0x00 } },
  { .data = { 0x6F, 0x00 } },
  { .data = { 0x70, 0x00 } },
  { .data = { 0x71, 0x00 } },
  { .data = { 0x72, 0x06 } },
  { .data = { 0x73, 0x86 } },
  { .data = { 0x74, 0x00 } },
  { .data = { 0x75, 0x07 } },
  { .data = { 0x76, 0x00 } },
  { .data = { 0x77, 0x5D } },
  { .data = { 0x78, 0x19 } },
  { .data = { 0x79, 0x00 } },
  { .data = { 0x7A, 0x05 } },
  { .data = { 0x7B, 0x05 } },
  { .data = { 0x7C, 0x00 } },
  { .data = { 0x7D, 0x03 } },
  { .data = { 0x7E, 0x86 } },

  { .data = { 0xE0, 0x04 } },
  { .data = { 0x09, 0x11 } },

  { .data = { 0xE0, 0x00 } },

  { .data = { 0x11, 0x00 } },
};

static const struct pinephone_init_cmd timed_cmds[] = {
	{ .data = { 0x29, 0x00 } },
	{ .data = { 0x35, 0x00 } },
};

static int pinephone_prepare(struct drm_panel *panel)
{
	struct pinephone *ctx = panel_to_pinephone(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	unsigned int i;
	int ret;

	ret = regulator_enable(ctx->avdd);
	if (ret)
		return ret;

	msleep(5);

	ret = regulator_enable(ctx->dvdd);
	if (ret)
		return ret;

	msleep(5);

	ret = regulator_enable(ctx->cvdd);
	if (ret)
		return ret;

	msleep(30);

	gpiod_set_value(ctx->reset, 1);
	msleep(50);

	gpiod_set_value(ctx->reset, 0);
	msleep(50);

	gpiod_set_value(ctx->reset, 1);
	msleep(200);

	for (i = 0; i < ARRAY_SIZE(pinephone_init_cmds); i++) {
		const struct pinephone_init_cmd *cmd = &pinephone_init_cmds[i];

		ret = mipi_dsi_dcs_write_buffer(dsi, cmd->data, PINEPHONE_INIT_CMD_LEN);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int pinephone_enable(struct drm_panel *panel)
{
	struct pinephone *ctx = panel_to_pinephone(panel);
	const struct pinephone_init_cmd *cmd = &timed_cmds[1];

	msleep(150);

	mipi_dsi_dcs_set_display_on(ctx->dsi);
	msleep(50);
//	mipi_dsi_dcs_set_tear_on(ctx->dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);

	mipi_dsi_dcs_write_buffer(ctx->dsi, cmd->data, PINEPHONE_INIT_CMD_LEN);
	backlight_enable(ctx->backlight);

	return 0;
}

static int pinephone_disable(struct drm_panel *panel)
{
	struct pinephone *ctx = panel_to_pinephone(panel);

	backlight_disable(ctx->backlight);
	return mipi_dsi_dcs_set_display_off(ctx->dsi);
}

static int pinephone_unprepare(struct drm_panel *panel)
{
	struct pinephone *ctx = panel_to_pinephone(panel);
	int ret;

	ret = mipi_dsi_dcs_set_display_off(ctx->dsi);
	if (ret < 0)
		DRM_DEV_ERROR(panel->dev, "failed to set display off: %d\n",
			      ret);

	ret = mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
	if (ret < 0)
		DRM_DEV_ERROR(panel->dev, "failed to enter sleep mode: %d\n",
			      ret);

	msleep(200);

	gpiod_set_value(ctx->reset, 0);

	msleep(5);

  regulator_disable(ctx->cvdd);

  msleep(5);

	regulator_disable(ctx->dvdd);

	msleep(5);

	regulator_disable(ctx->avdd);

	return 0;
}

static const struct drm_display_mode pinephone_default_mode = {
	.clock = 74000,
//	.vrefresh = 60,

	.hdisplay = 720,
	.hsync_start = 720 + 30,
	.hsync_end = 720 + 30 + 30,
	.htotal = 720 + 30 + 30 + 65,

	.vdisplay = 1440,
	.vsync_start = 1440 + 8,
	.vsync_end = 1440 + 8 + 4,
	.vtotal = 1440 + 8 + 4 + 11,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
	.width_mm	= 65,
	.height_mm	= 130,
};

static int pinephone_get_modes(struct drm_panel *panel,
				 struct drm_connector *connector)
{
	struct pinephone *ctx = panel_to_pinephone(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &pinephone_default_mode);
	if (!mode) {
		DRM_DEV_ERROR(&ctx->dsi->dev, "failed to add mode %ux%ux@%u\n",
			      pinephone_default_mode.hdisplay,
			      pinephone_default_mode.vdisplay,
//			      pinephone_default_mode.vrefresh);
			      60);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	drm_mode_probed_add(connector, mode);
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	return 1;
}

static const struct drm_panel_funcs pinephone_funcs = {
	.disable = pinephone_disable,
	.unprepare = pinephone_unprepare,
	.prepare = pinephone_prepare,
	.enable = pinephone_enable,
	.get_modes = pinephone_get_modes,
};

static int pinephone_dsi_probe(struct mipi_dsi_device *dsi)
{
	struct pinephone *ctx;

	ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);
	ctx->dsi = dsi;

	drm_panel_init(&ctx->panel, &dsi->dev, &pinephone_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ctx->dvdd = devm_regulator_get(&dsi->dev, "dvdd");
	if (IS_ERR(ctx->dvdd)) {
		DRM_DEV_ERROR(&dsi->dev, "Couldn't get dvdd regulator\n");
		return PTR_ERR(ctx->dvdd);
	}

	ctx->avdd = devm_regulator_get(&dsi->dev, "avdd");
	if (IS_ERR(ctx->avdd)) {
		DRM_DEV_ERROR(&dsi->dev, "Couldn't get avdd regulator\n");
		return PTR_ERR(ctx->avdd);
	}

	ctx->cvdd = devm_regulator_get(&dsi->dev, "cvdd");
	if (IS_ERR(ctx->cvdd)) {
		DRM_DEV_ERROR(&dsi->dev, "Couldn't get cvdd regulator\n");
		return PTR_ERR(ctx->cvdd);
	}

	ctx->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset)) {
		DRM_DEV_ERROR(&dsi->dev, "Couldn't get our reset GPIO\n");
		return PTR_ERR(ctx->reset);
	}

	ctx->backlight = devm_of_find_backlight(&dsi->dev);
	if (IS_ERR(ctx->backlight))
		return PTR_ERR(ctx->backlight);

	drm_panel_add(&ctx->panel);
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	return mipi_dsi_attach(dsi);
}

static void pinephone_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct pinephone *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id pinephone_of_match[] = {
	{ .compatible = "pinephone,jd9365da", },
	{ }
};
MODULE_DEVICE_TABLE(of, pinephone_of_match);

static struct mipi_dsi_driver pinephone_driver = {
	.probe = pinephone_dsi_probe,
	.remove = pinephone_dsi_remove,
	.driver = {
		.name = "pinephone-jd9365da",
		.of_match_table = pinephone_of_match,
	},
};
module_mipi_dsi_driver(pinephone_driver);

MODULE_AUTHOR("Marius Gripsgard <marius@ubports.com>");
MODULE_DESCRIPTION("pinephone MIPI-DSI LCD panel with jd9365da controller");
MODULE_LICENSE("GPL");
