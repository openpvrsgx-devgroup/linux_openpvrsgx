// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic JZ4730 I2C bus driver
 *
 * Copyright (C) 2006 - 2009 Ingenic Semiconductor Inc.
 * Copyright (C) 2015 Imagination Technologies
 * Copyright (C) 2017 Paul Boddie <paul@boddie.org.uk>
 * Copyright (C) 2020 - 2021 H. Nikolaus Schaller <hns@goldelico.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/time.h>

#define JZ4730_REG_I2C_DR	0x00
#define JZ4730_REG_I2C_CR	0x04
#define JZ4730_REG_I2C_SR	0x08
#define JZ4730_REG_I2C_GR	0x0C

#define JZ4730_I2C_CR_IEN	BIT(4)
#define JZ4730_I2C_CR_STA	BIT(3)
#define JZ4730_I2C_CR_STO	BIT(2)
#define JZ4730_I2C_CR_AC	BIT(1)
#define JZ4730_I2C_CR_I2CE	BIT(0)

#define JZ4730_I2C_SR_STX	BIT(4)
#define JZ4730_I2C_SR_BUSY	BIT(3)
#define JZ4730_I2C_SR_TEND	BIT(2)
#define JZ4730_I2C_SR_DRF	BIT(1)
#define JZ4730_I2C_SR_ACKF	BIT(0)

enum {
	STATE_IDLE,
	STATE_SEND_ADDR,
	STATE_SEND_ADDR10,
	STATE_DATA,
	STATE_DONE,
	STATE_WAIT_STO
};

struct jz4730_i2c {
	void __iomem		*iomem;
	struct clk		*clk;
	struct i2c_adapter	adap;
	int			speed;
};

static inline unsigned char jz4730_i2c_readb(struct jz4730_i2c *i2c,
					      unsigned long offset)
{
	return readb(i2c->iomem + offset);
}

static inline void jz4730_i2c_writeb(struct jz4730_i2c *i2c,
				     unsigned long offset, unsigned char val)
{
	writeb(val, i2c->iomem + offset);
}

static inline void jz4730_i2c_writew(struct jz4730_i2c *i2c,
				     unsigned long offset, unsigned short val)
{
	writew(val, i2c->iomem + offset);
}

static void jz4730_i2c_updateb(struct jz4730_i2c *i2c,
				      unsigned long offset,
				      unsigned char affected,
				      unsigned char val)
{
	writeb((readb(i2c->iomem + offset) & ~affected) | val,
		i2c->iomem + offset);
}

static int jz4730_i2c_set_speed(struct jz4730_i2c *i2c)
{
	int dev_clk_khz = clk_get_rate(i2c->clk) / 1000;
	int i2c_clk = i2c->speed;

	/* Set the I2C clock divider. */

	/* REVISIT: check for overflow? We only have 16 bit divider resolution */
	jz4730_i2c_writew(i2c, JZ4730_REG_I2C_GR, dev_clk_khz / (16 * i2c_clk) - 1);

	return 0;
}

/* compiler should be able to optimize away unneeded checks */
static /*inline*/ int wait_done(struct jz4730_i2c *i2c, struct i2c_msg *msg, bool drf, bool tend)
{
	bool ackf = (msg && !(msg->flags & I2C_M_IGNORE_NAK));
	u8 status;
	int ret;

	ret = readb_poll_timeout_atomic(i2c->iomem + JZ4730_REG_I2C_SR, status,
				((ackf && (status & JZ4730_I2C_SR_ACKF)) ||
				 (tend && (status & JZ4730_I2C_SR_TEND)) ||
				 (drf == !!(status & JZ4730_I2C_SR_DRF))
				),
				1, 10000);
	if (ackf && ret >= 0 && (status & JZ4730_I2C_SR_ACKF))
		return -EIO;
	if (ret < 0)
		dev_warn(&i2c->adap.dev, "%s drf=%d tend=%d status=0x%02x timed out\n", __func__, drf, tend, status);
	return ret;
}

static int jz4730_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msg,
			   int count)
{
	struct jz4730_i2c *i2c = adap->algo_data;
	int i;
	int ret;

	ret = 0;

	for (i = 0; i < count; i++, msg++) {
		u8 addr;
		unsigned int pos;

		jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_CR, JZ4730_I2C_CR_AC, 0);

		if (!(msg->flags & I2C_M_NOSTART)) {
			jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_CR,
				JZ4730_I2C_CR_STA, JZ4730_I2C_CR_STA);

			if (msg->flags & I2C_M_TEN) {
				/* 11110aad where aa = address[9:8] and d = direction */
				addr = ((msg->addr >> 7) & 0x06) | 0xf0;
			} else {
				/* aaaaaaad where aa = address[6:0] and d = direction */
				addr = msg->addr << 1;
			}

			if (msg->flags & I2C_M_REV_DIR_ADDR)
				addr |= (msg->flags & I2C_M_RD ? 0 : BIT(0));
			else
				addr |= (msg->flags & I2C_M_RD ? BIT(0) : 0);

			jz4730_i2c_writeb(i2c, JZ4730_REG_I2C_DR, addr);
			jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_SR, JZ4730_I2C_SR_DRF,
					  JZ4730_I2C_SR_DRF);

			/* wait for DRF=0 */
			ret = wait_done(i2c, msg, false, false);
			if (ret < 0)
				break;

			if (msg->flags & I2C_M_TEN) {
				/* aaaaaaaa where aa = address[7:0] */

				jz4730_i2c_writeb(i2c, JZ4730_REG_I2C_DR, msg->addr & 0xff);
				jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_SR, JZ4730_I2C_SR_DRF,
						 JZ4730_I2C_SR_DRF);

				/* wait for DRF=0 */
				ret = wait_done(i2c, msg, false, false);
				if (ret < 0)
					break;
			}
		}

		pos = 0;

		if (msg->flags & I2C_M_RD) {
			while (pos < msg->len) {
				if (pos == 0) {
					/* wait for TEND=1 (while DRF=0) */
					ret = wait_done(i2c, msg, true, true);
					if (ret < 0)
						break;

					/* acknowledge received bytes */
					jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_CR,
							JZ4730_I2C_CR_AC, 0);
				}

				/* wait for DRF=1 */
				ret = wait_done(i2c, msg, true, false);
				if (ret < 0)
					break;

				/*
				 * Assert non-acknowledgement condition before final byte.
				 * to indicate that we do not expect more bytes from client
				 */
				if (pos + 1 >= msg->len)
					jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_CR,
							JZ4730_I2C_CR_AC, JZ4730_I2C_CR_AC);

				msg->buf[pos++] = jz4730_i2c_readb(i2c, JZ4730_REG_I2C_DR);
				jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_SR, JZ4730_I2C_SR_DRF, 0);
			}

			if (ret >= 0) {
				/* extra read for NACK of more bytes */
			//	jz4730_i2c_readb(i2c, JZ4730_REG_I2C_DR);
			//	jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_SR, JZ4730_I2C_SR_DRF, 0);

				/* wait for DRF=1 */
				// ignore ret here?
				ret = wait_done(i2c, msg, true, false);

				jz4730_i2c_readb(i2c, JZ4730_REG_I2C_DR);
				jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_SR, JZ4730_I2C_SR_DRF, 0);
			}

		} else {
			while (pos < msg->len) {
				jz4730_i2c_writeb(i2c, JZ4730_REG_I2C_DR, msg->buf[pos++]);
				jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_SR, JZ4730_I2C_SR_DRF,
						 JZ4730_I2C_SR_DRF);

				/* wait for DRF=0 */
				ret = wait_done(i2c, msg, false, false);
				if (ret < 0)
					break;
			}
			if (ret >= 0 && i + 1 < count) {
				/* wait for TEND=1 (while DRF=0) */
				ret = wait_done(i2c, msg, true, true);
				if (ret < 0)
					break;
			}
		}
		if (ret < 0)
			break;	// some error

		if (i + 1 >= count || (msg->flags & I2C_M_STOP)) {
			/* last byte sent or received */
			jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_CR,
				JZ4730_I2C_CR_STO, JZ4730_I2C_CR_STO);

// CHECKME: can this request STO twice?

			/* wait for TEND=1 (while DRF=0) */
			ret = wait_done(i2c, msg, true, true);
			if (ret < 0)
				break;
		}
	}

	if (ret < 0) {
		jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_CR,
			JZ4730_I2C_CR_STO, JZ4730_I2C_CR_STO);	// be sure to stop
		jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_SR, JZ4730_I2C_SR_DRF, 0);

		/* CHECKME: should we wait here for BUSY = 0 - the next STA may fail or be ignored? */
		dev_warn(&i2c->adap.dev, "%s %d/%d error ret=%d status=0x%02x\n", __func__, i, count, ret, jz4730_i2c_readb(i2c, JZ4730_REG_I2C_SR));
	} else {
		ret = count;	/* all sent */
	}

	return ret;
}

static u32 jz4730_i2c_functionality(struct i2c_adapter *adap)
{
	/* see https://www.kernel.org/doc/Documentation/i2c/i2c-protocol */
	/* REVIST: NOSTART and PROTOCOL_MANGLING is untested */
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_10BIT_ADDR /* | I2C_FUNC_NOSTART | I2C_FUNC_PROTOCOL_MANGLING */;
}

static const struct i2c_algorithm jz4730_i2c_algorithm = {
	.master_xfer	= jz4730_i2c_xfer,
	.functionality	= jz4730_i2c_functionality,
};

static const struct of_device_id jz4730_i2c_of_matches[] = {
	{ .compatible = "ingenic,jz4730-i2c", },
	/* JZ4740 has the same I2C controller so we do not need to differentiate */
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, jz4730_i2c_of_matches);

static int jz4730_i2c_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int clk_freq;
	struct resource *r;
	struct jz4730_i2c *i2c;

	i2c = devm_kzalloc(&pdev->dev, sizeof(struct jz4730_i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	i2c->adap.owner		= THIS_MODULE;
	i2c->adap.algo		= &jz4730_i2c_algorithm;
	i2c->adap.algo_data	= i2c;
	i2c->adap.retries	= 5;
	i2c->adap.dev.parent	= &pdev->dev;
	i2c->adap.dev.of_node	= pdev->dev.of_node;
	sprintf(i2c->adap.name, "%s", pdev->name);

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2c->iomem = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(i2c->iomem))
		return PTR_ERR(i2c->iomem);

	platform_set_drvdata(pdev, i2c);

	i2c->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(i2c->clk))
		return PTR_ERR(i2c->clk);

	ret = clk_prepare_enable(i2c->clk);
	if (ret)
		return ret;

	ret = of_property_read_u32(pdev->dev.of_node, "clock-frequency",
				   &clk_freq);
	if (ret) {
		dev_err(&pdev->dev, "clock-frequency not specified in DT\n");
		goto err;
	}

	i2c->speed = clk_freq / 1000;
	if (i2c->speed == 0) {
		ret = -EINVAL;
		dev_err(&pdev->dev, "clock-frequency minimum is 1000\n");
		goto err;
	}

	dev_info(&pdev->dev, "Bus frequency is %d kHz\n", i2c->speed);

	/* Configure and enable the peripheral. */

	jz4730_i2c_set_speed(i2c);

	ret = i2c_add_adapter(&i2c->adap);
	if (ret < 0)
		goto err;

	jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_CR, JZ4730_I2C_CR_IEN | JZ4730_I2C_CR_I2CE,
			    JZ4730_I2C_CR_IEN | JZ4730_I2C_CR_I2CE);

	return 0;

err:
	clk_disable_unprepare(i2c->clk);
	return ret;
}

static int jz4730_i2c_remove(struct platform_device *pdev)
{
	struct jz4730_i2c *i2c = platform_get_drvdata(pdev);

	jz4730_i2c_updateb(i2c, JZ4730_REG_I2C_CR, JZ4730_I2C_CR_IEN | JZ4730_I2C_CR_I2CE, 0);
	clk_disable_unprepare(i2c->clk);
	i2c_del_adapter(&i2c->adap);
	return 0;
}

static struct platform_driver jz4730_i2c_driver = {
	.probe		= jz4730_i2c_probe,
	.remove		= jz4730_i2c_remove,
	.driver		= {
		.name	= "jz4730-i2c",
		.of_match_table = jz4730_i2c_of_matches,
	},
};

module_platform_driver(jz4730_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paul Boddie <paul@boddie.org.uk>");
MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("I2C driver for JZ4730 SoC");
