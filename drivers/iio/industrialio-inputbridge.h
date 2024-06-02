/* SPDX-License-Identifier: GPL-2.0 */
/*
 * The Industrial I/O core, bridge to input devices
 *
 * Copyright (c) 2016-2019 Golden Delicious Computers GmbH&Co. KG
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#if defined(CONFIG_IIO_INPUT_BRIDGE)

extern int iio_device_register_inputbridge(struct iio_dev *indio_dev);
extern void iio_device_unregister_inputbridge(struct iio_dev *indio_dev);

#else

static inline int iio_device_register_inputbridge(struct iio_dev *indio_dev)
{
	return 0;
}

static inline void iio_device_unregister_inputbridge(struct iio_dev *indio_dev)
{
}

#endif
