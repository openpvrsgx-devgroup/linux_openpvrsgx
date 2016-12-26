// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Driver for BOE W677L panel with Orise OTM1283A controller
 *
 * Copyright 2011 Texas Instruments, Inc.
 * Author: Archit Taneja <archit@ti.com>
 * Author: Tomi Valkeinen <tomi.valkeinen@ti.com>
 *
 * based on d2l panel driver by Jerry Alexander <x0135174@ti.com>
 *
 * Copyright (C) 2014-2019 Golden Delicious Computers
 * by H. Nikolaus Schaller <hns@goldelico.com>
 * based on lg4591 panel driver
 * harmonized with latest panel-dsi-cm.c and panel-orisetech-otm8009a.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define LOG 1
#define OPTIONAL 0

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#if LOG
#undef dev_dbg
#define dev_dbg dev_info
#endif

/* extended DCS commands (not defined in mipi_display.h) */
#define DCS_READ_DDB_START		0x02
#define DCS_READ_NUM_ERRORS		0x05
#define DCS_READ_BRIGHTNESS		0x52	/* read brightness */
#define DCS_READ_CTRL_DISPLAY	0x53	/* read control */
#define DCS_WRITE_CABC			0x55
#define DCS_READ_CABC			0x56
#define MCS_READID1		0xda
#define MCS_READID2		0xdb
#define MCS_READID3		0xdc

/* DCS NOP is 1-byte 0x00 while 2-byte 0x00 is address shift function of this controller */

#define IS_MCS(CMD, LEN) ((((CMD) == 0) && (LEN) == 2) || (((CMD) >= 0xb0 && (CMD) <= 0xd9)) || ((CMD) >= 0xe0))

/* horizontal * vertical * refresh */
#define W677L_W				(720)
#define W677L_H				(1280)
#define W677L_WIDTH			(W677L_W+80+88)
#define W677L_HEIGHT			(W677L_H+160)
#define W677L_FPS			(60)
#define W677L_PIXELCLOCK		(W677L_WIDTH * W677L_HEIGHT * W677L_FPS)	/* Full HD * 60 fps */
/* panel has 16.7M colors = RGB888 = 3*8 bit per pixel */
#define W677L_PIXELFORMAT		OMAP_DSS_DSI_FMT_RGB888	/* 16.7M color = RGB888 */
#define W677L_BIT_PER_PIXEL	(3*8)
/* the panel can handle 4 lanes */
#define W677L_LANES			4
/* high speed clock is running at double data rate, i.e. half speed
 * (take care of integer overflows!)
 * hsck =  bit/pixel * 110% * pixel clock / lanes / 2 clock edges
 * real clock rate may be rounded up or down depending on divisors
 */
#define W677L_HS_CLOCK			(W677L_BIT_PER_PIXEL * (W677L_PIXELCLOCK / (W677L_LANES * 2)))
/* low power clock is quite arbitrarily choosen to be roughly 10 MHz */
#define W677L_LP_CLOCK			9200000	/* low power clock */

static const struct drm_display_mode default_mode = {
	.clock			= W677L_PIXELCLOCK / 1000,	/* kHz */
	.hdisplay		= W677L_W,
	.hsync_start		= W677L_W + 5,
	.hsync_end		= W677L_W + 5 + 5,
	.htotal			= W677L_WIDTH,
	.vdisplay		= W677L_H,
	.vsync_start		= W677L_H + 50,
	.vsync_end		= W677L_HEIGHT - 50,
	.vtotal			= W677L_HEIGHT,
	.flags			= 0,
	.width_mm		= 63,
	.height_mm		= 112,
};

struct otm1283a {
	struct device *dev;
	struct backlight_device *bl_dev;
	struct drm_panel panel;
	struct gpio_desc *reset_gpio;
	bool prepared;
	bool enabled;

	struct gpio_desc *regulator_gpio;
	enum drm_panel_orientation orientation;
};

static inline struct otm1283a *panel_to_otm1283a(struct drm_panel *panel)
{
	return container_of(panel, struct otm1283a, panel);
}

typedef u8 w677l_reg[20];	/* data[0] is length, data[1] is first byte */

static w677l_reg init_seq[] = {
	{ 2, 0x00, 0x00, },
	{ 4, 0xff, 0x12, 0x83, 0x01, },	//EXTC=1
	{ 2, 0x00, 0x80, },	            //Orise mode enable
	{ 3, 0xff, 0x12, 0x83, },


//-------------------- panel setting --------------------//
	{ 2, 0x00, 0x80, },              //TCON Setting
	{ 10, 0xc0, 0x00, 0x64, 0x00, 0x0f, 0x11, 0x00, 0x64, 0x0f, 0x11, },

	{ 2, 0x00, 0x90, },             //Panel Timing Setting
	{ 7, 0xc0, 0x00, 0x5c, 0x00, 0x01, 0x00, 0x04, },

	{ 2, 0x00, 0x87, },
	{ 2, 0xc4, 0x18, },

	{ 2, 0x00, 0xb3, },            //Interval Scan Frame: 0 frame,  column inversion
	{ 3, 0xc0, 0x00, 0x50, },

	{ 2, 0x00, 0x81, },              //frame rate:60Hz
	{ 2, 0xc1, 0x66, },

	{ 2, 0x00, 0x81, },
	{ 3, 0xc4, 0x82, 0x02, },

	{ 2, 0x00, 0x90, },
	{ 2, 0xc4, 0x49, },

	{ 2, 0x00, 0xc6, },
	{ 2, 0xb0, 0x03, },

	{ 2, 0x00, 0x90, },             //Mode-3
	{ 5, 0xf5, 0x02, 0x11, 0x02, 0x11, },

	{ 2, 0x00, 0x90, },		//2xVPNL,  1.5*=00,  2*=50,  3*=a0
	{ 2, 0xc5, 0x50, },

	{ 2, 0x00, 0x94, },		//Frequency
	{ 2, 0xc5, 0x66, },

	{ 2, 0x00, 0xb2, },		//VGLO1 setting
	{ 3, 0xf5, 0x00, 0x00, },

	{ 2, 0x00, 0xb4, },		//VGLO1_S setting
	{ 3, 0xf5, 0x00, 0x00, },

	{ 2, 0x00, 0xb6, },		//VGLO2 setting
	{ 3, 0xf5, 0x00, 0x00, },

	{ 2, 0x00, 0xb8, },		//VGLO2_S setting
	{ 3, 0xf5, 0x00, 0x00, },

	{ 2, 0x00, 0x94, },		//VCL ON
	{ 2, 0xf5, 0x02, },

	{ 2, 0x00, 0xBA, },		//VSP ON
	{ 2, 0xf5, 0x03, },

	{ 2, 0x00, 0xb2, },		//VGHO Option
	{ 2, 0xc5, 0x40, },

	{ 2, 0x00, 0xb4, },		//VGLO Option
	{ 2, 0xc5, 0xC0, },

//-------------------- power setting --------------------//
	{ 2, 0x00, 0xa0, },		//dcdc setting
	{ 15, 0xc4, 0x05, 0x10, 0x06, 0x02, 0x05, 0x15, 0x10, 0x05, 0x10, 0x07, 0x02, 0x05, 0x15, 0x10, },

	{ 2, 0x00, 0xb0, },		//clamp voltage setting
	{ 3, 0xc4, 0x00, 0x00, },

	{ 2, 0x00, 0x91, },		//VGH=13V,  VGL=-12V,  pump ratio:VGH=6x,  VGL=-5x
	{ 3, 0xc5, 0x19, 0x50, },

	{ 2, 0x00, 0x00, },		//GVDD=4.87V,  NGVDD=-4.87V
	{ 3, 0xd8, 0xbc, 0xbc, },

	{ 2, 0x00, 0x00, },		//VCOMDC=-1.1
	{ 2, 0xd9, 0x5a, },  		//5d  6f

	{ 2, 0x00, 0x00, },
	{ 17, 0xE1, 0x01, 0x07, 0x0b, 0x0d, 0x06, 0x0d, 0x0b, 0x0a, 0x04, 0x07, 0x10, 0x08, 0x0f, 0x11, 0x0a, 0x01, },

	{ 2, 0x00, 0x00, },
	{ 17, 0xE2, 0x01, 0x07, 0x0b, 0x0d, 0x06, 0x0d, 0x0b, 0x0a, 0x04, 0x07, 0x10, 0x08, 0x0f, 0x11, 0x0a, 0x01, },

	{ 2, 0x00, 0xb0, },		//VDD_18V=1.7V,  LVDSVDD=1.55V
	{ 3, 0xc5, 0x04, 0xB8, },

	{ 2, 0x00, 0xbb, },		//LVD voltage level setting
	{ 2, 0xc5, 0x80, },

//-------------------- panel timing state control --------------------//
	{ 2, 0x00, 0x80, },		//panel timing state control
	{ 12, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0x90, },		//panel timing state control
	{ 16, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xa0, },		//panel timing state control
	{ 16, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xb0, },		//panel timing state control
	{ 16, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xc0, },		//panel timing state control
	{ 16, 0xcb, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xd0, },		//panel timing state control
	{ 16, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x00, 0x00, },

	{ 2, 0x00, 0xe0, },		//panel timing state control
	{ 15, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x05, },

	{ 2, 0x00, 0xf0, },		//panel timing state control
	{ 12, 0xcb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, },

//-------------------- panel pad mapping control --------------------//
	{ 2, 0x00, 0x80, },		//panel pad mapping control
	{ 16, 0xcc, 0x0a, 0x0c, 0x0e, 0x10, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0x90, },		//panel pad mapping control
	{ 16, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0x2d, 0x09, 0x0b, 0x0d, 0x0f, 0x01, 0x03, 0x00, 0x00, },

	{ 2, 0x00, 0xa0, },		//panel pad mapping control
	{ 15, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0x2d, },

	{ 2, 0x00, 0xb0, },		//panel pad mapping control
	{ 16, 0xcc, 0x0F, 0x0D, 0x0B, 0x09, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xc0, },		//panel pad mapping control
	{ 16, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2d, 0x2e, 0x10, 0x0E, 0x0C, 0x0A, 0x04, 0x02, 0x00, 0x00, },

	{ 2, 0x00, 0xd0, },		//panel pad mapping control
	{ 15, 0xcc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2d, 0x2e, },

//-------------------- panel timing setting --------------------//
	{ 2, 0x00, 0x80, },		//panel VST setting
	{ 13, 0xce, 0x8D, 0x03, 0x00, 0x8C, 0x03, 0x00, 0x8B, 0x03, 0x00, 0x8A, 0x03, 0x00, },

	{ 2, 0x00, 0x90, },		//panel VEND setting
	{ 15, 0xce, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xa0, },		//panel CLKA1/2 setting
	{ 15, 0xce, 0x38, 0x0B, 0x04, 0xFC, 0x00, 0x00, 0x00, 0x38, 0x0A, 0x04, 0xFD, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xb0, },		//panel CLKA3/4 setting
	{ 15, 0xce, 0x38,  0x09,  0x04, 0xFE,  0x00, 0x00,  0x00,  0x38, 0x08,  0x04, 0xFF,  0x00,  0x00, 0x00, },

	{ 2, 0x00, 0xc0, },		//panel CLKb1/2 setting
	{ 15, 0xce, 0x38,  0x07,  0x05, 0x00,  0x00, 0x00,  0x00,  0x38, 0x06,  0x05, 0x01,  0x00,  0x00, 0x00, },

	{ 2, 0x00, 0xd0, },		//panel CLKb3/4 setting
	{ 15, 0xce, 0x38,  0x05,  0x05, 0x02,  0x00, 0x00,  0x00,  0x38, 0x04,  0x05, 0x03,  0x00,  0x00, 0x00, },

	{ 2, 0x00, 0x80, },		//panel CLKc1/2 setting
	{ 15, 0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0x90, },		//panel CLKc3/4 setting
	{ 15, 0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xa0, },		//panel CLKd1/2 setting
	{ 15, 0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xb0, },		//panel CLKd3/4 setting
	{ 15, 0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },

	{ 2, 0x00, 0xc0, },		//panel ECLK setting
	{ 12, 0xcf, 0x01, 0x01, 0x20, 0x20, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x08, },

	{ 2, 0x00, 0xb5, },             //TCON_GOA_OUT Setting
	{ 7, 0xc5, 0x33, 0xf1, 0xff, 0x33, 0xf1, 0xff, },  //normal output with VGH/VGL

	{ 2, 0x00, 0xa0, },
	{ 2, 0xc1, 0x02, },

	{ 2, 0x00, 0xb1, },
	{ 2, 0xc6, 0x04, },

	{ 2, 0x00, 0x00, },             //Orise mode disable
	/* this might not go through the SSD2858! */
	{ 3, 0xff, 0xff, 0xff, 0xff, },

#if 0
	{ 2, DCS_CTRL_DISPLAY, 0x24, },	// LEDPWM ON
	{ 2, DCS_WRITE_CABC, 0x00, },	// CABC off
#endif
};

static w677l_reg sleep_out[] = {
//	{ 1, MIPI_DCS_SET_DISPLAY_ON, },
	{ 1, MIPI_DCS_EXIT_SLEEP_MODE, },
};

static w677l_reg display_on[] = {
	{ 1, MIPI_DCS_SET_DISPLAY_ON, },
//	{ 1, MIPI_DCS_EXIT_SLEEP_MODE, },
};

static int w677l_write(struct otm1283a *ctx, u8 *buf, int len)
{
	int r;
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);

#if LOG
	int i;
	static char logbuf[256];
	sprintf(logbuf, "%s", IS_MCS(buf[0], len) ? "gen" : "dcs");
	for (i = 0; i < len; i++)
		sprintf(logbuf + strlen(logbuf), " %02x", buf[i]);
	printk("dsi: %s(%s) [%d]", __func__, logbuf, dsi->channel);
#endif

	if (IS_MCS(buf[0], len))
		{
		/* this is a "manufacturer command" that must be sent as a "generic write command" */
		r = mipi_dsi_generic_write(dsi, buf, len);
		}
	else
		{ /* this is a "user command" that must be sent as "DCS command" */
		r = mipi_dsi_dcs_write_buffer(dsi, buf, len);
		}

	if (r)
		dev_err(ctx->dev, "write cmd/reg(%x) failed: %d\n",
				buf[0], r);

	return r;
}

static int w677l_read(struct otm1283a *ctx, u8 dcs_cmd, u8 *buf, int len)
{
	int r;
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);

	r = mipi_dsi_set_maximum_return_packet_size(dsi, len);

	if (r) {
		dev_err(ctx->dev, "can't set max rx packet size\n");
		return -EIO;
	}

	if (IS_MCS(dcs_cmd, len))
		{ /* this is a "manufacturer command" that must be sent as a "generic read command" */
		r = mipi_dsi_generic_read(dsi, NULL, 0, buf, len);
		}
	else
		{ /* this is a "user command" that must be sent as "DCS command" */
		r = mipi_dsi_dcs_read(dsi, dcs_cmd, buf, len);
		}

	if (r)
		dev_err(ctx->dev, "read cmd/reg(%02x, %d) failed: %d\n",
				dcs_cmd, len, r);

#if LOG
	{
	int i;
	static char logbuf[256];

	sprintf(logbuf, "%02x:", dcs_cmd);
	for (i = 0; i < len; i++)
		sprintf(logbuf + strlen(logbuf), " %02x", buf[i]);
	printk("dsi: %s(%s) [%d] -> %d\n", __func__, logbuf, dsi->channel, r);
	}
#endif

	return r;
}

static int w677l_write_sequence(struct otm1283a *ctx,
		w677l_reg *seq, int len)
{
	int r, i;

	for (i = 0; i < len; i++) {
		r = w677l_write(ctx, &seq[i][1], seq[i][0]);
		if (r) {
			dev_err(ctx->dev, "sequence failed: %d\n", i);
			return -EINVAL;
		}
	}

	return 0;
}

static int w677l_reset(struct otm1283a *ctx, int activate)
{
	dev_dbg(ctx->dev, "%s(%s)\n", __func__, activate?"active":"inactive");

	gpiod_set_value(ctx->reset_gpio, !activate);
	return 0;
}

static int w677l_regulator(struct otm1283a *ctx, int state)
{
	dev_dbg(ctx->dev, "%s(%s)\n", __func__, state?"on":"off");

	gpiod_set_value(ctx->regulator_gpio, state);	/* switch regulator */
	return 0;
}

static int w677l_update_brightness(struct otm1283a *ctx, int level)
{
	int r;
#if 1
	u8 buf[2];
	buf[0] = MIPI_DCS_SET_DISPLAY_BRIGHTNESS;
	buf[1] = level;
#else
	u8 buf[3];
	buf[0] = MIPI_DCS_SET_DISPLAY_BRIGHTNESS;
	buf[1] = level >> 4;	/* 12 bit mode */
	buf[2] = buf[1] + ((level & 0x0f) << 4);
#endif

	dev_dbg(ctx->dev, "%s(%d)\n", __func__, level);

	r = w677l_write(ctx, buf, sizeof(buf));
	if (r)
		return r;
	return 0;
}

static int w677l_init_sequence(struct otm1283a *ctx)
{
	int r;
#if 0
	r = w677l_write_sequence(ctx, nop, ARRAY_SIZE(nop));
	if (r)
		return r;
#endif
	r = w677l_write_sequence(ctx, sleep_out, ARRAY_SIZE(sleep_out));
	if (r)
		return r;

	msleep(10);

	r = w677l_write_sequence(ctx, init_seq, ARRAY_SIZE(init_seq));
	if (r) {
		dev_err(ctx->dev, "failed to configure panel\n");
		return r;
	}

	r = w677l_update_brightness(ctx, 255);
	if (r)
		return r;

#if 1	/* this is recommended by the latest data sheet */
	r = w677l_write_sequence(ctx, display_on, ARRAY_SIZE(display_on));
	if (r)
		return r;
#endif

	return 0;
}

static void w677l_query_registers(struct otm1283a *ctx)
{ /* read back some registers through DCS commands */
	u8 ret[8];
	int r;

	r = w677l_read(ctx, 0x05, ret, 1);
	printk("%s: [RDNUMED] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, 0x0a, ret, 1);  // power mode 0x10=sleep off; 0x04=display on
	printk("%s: [RDDPM] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, 0x0b, ret, 1);  // address mode
	printk("%s: [RDDMADCTL] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, MIPI_DCS_GET_PIXEL_FORMAT, ret, 1);     // pixel format 0x70 = RGB888
	printk("%s: [PIXFMT] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, 0x0d, ret, 1);  // display mode 0x80 = command 0x34/0x35
	printk("%s: [RDDIM] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, 0x0e, ret, 1);  // signal mode
	printk("%s: [RDDIM] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, MIPI_DCS_GET_DIAGNOSTIC_RESULT, ret, 1);        // diagnostic 0x40 = functional
	printk("%s: [RDDSDR] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, 0x45, ret, 2);  // get scanline
	printk("%s: [RDSCNL] = %02x%02x\n", __func__, ret[0], ret[1]);
	r = w677l_read(ctx, MCS_READID1, ret, 1);
	printk("%s: [RDID1] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, MCS_READID2, ret, 1);
	printk("%s: [RDID2] = %02x\n", __func__, ret[0]);
	r = w677l_read(ctx, MCS_READID3, ret, 1);
	printk("%s: [RDID3] = %02x\n", __func__, ret[0]);
}

static int w677l_disable(struct drm_panel *panel)
{
	struct otm1283a *ctx = panel_to_otm1283a(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;

	dev_dbg(ctx->dev, "%s\n", __func__);

	if (!ctx->enabled)
		return 0; /* This is not an issue so we return 0 here */

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret)
		return ret;

	msleep(120);

	ctx->enabled = false;

	return 0;
}

static int w677l_unprepare(struct drm_panel *panel)
{
	struct otm1283a *ctx = panel_to_otm1283a(panel);

	dev_dbg(ctx->dev, "%s\n", __func__);

	if (!ctx->prepared)
		return 0;

	dev_dbg(ctx->dev, "power_off()\n");

	mdelay(10);
	w677l_reset(ctx, true);	/* activate reset */
	mdelay(10);
	w677l_regulator(ctx, false);	/* switch power off */
	mdelay(20);
	/* here we could also power off IOVCC */

	dev_dbg(ctx->dev, "%s finished\n", __func__);

	ctx->prepared = false;

	return 0;
}

static int w677l_prepare(struct drm_panel *panel)
{
	struct otm1283a *ctx = panel_to_otm1283a(panel);
	int r;

	dev_dbg(ctx->dev, "%s\n", __func__);

	if (ctx->prepared)
		return 0;

//	dev_dbg(ctx->dev, "hs_clk_min=%lu\n", w677l_dsi_config.hs_clk_min);
	dev_dbg(ctx->dev, "power_on()\n");

	w677l_reset(ctx, true);	/* activate reset */

	w677l_regulator(ctx, true);	/* switch power on */
	msleep(50);

	w677l_reset(ctx, false);	/* release reset */
	msleep(10);

	if (r)
		dev_err(ctx->dev, "%s failed\n", __func__);

#if 0	/* should be here but prepare is not called for omapdrm */
	r = w677l_init_sequence(ctx);
	if (r)
		return r;
#endif

	ctx->prepared = true;

	return r;
}

static int w677l_enable(struct drm_panel *panel)
{
	struct otm1283a *ctx = panel_to_otm1283a(panel);

	dev_dbg(ctx->dev, "%s\n", __func__);

	if (ctx->enabled)
		return 0;

#if 1
	{
	int ret;
	ret = w677l_init_sequence(ctx);
	if (ret)
		return ret;
	}
#endif

	w677l_query_registers(ctx);

	dev_dbg(ctx->dev, "%s() powered on()\n", __func__);

	backlight_enable(ctx->bl_dev);

	ctx->enabled = true;

	return 0;
}

static int w677l_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct otm1283a *ctx = panel_to_otm1283a(panel);

	dev_dbg(panel->dev, "%s\n", __func__);

	mode = drm_mode_duplicate(connector->dev, &default_mode);
	if (!mode) {
		dev_err(panel->dev, "failed to add mode %ux%u@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;	// REVISIT: do we need this?
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_connector_set_panel_orientation(connector, ctx->orientation);

	dev_dbg(panel->dev, "%s done\n", __func__);

	return 1;
}

static const struct drm_panel_funcs w677l_panel_funcs = {
	.disable = w677l_disable,
	.unprepare = w677l_unprepare,
	.prepare = w677l_prepare,
	.enable = w677l_enable,
	.get_modes = w677l_get_modes,
};

static int w677l_set_brightness(struct backlight_device *bd)
{
	struct otm1283a *ctx = bl_get_data(bd);
	int bl = bd->props.brightness;
	int r = 0;

	dev_dbg(ctx->dev, "%s (%d)\n", __func__, bl);

	r = w677l_update_brightness(ctx, bl);

	return r;
}

static int w677l_get_brightness(struct backlight_device *bd)
{
	struct otm1283a *ctx = bl_get_data(bd);
	u8 data[16];
	u16 brightness = 0;
	int r = 0;

	dev_dbg(ctx->dev, "%s\n", __func__);

	if (ctx->enabled) {
		r = w677l_read(ctx, DCS_READ_BRIGHTNESS, data, 2);
		brightness = (data[0]<<4) + (data[1]>>4);
	}

	if (r < 0) {
		dev_err(ctx->dev, "get_brightness: read error\n");
		return bd->props.brightness;
	}

	dev_dbg(ctx->dev, "get_brightness -> %d\n", brightness);

	return brightness>>4;	/* get into range 0..255 */
}

static const struct backlight_ops w677l_bl_dev_ops  = {
	.get_brightness = w677l_get_brightness,
	.update_status = w677l_set_brightness,
};

#if OPTIONAL
// dsicm_attr_group
#endif

static int w677l_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct otm1283a *ctx;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio)) {
		ret = PTR_ERR(ctx->reset_gpio);
		dev_err(&dsi->dev, "reset gpio request failed: %d", ret);
		return ret;
	}

	ctx->regulator_gpio = devm_gpiod_get_optional(&dsi->dev, "regulator", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->regulator_gpio)) {
		ret = PTR_ERR(ctx->regulator_gpio);
		dev_err(&dsi->dev, "regulator gpio request failed: %d", ret);
		return ret;
	}

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_LPM;

	dsi->hs_rate = W677L_HS_CLOCK;
	dsi->hs_rate = 105 * (W677L_HS_CLOCK / 100);	/* allow for 5% overclocking */
	dsi->lp_rate = W677L_LP_CLOCK;

	of_drm_get_panel_orientation(dev->of_node, &ctx->orientation);

	drm_panel_init(&ctx->panel, dev, &w677l_panel_funcs, DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&ctx->panel);
printk("%s: drm_panel_of_backlight -> r=%d bl=%px", __func__, ret, ctx->panel.backlight);
	if (ret)
		return ret;

	if (!ctx->panel.backlight) {
		/* drm_panel can use a backlight phandle */
		/* register as backlight device */
		ctx->bl_dev = devm_backlight_device_register(dev, dev_name(dev),
						     dsi->host->dev, ctx,
						     &w677l_bl_dev_ops,
						     NULL);
		if (IS_ERR(ctx->bl_dev)) {
			ret = PTR_ERR(ctx->bl_dev);
			dev_err(dev, "failed to register backlight: %d\n", ret);
			return ret;
		}

		ctx->bl_dev->props.max_brightness = 255;
		ctx->bl_dev->props.brightness = 200;
		ctx->bl_dev->props.power = FB_BLANK_POWERDOWN;
		ctx->bl_dev->props.type = BACKLIGHT_RAW;
	}

#if OPTIONAL	/* register some additional tools */
	ret = sysfs_create_group(&dev->kobj, &dsicm_attr_group);
	if (ret) {
		dev_err(dev, "failed to create sysfs files\n");
		return ret;
	}
#endif

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		goto err_dsi_attach;

	dev_dbg(dev, "%s ok\n", __func__);

	return 0;

err_dsi_attach:
	drm_panel_remove(&ctx->panel);
#if OPTIONAL
	sysfs_remove_group(&dsi->dev.kobj, &dsicm_attr_group);
#endif
	dev_dbg(dev, "%s nok\n", __func__);
	return ret;
}

static int w677l_remove(struct mipi_dsi_device *dsi)
{
	struct otm1283a *ctx = mipi_dsi_get_drvdata(dsi);

	dev_dbg(&dsi->dev, "%s\n", __func__);

	mipi_dsi_detach(dsi);

	drm_panel_remove(&ctx->panel);
#if OPTIONAL
	sysfs_remove_group(&dsi->dev.kobj, &dsicm_attr_group);
#endif

	w677l_reset(ctx, true);	/* activate reset */

	return 0;
}

static const struct of_device_id w677l_of_match[] = {
	{ .compatible = "boe,btl507212-w677l", },
	{},
};

MODULE_DEVICE_TABLE(of, w677l_of_match);

static struct mipi_dsi_driver w677l_driver = {
	.probe = w677l_probe,
	.remove = w677l_remove,
	.driver = {
		.name = "panel-btl507212-w677l",
		.of_match_table = w677l_of_match,
		.suppress_bind_attrs = true,
	},
};

module_mipi_dsi_driver(w677l_driver);

MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("btl507212-w677l driver");
MODULE_LICENSE("GPL");
