/*
 * si47xx.h  --  Si47xx ASoC codec driver
 *
 * Copyright 2008 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * Adapted: 2011, Nikolaus Schaller <hns@goldelico.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SI47XX_H
#define _SI47XX_H

#define SI47XX_DACLVOL   0x00
#define SI47XX_DACRVOL   0x01
#define SI47XX_DACCTL    0x02
#define SI47XX_IFCTL     0x03

struct si47xx_setup_data {
	int            spi;
	int            i2c_bus;
	unsigned short i2c_address;
};

extern struct snd_soc_dai si47xx_dai;
extern struct snd_soc_codec_device soc_codec_dev_si47xx;

#endif
