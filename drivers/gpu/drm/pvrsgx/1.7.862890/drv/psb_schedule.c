/**************************************************************************
 * Copyright (c) 2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics.com>
 **************************************************************************/

#include <drm/drmP.h>
#include "psb_drm.h"
#include "psb_drv.h"
#include "psb_reg.h"
#include "ttm/ttm_execbuf_util.h"


static void psb_powerdown_msvdx(struct work_struct *work)
{
	struct psb_scheduler *scheduler =
	    container_of(work, struct psb_scheduler, msvdx_suspend_wq.work);

	if (!mutex_trylock(&scheduler->msvdx_power_mutex))
		return;

	psb_try_power_down_msvdx(scheduler->dev);
	mutex_unlock(&scheduler->msvdx_power_mutex);
}

int psb_scheduler_init(struct drm_device *dev,
		       struct psb_scheduler *scheduler)
{
	memset(scheduler, 0, sizeof(*scheduler));
	scheduler->dev = dev;
	mutex_init(&scheduler->msvdx_power_mutex);

	INIT_DELAYED_WORK(&scheduler->msvdx_suspend_wq,
			  &psb_powerdown_msvdx);

	return 0;
}

