/*************************************************************************/ /*!
@File
@Title          Linux fence interface
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Linux module setup
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include <linux/version.h>

#include "srvkm.h"
#include "syscommon.h"
#include "pvr_linux_fence.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0))
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
#include <linux/dma-fence.h>
#else
#include <linux/fence.h>
#endif
#include <linux/dma-resv.h>
#include <linux/list.h>

#include "dmabuf.h"

#define	BLOCKED_ON_READ		1
#define	BLOCKED_ON_WRITE	2

#if defined(CONFIG_PREEMPT_RT_BASE) || defined(CONFIG_PREEMPT_RT)
 #define PVR_BUILD_FOR_RT_LINUX
#endif

struct pvr_fence_context
{
	struct mutex mutex;
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
	IMG_HANDLE hNativeSync;
	struct work_struct fence_work;
	struct list_head fence_frame_list;
	struct list_head fence_context_notify_list;
	struct list_head fence_context_list;
};

struct pvr_fence_frame;



struct pvr_blocking_fence
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	struct dma_fence *fence;
	struct dma_fence_cb cb;
#else
	struct fence *fence;
	struct fence_cb cb;
#endif
	struct pvr_fence_frame *pvr_fence_frame;
	bool installed;
};

struct pvr_fence_frame
{
	struct list_head fence_frame_list;
	struct pvr_fence_context *pvr_fence_context;
	u32 tag;
	bool is_dst;
	IMG_UINT32 ui32ReadOpsPending;
	IMG_UINT32 ui32ReadOps2Pending;
	IMG_UINT32 ui32WriteOpsPending;
	int blocked_on;
	struct pvr_blocking_fence *blocking_fences;
	unsigned blocking_fence_count;
	atomic_t blocking_count;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	struct dma_fence *fence_to_signal;
#else
	struct fence *fence_to_signal;
#endif
	bool unblock;
	bool have_blocking_fences;
};

struct pvr_fence
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	struct dma_fence fence;
#else
	struct fence fence;
#endif
	spinlock_t lock;
	struct pvr_fence_context *pvr_fence_context;
	u32 tag;
};

static LIST_HEAD(fence_context_list);
static LIST_HEAD(fence_context_notify_list);
static DEFINE_MUTEX(pvr_fence_mutex);

static struct workqueue_struct *workqueue;
static unsigned fence_context;
static atomic_t fence_seqno = ATOMIC_INIT(0);
static atomic_t fences_outstanding = ATOMIC_INIT(0);

#if defined(DEBUG)
static atomic_t fences_allocated = ATOMIC_INIT(0);
static atomic_t fences_signalled = ATOMIC_INIT(0);
static atomic_t callbacks_installed = ATOMIC_INIT(0);
static atomic_t callbacks_called = ATOMIC_INIT(0);
#endif

#if defined(PVR_DRM_DRIVER_NAME)
static const char *drvname = PVR_DRM_DRIVER_NAME;
#else
static const char *drvname = "pvr";
#endif
static const char *timeline_name = "PVR";

static unsigned next_seqno(void)
{
	return atomic_inc_return(&fence_seqno) - 1;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static const char *get_driver_name(struct dma_fence *fence)
#else
static const char *get_driver_name(struct fence *fence)
#endif
{
	return drvname;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static const char *get_timeline_name(struct dma_fence *fence)
#else
static const char *get_timeline_name(struct fence *fence)
#endif
{
	return timeline_name;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static bool enable_signaling(struct dma_fence *fence)
#else
static bool enable_signaling(struct fence *fence)
#endif
{
	return true;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static void release_fence(struct dma_fence *fence)
#else
static void release_fence(struct fence *fence)
#endif
{
	struct pvr_fence *pvr_fence = container_of(fence, struct pvr_fence, fence);
	kfree(pvr_fence);

	atomic_dec(&fences_outstanding);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static struct dma_fence_ops fence_ops =
#else
static struct fence_ops fence_ops =
#endif
{
	.get_driver_name = get_driver_name,
	.get_timeline_name = get_timeline_name,
	.enable_signaling = enable_signaling,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	.wait = dma_fence_default_wait,
#else
	.wait = fence_default_wait,
#endif
	.release = release_fence
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static inline bool is_pvr_fence(const struct dma_fence *fence)
#else
static inline bool is_pvr_fence(const struct fence *fence)
#endif
{
	return fence->ops == &fence_ops;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static struct dma_fence *create_fence_to_signal(struct pvr_fence_frame *pvr_fence_frame)
#else
static struct fence *create_fence_to_signal(struct pvr_fence_frame *pvr_fence_frame)
#endif
{
	struct pvr_fence *pvr_fence;
	unsigned seqno = next_seqno();

	pvr_fence = kmalloc(sizeof(*pvr_fence), GFP_KERNEL);
	if (!pvr_fence)
	{
		return NULL;
	}

	spin_lock_init(&pvr_fence->lock);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	dma_fence_init(&pvr_fence->fence, &fence_ops, &pvr_fence->lock, fence_context, seqno);
#else
	fence_init(&pvr_fence->fence, &fence_ops, &pvr_fence->lock, fence_context, seqno);
#endif
	pvr_fence->pvr_fence_context = pvr_fence_frame->pvr_fence_context;
	pvr_fence->tag = pvr_fence_frame->tag;

	pvr_fence_frame->fence_to_signal = &pvr_fence->fence;

#if defined(DEBUG)
	atomic_inc(&fences_allocated);
#endif
	atomic_inc(&fences_outstanding);

	return pvr_fence_frame->fence_to_signal;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static inline bool is_blocking_fence(const struct dma_fence *fence,
#else
static inline bool is_blocking_fence(const struct fence *fence,
#endif
				const struct pvr_fence_frame *pvr_fence_frame)
{
	if (is_pvr_fence(fence))
	{
		struct pvr_fence *pvr_fence = container_of(fence, struct pvr_fence, fence);

		return pvr_fence->pvr_fence_context != pvr_fence_frame->pvr_fence_context && pvr_fence->tag != pvr_fence_frame->tag;
	}

	return true;
}

static void signal_and_put_fence(struct pvr_fence_frame *pvr_fence_frame)
{
	if (pvr_fence_frame->fence_to_signal)
	{
		struct pvr_fence_context *pvr_fence_context = pvr_fence_frame->pvr_fence_context;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
		dma_fence_signal(pvr_fence_frame->fence_to_signal);
		dma_fence_put(pvr_fence_frame->fence_to_signal);
#else
		fence_signal(pvr_fence_frame->fence_to_signal);
		fence_put(pvr_fence_frame->fence_to_signal);
#endif

		pvr_fence_frame->fence_to_signal = NULL;

		queue_work(workqueue, &pvr_fence_context->fence_work);

#if defined(DEBUG)
		atomic_inc(&fences_signalled);
#endif
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static void blocking_fence_signalled(struct dma_fence *fence, struct dma_fence_cb *cb)
#else
static void blocking_fence_signalled(struct fence *fence, struct fence_cb *cb)
#endif
{
	struct pvr_blocking_fence *pvr_blocking_fence = container_of(cb, struct pvr_blocking_fence, cb);
	struct pvr_fence_frame *pvr_fence_frame = pvr_blocking_fence->pvr_fence_frame;
	struct pvr_fence_context *pvr_fence_context = pvr_fence_frame->pvr_fence_context;

	if (atomic_dec_and_test(&pvr_fence_frame->blocking_count))
	{
		queue_work(workqueue, &pvr_fence_context->fence_work);
	}
#if defined(DEBUG)
	atomic_inc(&callbacks_called);
#endif
}

static bool allocate_blocking_fence_storage(struct pvr_fence_frame *pvr_fence_frame, unsigned count)
{
	pvr_fence_frame->blocking_fences = kzalloc(count * sizeof(*pvr_fence_frame->blocking_fences), GFP_KERNEL);
	if (pvr_fence_frame->blocking_fences)
	{	
		pvr_fence_frame->blocking_fence_count = count;
		return true;
	}
	return false;
}

static void free_blocking_fence_storage(struct pvr_fence_frame *pvr_fence_frame)
{
	if (pvr_fence_frame->blocking_fence_count)
	{
		kfree(pvr_fence_frame->blocking_fences);
		pvr_fence_frame->blocking_fence_count = 0;
	}
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static int install_and_get_blocking_fence(struct pvr_fence_frame *pvr_fence_frame, unsigned index, struct dma_fence *fence)
#else
static int install_and_get_blocking_fence(struct pvr_fence_frame *pvr_fence_frame, unsigned index, struct fence *fence)
#endif
{
	struct pvr_blocking_fence *pvr_blocking_fence = &pvr_fence_frame->blocking_fences[index];
	int ret;

	BUG_ON(index >= pvr_fence_frame->blocking_fence_count);

	pvr_blocking_fence->fence = fence;
	pvr_blocking_fence->pvr_fence_frame = pvr_fence_frame;

	atomic_inc(&pvr_fence_frame->blocking_count);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	ret = dma_fence_add_callback(pvr_blocking_fence->fence,
#else
	ret = fence_add_callback(pvr_blocking_fence->fence,
#endif
			&pvr_blocking_fence->cb,
			blocking_fence_signalled);

	pvr_blocking_fence->installed = !ret;
	if (!pvr_blocking_fence->installed)
	{
		atomic_dec(&pvr_fence_frame->blocking_count);
		return 1;
	}
	else
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
		dma_fence_get(fence);
#else
		fence_get(fence);
#endif
#if defined(DEBUG)
		atomic_inc(&callbacks_installed);
#endif
		return 0;
	}
}

static void uninstall_and_put_blocking_fence(struct pvr_fence_frame *pvr_fence_frame, unsigned index)
{
	struct pvr_blocking_fence *pvr_blocking_fence = &pvr_fence_frame->blocking_fences[index];

	BUG_ON(index >= pvr_fence_frame->blocking_fence_count);

	if (pvr_blocking_fence->installed)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
		if (dma_fence_remove_callback(pvr_blocking_fence->fence, &pvr_blocking_fence->cb))
#else
		if (fence_remove_callback(pvr_blocking_fence->fence, &pvr_blocking_fence->cb))
#endif
		{
			atomic_dec(&pvr_fence_frame->blocking_count);
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
		dma_fence_put(pvr_blocking_fence->fence);
#else
		fence_put(pvr_blocking_fence->fence);
#endif
	}
}

static inline int update_reservation_return_value(int ret, bool blocked_on_write)
{
	return ret < 0 ? ret : (ret ? 0 : (blocked_on_write ? BLOCKED_ON_WRITE : BLOCKED_ON_READ));
}

static int update_dma_resv_fences_dst(struct pvr_fence_frame *pvr_fence_frame,
						struct dma_resv *resv)
{
	struct dma_resv_list *flist;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	struct dma_fence *fence_to_signal;
#else
	struct fence *fence_to_signal;
#endif
	unsigned shared_fence_count;
	unsigned blocking_fence_count;
	unsigned i;
	int ret;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0))
	flist = dma_resv_shared_list(resv);
#else
	flist = dma_resv_get_list(resv);
#endif
	shared_fence_count = flist ? flist->shared_count : 0;

	fence_to_signal = create_fence_to_signal(pvr_fence_frame);
	if (!fence_to_signal)
	{
		return -ENOMEM;
	}

	if (!pvr_fence_frame->have_blocking_fences)
	{
		dma_resv_add_excl_fence(resv, fence_to_signal);
		return 0;
	}

	if (!shared_fence_count)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0))
		struct dma_fence *fence = dma_resv_excl_fence(resv);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
		struct dma_fence *fence = dma_resv_get_excl(resv);
#else
		struct fence *fence = dma_resv_get_excl(resv);
#endif

		if (fence && is_blocking_fence(fence, pvr_fence_frame))
		{
			if (allocate_blocking_fence_storage(pvr_fence_frame, 1))
			{	
				ret = install_and_get_blocking_fence(pvr_fence_frame, 0, fence);
			}
			else
			{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
				dma_fence_put(fence_to_signal);
#else
				fence_put(fence_to_signal);
#endif
				return -ENOMEM;
			}
		}
		else
		{
			ret = 1;
		}

		dma_resv_add_excl_fence(resv, fence_to_signal);
		return update_reservation_return_value(ret, true);
	}

	for (i = 0, blocking_fence_count = 0; i < shared_fence_count; i++)
	{

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0))
		struct dma_fence *fence = rcu_dereference_check(flist->shared[i], dma_resv_held(resv));
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
		struct dma_fence *fence = rcu_dereference_protected(flist->shared[i], dma_resv_held(resv));
#else
		struct fence *fence = rcu_dereference_protected(flist->shared[i], dma_resv_held(resv));
#endif

		if (is_blocking_fence(fence, pvr_fence_frame))
		{
			blocking_fence_count++;
		}
	}

	ret = 1;
	if (blocking_fence_count)
	{
		if (allocate_blocking_fence_storage(pvr_fence_frame, blocking_fence_count))
		{
			for (i = 0; i < blocking_fence_count; i++)
			{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0))
				struct dma_fence *fence = rcu_dereference_check(flist->shared[i], dma_resv_held(resv));
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
				struct dma_fence *fence = rcu_dereference_protected(flist->shared[i], dma_resv_held(resv));
#else
				struct fence *fence = rcu_dereference_protected(flist->shared[i], dma_resv_held(resv));
#endif

				if (is_blocking_fence(fence, pvr_fence_frame))
				{
					if (!install_and_get_blocking_fence(pvr_fence_frame, i, fence))
					{
						ret = 0;
					}
				}
			}
		}
		else
		{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
			dma_fence_put(fence_to_signal);
#else
			fence_put(fence_to_signal);
#endif
			return -ENOMEM;
		}
	}

	dma_resv_add_excl_fence(resv, fence_to_signal);
	return update_reservation_return_value(ret, false);
}

static int update_dma_resv_fences_src(struct pvr_fence_frame *pvr_fence_frame,
						struct dma_resv *resv)
{
	struct dma_resv_list *flist;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	struct dma_fence *fence_to_signal = NULL;
	struct dma_fence *blocking_fence = NULL;
#else
	struct fence *fence_to_signal = NULL;
	struct fence *blocking_fence = NULL;
#endif
	bool reserve = true;
	unsigned shared_fence_count;
	unsigned i;
	int ret;

	if (!pvr_fence_frame->have_blocking_fences)
	{
		ret = dma_resv_reserve_shared(resv, 1);
		if (ret)
		{
			return ret;
		}

		fence_to_signal = create_fence_to_signal(pvr_fence_frame);
		if (!fence_to_signal)
		{
			return -ENOMEM;
		}

		dma_resv_add_shared_fence(resv, fence_to_signal);

		return 0;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0))
	flist = dma_resv_shared_list(resv);
#else
	flist = dma_resv_get_list(resv);
#endif
	shared_fence_count = flist ? flist->shared_count : 0;

	/*
	 * There can't be more than one shared fence for a given
	 * fence context, so if a PVR fence is already in the list,
	 * we don't need to reserve space for the new one, but need
	 * to block on it if it isn't ours.
	 */
	for (i = 0; i < shared_fence_count; i++)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0))
		struct dma_fence *fence = rcu_dereference_check(flist->shared[i], dma_resv_held(resv));
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
		struct dma_fence *fence = rcu_dereference_protected(flist->shared[i], dma_resv_held(resv));
#else
		struct fence *fence = rcu_dereference_protected(flist->shared[i], dma_resv_held(resv));
#endif

		if (is_pvr_fence(fence))
		{
			reserve = false;

			if (is_blocking_fence(fence, pvr_fence_frame))
			{
				blocking_fence = fence;
			}
			break;
		}
	}

	if (reserve)
	{
		ret = dma_resv_reserve_shared(resv, 1);
		if (ret)
		{
			return ret;
		}
	}

	fence_to_signal = create_fence_to_signal(pvr_fence_frame);
	if (!fence_to_signal)
	{
		return -ENOMEM;
	}

	if (!blocking_fence && !shared_fence_count)
	{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0))
		struct dma_fence *fence = dma_resv_excl_fence(resv);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
		struct dma_fence *fence = dma_resv_get_excl(resv);
#else
		struct fence *fence = dma_resv_get_excl(resv);
#endif

		if (fence && is_blocking_fence(fence, pvr_fence_frame))
		{
			blocking_fence = fence;
		}
	}

	if (blocking_fence)
	{
		if (allocate_blocking_fence_storage(pvr_fence_frame, 1))
		{	
			ret = install_and_get_blocking_fence(pvr_fence_frame, 0, blocking_fence);
		}
		else
		{
			ret = -ENOMEM;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
			dma_fence_put(fence_to_signal);
#else
			fence_put(fence_to_signal);
#endif
			return ret;
		}
	}
	else
	{
		ret = 1;
	}

	dma_resv_add_shared_fence(resv, fence_to_signal);

	return update_reservation_return_value(ret, !shared_fence_count);
}

/* Must be called with pvr_fence_context mutex held */
static void destroy_fence_frame(struct pvr_fence_frame *pvr_fence_frame)
{
	unsigned i;

	signal_and_put_fence(pvr_fence_frame);

	for (i = 0; i < pvr_fence_frame->blocking_fence_count; i++)
	{
		uninstall_and_put_blocking_fence(pvr_fence_frame, i);
	}
	free_blocking_fence_storage(pvr_fence_frame);

	list_del(&pvr_fence_frame->fence_frame_list);

	kfree(pvr_fence_frame);
}

static inline bool sync_GE(const u32 a, const u32 b)
{
	return (a - b) < (U32_MAX / 2);
}
static inline bool sync_GT(const u32 a, const u32 b)
{
	return (a != b) && sync_GE(a, b);
}

static bool sync_is_ready(struct pvr_fence_frame *pvr_fence_frame)
{
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = pvr_fence_frame->pvr_fence_context->psSyncInfo;

	return (!pvr_fence_frame->is_dst ||
			sync_GE(psSyncInfo->psSyncData->ui32ReadOpsComplete,
					pvr_fence_frame->ui32ReadOpsPending)) &&
				sync_GE(psSyncInfo->psSyncData->ui32ReadOps2Complete,
					pvr_fence_frame->ui32ReadOps2Pending) &&
				sync_GE(psSyncInfo->psSyncData->ui32WriteOpsComplete,
					pvr_fence_frame->ui32WriteOpsPending);
}

static bool sync_gpu_read_op_is_complete(struct pvr_fence_frame *pvr_fence_frame)
{
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = pvr_fence_frame->pvr_fence_context->psSyncInfo;

	/*
	 * If there aren't any blocking fences, we will have recorded the
	 * read ops pending value after it had been updated for the GPU
	 * op.
	 */
	return pvr_fence_frame->have_blocking_fences ?
			sync_GT(psSyncInfo->psSyncData->ui32ReadOpsComplete,
				pvr_fence_frame->ui32ReadOpsPending) : 
			sync_GE(psSyncInfo->psSyncData->ui32ReadOpsComplete,
				pvr_fence_frame->ui32ReadOpsPending);
}

static bool sync_gpu_write_op_is_complete(struct pvr_fence_frame *pvr_fence_frame)
{
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = pvr_fence_frame->pvr_fence_context->psSyncInfo;

	/*
	 * If there aren't any blocking fences, we will have recorded the
	 * write ops pending value after it had been updated for the GPU
	 * op.
	 */
	return pvr_fence_frame->have_blocking_fences ?
			sync_GT(psSyncInfo->psSyncData->ui32WriteOpsComplete,
				pvr_fence_frame->ui32WriteOpsPending) :
			sync_GE(psSyncInfo->psSyncData->ui32WriteOpsComplete,
				pvr_fence_frame->ui32WriteOpsPending);
}

static void sync_complete_read_op(struct pvr_fence_frame *pvr_fence_frame)
{
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = pvr_fence_frame->pvr_fence_context->psSyncInfo;

	psSyncInfo->psSyncData->ui32ReadOps2Complete = ++pvr_fence_frame->ui32ReadOps2Pending;
}

static void sync_complete_write_op(struct pvr_fence_frame *pvr_fence_frame)
{
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = pvr_fence_frame->pvr_fence_context->psSyncInfo;

	psSyncInfo->psSyncData->ui32WriteOpsComplete = ++pvr_fence_frame->ui32WriteOpsPending;
}

static void do_fence_work(struct work_struct *work)
{
	struct pvr_fence_context *pvr_fence_context = container_of(work, struct pvr_fence_context, fence_work);
	bool schedule_device_callbacks = false;

	mutex_lock(&pvr_fence_context->mutex);
	for(;;)
	{
		struct pvr_fence_frame *pvr_fence_frame;
		bool reprocess = false;
		bool next_frame = false;

		pvr_fence_frame = list_first_entry_or_null(&pvr_fence_context->fence_frame_list, struct pvr_fence_frame, fence_frame_list);

		if (pvr_fence_frame)
		{
			if (!atomic_read(&pvr_fence_frame->blocking_count) && sync_is_ready(pvr_fence_frame))
			{
				schedule_device_callbacks = true;

				switch (pvr_fence_frame->blocked_on)
				{
					case BLOCKED_ON_READ:
						sync_complete_read_op(pvr_fence_frame);
						pvr_fence_frame->blocked_on = 0;
						reprocess = true;
						break;
					case BLOCKED_ON_WRITE:
						sync_complete_write_op(pvr_fence_frame);
						pvr_fence_frame->blocked_on = 0;
						reprocess = true;
						break;
					default:
						next_frame = pvr_fence_frame->is_dst ? sync_gpu_write_op_is_complete(pvr_fence_frame) : sync_gpu_read_op_is_complete(pvr_fence_frame);
						break;
				}

				next_frame |= pvr_fence_frame->unblock;
			}
		}

		if (next_frame)
		{
			destroy_fence_frame(pvr_fence_frame);
		}
		else
		{
			if (!reprocess)
			{
				break;
			}
		}

	}
	mutex_unlock(&pvr_fence_context->mutex);

	if (schedule_device_callbacks)
	{
		PVRSRVScheduleDeviceCallbacks();
	}

}

void PVRLinuxFenceContextDestroy(IMG_HANDLE hFenceContext)
{

	struct pvr_fence_context *pvr_fence_context = (struct pvr_fence_context *)hFenceContext;
	struct list_head *entry, *temp;

	mutex_lock(&pvr_fence_mutex);
	mutex_lock(&pvr_fence_context->mutex);

	list_del(&pvr_fence_context->fence_context_list);
	if (!list_empty(&pvr_fence_context->fence_context_notify_list))
	{
		list_del(&pvr_fence_context->fence_context_notify_list);
	}
	mutex_unlock(&pvr_fence_mutex);

	list_for_each_safe(entry, temp, &pvr_fence_context->fence_frame_list)
	{
		struct pvr_fence_frame *pvr_fence_frame = list_entry(entry, struct pvr_fence_frame, fence_frame_list);
 
		destroy_fence_frame(pvr_fence_frame);
	}

	mutex_unlock(&pvr_fence_context->mutex);

	flush_work(&pvr_fence_context->fence_work);

	mutex_destroy(&pvr_fence_context->mutex);

	DmaBufFreeNativeSyncHandle(pvr_fence_context->hNativeSync);

	kfree(pvr_fence_context);
}

IMG_HANDLE PVRLinuxFenceContextCreate(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, IMG_HANDLE hImport)
{
	struct pvr_fence_context *pvr_fence_context;

	pvr_fence_context = kzalloc(sizeof(*pvr_fence_context), GFP_KERNEL);
	if (!pvr_fence_context)
	{
		return NULL;
	}

	pvr_fence_context->hNativeSync = DmaBufGetNativeSyncHandle(hImport);
	if (!pvr_fence_context->hNativeSync)
	{
		kfree(pvr_fence_context);
		return NULL;
	}

	INIT_LIST_HEAD(&pvr_fence_context->fence_frame_list);
	INIT_LIST_HEAD(&pvr_fence_context->fence_context_list);
	INIT_LIST_HEAD(&pvr_fence_context->fence_context_notify_list);

	mutex_init(&pvr_fence_context->mutex);
	
	INIT_WORK(&pvr_fence_context->fence_work, do_fence_work);

	pvr_fence_context->psSyncInfo = psSyncInfo;
	mutex_lock(&pvr_fence_mutex);
	list_add_tail(&pvr_fence_context->fence_context_list, &fence_context_list);
	mutex_unlock(&pvr_fence_mutex);

	return (IMG_HANDLE)pvr_fence_context;
}

static int process_reservation_object(struct pvr_fence_context *pvr_fence_context, struct dma_resv *resv, bool is_dst, u32 tag, bool have_blocking_fences)
{
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = pvr_fence_context->psSyncInfo;
	struct pvr_fence_frame *pvr_fence_frame;
	int ret;

	pvr_fence_frame = kzalloc(sizeof(*pvr_fence_frame), GFP_KERNEL);
	if (!pvr_fence_frame)
	{
		return -ENOMEM;
	}

	pvr_fence_frame->is_dst = is_dst;
	pvr_fence_frame->tag = tag;
	pvr_fence_frame->pvr_fence_context = pvr_fence_context;
	pvr_fence_frame->have_blocking_fences = have_blocking_fences;
	atomic_set(&pvr_fence_frame->blocking_count, 0);
	INIT_LIST_HEAD(&pvr_fence_frame->fence_frame_list);

	ret = is_dst ?
		update_dma_resv_fences_dst(pvr_fence_frame, resv) :
		update_dma_resv_fences_src(pvr_fence_frame, resv);
	if (ret < 0)
	{
		kfree(pvr_fence_frame);
		return ret;
	}
	else
	{
		BUG_ON(ret && !have_blocking_fences);

		pvr_fence_frame->blocked_on = ret;


		/*
		 * If there are no blocking fences, the ops pending values
		 * are recorded after being updated for the GPU operation,
		 * rather than before, so the test for completion of the
		 * operation is different for the two cases.
		 */
		pvr_fence_frame->ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOpsPending;

		pvr_fence_frame->ui32ReadOps2Pending = (pvr_fence_frame->blocked_on == BLOCKED_ON_READ) ? SyncTakeReadOp2(psSyncInfo, SYNC_OP_CLASS_LINUX_FENCE) : psSyncInfo->psSyncData->ui32ReadOps2Pending;

		pvr_fence_frame->ui32WriteOpsPending = (pvr_fence_frame->blocked_on == BLOCKED_ON_WRITE) ? SyncTakeWriteOp(psSyncInfo, SYNC_OP_CLASS_LINUX_FENCE) : psSyncInfo->psSyncData->ui32WriteOpsPending;

		list_add_tail(&pvr_fence_frame->fence_frame_list, &pvr_fence_context->fence_frame_list);
	}

	return 0;
}

static int process_syncinfo(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, bool is_dst, u32 tag, bool have_blocking_fences)
{
	struct pvr_fence_context *pvr_fence_context = (struct pvr_fence_context *)psSyncInfo->hFenceContext;
	struct dma_resv *resv;
	int ret = 0;

	if (!pvr_fence_context)
	{
		return 0;
	}

	mutex_lock(&pvr_fence_context->mutex);
	if ((resv = DmaBufGetReservationObject(pvr_fence_context->hNativeSync)))
	{
		ret = process_reservation_object(pvr_fence_context,
							resv,
							is_dst,
							tag,
							have_blocking_fences);
	}
	mutex_unlock(&pvr_fence_context->mutex);

	mutex_lock(&pvr_fence_mutex);
	mutex_lock(&pvr_fence_context->mutex);
	if (list_empty(&pvr_fence_context->fence_context_notify_list))
	{
		list_add_tail(&pvr_fence_context->fence_context_notify_list, &fence_context_notify_list);
		queue_work(workqueue, &pvr_fence_context->fence_work);
	}
	mutex_unlock(&pvr_fence_context->mutex);
	mutex_unlock(&pvr_fence_mutex);

	return ret;
}


static inline bool sync_enabled(const IMG_BOOL *pbEnabled,
				const IMG_HANDLE *phSyncInfo,
				unsigned index)
{
	return (!pbEnabled || pbEnabled[index]) && phSyncInfo && phSyncInfo[index];
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
static inline bool fence_is_blocking(const struct dma_fence *fence,
#else
static inline bool fence_is_blocking(const struct fence *fence,
#endif
			       const PVRSRV_KERNEL_SYNC_INFO *psSyncInfo)
{

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
#else
	if (test_bit(FENCE_FLAG_SIGNALED_BIT, &fence->flags))
#endif
	{
		return false;
	}

	if (is_pvr_fence(fence))
	{
		struct pvr_fence *pvr_fence = container_of(fence, struct pvr_fence, fence);

		return pvr_fence->pvr_fence_context->psSyncInfo != psSyncInfo;
	}

	return true;
}

static bool resv_is_blocking(struct dma_resv *resv,
				      const PVRSRV_KERNEL_SYNC_INFO *psSyncInfo,
				      bool is_dst)
{
	struct dma_resv_list *flist;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	struct dma_fence *fence;
#else
	struct fence *fence;
#endif
	bool blocking;
	unsigned shared_count;
	unsigned seq;

retry:
	shared_count = 0;
	blocking = false;

#if defined(PVR_BUILD_FOR_RT_LINUX)
	seq = read_seqbegin(&resv->seq);
#else
	seq = read_seqcount_begin(&resv->seq);
#endif
	rcu_read_lock();

	flist = rcu_dereference(resv->fence);
#if defined(PVR_BUILD_FOR_RT_LINUX)
	if (read_seqretry(&resv->seq, seq))
#else
	if (read_seqcount_retry(&resv->seq, seq))
#endif
	{
		goto unlock_retry;
	}

	if (flist)
	{
		shared_count = flist->shared_count;
	}

	if (is_dst)
	{
		unsigned i;

		for (i = 0; (i < shared_count) && !blocking; i++)
		{
			fence = rcu_dereference(flist->shared[i]);

			blocking = fence_is_blocking(fence, psSyncInfo);
		}
	}

	if (!blocking && !shared_count)
	{
		fence = rcu_dereference(resv->fence_excl);
#if defined(PVR_BUILD_FOR_RT_LINUX)
		if (read_seqretry(&resv->seq, seq))
#else
		if (read_seqcount_retry(&resv->seq, seq))
#endif
		{
			goto unlock_retry;
		}

		blocking = fence && fence_is_blocking(fence, psSyncInfo);
	}

	rcu_read_unlock();

	return blocking;

unlock_retry:
	rcu_read_unlock();
	goto retry;
}

static unsigned count_reservation_objects(unsigned num_syncs,
					IMG_HANDLE *phSyncInfo,
					const IMG_BOOL *pbEnabled,
					bool is_dst,
					bool *have_blocking_fences)
{
	unsigned i;
	unsigned count = 0;
	bool blocking_fences = false;

	for (i = 0; i < num_syncs; i++)
	{
		PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
		struct pvr_fence_context *pvr_fence_context;

		if (!sync_enabled(pbEnabled, phSyncInfo, i))
		{
			continue;
		}

		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)phSyncInfo[i];
		pvr_fence_context = (struct pvr_fence_context *)psSyncInfo->hFenceContext;
		if (pvr_fence_context)
		{
			struct dma_resv *resv;

			if ((resv = DmaBufGetReservationObject(pvr_fence_context->hNativeSync)))
			{
				count++;

				if (!blocking_fences)
				{
					blocking_fences = resv_is_blocking(resv,
								psSyncInfo,
								is_dst);
				}
			}
		}
	}

	*have_blocking_fences = blocking_fences;
	return count;
}

static unsigned get_reservation_objects(unsigned num_resvs,
					struct dma_resv **resvs,
					unsigned num_syncs,
					IMG_HANDLE *phSyncInfo,
					const IMG_BOOL *pbEnabled)
{
	unsigned i;
	unsigned count = 0;

	for (i = 0; i < num_syncs; i++)
	{
		PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
		struct pvr_fence_context *pvr_fence_context;
		
		if (!sync_enabled(pbEnabled, phSyncInfo, i))
		{
			continue;
		}

		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)phSyncInfo[i];
		pvr_fence_context = (struct pvr_fence_context *)psSyncInfo->hFenceContext;
		if (pvr_fence_context)
		{
			struct dma_resv *resv;

			if ((resv = DmaBufGetReservationObject(pvr_fence_context->hNativeSync)))
			{
				BUG_ON(count >= num_resvs);
				resvs[count++] = resv;
			}
		}
	}

	return count;
}

static void get_all_reservation_objects(unsigned num_resvs,
					struct dma_resv **resvs,
					IMG_UINT32 ui32NumSrcSyncs,
					IMG_HANDLE *phSrcSyncInfo,
					const IMG_BOOL *pbSrcEnabled,
					IMG_UINT32 ui32NumDstSyncs,
					IMG_HANDLE *phDstSyncInfo,
					const IMG_BOOL *pbDstEnabled)
{
	unsigned num_src_resvs;

	num_src_resvs = get_reservation_objects(num_resvs,
						resvs,
						ui32NumSrcSyncs,
						phSrcSyncInfo,
						pbSrcEnabled);

	get_reservation_objects(num_resvs - num_src_resvs, 
					&resvs[num_src_resvs],
					ui32NumDstSyncs,
					phDstSyncInfo,
					pbDstEnabled);
}

static void unlock_reservation_objects(unsigned num_resvs,
					struct dma_resv **resvs)
{
	unsigned i;

	for (i = 0; i < num_resvs; i++)
	{
		if (resvs[i])
		{
			ww_mutex_unlock(&(resvs[i]->lock));
		}
	}
}

static int lock_reservation_objects_no_retry(struct ww_acquire_ctx *ww_acquire_ctx,
						bool interruptible,
						unsigned num_resvs,
						struct dma_resv **resvs,
						struct dma_resv **contended_resv)
{
	unsigned i;

	for (i = 0; i < num_resvs; i++)
	{
		int ret;

		if (!resvs[i])
		{
			continue;
		}
		if (resvs[i] == *contended_resv)
		{
			*contended_resv = NULL;
			continue;
		}

		ret = interruptible ?
			ww_mutex_lock_interruptible(&(resvs[i]->lock), ww_acquire_ctx) :
			ww_mutex_lock(&(resvs[i]->lock), ww_acquire_ctx);
		if (ret)
		{
			if (ret == -EALREADY)
			{
				resvs[i] = NULL;
				continue;
			}

			unlock_reservation_objects(i, resvs);

			if (*contended_resv)
			{
				ww_mutex_unlock(&((*contended_resv)->lock));
				*contended_resv = NULL;
			}

			if (ret == -EDEADLK)
			{
				*contended_resv = resvs[i];
			}

			return ret;
		}
	}

	return 0;
}

static int lock_reservation_objects(struct ww_acquire_ctx *ww_acquire_ctx,
					bool interruptible,
					unsigned num_resvs,
					struct dma_resv **resvs)
{
	int ret;
	struct dma_resv *contended_resv = NULL;

	do {
		ret = lock_reservation_objects_no_retry(ww_acquire_ctx,
							interruptible,
							num_resvs,
							resvs,
							&contended_resv);
		if (ret == -EDEADLK)
		{
			if (interruptible)
			{
				int res = ww_mutex_lock_slow_interruptible(
						&(contended_resv->lock),
						ww_acquire_ctx);
				if (res)
				{
					return res;
				}
			}
			else
			{
				ww_mutex_lock_slow(&(contended_resv->lock), ww_acquire_ctx);
			}
		}
	} while (ret == -EDEADLK);

	return ret;
}

static int process_syncinfos(u32 tag,
				bool is_dst,
				bool have_blocking_fences,
				IMG_UINT32 ui32NumSyncs,
				IMG_HANDLE *phSyncInfo,
				const IMG_BOOL *pbEnabled)
{
	unsigned i;

	for (i = 0; i < ui32NumSyncs; i++)
	{
		if (sync_enabled(pbEnabled, phSyncInfo, i))
		{
			PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)phSyncInfo[i];
			int ret;
				
			ret = process_syncinfo(psSyncInfo,
						is_dst,
						tag,
						have_blocking_fences);
			if (ret)
			{
				break;
			}
		}
	}

	return 0;
}

static int process_all_syncinfos(u32 tag,
				IMG_UINT32 ui32NumSrcSyncs,
				IMG_HANDLE *phSrcSyncInfo,
				const IMG_BOOL *pbSrcEnabled,
				IMG_UINT32 ui32NumDstSyncs,
				IMG_HANDLE *phDstSyncInfo,
				const IMG_BOOL *pbDstEnabled,
				bool have_blocking_fences)
{
	int ret;

	ret = process_syncinfos(tag,
				false,
				have_blocking_fences,
				ui32NumSrcSyncs,
				phSrcSyncInfo,
				pbSrcEnabled);

	if (ret)
	{
		return ret;
	}

	ret = process_syncinfos(tag,
				true,
				have_blocking_fences,
				ui32NumDstSyncs,
				phDstSyncInfo,
				pbDstEnabled);

	return ret;
}

static void unblock_frames(u32 tag,
				IMG_UINT32 ui32NumSyncs,
				IMG_HANDLE *phSyncInfo,
				const IMG_BOOL *pbEnabled)
{
	unsigned i;

	for (i = 0; i < ui32NumSyncs; i++)
	{
		PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
		struct pvr_fence_context *pvr_fence_context;
		struct list_head *entry;

		if (!sync_enabled(pbEnabled, phSyncInfo, i))
		{
			continue;
		}

		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)phSyncInfo[i];
		pvr_fence_context = (struct pvr_fence_context *)psSyncInfo->hFenceContext;
		if (!pvr_fence_context)
		{
			continue;
		}


		mutex_lock(&pvr_fence_context->mutex);

		list_for_each(entry, &pvr_fence_context->fence_frame_list)
		{
			struct pvr_fence_frame *pvr_fence_frame = list_entry(entry, struct pvr_fence_frame, fence_frame_list);
 
			if (pvr_fence_frame->tag == tag)
			{
				pvr_fence_frame->unblock = true;
			}
		}

		mutex_unlock(&pvr_fence_context->mutex);
	}

	return;
}

static void unblock_all_frames(u32 tag,
				IMG_UINT32 ui32NumSrcSyncs,
				IMG_HANDLE *phSrcSyncInfo,
				const IMG_BOOL *pbSrcEnabled,
				IMG_UINT32 ui32NumDstSyncs,
				IMG_HANDLE *phDstSyncInfo,
				const IMG_BOOL *pbDstEnabled)
{
	unblock_frames(tag,
			ui32NumSrcSyncs,
			phSrcSyncInfo,
			pbSrcEnabled);

	unblock_frames(tag,
			ui32NumDstSyncs,
			phDstSyncInfo,
			pbDstEnabled);
}

static PVRSRV_ERROR pvr_error(int error)
{
	switch(error)
	{
		case 0:
			return PVRSRV_OK;
		case -EINTR:
			return PVRSRV_ERROR_RETRY;
		case -ENOMEM:
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		default:
			break;
	}

	return PVRSRV_ERROR_OUT_OF_MEMORY;
}

static u32 new_frame_tag(void)
{
	static u32 frame_tag;

	return (++frame_tag) ? frame_tag : ++frame_tag;
}

IMG_UINT32 PVRLinuxFenceNumResvObjs(IMG_BOOL *pbBlockingFences,
				IMG_UINT32 ui32NumSrcSyncs,
				IMG_HANDLE *phSrcSyncInfo,
				const IMG_BOOL *pbSrcEnabled,
				IMG_UINT32 ui32NumDstSyncs,
				IMG_HANDLE *phDstSyncInfo,
				const IMG_BOOL *pbDstEnabled)
{
	unsigned count;
	bool blocking_fences_src, blocking_fences_dst;

	count = count_reservation_objects(ui32NumSrcSyncs,
					phSrcSyncInfo,
					pbSrcEnabled,
					false,
					&blocking_fences_src);

	count += count_reservation_objects(ui32NumDstSyncs,
						phDstSyncInfo,
						pbDstEnabled,
						true,
						&blocking_fences_dst);

	*pbBlockingFences = (IMG_BOOL) (blocking_fences_src |
						blocking_fences_dst);

	return count;
}

PVRSRV_ERROR PVRLinuxFenceProcess(IMG_UINT32 *pui32Tag,
				IMG_UINT32 ui32NumResvObjs,
				IMG_BOOL bBlockingFences,
				IMG_UINT32 ui32NumSrcSyncs,
				IMG_HANDLE *phSrcSyncInfo,
				const IMG_BOOL *pbSrcEnabled,
				IMG_UINT32 ui32NumDstSyncs,
				IMG_HANDLE *phDstSyncInfo,
				const IMG_BOOL *pbDstEnabled)
{
	u32 tag;
	struct ww_acquire_ctx ww_acquire_ctx;
	struct dma_resv **resvs = NULL;
	int ret;

	if (!ui32NumResvObjs)
	{
		*pui32Tag = 0;
		ret = 0;
		goto exit;
	}
	tag = new_frame_tag();

	resvs = kmalloc(ui32NumResvObjs * sizeof(*resvs), GFP_KERNEL);
	if (!resvs)
	{
		ret = -ENOMEM;
		goto exit;
	}

	get_all_reservation_objects(ui32NumResvObjs,
					resvs,
					ui32NumSrcSyncs,
					phSrcSyncInfo,
					pbSrcEnabled,
					ui32NumDstSyncs,
					phDstSyncInfo,
					pbDstEnabled);

	ww_acquire_init(&ww_acquire_ctx, &reservation_ww_class);

	/*
	 * If there are no blocking fences, we will be processing
	 * reservation objects after the GPU operation has been
	 * started, so returning an error that may result in the
	 * GPU operation being retried may be inappropriate.
	 */
	ret = lock_reservation_objects(&ww_acquire_ctx,
				       (bool)bBlockingFences,
				       ui32NumResvObjs,
				       resvs);
	if (ret)
	{
		ww_acquire_fini(&ww_acquire_ctx);
		goto exit;
	}
	ww_acquire_done(&ww_acquire_ctx);

	ret = process_all_syncinfos(tag,
					ui32NumSrcSyncs,
					phSrcSyncInfo,
					pbSrcEnabled,
					ui32NumDstSyncs,
					phDstSyncInfo,
					pbDstEnabled,
					(bool)bBlockingFences);

	unlock_reservation_objects(ui32NumResvObjs, resvs);

	ww_acquire_fini(&ww_acquire_ctx);

	if (ret)
	{
		unblock_all_frames(tag,
					ui32NumSrcSyncs,
					phSrcSyncInfo,
					pbSrcEnabled,
					ui32NumDstSyncs,
					phDstSyncInfo,
					pbDstEnabled);
	}
	else
	{
		*pui32Tag = tag;
	}

exit:
	if (resvs)
	{
		kfree(resvs);
	}
	return pvr_error(ret);

}

void PVRLinuxFenceRelease(IMG_UINT32 uTag,
				IMG_UINT32 ui32NumSrcSyncs,
				IMG_HANDLE *phSrcSyncInfo,
				const IMG_BOOL *pbSrcEnabled,
				IMG_UINT32 ui32NumDstSyncs,
				IMG_HANDLE *phDstSyncInfo,
				const IMG_BOOL *pbDstEnabled)
{
	if (uTag)
	{
		unblock_all_frames(uTag,
				ui32NumSrcSyncs,
				phSrcSyncInfo,
				pbSrcEnabled,
				ui32NumDstSyncs,
				phDstSyncInfo,
				pbDstEnabled);
	}
}

static void check_frames(struct pvr_fence_context *pvr_fence_context)
{
	if (list_empty(&pvr_fence_context->fence_frame_list))
	{
		list_del_init(&pvr_fence_context->fence_context_notify_list);
	}
	else
	{
		queue_work(workqueue, &pvr_fence_context->fence_work);
	}
}

void PVRLinuxFenceCheckAll(void)
{
	struct list_head *entry, *temp;

	mutex_lock(&pvr_fence_mutex);
	list_for_each_safe(entry, temp, &fence_context_notify_list)
	{
		struct pvr_fence_context *pvr_fence_context = list_entry(entry, struct pvr_fence_context, fence_context_notify_list);

		mutex_lock(&pvr_fence_context->mutex);
		check_frames(pvr_fence_context);
		mutex_unlock(&pvr_fence_context->mutex);
	}
	mutex_unlock(&pvr_fence_mutex);
}

void PVRLinuxFenceDeInit(void)
{
	unsigned fences_remaining;
	bool contexts_leaked;

	if (workqueue)
	{
		destroy_workqueue(workqueue);
	}

	fences_remaining = atomic_read(&fences_outstanding);
	if (fences_remaining)
	{
		printk(KERN_WARNING "%s: %u fences leaked\n",
				__func__, fences_remaining);
	}

#if defined(DEBUG)
	printk(KERN_INFO "%s: %u fences allocated\n",
			__func__, atomic_read(&fences_allocated));

	printk(KERN_INFO "%s: %u fences signalled\n",
			__func__, atomic_read(&fences_signalled));

	printk(KERN_INFO "%s: %u callbacks installed\n",
			__func__, atomic_read(&callbacks_installed));

	printk(KERN_INFO "%s: %u callbacks called\n",
			__func__, atomic_read(&callbacks_called));
#endif

	mutex_lock(&pvr_fence_mutex);
	contexts_leaked = !list_empty(&fence_context_list);
	mutex_unlock(&pvr_fence_mutex);

	mutex_destroy(&pvr_fence_mutex);

	BUG_ON(contexts_leaked);
}

int PVRLinuxFenceInit(void)
{
	workqueue = create_workqueue("PVR Linux Fence");
	if (!workqueue)
	{
		return -ENOMEM;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
	fence_context = dma_fence_context_alloc(1);
#else
	fence_context = fence_context_alloc(1);
#endif
	return 0;
}
#else	/* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17)) */
IMG_HANDLE PVRLinuxFenceContextCreate(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, IMG_HANDLE hImport)
{
	(void) psSyncInfo;
	(void) hImport;

	return (IMG_HANDLE)(IMG_UINTPTR_T)0xbad;
}

void PVRLinuxFenceContextDestroy(IMG_HANDLE hFenceContext)
{
	(void) hFenceContext;
}

IMG_UINT32 PVRLinuxFenceNumResvObjs(IMG_BOOL *pbBlockingFences,
				IMG_UINT32 ui32NumSrcSyncs,
				IMG_HANDLE *phSrcSyncInfo,
				const IMG_BOOL *pbSrcEnabled,
				IMG_UINT32 ui32NumDstSyncs,
				IMG_HANDLE *phDstSyncInfo,
				const IMG_BOOL *pbDstEnabled)
{
	(void) pbBlockingFences;
	(void) ui32NumSrcSyncs;
	(void) phSrcSyncInfo;
	(void) pbSrcEnabled;
	(void) ui32NumDstSyncs;
	(void) phDstSyncInfo;
	(void) pbDstEnabled;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRLinuxFenceProcess(IMG_UINT32 *pui32Tag,
				IMG_UINT32 ui32NumResvObjs,
				IMG_BOOL bBlockingFences,
				IMG_UINT32 ui32NumSrcSyncs,
				IMG_HANDLE *phSrcSyncInfo,
				const IMG_BOOL *pbSrcEnabled,
				IMG_UINT32 ui32NumDstSyncs,
				IMG_HANDLE *phDstSyncInfo,
				const IMG_BOOL *pbDstEnabled)
{
	(void) pui32Tag;
	(void) ui32NumResvObjs;
	(void) bBlockingFences;
	(void) ui32NumSrcSyncs;
	(void) phSrcSyncInfo;
	(void) pbSrcEnabled;
	(void) ui32NumDstSyncs;
	(void) phDstSyncInfo;
	(void) pbDstEnabled;

	return PVRSRV_OK;
}

void PVRLinuxFenceRelease(IMG_UINT32 ui32Tag,
				IMG_UINT32 ui32NumSrcSyncs,
				IMG_HANDLE *phSrcSyncInfo,
				const IMG_BOOL *pbSrcEnabled,
				IMG_UINT32 ui32NumDstSyncs,
				IMG_HANDLE *phDstSyncInfo,
				const IMG_BOOL *pbDstEnabled)
{
	(void) ui32Tag;
	(void) ui32NumSrcSyncs;
	(void) phSrcSyncInfo;
	(void) pbSrcEnabled;
	(void) ui32NumDstSyncs;
	(void) phDstSyncInfo;
	(void) pbDstEnabled;
}

void PVRLinuxFenceCheckAll(void)
{
}

int PVRLinuxFenceInit(void)
{
	return 0;
}

void PVRLinuxFenceDeInit(void)
{
}
#endif	/* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17)) */
