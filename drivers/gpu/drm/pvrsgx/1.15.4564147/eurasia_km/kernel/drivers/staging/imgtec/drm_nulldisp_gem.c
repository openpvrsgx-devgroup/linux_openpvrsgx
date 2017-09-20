/* -*- mode: c; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* vi: set ts=8 sw=8 sts=8: */
/*************************************************************************/ /*!
@File
@Codingstyle    LinuxKernel
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#include <linux/atomic.h>
#include <linux/mm_types.h>
#include <linux/dma-buf.h>
#include <linux/reservation.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/mutex.h>
#include <linux/capability.h>

#include "drm_nulldisp_gem.h"

struct nulldisp_gem_object {
	struct drm_gem_object base;

	atomic_t pg_refcnt;
	struct page **pages;
	dma_addr_t *addrs;

	struct reservation_object _resv;
	struct reservation_object *resv;
};

#define to_nulldisp_obj(obj) \
	container_of(obj, struct nulldisp_gem_object, base)

struct page **nulldisp_gem_object_get_pages(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;
	struct nulldisp_gem_object *nulldisp_obj = to_nulldisp_obj(obj);
	struct page **pages;

	WARN_ON(!mutex_is_locked(&dev->struct_mutex));

	if (atomic_inc_return(&nulldisp_obj->pg_refcnt) == 1) {
		unsigned npages = obj->size >> PAGE_SHIFT;
		dma_addr_t *addrs;
		unsigned i;

		pages = drm_gem_get_pages(obj);
		if (IS_ERR(pages))
			goto dec_refcnt;

		addrs = kmalloc(npages * sizeof(*addrs), GFP_KERNEL);
		if (!addrs) {
			pages = ERR_PTR(-ENOMEM);
			goto free_pages;
		}

		for (i = 0; i < npages; i++) {
			addrs[i] = dma_map_page(dev->dev, pages[i],
					0, PAGE_SIZE, DMA_BIDIRECTIONAL);
		}

		nulldisp_obj->pages = pages;
		nulldisp_obj->addrs = addrs;
	}

	return nulldisp_obj->pages;

free_pages:
	drm_gem_put_pages(obj, pages, false, false);
dec_refcnt:
	atomic_dec(&nulldisp_obj->pg_refcnt);
	return pages;
}

static void nulldisp_gem_object_put_pages(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;
	struct nulldisp_gem_object *nulldisp_obj = to_nulldisp_obj(obj);

	WARN_ON(!mutex_is_locked(&dev->struct_mutex));

	if (WARN_ON(atomic_read(&nulldisp_obj->pg_refcnt) == 0))
		return;

	if (atomic_dec_and_test(&nulldisp_obj->pg_refcnt)) {
		unsigned npages = obj->size >> PAGE_SHIFT;
		unsigned i;

		for (i = 0; i < npages; i++) {
			dma_unmap_page(dev->dev, nulldisp_obj->addrs[i],
				PAGE_SIZE, DMA_BIDIRECTIONAL);
		}

		kfree(nulldisp_obj->addrs);
		nulldisp_obj->addrs = NULL;

		drm_gem_put_pages(obj, nulldisp_obj->pages, true, true);
		nulldisp_obj->pages = NULL;
	}
}

int nulldisp_gem_object_vm_fault(struct vm_area_struct *vma,
					struct vm_fault *vmf)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	struct nulldisp_gem_object *nulldisp_obj = to_nulldisp_obj(obj);
	unsigned long addr = (unsigned long)vmf->virtual_address;
	unsigned long pg_off;
	struct page *page;

	/*
	 * nulldisp_gem_object_get_pages should have been called in
	 * nulldisp_gem_mmap so there's no need to do it here.
	 */
	if (WARN_ON(atomic_read(&nulldisp_obj->pg_refcnt) == 0))
		return VM_FAULT_SIGBUS;

	pg_off = (addr - vma->vm_start) >> PAGE_SHIFT;
	page = nulldisp_obj->pages[pg_off];

	get_page(page);
	vmf->page = page;

	return 0;
}

void nulldisp_gem_vm_open(struct vm_area_struct *vma)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	struct drm_device *dev = obj->dev;

	drm_gem_vm_open(vma);

	mutex_lock(&dev->struct_mutex);
	(void) nulldisp_gem_object_get_pages(obj);
	mutex_unlock(&dev->struct_mutex);
}

void nulldisp_gem_vm_close(struct vm_area_struct *vma)
{
	struct drm_gem_object *obj = vma->vm_private_data;
	struct drm_device *dev = obj->dev;

	mutex_lock(&dev->struct_mutex);
	(void) nulldisp_gem_object_put_pages(obj);
	mutex_unlock(&dev->struct_mutex);

	drm_gem_vm_close(vma);
}

void nulldisp_gem_object_free(struct drm_gem_object *obj)
{
	struct nulldisp_gem_object *nulldisp_obj = to_nulldisp_obj(obj);

	WARN_ON(atomic_read(&nulldisp_obj->pg_refcnt) != 0);

	drm_gem_free_mmap_offset(obj);

	reservation_object_fini(nulldisp_obj->resv);

	kfree(nulldisp_obj);
}

int nulldisp_gem_prime_pin(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;
	struct page **pages;

	mutex_lock(&dev->struct_mutex);
	pages = nulldisp_gem_object_get_pages(obj);
	mutex_unlock(&dev->struct_mutex);

	return IS_ERR(pages) ? PTR_ERR(pages) : 0;
}

void nulldisp_gem_prime_unpin(struct drm_gem_object *obj)
{
	struct drm_device *dev = obj->dev;

	mutex_lock(&dev->struct_mutex);
	nulldisp_gem_object_put_pages(obj);
	mutex_unlock(&dev->struct_mutex);
}

struct sg_table *
nulldisp_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct nulldisp_gem_object *nulldisp_obj = to_nulldisp_obj(obj);
	int nr_pages = obj->size >> PAGE_SHIFT;

	/*
	 * nulldisp_gem_prime_pin should have been called in which case we don't
	 * need to call nulldisp_gem_object_get_pages.
	 */
	if (WARN_ON(atomic_read(&nulldisp_obj->pg_refcnt) == 0))
		return NULL;

	return drm_prime_pages_to_sg(nulldisp_obj->pages, nr_pages);
}

struct drm_gem_object *
nulldisp_gem_prime_import_sg_table(struct drm_device *dev,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
				   struct dma_buf_attachment *attach,
#else
				   size_t size,
#endif
				   struct sg_table *sgt)
{
	/* No support for importing dma-bufs from other devices or drivers */
	return NULL;
}

struct dma_buf *nulldisp_gem_prime_export(struct drm_device *dev,
						 struct drm_gem_object *obj,
						 int flags)
{
	/* Read/write access required */
	flags |= O_RDWR;
	return drm_gem_prime_export(dev, obj, flags);
}

void *nulldisp_gem_prime_vmap(struct drm_gem_object *obj)
{
	struct nulldisp_gem_object *nulldisp_obj = to_nulldisp_obj(obj);
	int nr_pages = obj->size >> PAGE_SHIFT;

	/*
	 * nulldisp_gem_prime_pin should have been called in which case we don't
	 * need to call nulldisp_gem_object_get_pages.
	 */
	if (WARN_ON(atomic_read(&nulldisp_obj->pg_refcnt) == 0))
		return NULL;


	return vmap(nulldisp_obj->pages, nr_pages, 0, PAGE_KERNEL);
}

void nulldisp_gem_prime_vunmap(struct drm_gem_object *obj, void *vaddr)
{
	vunmap(vaddr);
}

int nulldisp_gem_prime_mmap(struct drm_gem_object *obj,
				   struct vm_area_struct *vma)
{
	int err;

	mutex_lock(&obj->dev->struct_mutex);
	(void) nulldisp_gem_object_get_pages(obj);
	err = drm_gem_mmap_obj(obj, obj->size, vma);
	mutex_unlock(&obj->dev->struct_mutex);

	return err;
}

struct reservation_object *
nulldisp_gem_prime_res_obj(struct drm_gem_object *obj)
{
	struct nulldisp_gem_object *nulldisp_obj = to_nulldisp_obj(obj);

	return nulldisp_obj->resv;
}

int nulldisp_gem_dumb_create(struct drm_file *file,
				    struct drm_device *dev,
				    struct drm_mode_create_dumb *args)
{
	struct nulldisp_gem_object *nulldisp_obj;
	struct drm_gem_object *obj;
	struct address_space *mapping;
	u32 handle;
	u32 pitch;
	size_t size;
	int err;

	pitch = args->width * (ALIGN(args->bpp, 8) >> 3);
	size = PAGE_ALIGN(pitch * args->height);

	nulldisp_obj = kzalloc(sizeof(*nulldisp_obj), GFP_KERNEL);
	if (!nulldisp_obj)
		return -ENOMEM;
	nulldisp_obj->resv = &nulldisp_obj->_resv;
	obj = &nulldisp_obj->base;

	err = drm_gem_object_init(dev, obj, size);
	if (err) {
		kfree(nulldisp_obj);
		return err;
	}

	mapping = file_inode(obj->filp)->i_mapping;
	mapping_set_gfp_mask(mapping, GFP_USER | __GFP_DMA32);

	err = drm_gem_handle_create(file, obj, &handle);
	if (err)
		goto exit;

	reservation_object_init(nulldisp_obj->resv);

	args->handle = handle;
	args->pitch = pitch;
	args->size = size;
exit:
	drm_gem_object_unreference_unlocked(obj);
	return err;
}

int nulldisp_gem_dumb_map_offset(struct drm_file *file,
					struct drm_device *dev,
					uint32_t handle,
					uint64_t *offset)
{
	struct drm_gem_object *obj;
	int err;

	mutex_lock(&dev->struct_mutex);

	obj = drm_gem_object_lookup(dev, file, handle);
	if (!obj) {
		err = -ENOENT;
		goto exit_unlock;
	}

	err = drm_gem_create_mmap_offset(obj);
	if (err)
		goto exit_obj_unref;

	*offset = drm_vma_node_offset_addr(&obj->vma_node);

exit_obj_unref:
	drm_gem_object_unreference_unlocked(obj);
exit_unlock:
	mutex_unlock(&dev->struct_mutex);
	return err;
}

struct reservation_object *nulldisp_gem_get_resv(struct drm_gem_object *obj)
{
	return (to_nulldisp_obj(obj)->resv);
}
