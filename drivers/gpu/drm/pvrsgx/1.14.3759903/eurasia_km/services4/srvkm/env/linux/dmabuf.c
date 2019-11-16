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
#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#endif

#include "services_headers.h"
#include "pvr_debug.h"
#include "linkage.h"

struct dmabuf_import
{
	struct dma_buf *dma_buf;

	struct dma_buf_attachment *attachment;

	struct sg_table *sg_table;

#if defined(PDUMP)
	void *kvaddr;
#endif /* defined(PDUMP) */
};

PVRSRV_ERROR DmaBufImportAndAcquirePhysAddr(IMG_INT32 i32DmaBufFD,
											   IMG_UINT32 *pui32PageCount,
											   IMG_SYS_PHYADDR **ppsSysPhysAddr,
											   IMG_PVOID  *ppvKernAddr,
											   IMG_HANDLE *phPriv,
											   IMG_HANDLE *phUnique)
{
	struct dmabuf_import *import;
	struct device *dev = NULL;
	struct scatterlist *sg;
	unsigned npages = 0;
	unsigned pti = 0;
	IMG_SYS_PHYADDR *spaddr;
	unsigned i;
	PVRSRV_ERROR eError = PVRSRV_ERROR_INVALID_PARAMS;

	import = kzalloc(sizeof(*import), GFP_KERNEL);
	if (!import)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto error_import_alloc;
	}

	dev = PVRLDMGetDevice();
	if (!dev)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Couldn't get device", __func__));
		eError = PVRSRV_ERROR_NOT_SUPPORTED;
		goto error_bad_dev;
	}

	import->dma_buf = dma_buf_get((int)i32DmaBufFD);
	if (IS_ERR(import->dma_buf))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: dma_buf_get failed: %ld", __func__, PTR_ERR(import->dma_buf)));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto error_bad_fd;
	}

	import->attachment = dma_buf_attach(import->dma_buf, dev);
	if (IS_ERR(import->attachment))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: dma_buf_attach failed: %ld", __func__, PTR_ERR(import->dma_buf)));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto error_attach_failed;
	}

	import->sg_table = dma_buf_map_attachment(import->attachment,
							DMA_BIDIRECTIONAL);
	if (IS_ERR(import->sg_table))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: dma_buf_map_attachment failed: %ld", __func__, PTR_ERR(import->dma_buf)));
		eError = PVRSRV_ERROR_BAD_MAPPING;
		goto error_map_attachment_failed;
	}

	for_each_sg(import->sg_table->sgl, sg, import->sg_table->nents, i)
	{
		npages += PAGE_ALIGN(sg_dma_len(sg) / PAGE_SIZE);
	}

       /* The following allocation will be freed by the caller */
        eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
                        npages * sizeof(IMG_SYS_PHYADDR),
                        (IMG_VOID **)&spaddr, IMG_NULL,
                        "Array of Page Addresses");
	if (eError != PVRSRV_OK)
        {
		PVR_DPF((PVR_DBG_ERROR, "%s: OSAllocMem failed: %s", __func__, PVRSRVGetErrorStringKM(eError)));
		goto error_address_table_alloc_failed;
        }

	for_each_sg(import->sg_table->sgl, sg, import->sg_table->nents, i)
	{
		unsigned j;

		for (j = 0; j < sg_dma_len(sg); j += PAGE_SIZE)
		{
			IMG_CPU_PHYADDR cpaddr;

			cpaddr.uiAddr = sg_phys(sg) + j;

			BUG_ON(pti >= npages);
			spaddr[pti++] = SysCpuPAddrToSysPAddr(cpaddr);
		}
	}

#if defined(PDUMP)
	{
		int err = dma_buf_begin_cpu_access(import->dma_buf, 0,
						import->dma_buf->size,
						DMA_BIDIRECTIONAL);
		if (err)
		{
			PVR_DPF((PVR_DBG_MESSAGE, "%s: dma_buf_begin_cpu_access failed: %d", __func__, err));
		}
		else
		{
			import->kvaddr = dma_buf_vmap(import->dma_buf);
		}
	}
#endif /* defined(PDUMP) */

	*pui32PageCount = pti;
	*ppsSysPhysAddr = spaddr;
	*phPriv = (IMG_HANDLE)import;
	*phUnique = (IMG_HANDLE)import->dma_buf;

	return PVRSRV_OK;

error_address_table_alloc_failed:
	dma_buf_unmap_attachment(import->attachment, import->sg_table,
							DMA_BIDIRECTIONAL);
error_map_attachment_failed:
	dma_buf_detach(import->dma_buf, import->attachment);
error_attach_failed:
	dma_buf_put(import->dma_buf);
error_bad_fd:
error_bad_dev:
	kfree(import);
error_import_alloc:
	return eError;
}

IMG_VOID DmaBufUnimportAndReleasePhysAddr(IMG_HANDLE hPriv)
{
	struct dmabuf_import *import = (struct dmabuf_import *)hPriv;

#if defined(PDUMP)
	if (import->kvaddr)
	{
		dma_buf_vunmap(import->dma_buf, import->kvaddr);
		dma_buf_end_cpu_access(import->dma_buf, 0,
					import->dma_buf->size,
					DMA_BIDIRECTIONAL);
	}
#endif /* defined(PDUMP) */

	dma_buf_unmap_attachment(import->attachment, import->sg_table,
							DMA_BIDIRECTIONAL);
	dma_buf_detach(import->dma_buf, import->attachment);
	dma_buf_put(import->dma_buf);
	kfree(import);
}

#endif /* defined(SUPPORT_DMABUF) */
