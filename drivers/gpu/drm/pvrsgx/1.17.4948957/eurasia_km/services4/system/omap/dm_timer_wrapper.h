/* SPDX-License-Identifier: (GPL-2.0 OR MIT) */
#ifndef DM_TIMER_WRAPPER
#define DM_TIMER_WRAPPER

/* helpers for SGX_DEBUG=y and OMAP timers used by omap/sysutils_linux. */
#if defined(CONFIG_SGX_DEBUG) && (LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0))

#include <clocksource/timer-ti-dm.h>

#if 0	// unused
static struct omap_dm_timer *omap_dm_timer_request_specific(int timer_id)
{
	printk("%s\n", __func__);

	return omap_dm_timer_request_by_cap(0);	// no special capability like OMAP_TIMER_ALWON
}

static int omap_dm_timer_set_source(struct omap_dm_timer *timer, int source)
{
	printk("%s\n", __func__);
	return 0;
}

static int omap_dm_timer_set_load_start(struct omap_dm_timer *timer, int autoreload, unsigned int value)
{
	printk("%s\n", __func__);
	return 0;
}

static int omap_dm_timer_start(struct omap_dm_timer *timer)
{
	printk("%s\n", __func__);
	return 0;
}

static void omap_dm_timer_enable(struct omap_dm_timer *timer)
{
	printk("%s\n", __func__);
}

#endif

static void omap_dm_timer_disable(struct omap_dm_timer *timer)
{
	printk("%s\n", __func__);
}

static int omap_dm_timer_stop(struct omap_dm_timer *timer)
{
	printk("%s\n", __func__);
	return -EINVAL;
}

static int omap_dm_timer_free(struct omap_dm_timer *timer)
{
	printk("%s\n", __func__);
	return -EINVAL;
}

/* no idea why we need this but otherwise we get a link error */
void __bad_xchg(volatile void *ptr, int size)
{
    printk(KERN_ERR "%s: ptr %p size %u\n",
        __FUNCTION__, ptr, size);
    BUG();
}

#endif

#endif	/* DM_TIMER_WRAPPER */