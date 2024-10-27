/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>

#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#endif

#if defined(DC_NOHW_DISCONTIG_BUFFERS)
#include <linux/vmalloc.h>
#include <asm/page.h>
#else
#include <asm/dma-mapping.h>
#endif

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "dc_nohw.h"
#include "pvrmodule.h"

#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#endif

#if defined(DC_USE_SET_MEMORY)
	#undef	DC_USE_SET_MEMORY
#endif

#if !defined(DC_NOHW_DISCONTIG_BUFFERS)
	#if defined(__i386__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)) && defined(SUPPORT_LINUX_X86_PAT) && defined(SUPPORT_LINUX_X86_WRITECOMBINE)
		#include <asm/cacheflush.h>
		#define	DC_USE_SET_MEMORY
	#endif
#endif	

#define DRVNAME "dcnohw"

#if !defined(SUPPORT_DRI_DRM)
MODULE_SUPPORTED_DEVICE(DRVNAME);
#endif

#define unref__ __attribute__ ((unused))

#if defined(DC_NOHW_GET_BUFFER_DIMENSIONS)
static unsigned long width = DC_NOHW_BUFFER_WIDTH;
static unsigned long height = DC_NOHW_BUFFER_HEIGHT;
static unsigned long depth = DC_NOHW_BUFFER_BIT_DEPTH;

module_param(width, ulong, S_IRUGO);
module_param(height, ulong, S_IRUGO);
module_param(depth, ulong, S_IRUGO);

IMG_BOOL GetBufferDimensions(IMG_UINT32 *pui32Width, IMG_UINT32 *pui32Height, PVRSRV_PIXEL_FORMAT *pePixelFormat, IMG_UINT32 *pui32Stride)
{
	if (width == 0 || height == 0 || depth == 0 ||
		depth != dc_nohw_roundup_bit_depth(depth))
	{
		printk(KERN_WARNING DRVNAME ": Illegal module parameters (width %lu, height %lu, depth %lu)\n", width, height, depth);
		return IMG_FALSE;
	}

	*pui32Width = (IMG_UINT32)width;
	*pui32Height = (IMG_UINT32)height;

	switch(depth)
	{
		case 32:
			*pePixelFormat = PVRSRV_PIXEL_FORMAT_ARGB8888;
			break;
		case 16:
			*pePixelFormat = PVRSRV_PIXEL_FORMAT_RGB565;
			break;
		default:
			printk(KERN_WARNING DRVNAME ": Display depth %lu not supported\n", depth);
			*pePixelFormat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
			return IMG_FALSE;
	}
			
	*pui32Stride = dc_nohw_byte_stride(width, depth);

#if defined(DEBUG)
	printk(KERN_INFO DRVNAME " Width: %lu\n", (unsigned long)*pui32Width);
	printk(KERN_INFO DRVNAME " Height: %lu\n", (unsigned long)*pui32Height);
	printk(KERN_INFO DRVNAME " Depth: %lu bits\n", depth);
	printk(KERN_INFO DRVNAME " Stride: %lu bytes\n", (unsigned long)*pui32Stride);
#endif	

	return IMG_TRUE;
}
#endif	

#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device unref__ *dev)
#else
static int __init DC_NOHW_Init(void)
#endif
{
	if(Init() != DC_OK)
	{
		return -ENODEV;
	}

	return 0;
} 

#if defined(SUPPORT_DRI_DRM)
void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device unref__ *dev)
#else
static void __exit DC_NOHW_Cleanup(void)
#endif
{
	if(Deinit() != DC_OK)
	{
		printk (KERN_INFO DRVNAME ": DC_NOHW_Cleanup: can't deinit device\n");
	}
} 


void *AllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void FreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

#if defined(DC_NOHW_DISCONTIG_BUFFERS)

#define RANGE_TO_PAGES(range) (((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)
#define	VMALLOC_TO_PAGE_PHYS(vAddr) page_to_phys(vmalloc_to_page(vAddr))

DC_ERROR AllocDiscontigMemory(unsigned long ulSize,
                              DC_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **ppPhysAddr)
{
	unsigned long ulPages = RANGE_TO_PAGES(ulSize);
	IMG_SYS_PHYADDR *pPhysAddr;
	unsigned long ulPage;
	IMG_CPU_VIRTADDR LinAddr;

	LinAddr = __vmalloc(ulSize, GFP_KERNEL | __GFP_HIGHMEM, pgprot_noncached(PAGE_KERNEL));
	if (!LinAddr)
	{
		return DC_ERROR_OUT_OF_MEMORY;
	}

	pPhysAddr = kmalloc(ulPages * sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
	if (!pPhysAddr)
	{
		vfree(LinAddr);
		return DC_ERROR_OUT_OF_MEMORY;
	}

	*pLinAddr = LinAddr;

	for (ulPage = 0; ulPage < ulPages; ulPage++)
	{
		pPhysAddr[ulPage].uiAddr = VMALLOC_TO_PAGE_PHYS(LinAddr);

		LinAddr += PAGE_SIZE;
	}

	*ppPhysAddr = pPhysAddr;

	return DC_OK;
}

void FreeDiscontigMemory(unsigned long ulSize,
                         DC_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr)
{
	kfree(pPhysAddr);

	vfree(LinAddr);
}
#else	
DC_ERROR AllocContigMemory(unsigned long ulSize,
                           DC_HANDLE unref__ *phMemHandle,
                           IMG_CPU_VIRTADDR *pLinAddr,
                           IMG_CPU_PHYADDR *pPhysAddr)
{
#if defined(DC_USE_SET_MEMORY)
	void *pvLinAddr;
	unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
	int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);
	int iError;

	pvLinAddr = kmalloc(ulAlignedSize, GFP_KERNEL);
	iError = set_memory_wc((unsigned long)pvLinAddr, iPages);
	if (iError != 0)
	{
		printk(KERN_ERR DRVNAME ": AllocContigMemory:  set_memory_wc failed (%d)\n", iError);

		return DC_ERROR_OUT_OF_MEMORY;
	}

	pPhysAddr->uiAddr = virt_to_phys(pvLinAddr);
	*pLinAddr = pvLinAddr;

	return DC_OK;
#else	
	dma_addr_t dma;
	IMG_VOID *pvLinAddr;

	pvLinAddr = dma_alloc_coherent(NULL, ulSize, &dma, GFP_KERNEL);

	if (pvLinAddr == NULL)
	{
		return DC_ERROR_OUT_OF_MEMORY;
	}

	pPhysAddr->uiAddr = dma;
	*pLinAddr = pvLinAddr;

	return DC_OK;
#endif	
}

void FreeContigMemory(unsigned long ulSize,
                      DC_HANDLE unref__ hMemHandle,
                      IMG_CPU_VIRTADDR LinAddr,
                      IMG_CPU_PHYADDR PhysAddr)
{
#if defined(DC_USE_SET_MEMORY)
	unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
	int iError;
	int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);

	iError = set_memory_wb((unsigned long)LinAddr, iPages);
	if (iError != 0)
	{
		printk(KERN_ERR DRVNAME ": FreeContigMemory:  set_memory_wb failed (%d)\n", iError);
	}
	kfree(LinAddr);
#else	
	dma_free_coherent(NULL, ulSize, LinAddr, (dma_addr_t)PhysAddr.uiAddr);
#endif	
}
#endif	

DC_ERROR OpenPVRServices (DC_HANDLE *phPVRServices)
{
	
	*phPVRServices = 0;
	return DC_OK;
}

DC_ERROR ClosePVRServices (DC_HANDLE unref__ hPVRServices)
{
	
	return DC_OK;
}

DC_ERROR GetLibFuncAddr (DC_HANDLE unref__ hExtDrv, char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return DC_ERROR_INVALID_PARAMS;
	}

	
	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return DC_OK;
}

#if !defined(SUPPORT_DRI_DRM)
module_init(DC_NOHW_Init);
module_exit(DC_NOHW_Cleanup);
#endif
