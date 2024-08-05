/*
 * Copyright (c) 2011, Intel Corporation.
 * All Rights Reserved.
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
 *
 * Authors: Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 */

#include <drm/drmP.h>
#include "psb_drv.h"
#include "psb_msvdx.h"


static void psb_fence_poll(struct ttm_fence_device *fdev,
			   uint32_t fence_class, uint32_t waiting_types)
{
	struct drm_psb_private *dev_priv =
	    container_of(fdev, struct drm_psb_private, fdev);
	uint32_t sequence = 0;
	struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;


	if (unlikely(!dev_priv))
		return;

	if (waiting_types == 0)
		return;

	switch (fence_class) {
	case PSB_ENGINE_VIDEO:
		sequence = msvdx_priv->msvdx_current_sequence;
		break;
	default:
		break;
	}

	/* DRM_ERROR("Polling fence sequence, got 0x%08x\n", sequence); */
	ttm_fence_handler(fdev, fence_class, sequence,
			_PSB_FENCE_TYPE_EXE, 0);
}

void psb_fence_error(struct drm_device *dev,
		     uint32_t fence_class,
		     uint32_t sequence, uint32_t type, int error)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_fence_device *fdev = &dev_priv->fdev;
	unsigned long irq_flags;
	struct ttm_fence_class_manager *fc =
	    &fdev->fence_class[fence_class];

	BUG_ON(fence_class >= PSB_NUM_ENGINES);
	write_lock_irqsave(&fc->lock, irq_flags);
	ttm_fence_handler(fdev, fence_class, sequence, type, error);
	write_unlock_irqrestore(&fc->lock, irq_flags);
}

int psb_fence_emit_sequence(struct ttm_fence_device *fdev,
			    uint32_t fence_class,
			    uint32_t flags, uint32_t *sequence,
			    unsigned long *timeout_jiffies)
{
	struct drm_psb_private *dev_priv =
	    container_of(fdev, struct drm_psb_private, fdev);
	uint32_t seq = 0;

	if (!dev_priv)
		return -EINVAL;

	if (fence_class >= PSB_NUM_ENGINES)
		return -EINVAL;

	switch (fence_class) {
	case PSB_ENGINE_VIDEO:
		spin_lock(&dev_priv->sequence_lock);
		seq = dev_priv->sequence[fence_class]++;
		dev_priv->sequence[fence_class] &= fdev->fence_class[fence_class].sequence_mask;
		if (dev_priv->sequence[fence_class] == 0)
			dev_priv->sequence[fence_class]++;

		spin_unlock(&dev_priv->sequence_lock);
		break;
	default:
		DRM_ERROR("Unexpected fence class\n");
		return -EINVAL;
	}

	*sequence = seq;
	*timeout_jiffies = jiffies + DRM_HZ * 3;

	return 0;
}

static void psb_fence_lockup(struct ttm_fence_object *fence,
			     uint32_t fence_types)
{
	struct ttm_fence_device *fdev = fence->fdev;
	struct ttm_fence_class_manager *fc = ttm_fence_fc(fence);
	struct drm_psb_private *dev_priv =
		container_of(fdev, struct drm_psb_private, fdev);
	struct drm_device *dev =  (struct drm_device *)dev_priv->dev;

	if (fence->fence_class == PSB_ENGINE_VIDEO) {
		struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;

		DRM_ERROR("MSVDX timeout (probable lockup) detected, flush queued cmdbuf");

		psb_msvdx_flush_cmd_queue(dev);

		write_lock(&fc->lock);
		ttm_fence_handler(fence->fdev, fence->fence_class,
				  fence->sequence, fence_types, -EBUSY);
		write_unlock(&fc->lock);
		
		msvdx_priv->msvdx_needs_reset = 1;
	} else 
		DRM_ERROR("Unsupported fence class\n");
}

void psb_fence_handler(struct drm_device *dev, uint32_t fence_class)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_fence_device *fdev = &dev_priv->fdev;
	struct ttm_fence_class_manager *fc =
	    &fdev->fence_class[fence_class];
	unsigned long irq_flags;

	write_lock_irqsave(&fc->lock, irq_flags);
	psb_fence_poll(fdev, fence_class, _PSB_FENCE_TYPE_EXE);
	write_unlock_irqrestore(&fc->lock, irq_flags);
}


static struct ttm_fence_driver psb_ttm_fence_driver = {
	.has_irq = NULL,
	.emit = psb_fence_emit_sequence,
	.flush = NULL,
	.poll = psb_fence_poll,
	.needed_flush = NULL,
	.wait = NULL,
	.signaled = NULL,
	.lockup = psb_fence_lockup,
};

int psb_ttm_fence_device_init(struct ttm_fence_device *fdev)
{
	struct drm_psb_private *dev_priv =
		container_of(fdev, struct drm_psb_private, fdev);
	struct ttm_fence_class_init fci = {.wrap_diff = (1 << 30),
		.flush_diff = (1 << 29),
		.sequence_mask = 0xFFFFFFFF
	};

	return ttm_fence_device_init(PSB_NUM_ENGINES,
				     dev_priv->mem_global_ref.object,
				     fdev, &fci, 1,
				     &psb_ttm_fence_driver);
}
