/*
 * Copyright (C) 2012 Texas Instruments, Inc
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

#ifndef SGXFREQ_H
#define SGXFREQ_H

#include <linux/version.h>
#include <linux/device.h>
#include <linux/time.h>

#define SGXFREQ_NAME_LEN 16

//#define SGXFREQ_DEBUG_FTRACE
#if defined(SGXFREQ_DEBUG_FTRACE)
#define SGXFREQ_TRACE(...) trace_printk(__VA_ARGS__)
#else
#define SGXFREQ_TRACE(...)
#endif

struct sgxfreq_sgx_data {
	bool clk_on;
	bool active;
};

struct sgxfreq_governor {
	char name[SGXFREQ_NAME_LEN];
	int (*gov_start) (struct sgxfreq_sgx_data *data);
	void (*gov_stop) (void);
	void (*sgx_clk_on) (void);
	void (*sgx_clk_off) (void);
	void (*sgx_active) (void);
	void (*sgx_idle) (void);
	void (*sgx_frame_done) (void);
	struct list_head governor_list;
};

/* sgxfreq_init must be called before any other api */
int sgxfreq_init(struct device *dev);
int sgxfreq_deinit(void);

int sgxfreq_register_governor(struct sgxfreq_governor *governor);
void sgxfreq_unregister_governor(struct sgxfreq_governor *governor);

int sgxfreq_set_governor(const char *name);

int sgxfreq_get_freq_list(unsigned long **pfreq_list);

unsigned long sgxfreq_get_freq_min(void);
unsigned long sgxfreq_get_freq_max(void);

unsigned long sgxfreq_get_freq_floor(unsigned long freq);
unsigned long sgxfreq_get_freq_ceil(unsigned long freq);

unsigned long sgxfreq_get_freq(void);
unsigned long sgxfreq_get_freq_request(void);
unsigned long sgxfreq_get_freq_limit(void);

unsigned long sgxfreq_set_freq_request(unsigned long freq_request);
unsigned long sgxfreq_set_freq_limit(unsigned long freq_limit);

unsigned long sgxfreq_get_total_active_time(void);
unsigned long sgxfreq_get_total_idle_time(void);

/* Helper functions */
static inline unsigned long __tv2msec(struct timeval tv)
{
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

static inline unsigned long __delta32(unsigned long a, unsigned long b)
{
	if (a >= b)
		return a - b;
	else
		return 1 + (0xFFFFFFFF - b) + a;
}

/* External notifications to sgxfreq */
void sgxfreq_notif_sgx_clk_on(void);
void sgxfreq_notif_sgx_clk_off(void);
void sgxfreq_notif_sgx_active(void);
void sgxfreq_notif_sgx_idle(void);
void sgxfreq_notif_sgx_frame_done(void);

#endif
