/*
	vsense.h

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.
*/

#ifndef _VSENSE_H_
#define _VSENSE_H_

struct vsense_platform_data {
	int gpio_irq;
	int gpio_reset;
};

#endif
