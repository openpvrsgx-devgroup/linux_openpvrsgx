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
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 **************************************************************************/

#include <drm/drmP.h>
#include "psb_drv.h"
#include "psb_reg.h"
#include "psb_intel_reg.h"
#include "psb_msvdx.h"
#include <linux/spinlock.h>


void psb_schedule_watchdog(struct drm_psb_private *dev_priv)
{
	struct timer_list *wt = &dev_priv->watchdog_timer;
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	if (dev_priv->timer_available && !timer_pending(wt)) {
		wt->expires = jiffies + PSB_WATCHDOG_DELAY;
		add_timer(wt);
	}
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);
}


static void psb_watchdog_func(unsigned long data)
{
	struct drm_psb_private *dev_priv = (struct drm_psb_private *) data;
	int msvdx_lockup;
	int msvdx_idle;
	unsigned long irq_flags;

	psb_msvdx_lockup(dev_priv, &msvdx_lockup, &msvdx_idle);

	if (msvdx_lockup) {
		spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
		dev_priv->timer_available = 0;
		spin_unlock_irqrestore(&dev_priv->watchdog_lock,
				       irq_flags);
		if (msvdx_lockup)
			schedule_work(&dev_priv->msvdx_watchdog_wq);
	}
	if (!msvdx_idle)
		psb_schedule_watchdog(dev_priv);
}

void psb_msvdx_flush_cmd_queue(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_msvdx_cmd_queue *msvdx_cmd;
	struct list_head *list, *next;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

	/*Flush the msvdx cmd queue and signal all fences in the queue */
	list_for_each_safe(list, next, &msvdx_priv->msvdx_queue) {
		msvdx_cmd =
		    list_entry(list, struct psb_msvdx_cmd_queue, head);
		PSB_DEBUG_GENERAL("MSVDXQUE: flushing sequence:%d\n",
				  msvdx_cmd->sequence);
		msvdx_priv->msvdx_current_sequence = msvdx_cmd->sequence;
		psb_fence_error(dev, PSB_ENGINE_VIDEO,
				msvdx_priv->msvdx_current_sequence,
				_PSB_FENCE_TYPE_EXE, DRM_CMD_HANG);
		list_del(list);
		kfree(msvdx_cmd->cmd);
		kfree(msvdx_cmd
			 );
	}
}

static void psb_msvdx_reset_wq(struct work_struct *work)
{
	struct drm_psb_private *dev_priv =
	    container_of(work, struct drm_psb_private, msvdx_watchdog_wq);
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

	struct psb_scheduler *scheduler = &dev_priv->scheduler;
	unsigned long irq_flags;

	mutex_lock(&msvdx_priv->msvdx_mutex);
	msvdx_priv->msvdx_needs_reset = 1;
	msvdx_priv->msvdx_current_sequence++;
	PSB_DEBUG_GENERAL
	    ("MSVDXFENCE: incremented msvdx_current_sequence to :%d\n",
	     msvdx_priv->msvdx_current_sequence);

	psb_fence_error(scheduler->dev, PSB_ENGINE_VIDEO,
			msvdx_priv->msvdx_current_sequence,
			_PSB_FENCE_TYPE_EXE, DRM_CMD_HANG);

	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	dev_priv->timer_available = 1;
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);

	spin_lock_irqsave(&msvdx_priv->msvdx_lock, irq_flags);
	psb_msvdx_flush_cmd_queue(scheduler->dev);
	spin_unlock_irqrestore(&msvdx_priv->msvdx_lock, irq_flags);

	psb_schedule_watchdog(dev_priv);
	mutex_unlock(&msvdx_priv->msvdx_mutex);
}

void psb_watchdog_init(struct drm_psb_private *dev_priv)
{
	struct timer_list *wt = &dev_priv->watchdog_timer;
	unsigned long irq_flags;

	spin_lock_init(&dev_priv->watchdog_lock);
	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	init_timer(wt);
	INIT_WORK(&dev_priv->msvdx_watchdog_wq, &psb_msvdx_reset_wq);
	wt->data = (unsigned long) dev_priv;
	wt->function = &psb_watchdog_func;
	dev_priv->timer_available = 1;
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);
}

void psb_watchdog_takedown(struct drm_psb_private *dev_priv)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&dev_priv->watchdog_lock, irq_flags);
	dev_priv->timer_available = 0;
	spin_unlock_irqrestore(&dev_priv->watchdog_lock, irq_flags);
	(void) timer_delete_sync(&dev_priv->watchdog_timer);
}


