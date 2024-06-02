/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_I2C_TSC2007_H
#define __LINUX_I2C_TSC2007_H

/* linux/platform_data/tsc2007.h */

struct tsc2007_platform_data {
	u16	model;				/* 2007. */
	u16	x_plate_ohms;	/* must be non-zero value */
	bool	swap_xy;	/* swap x and y axis */
	bool	invert_x;
	bool	invert_y;
	u16	min_x;	/* min and max values reported by ADC */
	u16	min_y;
	u16	max_x;
	u16	max_y;
	u16	max_rt; /* max. resistance above which samples are ignored */
	unsigned long poll_period; /* time (in ms) between samples */
	int	fuzzx; /* fuzz factor for X, Y and pressure axes */
	int	fuzzy;
	int	fuzzz;

	int	(*get_pendown_state)(struct device *);
	/* If needed, clear 2nd level interrupt source */
	void	(*clear_penirq)(void);
	int	(*init_platform_hw)(void);
	void	(*exit_platform_hw)(void);
};

#endif
