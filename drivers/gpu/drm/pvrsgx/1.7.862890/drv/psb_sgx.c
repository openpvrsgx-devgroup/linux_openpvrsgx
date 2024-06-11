/**************************************************************************
 * Copyright (c) 2011, Intel Corporation.
 * All Rights Reserved.
 * Copyright (c) 2008, Tungsten Graphics, Inc. Cedar Park, TX. USA.
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
 **************************************************************************/

#include <drm/drmP.h>
#include "psb_drv.h"
#include "psb_drm.h"
#include "psb_reg.h"
#include "psb_msvdx.h"
#include "ttm/ttm_bo_api.h"
#include "ttm/ttm_execbuf_util.h"
#include "psb_ttm_userobj_api.h"
#include "ttm/ttm_placement.h"
#include "psb_sgx.h"
#include "psb_intel_reg.h"
#include "psb_powermgmt.h"


static inline int psb_same_page(unsigned long offset,
				unsigned long offset2)
{
	return (offset & PAGE_MASK) == (offset2 & PAGE_MASK);
}

static inline unsigned long psb_offset_end(unsigned long offset,
					      unsigned long end)
{
	offset = (offset + PAGE_SIZE) & PAGE_MASK;
	return (end < offset) ? end : offset;
}

static void psb_idle_engine(struct drm_device *dev, int engine);

struct psb_dstbuf_cache {
	unsigned int dst;
	struct ttm_buffer_object *dst_buf;
	unsigned long dst_offset;
	uint32_t *dst_page;
	unsigned int dst_page_offset;
	struct ttm_bo_kmap_obj dst_kmap;
	bool dst_is_iomem;
};

struct psb_validate_buffer {
	struct ttm_validate_buffer base;
	struct psb_validate_req req;
	int ret;
	struct psb_validate_arg __user *user_val_arg;
	uint32_t flags;
	uint32_t offset;
	int po_correct;
};

static int psb_check_presumed(struct psb_validate_req *req,
			      struct ttm_buffer_object *bo,
			      struct psb_validate_arg __user *data,
			      int *presumed_ok)
{
	struct psb_validate_req __user *user_req = &(data->d.req);

	*presumed_ok = 0;

	if (bo->mem.mem_type == TTM_PL_SYSTEM) {
		*presumed_ok = 1;
		return 0;
	}

	if (unlikely(!(req->presumed_flags & PSB_USE_PRESUMED)))
		return 0;

	if (bo->offset == req->presumed_gpu_offset) {
		*presumed_ok = 1;
		return 0;
	}

	return __put_user(req->presumed_flags & ~PSB_USE_PRESUMED,
			  &user_req->presumed_flags);
}


static void psb_unreference_buffers(struct psb_context *context)
{
	struct ttm_validate_buffer *entry, *next;
	struct list_head *list = &context->validate_list;

	list_for_each_entry_safe(entry, next, list, head) {
		list_del(&entry->head);
		ttm_bo_unref(&entry->bo);
	}

	list = &context->kern_validate_list;

	list_for_each_entry_safe(entry, next, list, head) {
		list_del(&entry->head);
		ttm_bo_unref(&entry->bo);
	}
}


static int psb_lookup_validate_buffer(struct drm_file *file_priv,
				      uint64_t data,
				      struct psb_validate_buffer *item)
{
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;

	item->user_val_arg =
	    (struct psb_validate_arg __user *) (unsigned long) data;

	if (unlikely(copy_from_user(&item->req, &item->user_val_arg->d.req,
				    sizeof(item->req)) != 0)) {
		DRM_ERROR("Lookup copy fault.\n");
		return -EFAULT;
	}

	item->base.bo =
	    ttm_buffer_object_lookup(tfile, item->req.buffer_handle);

	if (unlikely(item->base.bo == NULL)) {
		DRM_ERROR("Bo lookup fault.\n");
		return -EINVAL;
	}

	return 0;
}

static int psb_reference_buffers(struct drm_file *file_priv,
				 uint64_t data,
				 struct psb_context *context)
{
	struct psb_validate_buffer *item;
	int ret;

	while (likely(data != 0)) {
		if (unlikely(context->used_buffers >=
			     PSB_NUM_VALIDATE_BUFFERS)) {
			DRM_ERROR("Too many buffers "
				  "on validate list.\n");
			ret = -EINVAL;
			goto out_err0;
		}

		item = &context->buffers[context->used_buffers];

		ret = psb_lookup_validate_buffer(file_priv, data, item);
		if (unlikely(ret != 0))
			goto out_err0;

		item->base.reserved = 0;
		list_add_tail(&item->base.head, &context->validate_list);
		context->used_buffers++;
		data = item->req.next;
	}
	return 0;

out_err0:
	psb_unreference_buffers(context);
	return ret;
}

static int
psb_placement_fence_type(struct ttm_buffer_object *bo,
			 uint64_t set_val_flags,
			 uint64_t clr_val_flags,
			 uint32_t new_fence_class,
			 uint32_t *new_fence_type)
{
	int ret;
	uint32_t n_fence_type;
	/*
	uint32_t set_flags = set_val_flags & 0xFFFFFFFF;
	uint32_t clr_flags = clr_val_flags & 0xFFFFFFFF;
	*/
	struct ttm_fence_object *old_fence;
	uint32_t old_fence_type;
	struct ttm_placement placement;

	if (unlikely
	    (!(set_val_flags &
	       (PSB_GPU_ACCESS_READ | PSB_GPU_ACCESS_WRITE)))) {
		DRM_ERROR
		    ("GPU access type (read / write) is not indicated.\n");
		return -EINVAL;
	}

	/* User space driver doesn't set any TTM placement flags in set_val_flags or clr_val_flags */
	placement.num_placement = 0;/* FIXME  */
	placement.num_busy_placement = 0;
	placement.fpfn = 0;
        placement.lpfn = 0;
	ret = psb_ttm_bo_check_placement(bo, &placement);
	if (unlikely(ret != 0))
		return ret;

	switch (new_fence_class) {
	default:
		n_fence_type = _PSB_FENCE_TYPE_EXE;
	}

	*new_fence_type = n_fence_type;
	old_fence = (struct ttm_fence_object *) bo->sync_obj;
	old_fence_type = (uint32_t) (unsigned long) bo->sync_obj_arg;

	if (old_fence && ((new_fence_class != old_fence->fence_class) ||
			  ((n_fence_type ^ old_fence_type) &
			   old_fence_type))) {
		ret = ttm_bo_wait(bo, 0, 1, 0);
		if (unlikely(ret != 0))
			return ret;
	}
	/*
	bo->proposed_flags = (bo->proposed_flags | set_flags)
		& ~clr_flags & TTM_PL_MASK_MEMTYPE;
	*/
	return 0;
}

int psb_validate_kernel_buffer(struct psb_context *context,
			       struct ttm_buffer_object *bo,
			       uint32_t fence_class,
			       uint64_t set_flags, uint64_t clr_flags)
{
	struct psb_validate_buffer *item;
	uint32_t cur_fence_type;
	int ret;

	if (unlikely(context->used_buffers >= PSB_NUM_VALIDATE_BUFFERS)) {
		DRM_ERROR("Out of free validation buffer entries for "
			  "kernel buffer validation.\n");
		return -ENOMEM;
	}

	item = &context->buffers[context->used_buffers];
	item->user_val_arg = NULL;
	item->base.reserved = 0;

	ret = ttm_bo_reserve(bo, 1, 0, 1, context->val_seq);
	if (unlikely(ret != 0))
		goto out_unlock;

	spin_lock(&bo->bdev->fence_lock);
	ret = psb_placement_fence_type(bo, set_flags, clr_flags, fence_class,
				       &cur_fence_type);
	if (unlikely(ret != 0)) {
		ttm_bo_unreserve(bo);
		goto out_unlock;
	}

	item->base.bo = ttm_bo_reference(bo);
	item->base.new_sync_obj_arg = (void *) (unsigned long) cur_fence_type;
	item->base.reserved = 1;

	list_add_tail(&item->base.head, &context->kern_validate_list);
	context->used_buffers++;
	/*
	ret = ttm_bo_validate(bo, 1, 0, 0);
	if (unlikely(ret != 0))
		goto out_unlock;
	*/
	item->offset = bo->offset;
	item->flags = bo->mem.placement;
	context->fence_types |= cur_fence_type;

out_unlock:
	spin_unlock(&bo->bdev->fence_lock);
	return ret;
}


static int psb_validate_buffer_list(struct drm_file *file_priv,
				    uint32_t fence_class,
				    struct psb_context *context,
				    int *po_correct)
{
	struct psb_validate_buffer *item;
	struct ttm_buffer_object *bo;
	int ret;
	struct psb_validate_req *req;
	uint32_t fence_types = 0;
	uint32_t cur_fence_type;
	struct ttm_validate_buffer *entry;
	struct list_head *list = &context->validate_list;
	/*
	struct ttm_placement placement;
	uint32_t flags;
	*/

	*po_correct = 1;

	list_for_each_entry(entry, list, head) {
		item =
		    container_of(entry, struct psb_validate_buffer, base);
		bo = entry->bo;
		item->ret = 0;
		req = &item->req;

		spin_lock(&bo->bdev->fence_lock);
		ret = psb_placement_fence_type(bo,
					       req->set_flags,
					       req->clear_flags,
					       fence_class,
					       &cur_fence_type);
		if (unlikely(ret != 0))
			goto out_err;

		/*
                flags = item->req.pad64 | TTM_PL_FLAG_WC | TTM_PL_FLAG_UNCACHED;
                placement.num_placement = 1;
                placement.placement = &flags;
                placement.num_busy_placement = 0;
                placement.fpfn = 0;
                placement.lpfn = 0;
                ret = ttm_bo_validate(bo, &placement, 1, 0, 0);
		if (unlikely(ret != 0))
			goto out_err;
		*/
		fence_types |= cur_fence_type;
		entry->new_sync_obj_arg = (void *)
			(unsigned long) cur_fence_type;

		item->offset = bo->offset;
		item->flags = bo->mem.placement;
		spin_unlock(&bo->bdev->fence_lock);

		ret =
		    psb_check_presumed(&item->req, bo, item->user_val_arg,
				       &item->po_correct);
		if (unlikely(ret != 0))
			goto out_err;

		if (unlikely(!item->po_correct))
			*po_correct = 0;

		item++;
	}

	context->fence_types |= fence_types;

	return 0;
out_err:
	spin_unlock(&bo->bdev->fence_lock);
	item->ret = ret;
	return ret;
}

static void psb_clear_dstbuf_cache(struct psb_dstbuf_cache *dst_cache)
{
	if (dst_cache->dst_page) {
		ttm_bo_kunmap(&dst_cache->dst_kmap);
		dst_cache->dst_page = NULL;
	}
	dst_cache->dst_buf = NULL;
	dst_cache->dst = ~0;
}

static int psb_update_dstbuf_cache(struct psb_dstbuf_cache *dst_cache,
				   struct psb_validate_buffer *buffers,
				   unsigned int dst,
				   unsigned long dst_offset)
{
	int ret;

	PSB_DEBUG_GENERAL("Destination buffer is %d.\n", dst);

	if (unlikely(dst != dst_cache->dst || NULL == dst_cache->dst_buf)) {
		psb_clear_dstbuf_cache(dst_cache);
		dst_cache->dst = dst;
		dst_cache->dst_buf = buffers[dst].base.bo;
	}

	if (unlikely
	    (dst_offset >= dst_cache->dst_buf->num_pages * PAGE_SIZE)) {
		DRM_ERROR("Relocation destination out of bounds.\n");
		return -EINVAL;
	}

	if (!psb_same_page(dst_cache->dst_offset, dst_offset) ||
	    NULL == dst_cache->dst_page) {
		if (NULL != dst_cache->dst_page) {
			ttm_bo_kunmap(&dst_cache->dst_kmap);
			dst_cache->dst_page = NULL;
		}

		ret =
		    ttm_bo_kmap(dst_cache->dst_buf,
				dst_offset >> PAGE_SHIFT, 1,
				&dst_cache->dst_kmap);
		if (ret) {
			DRM_ERROR("Could not map destination buffer for "
				  "relocation.\n");
			return ret;
		}

		dst_cache->dst_page =
		    ttm_kmap_obj_virtual(&dst_cache->dst_kmap,
					 &dst_cache->dst_is_iomem);
		dst_cache->dst_offset = dst_offset & PAGE_MASK;
		dst_cache->dst_page_offset = dst_cache->dst_offset >> 2;
	}
	return 0;
}

static int psb_apply_reloc(struct drm_psb_private *dev_priv,
			   uint32_t fence_class,
			   const struct drm_psb_reloc *reloc,
			   struct psb_validate_buffer *buffers,
			   int num_buffers,
			   struct psb_dstbuf_cache *dst_cache,
			   int no_wait, int interruptible)
{
	uint32_t val;
	uint32_t background;
	unsigned int index;
	int ret;
	unsigned int shift;
	unsigned int align_shift;
	struct ttm_buffer_object *reloc_bo;


	PSB_DEBUG_GENERAL("Reloc type %d\n"
			  "\t where 0x%04x\n"
			  "\t buffer 0x%04x\n"
			  "\t mask 0x%08x\n"
			  "\t shift 0x%08x\n"
			  "\t pre_add 0x%08x\n"
			  "\t background 0x%08x\n"
			  "\t dst_buffer 0x%08x\n"
			  "\t arg0 0x%08x\n"
			  "\t arg1 0x%08x\n",
			  reloc->reloc_op,
			  reloc->where,
			  reloc->buffer,
			  reloc->mask,
			  reloc->shift,
			  reloc->pre_add,
			  reloc->background,
			  reloc->dst_buffer, reloc->arg0, reloc->arg1);

	if (unlikely(reloc->buffer >= num_buffers)) {
		DRM_ERROR("Illegal relocation buffer %d.\n",
			  reloc->buffer);
		return -EINVAL;
	}

	if (buffers[reloc->buffer].po_correct)
		return 0;

	if (unlikely(reloc->dst_buffer >= num_buffers)) {
		DRM_ERROR
		    ("Illegal destination buffer for relocation %d.\n",
		     reloc->dst_buffer);
		return -EINVAL;
	}

	ret =
	    psb_update_dstbuf_cache(dst_cache, buffers, reloc->dst_buffer,
				    reloc->where << 2);
	if (ret)
		return ret;

	reloc_bo = buffers[reloc->buffer].base.bo;

	if (unlikely(reloc->pre_add >= (reloc_bo->num_pages << PAGE_SHIFT))) {
		DRM_ERROR("Illegal relocation offset add.\n");
		return -EINVAL;
	}

	switch (reloc->reloc_op) {
	case PSB_RELOC_OP_OFFSET:
		val = reloc_bo->offset + reloc->pre_add;
		break;
	default:
		DRM_ERROR("Unimplemented relocation.\n");
		return -EINVAL;
	}

	shift =
	    (reloc->shift & PSB_RELOC_SHIFT_MASK) >> PSB_RELOC_SHIFT_SHIFT;
	align_shift =
	    (reloc->
	     shift & PSB_RELOC_ALSHIFT_MASK) >> PSB_RELOC_ALSHIFT_SHIFT;

	val = ((val >> align_shift) << shift);
	index = reloc->where - dst_cache->dst_page_offset;

	background = reloc->background;
	val = (background & ~reloc->mask) | (val & reloc->mask);
	dst_cache->dst_page[index] = val;

	PSB_DEBUG_GENERAL("Reloc buffer %d index 0x%08x, value 0x%08x\n",
			  reloc->dst_buffer, index,
			  dst_cache->dst_page[index]);

	return 0;
}

static int psb_ok_to_map_reloc(struct drm_psb_private *dev_priv,
			       unsigned int num_pages)
{
	int ret = 0;

	spin_lock(&dev_priv->reloc_lock);
	if (dev_priv->rel_mapped_pages + num_pages <= PSB_MAX_RELOC_PAGES) {
		dev_priv->rel_mapped_pages += num_pages;
		ret = 1;
	}
	spin_unlock(&dev_priv->reloc_lock);
	return ret;
}

static int psb_fixup_relocs(struct drm_file *file_priv,
			    uint32_t fence_class,
			    unsigned int num_relocs,
			    unsigned int reloc_offset,
			    uint32_t reloc_handle,
			    struct psb_context *context,
			    int no_wait, int interruptible)
{
	struct drm_device *dev = file_priv->minor->dev;
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_buffer_object *reloc_buffer = NULL;
	unsigned int reloc_num_pages;
	unsigned int reloc_first_page;
	unsigned int reloc_last_page;
	struct psb_dstbuf_cache dst_cache;
	struct drm_psb_reloc *reloc;
	struct ttm_bo_kmap_obj reloc_kmap;
	bool reloc_is_iomem;
	int count;
	int ret = 0;
	int registered = 0;
	uint32_t num_buffers = context->used_buffers;

	if (num_relocs == 0)
		return 0;

	memset(&dst_cache, 0, sizeof(dst_cache));
	memset(&reloc_kmap, 0, sizeof(reloc_kmap));

	reloc_buffer = ttm_buffer_object_lookup(tfile, reloc_handle);
	if (!reloc_buffer)
		goto out;

	if (unlikely(atomic_read(&reloc_buffer->reserved) != 1)) {
		DRM_ERROR("Relocation buffer was not on validate list.\n");
		ret = -EINVAL;
		goto out;
	}

	reloc_first_page = reloc_offset >> PAGE_SHIFT;
	reloc_last_page =
	    (reloc_offset +
	     num_relocs * sizeof(struct drm_psb_reloc)) >> PAGE_SHIFT;
	reloc_num_pages = reloc_last_page - reloc_first_page + 1;
	reloc_offset &= ~PAGE_MASK;

	if (reloc_num_pages > PSB_MAX_RELOC_PAGES) {
		DRM_ERROR("Relocation buffer is too large\n");
		ret = -EINVAL;
		goto out;
	}

	DRM_WAIT_ON(ret, dev_priv->rel_mapped_queue, 3 * DRM_HZ,
		    (registered =
		     psb_ok_to_map_reloc(dev_priv, reloc_num_pages)));

	if (ret == -EINTR) {
		ret = -ERESTART;
		goto out;
	}
	if (ret) {
		DRM_ERROR("Error waiting for space to map "
			  "relocation buffer.\n");
		goto out;
	}

	ret = ttm_bo_kmap(reloc_buffer, reloc_first_page,
			  reloc_num_pages, &reloc_kmap);

	if (ret) {
		DRM_ERROR("Could not map relocation buffer.\n"
			  "\tReloc buffer id 0x%08x.\n"
			  "\tReloc first page %d.\n"
			  "\tReloc num pages %d.\n",
			  reloc_handle, reloc_first_page, reloc_num_pages);
		goto out;
	}

	reloc = (struct drm_psb_reloc *)
	    ((unsigned long)
	     ttm_kmap_obj_virtual(&reloc_kmap,
				  &reloc_is_iomem) + reloc_offset);

	for (count = 0; count < num_relocs; ++count) {
		ret = psb_apply_reloc(dev_priv, fence_class,
				      reloc, context->buffers,
				      num_buffers, &dst_cache,
				      no_wait, interruptible);
		if (ret)
			goto out1;
		reloc++;
	}

out1:
	ttm_bo_kunmap(&reloc_kmap);
out:
	if (registered) {
		spin_lock(&dev_priv->reloc_lock);
		dev_priv->rel_mapped_pages -= reloc_num_pages;
		spin_unlock(&dev_priv->reloc_lock);
		DRM_WAKEUP(&dev_priv->rel_mapped_queue);
	}

	psb_clear_dstbuf_cache(&dst_cache);
	if (reloc_buffer)
		ttm_bo_unref(&reloc_buffer);
	return ret;
}

void psb_fence_or_sync(struct drm_file *file_priv,
		       uint32_t engine,
		       uint32_t fence_types,
		       uint32_t fence_flags,
		       struct list_head *list,
		       struct psb_ttm_fence_rep *fence_arg,
		       struct ttm_fence_object **fence_p)
{
	struct drm_device *dev = file_priv->minor->dev;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_fence_device *fdev = &dev_priv->fdev;
	int ret;
	struct ttm_fence_object *fence;
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	uint32_t handle;

	ret = ttm_fence_user_create(fdev, tfile,
				    engine, fence_types,
				    TTM_FENCE_FLAG_EMIT, &fence, &handle);
	if (ret) {

		/*
		 * Fence creation failed.
		 * Fall back to synchronous operation and idle the engine.
		 */

		psb_idle_engine(dev, engine);
		if (!(fence_flags & DRM_PSB_FENCE_NO_USER)) {

			/*
			 * Communicate to user-space that
			 * fence creation has failed and that
			 * the engine is idle.
			 */

			fence_arg->handle = ~0;
			fence_arg->error = ret;
		}

		ttm_eu_backoff_reservation(list);
		if (fence_p)
			*fence_p = NULL;
		return;
	}

	ttm_eu_fence_buffer_objects(list, fence);
	if (!(fence_flags & DRM_PSB_FENCE_NO_USER)) {
		struct ttm_fence_info info = ttm_fence_get_info(fence);
		fence_arg->handle = handle;
		fence_arg->fence_class = ttm_fence_class(fence);
		fence_arg->fence_type = ttm_fence_types(fence);
		fence_arg->signaled_types = info.signaled_types;
		fence_arg->error = 0;
	} else {
		ret =
		    ttm_ref_object_base_unref(tfile, handle,
					      ttm_fence_type);
		BUG_ON(ret);
	}

	if (fence_p)
		*fence_p = fence;
	else if (fence)
		ttm_fence_object_unref(&fence);
}


#if 0
static int psb_dump_page(struct ttm_buffer_object *bo,
			 unsigned int page_offset, unsigned int num)
{
	struct ttm_bo_kmap_obj kmobj;
	int is_iomem;
	uint32_t *p;
	int ret;
	unsigned int i;

	ret = ttm_bo_kmap(bo, page_offset, 1, &kmobj);
	if (ret)
		return ret;

	p = ttm_kmap_obj_virtual(&kmobj, &is_iomem);
	for (i = 0; i < num; ++i)
		PSB_DEBUG_GENERAL("0x%04x: 0x%08x\n", i, *p++);

	ttm_bo_kunmap(&kmobj);
	return 0;
}
#endif

static void psb_idle_engine(struct drm_device *dev, int engine)
{
	/*Fix me add video engile support*/
	return;
}

static int psb_handle_copyback(struct drm_device *dev,
			       struct psb_context *context,
			       int ret)
{
	int err = ret;
	struct ttm_validate_buffer *entry;
	struct psb_validate_arg arg;
	struct list_head *list = &context->validate_list;

	if (ret) {
		ttm_eu_backoff_reservation(list);
		ttm_eu_backoff_reservation(&context->kern_validate_list);
	}


	if (ret != -EAGAIN && ret != -EINTR && ret != -ERESTART) {
		list_for_each_entry(entry, list, head) {
			struct psb_validate_buffer *vbuf =
			    container_of(entry, struct psb_validate_buffer,
					 base);
			arg.handled = 1;
			arg.ret = vbuf->ret;
			if (!arg.ret) {
				struct ttm_buffer_object *bo = entry->bo;
				spin_lock(&bo->bdev->fence_lock);
				arg.d.rep.gpu_offset = bo->offset;
				arg.d.rep.placement = bo->mem.placement;
				arg.d.rep.fence_type_mask =
					(uint32_t) (unsigned long)
					entry->new_sync_obj_arg;
				spin_unlock(&bo->bdev->fence_lock);
			}

			if (__copy_to_user(vbuf->user_val_arg,
					   &arg, sizeof(arg)))
				err = -EFAULT;

			if (arg.ret)
				break;
		}
	}

	return err;
}

int psb_cmdbuf_ioctl(struct drm_device *dev, void *data,
		     struct drm_file *file_priv)
{
	struct drm_psb_cmdbuf_arg *arg = data;
	int ret = 0;
	struct ttm_object_file *tfile = psb_fpriv(file_priv)->tfile;
	struct ttm_buffer_object *cmd_buffer = NULL;
	struct psb_ttm_fence_rep fence_arg;
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *)file_priv->minor->dev->dev_private;
	struct psb_video_ctx *pos, *n;
	int engine;
	int po_correct;
	struct psb_context *context;

	((struct msvdx_private*)dev_priv->msvdx_private)->tfile = tfile;

	ret = ttm_read_lock(&dev_priv->ttm_lock, true);
	if (unlikely(ret != 0))
		return ret;

	if (arg->engine == PSB_ENGINE_VIDEO) {
		if (!ospm_power_using_hw_begin(OSPM_VIDEO_DEC_ISLAND,
						OSPM_UHB_FORCE_POWER_ON))
			return -EBUSY;
	} 

	ret = mutex_lock_interruptible(&dev_priv->cmdbuf_mutex);
	if (unlikely(ret != 0))
		goto out_err0;


	context = &dev_priv->context;
	context->used_buffers = 0;
	context->fence_types = 0;
	BUG_ON(!list_empty(&context->validate_list));
	BUG_ON(!list_empty(&context->kern_validate_list));

	if (unlikely(context->buffers == NULL)) {
		context->buffers = vmalloc(PSB_NUM_VALIDATE_BUFFERS *
					   sizeof(*context->buffers));
		if (unlikely(context->buffers == NULL)) {
			ret = -ENOMEM;
			goto out_err1;
		}
	}

	ret = psb_reference_buffers(file_priv,
				    arg->buffer_list,
				    context);

	if (unlikely(ret != 0))
		goto out_err1;

	context->val_seq = atomic_add_return(1, &dev_priv->val_seq);

/*
	ret = ttm_eu_reserve_buffers(&context->validate_list,
				     context->val_seq);
*/
	/* XXX new TTM removed driver managed sequence number,
	 * check if this has other side effects! */
	ret = ttm_eu_reserve_buffers(&context->validate_list);

	if (unlikely(ret != 0))
		goto out_err2;

	engine = arg->engine;
	ret = psb_validate_buffer_list(file_priv, engine,
				       context, &po_correct);
	if (unlikely(ret != 0))
		goto out_err3;

	if (!po_correct) {
		ret = psb_fixup_relocs(file_priv, engine, arg->num_relocs,
				       arg->reloc_offset,
				       arg->reloc_handle, context, 0, 1);
		if (unlikely(ret != 0))
			goto out_err3;

	}

	cmd_buffer = ttm_buffer_object_lookup(tfile, arg->cmdbuf_handle);
	if (unlikely(cmd_buffer == NULL)) {
		ret = -EINVAL;
		goto out_err4;
	}

	list_for_each_entry_safe(pos, n, &dev_priv->video_ctx, head) {
		if (pos->filp == file_priv->filp) {
			int entrypoint = pos->ctx_type & 0xff;

			PSB_DEBUG_GENERAL("Video:commands for profile %d, entrypoint %d",
					(pos->ctx_type >> 8), (pos->ctx_type & 0xff));

			if (entrypoint != VAEntrypointEncSlice &&
			    entrypoint != VAEntrypointEncPicture)
				dev_priv->msvdx_ctx = pos;

			break;
		}
	}

	switch (arg->engine) {
	case PSB_ENGINE_VIDEO:
		if (arg->cmdbuf_size == (16 + 32)) {
			/* Identify deblock msg cmdbuf */
			/* according to cmdbuf_size */
			struct ttm_bo_kmap_obj cmd_kmap;
			struct ttm_buffer_object *deblock;
			uint32_t *cmd;
			bool is_iomem;

			/* write regIO BO's address after deblcok msg */
			ret = ttm_bo_kmap(cmd_buffer, 0, 1, &cmd_kmap);
			if (unlikely(ret != 0))
				goto out_err4;
			cmd = (uint32_t *)(ttm_kmap_obj_virtual(&cmd_kmap,
							&is_iomem) + 16);
			deblock = ttm_buffer_object_lookup(tfile,
							   (uint32_t)(*cmd));
			*cmd = (uint32_t)deblock;
			ttm_bo_unref(&deblock); /* FIXME Should move this to interrupt handler? */
			ttm_bo_kunmap(&cmd_kmap);
		}

		ret = psb_cmdbuf_video(file_priv, &context->validate_list,
				       context->fence_types, arg,
				       cmd_buffer, &fence_arg);

		if (unlikely(ret != 0))
			goto out_err4;
		break;
	default:
		DRM_ERROR
		    ("Unimplemented command submission mechanism (%x).\n",
		     arg->engine);
		ret = -EINVAL;
		goto out_err4;
	}

	if (!(arg->fence_flags & DRM_PSB_FENCE_NO_USER)) {
		ret = copy_to_user((void __user *)
				   ((unsigned long) arg->fence_arg),
				   &fence_arg, sizeof(fence_arg));
	}

out_err4:
	if (cmd_buffer)
		ttm_bo_unref(&cmd_buffer);
out_err3:
	ret = psb_handle_copyback(dev, context, ret);
out_err2:
	psb_unreference_buffers(context);
out_err1:
	mutex_unlock(&dev_priv->cmdbuf_mutex);
out_err0:
	ttm_read_unlock(&dev_priv->ttm_lock);

	if (arg->engine == PSB_ENGINE_VIDEO)
		ospm_power_using_hw_end(OSPM_VIDEO_DEC_ISLAND);

	return ret;
}

