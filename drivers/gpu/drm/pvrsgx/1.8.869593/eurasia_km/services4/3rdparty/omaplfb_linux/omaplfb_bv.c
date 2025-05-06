/*
 * BltsVille support in omaplfb
 *
 * Copyright (C) 2012 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include <linux/ion.h>
#include <linux/omap_ion.h>
#include <mach/tiler.h>
#include <linux/gcbv-iface.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "omaplfb.h"

static int debugbv;
extern struct ion_client *gpsIONClient;

/*
 * Are the blit framebuffers in VRAM?
 */
#define OMAPLFB_BLT_FBS_VRAM 1

#define OMAPLFB_COMMAND_COUNT		1

#define	OMAPLFB_VSYNC_SETTLE_COUNT	5

#define	OMAPLFB_MAX_NUM_DEVICES		FB_MAX
#if (OMAPLFB_MAX_NUM_DEVICES > FB_MAX)
#error "OMAPLFB_MAX_NUM_DEVICES must not be greater than FB_MAX"
#endif

static IMG_BOOL gbBvInterfacePresent;
static struct bventry gsBvInterface;

static void print_bvparams(struct bvbltparams *bltparams,
                           unsigned int pSrc1DescInfo, unsigned int pSrc2DescInfo)
{
	struct bvphysdesc *physdesc = NULL;
	if (bltparams->flags & BVFLAG_BLEND)
	{
		printk(KERN_INFO "%s: param %s %x (%s), flags %ld\n",
			"bv", "blend", bltparams->op.blend,
			bltparams->op.blend == BVBLEND_SRC1OVER ? "src1over" : "??",
			bltparams->flags);
	}

	if (bltparams->flags & BVFLAG_ROP)
	{
		printk(KERN_INFO "%s: param %s %x (%s), flags %ld\n",
			"bv", "rop", bltparams->op.rop,
			bltparams->op.rop == 0xCCCC ? "srccopy" : "??",
			bltparams->flags);
	}

	if (bltparams->dstdesc->auxtype == BVAT_PHYSDESC)
		physdesc = bltparams->dstdesc->auxptr;

	printk(KERN_INFO "%s: dst %d,%d rect{%d,%d sz %d,%d}"
		" stride %ld desc 0x%p\n", "bv",
		bltparams->dstgeom->width,
		bltparams->dstgeom->height,
		bltparams->dstrect.left, bltparams->dstrect.top,
		bltparams->dstrect.width, bltparams->dstrect.height,
		bltparams->dstgeom->virtstride,
		physdesc ? physdesc->pagearray : NULL);

	printk(KERN_INFO "%s: src1 %d,%d rect{%d,%d sz %d,%d}"
		" stride %ld, virtaddr 0x%x (0x%x)\n", "bv",
		bltparams->src1geom->width,
		bltparams->src1geom->height, bltparams->src1rect.left,
		bltparams->src1rect.top, bltparams->src1rect.width,
		bltparams->src1rect.height, bltparams->src1geom->virtstride,
		(unsigned int)bltparams->src1.desc->virtaddr, pSrc1DescInfo);

	if (!(bltparams->flags & BVFLAG_BLEND))
		return;

	printk(KERN_INFO "%s: src2 %d,%d rect{%d,%d sz %d,%d}"
		" stride %ld, virtaddr 0x%x (0x%x)\n", "bv",
		bltparams->src2geom->width,
		bltparams->src2geom->height, bltparams->src2rect.left,
		bltparams->src2rect.top, bltparams->src2rect.width,
		bltparams->src2rect.height, bltparams->src2geom->virtstride,
		(unsigned int)bltparams->src2.desc->virtaddr, pSrc2DescInfo);
}

void OMAPLFBGetBltFBsBvHndl(OMAPLFB_FBINFO *psPVRFBInfo, IMG_UINTPTR_T *ppPhysAddr)
{
	if (++psPVRFBInfo->iBltFBsIdx >= OMAPLFB_NUM_BLT_FBS)
	{
		psPVRFBInfo->iBltFBsIdx = 0;
	}
	*ppPhysAddr = psPVRFBInfo->psBltFBsBvPhys[psPVRFBInfo->iBltFBsIdx];
}

static OMAPLFB_ERROR InitBltFBsCommon(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	IMG_INT n = OMAPLFB_NUM_BLT_FBS;

	psPVRFBInfo->psBltFBsNo = n;
	psPVRFBInfo->psBltFBsIonHndl = NULL;
	psPVRFBInfo->psBltFBsBvHndl = kzalloc(n * sizeof(*psPVRFBInfo->psBltFBsBvHndl), GFP_KERNEL);
	if (!psPVRFBInfo->psBltFBsBvHndl)
	{
		return OMAPLFB_ERROR_INIT_FAILURE;
	}

	psPVRFBInfo->psBltFBsBvPhys = kzalloc(n * sizeof(*psPVRFBInfo->psBltFBsBvPhys), GFP_KERNEL);
	if (!psPVRFBInfo->psBltFBsBvPhys)
	{
		return OMAPLFB_ERROR_INIT_FAILURE;
	}
	/* Freeing of resources is handled in deinit code */
	return OMAPLFB_OK;
}

/*
 * Initialize the blit framebuffers and create the Bltsville mappings, these
 * buffers are separate from the swapchain to reduce complexity
 */
static OMAPLFB_ERROR InitBltFBsVram(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	IMG_UINT uiBltFBSize = psDevInfo->sFBInfo.ulHeight * psDevInfo->psLINFBInfo->fix.line_length;
	IMG_UINT uiNumPages = uiBltFBSize >> PAGE_SHIFT;
	IMG_UINT uiFb;

	if (InitBltFBsCommon(psDevInfo) != OMAPLFB_OK)
	{
		return OMAPLFB_ERROR_INIT_FAILURE;
	}
	psPVRFBInfo->uiBltFBsByteStride = psDevInfo->psLINFBInfo->fix.line_length;

	for (uiFb = 0; uiFb < psPVRFBInfo->psBltFBsNo; uiFb++)
	{
		unsigned long *pPaddrs;
		enum bverror iBvErr;
		IMG_UINT j;
		struct bvbuffdesc *pBvDesc;
		struct bvphysdesc *pBvPhysDesc;
		IMG_UINT uiVramStart;
		IMG_UINT uiFbOff;

		pPaddrs = kzalloc(sizeof(*pPaddrs) *
				uiNumPages, GFP_KERNEL);
		if (!pPaddrs)
		{
			return OMAPLFB_ERROR_OUT_OF_MEMORY;
		}

		pBvPhysDesc = kzalloc(sizeof(*pBvPhysDesc), GFP_KERNEL);
		if (!pBvPhysDesc)
		{
			kfree(pPaddrs);
			return OMAPLFB_ERROR_OUT_OF_MEMORY;
		}

		pBvDesc = kzalloc(sizeof(*pBvDesc), GFP_KERNEL);
		if (!pBvDesc)
		{
			kfree(pPaddrs);
			kfree(pBvPhysDesc);
			return OMAPLFB_ERROR_OUT_OF_MEMORY;
		}
		/*
		 * Handle the swapchain buffers being located in TILER2D or in
		 * VRAM
		 */
		uiFbOff = psPVRFBInfo->bIs2D ? 0 :
					psDevInfo->sFBInfo.ulRoundedBufferSize *
					psDevInfo->psSwapChain->ulBufferCount;
		uiVramStart = psDevInfo->psLINFBInfo->fix.smem_start + uiFbOff +
					(uiBltFBSize * uiFb);

		for(j = 0; j < uiNumPages; j++)
		{
			pPaddrs[j] = uiVramStart + (j * PAGE_SIZE);
		}
		psPVRFBInfo->psBltFBsBvPhys[uiFb] = pPaddrs[0];

		pBvDesc->structsize = sizeof(*pBvDesc);
		pBvDesc->auxtype = BVAT_PHYSDESC;
		pBvPhysDesc->pagesize = PAGE_SIZE;
		pBvPhysDesc->pagearray = pPaddrs;
		pBvPhysDesc->pagecount = uiNumPages;
		pBvDesc->length = pBvPhysDesc->pagecount * pBvPhysDesc->pagesize;
		pBvDesc->auxptr = pBvPhysDesc;

		iBvErr = gsBvInterface.bv_map(pBvDesc);
		if (iBvErr)
		{
			WARN(1, "%s: BV map Blt FB buffer failed %d\n",
					__func__, iBvErr);
			kfree(pBvDesc);
			kfree(pBvPhysDesc);
			kfree(pPaddrs);
			return OMAPLFB_ERROR_GENERIC;
		}
		psPVRFBInfo->psBltFBsBvHndl[uiFb] = pBvDesc;
		kfree(pPaddrs);
		kfree(pBvPhysDesc);
	}
	return OMAPLFB_OK;
}

static PVRSRV_ERROR InitBltFBsMapTiler2D(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	struct bvbuffdesc *pBvDesc;
	struct bvphysdesc *pBvPhysDesc;
	struct bventry *pBvEntry;
	enum bverror eBvErr;
	int iFB;
	ion_phys_addr_t phys;
	size_t size;
	int res = PVRSRV_OK;

	pBvEntry = &gsBvInterface;
	ion_phys(gpsIONClient, psPVRFBInfo->psBltFBsIonHndl, &phys, &size);

	for (iFB = 0; iFB < psPVRFBInfo->psBltFBsNo; iFB++)
	{
		unsigned long *pPageList;

		struct tiler_view_t view;
		int wpages = psPVRFBInfo->uiBltFBsByteStride >> PAGE_SHIFT;
		int h = psLINFBInfo->var.yres;
		int x, y;

		phys += psPVRFBInfo->uiBltFBsByteStride * iFB;
		pPageList = kzalloc(
				wpages * h * sizeof(*pPageList),
		                GFP_KERNEL);
		if ( !pPageList) {
			printk(KERN_WARNING DRIVER_PREFIX
					": %s: Could not allocate page list\n",
					__FUNCTION__);
			return OMAPLFB_ERROR_INIT_FAILURE;
		}
		tilview_create(&view, phys, psLINFBInfo->var.xres, h);
		for (y = 0; y < h; y++) {
			for (x = 0; x < wpages; x++) {
				pPageList[y * wpages + x] = phys + view.v_inc * y
						+ (x << PAGE_SHIFT);
			}
		}
		pBvDesc = kzalloc(sizeof(*pBvDesc), GFP_KERNEL);
		pBvDesc->structsize = sizeof(*pBvDesc);
		pBvDesc->auxtype = BVAT_PHYSDESC;

		pBvPhysDesc = kzalloc(sizeof(*pBvPhysDesc), GFP_KERNEL);
		pBvPhysDesc->pagesize = PAGE_SIZE;
		pBvPhysDesc->pagearray = pPageList;
		pBvPhysDesc->pagecount = wpages * h;

		pBvDesc->auxptr = pBvPhysDesc;

		eBvErr = pBvEntry->bv_map(pBvDesc);

		pBvPhysDesc->pagearray = NULL;

		if (eBvErr)
		{
			WARN(1, "%s: BV map blt buffer failed %d\n",__func__, eBvErr);
			psPVRFBInfo->psBltFBsBvHndl[iFB]= NULL;
			kfree(pBvDesc);
			kfree(pBvPhysDesc);
			res = PVRSRV_ERROR_OUT_OF_MEMORY;
		}
		else
		{
			psPVRFBInfo->psBltFBsBvHndl[iFB] = pBvDesc;
			psPVRFBInfo->psBltFBsBvPhys[iFB] = pPageList[0];
		}
		kfree(pPageList);
	}

	return res;
}

/*
 * Allocate buffers from the blit 'framebuffers'. These buffers are not shared
 * with the SGX flip chain to reduce complexity
 */
static OMAPLFB_ERROR InitBltFBsTiler2D(OMAPLFB_DEVINFO *psDevInfo)
{
	/*
	 * Pick up the calculated bytes per pixel from the deduced
	 * OMAPLFB_FBINFO, get the rest of the display parameters from the
	 * struct fb_info
	 */
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	int res, w, h;

	struct omap_ion_tiler_alloc_data sAllocData = {
		.fmt = psPVRFBInfo->uiBytesPerPixel == 2 ? TILER_PIXEL_FMT_16BIT : TILER_PIXEL_FMT_32BIT,
		.flags = 0,
	};

	if (InitBltFBsCommon(psDevInfo) != OMAPLFB_OK)
	{
		return OMAPLFB_ERROR_INIT_FAILURE;
	}
	psPVRFBInfo->uiBltFBsByteStride = PAGE_ALIGN(psLINFBInfo->var.xres * psPVRFBInfo->uiBytesPerPixel);

	/* TILER will align width to 128-bytes */
	/* however, SGX must have full page width */
	w = ALIGN(psLINFBInfo->var.xres, PAGE_SIZE / psPVRFBInfo->uiBytesPerPixel);
	h = psLINFBInfo->var.yres;
	sAllocData.h = h;
	sAllocData.w = psPVRFBInfo->psBltFBsNo * w;

	printk(KERN_INFO DRIVER_PREFIX
		":BltFBs alloc %d x (%d x %d) [stride %d]\n",
		psPVRFBInfo->psBltFBsNo, w, h, psPVRFBInfo->uiBltFBsByteStride);
	res = omap_ion_nonsecure_tiler_alloc(gpsIONClient, &sAllocData);
	if (res < 0)
	{
		printk(KERN_ERR DRIVER_PREFIX
			"Could not allocate BltFBs\n");
		return OMAPLFB_ERROR_INIT_FAILURE;
	}

	psPVRFBInfo->psBltFBsIonHndl = sAllocData.handle;

	res = InitBltFBsMapTiler2D(psDevInfo);
	if (res != OMAPLFB_OK)
	{
		return OMAPLFB_ERROR_INIT_FAILURE;
	}
	return OMAPLFB_OK;
}

static struct bvbuffdesc *GetBvDescriptor(OMAPLFB_DEVINFO *psDevInfo, PDC_MEM_INFO *ppsMemInfos, IMG_UINT32 ui32Idx, IMG_UINT32 ui32NumMemInfos)
{
	struct bvbuffdesc *pBvDesc;
	if (!meminfo_idx_valid(ui32Idx, ui32NumMemInfos)) {
		WARN(1, "%s: index out of range\n", __func__);
		return NULL;
	}

	psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetBvHandle(ppsMemInfos[ui32Idx], (IMG_VOID**)&pBvDesc);
	WARN(!pBvDesc, "%s: null handle\n", __func__);
	return pBvDesc;
}

static int bvrect_same_origin(struct bvrect *rect1, struct bvrect *rect2)
{
	return rect1->left == rect2->left && rect1->top == rect2->top;
}

static int bvrect_same_size(struct bvrect *rect1, struct bvrect *rect2)
{
	return rect1->width == rect2->width && rect1->height == rect2->height;
}

static void OMAPLFBDoBatching(struct rgz_blt_entry *prev_entry,
	struct rgz_blt_entry *cur_entry, int is_last_blit)
{
	struct bvbltparams *cur_bltparams = &cur_entry->bp;
	struct bvbltparams *prev_bltparams;

	/* Override any batch flags coming from HWC */
	cur_bltparams->flags &= ~BVFLAG_BATCH_MASK;

	if (!prev_entry)
	{
		cur_bltparams->flags |= BVFLAG_BATCH_BEGIN;
		return;
	}
	else if (is_last_blit)
	{
		cur_bltparams->flags |= BVFLAG_BATCH_END;
	}
	else
	{
		cur_bltparams->flags |= BVFLAG_BATCH_CONTINUE;
	}

	/* Ommited BVBATCH_DST since the destination is always the same
	 * framebuffer descriptor per frame. On the first blit we don't
	 * need to set all flags but the delta on consequent blits.
	 */
	prev_bltparams = &prev_entry->bp;
	cur_bltparams->batch = prev_bltparams->batch;
	cur_bltparams->batchflags =
		(prev_bltparams->src1.desc == cur_bltparams->src1.desc ? 0 : BVBATCH_SRC1) |
		(prev_bltparams->src2.desc == cur_bltparams->src2.desc ? 0 : BVBATCH_SRC2) |
		(bvrect_same_origin(&prev_bltparams->src1rect, &cur_bltparams->src1rect) ? 0 : BVBATCH_SRC1RECT_ORIGIN) |
		(bvrect_same_origin(&prev_bltparams->src2rect, &cur_bltparams->src2rect) ? 0 : BVBATCH_SRC2RECT_ORIGIN) |
		(bvrect_same_origin(&prev_bltparams->dstrect, &cur_bltparams->dstrect) ? 0 : BVBATCH_DSTRECT_ORIGIN) |
		(bvrect_same_size(&prev_bltparams->src1rect, &cur_bltparams->src1rect) ? 0 : BVBATCH_SRC1RECT_SIZE) |
		(bvrect_same_size(&prev_bltparams->src2rect, &cur_bltparams->src2rect) ? 0 : BVBATCH_SRC2RECT_SIZE) |
		(bvrect_same_size(&prev_bltparams->dstrect, &cur_bltparams->dstrect) ? 0 : BVBATCH_DSTRECT_SIZE);
}

void OMAPLFBDoBlits(OMAPLFB_DEVINFO *psDevInfo, PDC_MEM_INFO *ppsMemInfos, struct omap_hwc_blit_data *blit_data, IMG_UINT32 ui32NumMemInfos)
{
	struct rgz_blt_entry *entry_list;
	struct bventry *bv_entry = &gsBvInterface;
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	int rgz_items = blit_data->rgz_items;
	int j;
	struct rgz_blt_entry *prev_entry = NULL;

	/* DSS pipes are setup up to this point, we can begin blitting here */
	entry_list = (struct rgz_blt_entry *) (blit_data->rgz_blts);
	for (j = 0; j < rgz_items; j++)
	{
		struct rgz_blt_entry *entry = &entry_list[j];
		enum bverror bv_error = 0;
		unsigned int meminfo_ix;
		unsigned int iSrc1DescInfo = 0, iSrc2DescInfo = 0;

		/* Destination data */
		entry->dstgeom.virtstride = psDevInfo->sFBInfo.uiBltFBsByteStride;
		entry->bp.dstdesc = psPVRFBInfo->psBltFBsBvHndl[psPVRFBInfo->iBltFBsIdx];
		entry->bp.dstgeom = &entry->dstgeom;

		/* Src1 buffer data */
		meminfo_ix = (unsigned int)entry->src1desc.auxptr;
		if (meminfo_ix == -1)
		{
			/* Making fill with transparent pixels, avoid mapping
			 * src1 this will change when the HWC starts to use
			 * HWC_BLT_FLAG_CLR
			 */
			static unsigned int pixel = 0;
			entry->bp.src1geom = &entry->src1geom;
			entry->bp.src1.desc = &entry->src1desc;
			entry->bp.src1.desc->virtaddr = (void*)&pixel;
		}
		else if (meminfo_ix & HWC_BLT_DESC_FLAG)
		{
			/* This code might be redundant? Do we want to ever blit out of the framebuffer? */
			if (meminfo_ix & HWC_BLT_DESC_FB)
			{
				entry->bp.src1.desc = entry->bp.dstdesc;
				entry->bp.src1geom = entry->bp.dstgeom;
			}
			else
			{
				WARN(1, "%s: Unable to determine scr1 buffer\n",
						__func__);
				continue;
			}
		}
		else
		{
			struct bvbuffdesc *pBvDesc;
			pBvDesc = GetBvDescriptor(psDevInfo, ppsMemInfos,
					meminfo_ix, ui32NumMemInfos);
			if (!pBvDesc)
				continue;

			entry->bp.src1.desc = pBvDesc;
			entry->bp.src1geom = &entry->src1geom;
		}

		/* Src2 buffer data
		 * Check if this blit involves src2 as the FB or another
		 * buffer, if the last case is true then map the src2 buffer
		 */
		if (entry->bp.flags & BVFLAG_BLEND)
		{
			meminfo_ix = (unsigned int)entry->src2desc.auxptr;
			if (meminfo_ix & HWC_BLT_DESC_FLAG)
			{
				if (meminfo_ix & HWC_BLT_DESC_FB)
				{
					/* Blending with destination (FB) */
					entry->bp.src2.desc = entry->bp.dstdesc;
					entry->bp.src2geom = entry->bp.dstgeom;
				}
				else
				{
					WARN(1, "%s: Unable to determine scr2 buffer\n",
							__func__);
					continue;
				}
			}
			else
			{
				struct bvbuffdesc *pBvDesc;
				pBvDesc = GetBvDescriptor(psDevInfo,
						ppsMemInfos, meminfo_ix,
						ui32NumMemInfos);
				if (!pBvDesc)
					continue;

				entry->bp.src2.desc = pBvDesc;
				entry->bp.src2geom = &entry->src2geom;
			}
		}
		else
		{
			entry->bp.src2.desc = NULL;
		}

		/* Use batching when there is more than one blit per frame */
		if (rgz_items > 1)
		{
			OMAPLFBDoBatching(prev_entry, entry, j >= rgz_items - 1);
			prev_entry = entry;
		}

		if (debugbv)
		{
			iSrc1DescInfo = (unsigned int)entry->src1desc.auxptr;
			iSrc2DescInfo = (unsigned int)entry->src2desc.auxptr;
			print_bvparams(&entry->bp, iSrc1DescInfo, iSrc2DescInfo);
		}

		bv_error = bv_entry->bv_blt(&entry->bp);
		if (bv_error)
			printk(KERN_ERR "%s: blit failed %d\n",
					__func__, bv_error);
	}
}

OMAPLFB_ERROR OMAPLFBInitBltFBs(OMAPLFB_DEVINFO *psDevInfo)
{
	return (OMAPLFB_BLT_FBS_VRAM) ? InitBltFBsVram(psDevInfo) : InitBltFBsTiler2D(psDevInfo);
}

void OMAPLFBDeInitBltFBs(OMAPLFB_DEVINFO *psDevInfo)
{
	struct bventry *bv_entry = &gsBvInterface;
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	struct bvbuffdesc *pBufDesc;

	if (!gbBvInterfacePresent)
	{
		return;
	}

	if (psPVRFBInfo->psBltFBsBvHndl)
	{
		IMG_INT i;
		for (i = 0; i < psPVRFBInfo->psBltFBsNo; i++)
		{
			pBufDesc = psPVRFBInfo->psBltFBsBvHndl[i];
			if (pBufDesc)
			{
				bv_entry->bv_unmap(pBufDesc);
				kfree(pBufDesc);
			}
		}
	}
	kfree(psPVRFBInfo->psBltFBsBvHndl);

	if (psPVRFBInfo->psBltFBsIonHndl)
	{
		ion_free(gpsIONClient, psPVRFBInfo->psBltFBsIonHndl);
	}
}

IMG_BOOL OMAPLFBInitBlt(void)
{
#if defined(CONFIG_GCBV)
	/* Get the GC2D Bltsville implementation */
	gcbv_init(&gsBvInterface);
	gbBvInterfacePresent = gsBvInterface.bv_map ? IMG_TRUE : IMG_FALSE;
#else
	gbBvInterfacePresent = IMG_FALSE;
#endif
	return gbBvInterfacePresent;
}
