/*
 * SGX utility functions
 *
 * Copyright (C) 2010 Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __SGX_UTIL_H
#define __SGX_UTIL_H

#include <linux/kernel.h>

#define OMAPLFB_PAGE_SIZE 4096

/* Greatest common divisor */
static unsigned long gcd(unsigned long a, unsigned long b)
{
	unsigned long r;

	if (a < b) {
		r = a;
		a = b;
		b = r;
	}

	while ((r = a % b) != 0) {
		a = b;
		b = r;
	}

	return b;
}

/*
 * Workout the smallest size that is aligned to both 4K (for the SGX)
 * and line length (for the fbdev driver).
 */
static unsigned int sgx_buffer_align(unsigned stride, unsigned size)
{
	unsigned lcm;

	if (!stride || !size)
		return 0;

	lcm = stride * OMAPLFB_PAGE_SIZE / gcd(stride,
					       OMAPLFB_PAGE_SIZE);

	return roundup(size, lcm);
}

#endif /* __SGX_UTIL_H */
