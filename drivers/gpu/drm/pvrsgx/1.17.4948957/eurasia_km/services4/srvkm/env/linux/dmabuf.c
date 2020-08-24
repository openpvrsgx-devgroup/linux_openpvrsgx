/*************************************************************************/ /*!
@Title          Dma_buf support code.
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

#include "dmabuf.h"

#if defined(SUPPORT_DMABUF)

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/dma-buf.h>
#include <linux/scatterlist.h>
#include <linux/version.h>
#if defined(SUPPORT_DRI_DRM) && (LINUX_VERSION_CODE < KERNEL_VERSION(5,5,0))
#include <drm/drmP.h>
#endif

#include "services_headers.h"
#include "pvr_debug.h"
#include "linkage.h"
#include "pvr_bridge.h"

struct dmabuf_import
{
	struct dma_buf *dma_buf;

	struct dma_buf_attachment *attachment;

	struct sg_table *sg_table;

#if defined(PDUMP)
	void *kvaddr;
#endif /* defined(PDUMP) */
};

IMG_VOID DmaBufUnimportAndReleasePhysAddr(IMG_HANDLE hImport)
{
	struct dmabuf_import *import;

	import = (struct dmabuf_import *)hImport;

#if defined(PDUMP)
	if (import->kvaddr)
	{
		dma_buf_vunmap(import->dma_buf, import->kvaddr);
		dma_buf_end_cpu_access(import->dma_buf,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
					0, import->dma_buf->size,
#endif
					DMA_BIDIRECTIONAL);
	}
#endif /* defined(PDUMP) */

	if (!IS_ERR_OR_NULL(import->sg_table))
	{
		dma_buf_unmap_attachment(import->attachment,
						import->sg_table,
						DMA_BIDIRECTIONAL);
	}

	if (!IS_ERR_OR_NULL(import->attachment))
	{
		dma_buf_detach(import->dma_buf, import->attachment);
	}

	if (!IS_ERR_OR_NULL(import->dma_buf))
	{
		dma_buf_put(import->dma_buf);
	}

	kfree(import);
}

PVRSRV_ERROR DmaBufImportAndAcquirePhysAddr(const IMG_INT32 i32FD,
											const IMG_SIZE_T uiDmaBufOffset,
											const IMG_SIZE_T uiSize,
											IMG_UINT32 *pui32PageCount,
											IMG_SYS_PHYADDR **ppsSysPhysAddr,
											IMG_SIZE_T *puiMemInfoOffset,
											IMG_PVOID  *ppvKernAddr,
											IMG_HANDLE *phImport,
											IMG_HANDLE *phUnique)
{
	struct dmabuf_import *import = NULL;
	struct device *dev = NULL;
	struct scatterlist *sg;
	size_t buf_size;
	size_t start_offset, end_offset, buf_offset, remainder;
	unsigned npages = 0;
	unsigned pti = 0;
	IMG_SYS_PHYADDR *spaddr = NULL;
	unsigned i;
	PVRSRV_ERROR eError = PVRSRV_ERROR_INVALID_PARAMS;
#if defined(PDUMP)
	int err;
#endif	/* defined(PDUMP) */

	import = kzalloc(sizeof(*import), GFP_KERNEL);
	if (!import)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Out of memory", __func__));
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto error;
	}

	dev = PVRLDMGetDevice();
	if (!dev)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Couldn't get device", __func__));
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		goto error;
	}

	import->dma_buf = dma_buf_get((int)i32FD);
	if (IS_ERR(import->dma_buf))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: dma_buf_get failed: %ld", __func__, PTR_ERR(import->dma_buf)));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto error;
	}

	buf_size = uiSize ? uiSize : import->dma_buf->size;

	if ((uiDmaBufOffset + buf_size) > import->dma_buf->size ||
		(size_t)(uiDmaBufOffset + buf_size) < buf_size)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Bad size and/or offset for FD", __func__));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto error;
	}

	import->attachment = dma_buf_attach(import->dma_buf, dev);
	if (IS_ERR(import->attachment))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: dma_buf_attach failed: %ld", __func__, PTR_ERR(import->attachment)));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto error;
	}

	import->sg_table = dma_buf_map_attachment(import->attachment,
							DMA_BIDIRECTIONAL);
	if (IS_ERR(import->sg_table))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: dma_buf_map_attachment failed: %ld", __func__, PTR_ERR(import->sg_table)));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto error;
	}

	start_offset = PAGE_MASK & uiDmaBufOffset;
	end_offset = PAGE_ALIGN(uiDmaBufOffset + buf_size);

	*puiMemInfoOffset = (uiDmaBufOffset - start_offset);

	npages = (end_offset - start_offset) >> PAGE_SHIFT;

	/* The following allocation will be freed by the caller */
	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
					npages * sizeof(IMG_SYS_PHYADDR),
					(IMG_VOID **)&spaddr, IMG_NULL,
					"Array of Page Addresses");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: OSAllocMem failed: %s", __func__, PVRSRVGetErrorStringKM(eError)));
		goto error;
	}

	buf_offset = 0;
	remainder = buf_size;
	start_offset = PAGE_MASK & uiDmaBufOffset;
	end_offset = PAGE_ALIGN(uiDmaBufOffset + buf_size);

	for_each_sg(import->sg_table->sgl, sg, import->sg_table->nents, i)
	{
		if (buf_offset >= end_offset)
		{
			break;
		}

		if ((start_offset >= buf_offset) && (start_offset < buf_offset + sg_dma_len(sg)))
		{
			size_t sg_start;
			size_t sg_pos;
			size_t sg_remainder;

			sg_start = start_offset - buf_offset;

			sg_remainder = MIN(sg_dma_len(sg) - sg_start, remainder);

			for (sg_pos = sg_start; sg_pos < sg_start + sg_remainder; sg_pos += PAGE_SIZE)
			{
				IMG_CPU_PHYADDR cpaddr;

				cpaddr.uiAddr = PAGE_MASK & (sg_phys(sg) + sg_pos);
				BUG_ON(pti >= npages);

				spaddr[pti++] = SysCpuPAddrToSysPAddr(cpaddr);
			}

			remainder -= sg_remainder;
			buf_offset += sg_dma_len(sg);
			start_offset = buf_offset;
		}
		else
		{
			buf_offset += sg_dma_len(sg);
		}
	}
	BUG_ON(remainder);

#if defined(PDUMP)
	err = dma_buf_begin_cpu_access(import->dma_buf,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
					0, import->dma_buf->size,
#endif
					DMA_BIDIRECTIONAL);
	if (err)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "%s: dma_buf_begin_cpu_access failed: %d", __func__, err));
	}
	else
	{
		import->kvaddr = dma_buf_vmap(import->dma_buf);
		*ppvKernAddr = import->kvaddr;
	}
#else	/* defined(PDUMP) */
	*ppvKernAddr = NULL;
#endif	/* defined(PDUMP) */

	*pui32PageCount = pti;
	*ppsSysPhysAddr = spaddr;
	*phImport = (IMG_HANDLE)import;
	*phUnique = (IMG_HANDLE)import->dma_buf;

	return PVRSRV_OK;
error:
	if (import)
	{
		DmaBufUnimportAndReleasePhysAddr((IMG_HANDLE)import);
	}

	if (spaddr)
	{
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				npages * sizeof(IMG_SYS_PHYADDR),
				(IMG_VOID **)&spaddr, IMG_NULL);
	}

	return eError;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0))
IMG_HANDLE DmaBufGetNativeSyncHandle(IMG_HANDLE hImport)
{
	struct dmabuf_import *import;
	struct dmabuf_resvinfo *info;

	import = (struct dmabuf_import *)hImport;

	info = kzalloc(sizeof(*info), GFP_KERNEL);

	if (info)
	{
		info->resv = import->dma_buf->resv;
	}

	return (IMG_HANDLE)info;
}

void DmaBufFreeNativeSyncHandle(IMG_HANDLE hSync)
{
	struct dmabuf_resvinfo *info;

	info = (struct dmabuf_resvinfo *)hSync;

	kfree(info);
}
#else	/* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0)) */
IMG_HANDLE DmaBufGetNativeSyncHandle(IMG_HANDLE hImport)
{
	(void) hImport;

	return IMG_NULL;
}

void DmaBufFreeNativeSyncHandle(IMG_HANDLE hSync)
{
	(void) hSync;
}
#endif	/* (LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0)) */

#endif /* defined(SUPPORT_DMABUF) */
