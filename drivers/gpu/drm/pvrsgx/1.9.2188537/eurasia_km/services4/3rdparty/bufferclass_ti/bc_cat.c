/**********************************************************************
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <img_defs.h>
#include <servicesext.h>
#include <kernelbuffer.h>
#include "bc_cat.h"
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
#include <linux/mutex.h>
#endif

#include <linux/uaccess.h>

#if defined(BC_DISCONTIG_BUFFERS)
#include <linux/vmalloc.h>
#endif

#define DEVNAME             "bccat"
#define DRVNAME             DEVNAME
#define DEVICE_COUNT        10
#define BC_EXAMPLE_NUM_BUFFERS  3
#define BUFFERCLASS_DEVICE_NAME "Example Bufferclass Device (SW)"

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(param) (param) = (param)
#endif


MODULE_SUPPORTED_DEVICE(DEVNAME);

#define unref__ __attribute__ ((unused))

typedef struct BC_CAT_BUFFER_TAG
{
    unsigned long                   ulSize;
    IMG_HANDLE                   hMemHandle;
#if defined(BC_DISCONTIG_BUFFERS)
        IMG_SYS_PHYADDR                         *psSysAddr;
#else

    IMG_SYS_PHYADDR              sSysAddr;
    IMG_SYS_PHYADDR              sPageAlignSysAddr;
#endif
    IMG_CPU_VIRTADDR             sCPUVAddr;
    PVRSRV_SYNC_DATA            *psSyncData;
    struct BC_CAT_BUFFER_TAG    *psNext;
} BC_CAT_BUFFER;


typedef struct BC_CAT_DEVINFO_TAG
{ 
    int ref;
    unsigned long                ulDeviceID;
    BC_CAT_BUFFER            *psSystemBuffer;
    unsigned long                ulNumBuffers;
    PVRSRV_BC_BUFFER2SRV_KMJTABLE    sPVRJTable;
    PVRSRV_BC_SRV2BUFFER_KMJTABLE    sBCJTable;
    IMG_HANDLE                hPVRServices;
    unsigned long                ulRefCount;
    BUFFER_INFO               sBufferInfo;
    enum BC_memory            buf_type;
} BC_CAT_DEVINFO;


typedef enum _BCE_ERROR_
{
        BCE_OK                             =  0,
        BCE_ERROR_GENERIC                  =  1,
        BCE_ERROR_OUT_OF_MEMORY            =  2,
        BCE_ERROR_TOO_FEW_BUFFERS          =  3,
        BCE_ERROR_INVALID_PARAMS           =  4,
        BCE_ERROR_INIT_FAILURE             =  5,
        BCE_ERROR_CANT_REGISTER_CALLBACK   =  6,
        BCE_ERROR_INVALID_DEVICE           =  7,
        BCE_ERROR_DEVICE_REGISTER_FAILED   =  8,
        BCE_ERROR_NO_PRIMARY               =  9
} BCE_ERROR;



extern IMG_IMPORT IMG_BOOL PVRGetBufferClassJTable(
                    PVRSRV_BC_BUFFER2SRV_KMJTABLE *psJTable);

static int bc_open(struct inode *i, struct file *f);
static int bc_release(struct inode *i, struct file *f);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int bc_ioctl(struct inode *inode, struct file *file,
                    unsigned int cmd, unsigned long arg);
#else
static long bc_ioctl(struct file *file,
                    unsigned int cmd, unsigned long arg);
static long bc_ioctl_unlocked(struct file *file,
                    unsigned int cmd, unsigned long arg);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static DEFINE_MUTEX(sBCExampleBridgeMutex);
#endif
static int bc_mmap(struct file *filp, struct vm_area_struct *vma);

static int BC_CreateBuffers(int id, bc_buf_params_t *p);
static PVRSRV_ERROR BC_DestroyBuffers(int id);
static PVRSRV_ERROR BC_Register(int id);
static PVRSRV_ERROR BC_Unregister(int id);

static PVRSRV_ERROR BCOpenPVRServices(IMG_HANDLE *phPVRServices);
static PVRSRV_ERROR BCClosePVRServices(IMG_HANDLE hPVRServices);

static IMG_VOID *BCAllocKernelMem(unsigned long ulSize);
static IMG_VOID BCFreeKernelMem(IMG_VOID *pvMem);

static BCE_ERROR BCAllocContigMemory(unsigned long ulSize,
                               IMG_HANDLE * phMemHandle,
                               IMG_CPU_VIRTADDR *pLinAddr,
                               IMG_CPU_PHYADDR *pPhysAddr);
static IMG_VOID BCFreeContigMemory(unsigned long ulSize, 
                          IMG_HANDLE hMemHandle,
                          IMG_CPU_VIRTADDR LinAddr, 
                          IMG_CPU_PHYADDR PhysAddr);

static IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(IMG_CPU_PHYADDR cpu_paddr);
static IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(IMG_SYS_PHYADDR sys_paddr);

static PVRSRV_ERROR BCGetLibFuncAddr(IMG_HANDLE hExtDrv,
                                     IMG_CHAR *szFunctionName,
                                     PFN_BC_GET_PVRJTABLE *ppfnFuncTable);
static BC_CAT_DEVINFO * GetAnchorPtr(int id);


static int major;
static struct class *bc_class;
static IMG_VOID *device[DEVICE_COUNT] = { 0 };
static PFN_BC_GET_PVRJTABLE pfnGetPVRJTable = IMG_NULL;
static int width_align;

static struct file_operations bc_cat_fops = {
    .open =  bc_open,
    .release = bc_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
	.ioctl = bc_ioctl,
#else
	.unlocked_ioctl = bc_ioctl_unlocked,
#ifdef CONFIG_COMPAT
       .compat_ioctl = bc_ioctl,
#endif
#endif
	.mmap =  bc_mmap,
};


/*****************************************************************************
 * func implementation
 * **************************************************************************/

#define file_to_id(file)  (iminor(file->f_path.dentry->d_inode))

static BC_CAT_DEVINFO * GetAnchorPtr(int id)
{
    return (BC_CAT_DEVINFO *)device[id];
}

static IMG_VOID SetAnchorPtr(int id, BC_CAT_DEVINFO *psDevInfo)
{
    device[id] = (IMG_VOID*)psDevInfo;
}


#if 0
static PVRSRV_ERROR OpenBCDevice(IMG_HANDLE *phDevice)
{
    BC_CAT_DEVINFO *psDevInfo;

    psDevInfo = GetAnchorPtr(id);
    *phDevice = (IMG_HANDLE)psDevInfo;

    return PVRSRV_OK;
}
#else

#define OPEN_FXN(id)                   \
static PVRSRV_ERROR OpenBCDevice##id(IMG_UINT32 ui32DeviceID, IMG_HANDLE *phDevice)\
{                                      \
    BC_CAT_DEVINFO *psDevInfo;           \
UNREFERENCED_PARAMETER(ui32DeviceID);    \
    psDevInfo = GetAnchorPtr (id);       \
    *phDevice = (IMG_HANDLE) psDevInfo;  \
    return PVRSRV_OK;                    \
}

OPEN_FXN(0)
OPEN_FXN(1)
OPEN_FXN(2)
OPEN_FXN(3)
OPEN_FXN(4)
OPEN_FXN(5)
OPEN_FXN(6)
OPEN_FXN(7)
OPEN_FXN(8)
OPEN_FXN(9)
#endif

static PVRSRV_ERROR CloseBCDevice(IMG_UINT32 handle , IMG_HANDLE hDevice)
{
    PVR_UNREFERENCED_PARAMETER(hDevice);

    return PVRSRV_OK;
}

static PVRSRV_ERROR GetBCBuffer(IMG_HANDLE            hDevice,
                                IMG_UINT32            ui32BufferNumber,
                                PVRSRV_SYNC_DATA    *psSyncData,
                                IMG_HANDLE            *phBuffer)
{
    BC_CAT_DEVINFO    *psDevInfo;

    if (!hDevice || !phBuffer)
        return PVRSRV_ERROR_INVALID_PARAMS;

    psDevInfo = (BC_CAT_DEVINFO*)hDevice;

    if (ui32BufferNumber < psDevInfo->sBufferInfo.ui32BufferCount)  {
        psDevInfo->psSystemBuffer[ui32BufferNumber].psSyncData = psSyncData;
        *phBuffer = (IMG_HANDLE)&psDevInfo->psSystemBuffer[ui32BufferNumber];
    } else {
        return PVRSRV_ERROR_INVALID_PARAMS;
    }

    return PVRSRV_OK;
}


static PVRSRV_ERROR GetBCInfo(IMG_HANDLE hDevice, BUFFER_INFO *psBCInfo)
{
    BC_CAT_DEVINFO    *psDevInfo;

    if (!hDevice || !psBCInfo)
        return PVRSRV_ERROR_INVALID_PARAMS;

    psDevInfo = (BC_CAT_DEVINFO*)hDevice;
    *psBCInfo = psDevInfo->sBufferInfo;

    return PVRSRV_OK;
}


static PVRSRV_ERROR GetBCBufferAddr(IMG_HANDLE        hDevice,
                                    IMG_HANDLE        hBuffer,
                                    IMG_SYS_PHYADDR    **ppsSysAddr,
                                    IMG_UINT32        *pui32ByteSize,
                                    IMG_VOID        **ppvCpuVAddr,
                                    IMG_HANDLE        *phOSMapInfo,
                                    IMG_BOOL        *pbIsContiguous,
                                    IMG_UINT32      *pui32TilingStride)
{
    BC_CAT_BUFFER *psBuffer;
    PVR_UNREFERENCED_PARAMETER(pui32TilingStride);

    if (!hDevice || !hBuffer || !ppsSysAddr || !pui32ByteSize)
        return PVRSRV_ERROR_INVALID_PARAMS;

    psBuffer = (BC_CAT_BUFFER *) hBuffer;

     *ppvCpuVAddr = psBuffer->sCPUVAddr;

        *phOSMapInfo    = IMG_NULL;
        *pui32ByteSize = (IMG_UINT32)psBuffer->ulSize;

#if defined(BC_DISCONTIG_BUFFERS)
        *ppsSysAddr = psBuffer->psSysAddr;
        *pbIsContiguous = IMG_FALSE;
#else
        *ppsSysAddr = &psBuffer->sPageAlignSysAddr;
        *pbIsContiguous = IMG_TRUE;
#endif


    return PVRSRV_OK;
}


static int BC_CreateBuffers(int id, bc_buf_params_t *p)
{
    BC_CAT_DEVINFO  *psDevInfo;
#if !defined(BC_DISCONTIG_BUFFERS)
    IMG_CPU_PHYADDR  paddr;
#endif
    IMG_UINT32       i, stride;
    unsigned long ulSize;
    PVRSRV_PIXEL_FORMAT pixel_fmt;
//IMG_UINT32 ui32MaxWidth = 320 * 4;
    if (p->count <= 0)
        return -EINVAL;

    if (p->width <= 1  || p->width % width_align || p->height <= 1)
        return -EINVAL;

    switch (p->fourcc) {
    case BC_PIX_FMT_YV12:
        pixel_fmt = PVRSRV_PIXEL_FORMAT_YV12;
        stride = p->width;
        break;
    case BC_PIX_FMT_I420:
        pixel_fmt = PVRSRV_PIXEL_FORMAT_I420;
        stride = p->width;
        break;

    case BC_PIX_FMT_NV12:
        pixel_fmt = PVRSRV_PIXEL_FORMAT_NV12;
        stride = p->width;
        break;
    case BC_PIX_FMT_UYVY:
        pixel_fmt = PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY;
        stride = p->width << 1;
        break;
    case BC_PIX_FMT_RGB565:
        pixel_fmt = PVRSRV_PIXEL_FORMAT_RGB565;
        stride = p->width << 1;
        break;
    case BC_PIX_FMT_YUYV:
        pixel_fmt = PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV;
        stride = p->width << 1;
        break;
    
    case BC_PIX_FMT_ARGB:
        pixel_fmt = PVRSRV_PIXEL_FORMAT_ARGB8888;
        stride = p->width << 2;
        break;

    default:
        return -EINVAL;
        break;
    }

    if (p->type != BC_MEMORY_MMAP && p->type != BC_MEMORY_USERPTR)
        return -EINVAL;

      psDevInfo = GetAnchorPtr(id);
        if (psDevInfo == NULL)
        {

                return (BCE_ERROR_DEVICE_REGISTER_FAILED);
        }
        if (psDevInfo->ulNumBuffers)
        {

                return (BCE_ERROR_GENERIC);
        }



    psDevInfo->buf_type = p->type;
    psDevInfo->psSystemBuffer =
            BCAllocKernelMem(sizeof(BC_CAT_BUFFER) * p->count);

    if (!psDevInfo->psSystemBuffer)
        return -ENOMEM;

    memset(psDevInfo->psSystemBuffer, 0, sizeof(BC_CAT_BUFFER) * p->count);

    ulSize = p->height * stride;
    if (pixel_fmt == PVRSRV_PIXEL_FORMAT_NV12)
        ulSize += (stride >> 1) * (p->height >> 1) << 1;

    if ((pixel_fmt == PVRSRV_PIXEL_FORMAT_I420) || (pixel_fmt == PVRSRV_PIXEL_FORMAT_YV12) )
    {
        ulSize += (stride >> 1) * (p->height >> 1);
        ulSize += (stride >> 1) * (p->height >> 1);
    }

    for (i=0; i < p->count; i++) {
        if (psDevInfo->buf_type == BC_MEMORY_MMAP) {

#if defined(BC_DISCONTIG_BUFFERS)
                if (BCAllocDiscontigMemory(ulSize,
                                                                        &psDevInfo->psSystemBuffer[i].hMemHandle,
                                                                        &psDevInfo->psSystemBuffer[i].sCPUVAddr,
                                                                        &psDevInfo->psSystemBuffer[i].psSysAddr) != BCE_OK)
                {
                        break;
                }
#else

            if (BCAllocContigMemory(ulSize,
                                  &psDevInfo->psSystemBuffer[i].hMemHandle,
                                  &psDevInfo->psSystemBuffer[i].sCPUVAddr,
                                  &paddr) != BCE_OK)
{
                /*TODO should free() and return failure*/
                break;
}

            psDevInfo->psSystemBuffer[i].sSysAddr = CpuPAddrToSysPAddrBC(paddr);
            psDevInfo->psSystemBuffer[i].sPageAlignSysAddr.uiAddr =
                    psDevInfo->psSystemBuffer[i].sSysAddr.uiAddr & 0xFFFFF000;
        }
#endif
        psDevInfo->ulNumBuffers++;
        psDevInfo->psSystemBuffer[i].ulSize = ulSize;
        psDevInfo->psSystemBuffer[i].psSyncData = NULL;
    }
    p->count = psDevInfo->ulNumBuffers;

    psDevInfo->sBufferInfo.ui32BufferCount = (IMG_UINT32)psDevInfo->ulNumBuffers;
    psDevInfo->sBufferInfo.pixelformat = pixel_fmt;
    psDevInfo->sBufferInfo.ui32Width = p->width;
    psDevInfo->sBufferInfo.ui32Height = p->height;
    psDevInfo->sBufferInfo.ui32ByteStride = stride;    
    psDevInfo->sBufferInfo.ui32BufferDeviceID = id;
    psDevInfo->sBufferInfo.ui32Flags = PVRSRV_BC_FLAGS_YUVCSC_FULL_RANGE |
                                       PVRSRV_BC_FLAGS_YUVCSC_BT601;

psDevInfo->sBCJTable.ui32TableSize    = sizeof(PVRSRV_BC_SRV2BUFFER_KMJTABLE);
        psDevInfo->sBCJTable.pfnOpenBCDevice  = OpenBCDevice0;
        psDevInfo->sBCJTable.pfnCloseBCDevice = CloseBCDevice;
        psDevInfo->sBCJTable.pfnGetBCBuffer   = GetBCBuffer;
        psDevInfo->sBCJTable.pfnGetBCInfo     = GetBCInfo;
        psDevInfo->sBCJTable.pfnGetBufferAddr = GetBCBufferAddr;
/*
if (psDevInfo->sBufferInfo.ui32Width < ui32MaxWidth)
	{
		switch(pixel_fmt)
		{
		    case PVRSRV_PIXEL_FORMAT_NV12:
		    case PVRSRV_PIXEL_FORMAT_I420:
			{
			    psDevInfo->sBufferInfo.ui32Width += 320;
				psDevInfo->sBufferInfo.ui32Height += 160;
				psDevInfo->sBufferInfo.ui32ByteStride = psDevInfo->sBufferInfo.ui32Width;
				break;
			}
		    case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_VYUY:
		    case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY:
		    case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV:
		    case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YVYU:
			{
			    psDevInfo->sBufferInfo.ui32Width += 320;
				psDevInfo->sBufferInfo.ui32Height += 160;
				psDevInfo->sBufferInfo.ui32ByteStride = ui32Width*2;
				break;
			}
		    case PVRSRV_PIXEL_FORMAT_RGB565:
			{
			    psDevInfo->sBufferInfo.ui32Width += 320;
				psDevInfo->sBufferInfo.ui32Height += 160;
				psDevInfo->sBufferInfo.ui32ByteStride = ui32Width*2;
				break;
			}
		    default:
			{
				return (BCE_ERROR_INVALID_PARAMS);
			}
		}
	}
	else
	{
		psDevInfo->sBufferInfo.ui32Width      = BC_EXAMPLE_WIDTH;
		psDevInfo->sBufferInfo.ui32Height     = BC_EXAMPLE_HEIGHT;
		psDevInfo->sBufferInfo.ui32ByteStride = BC_EXAMPLE_STRIDE;
	}
*/
        return (BCE_OK);

}


static PVRSRV_ERROR BC_DestroyBuffers(int id)
{
    BC_CAT_DEVINFO *psDevInfo;
    IMG_UINT32 i;
    
 psDevInfo = GetAnchorPtr(id);


        if (psDevInfo == NULL)
        {


                return (BCE_ERROR_DEVICE_REGISTER_FAILED);
        }

    if (psDevInfo->buf_type == BC_MEMORY_MMAP)
        for (i = 0; i < psDevInfo->ulNumBuffers; i++) {
#if defined(BC_DISCONTIG_BUFFERS)
                BCFreeDiscontigMemory(psDevInfo->psSystemBuffer[i].ulSize,
                         psDevInfo->psSystemBuffer[i].hMemHandle,
                         psDevInfo->psSystemBuffer[i].sCPUVAddr,
                         psDevInfo->psSystemBuffer[i].psSysAddr);
#else

            BCFreeContigMemory(psDevInfo->psSystemBuffer[i].ulSize,
                    psDevInfo->psSystemBuffer[i].hMemHandle,
                    psDevInfo->psSystemBuffer[i].sCPUVAddr,
                    SysPAddrToCpuPAddrBC(psDevInfo->psSystemBuffer[i].sSysAddr));
#endif
        }

  //  BCFreeKernelMem(psDevInfo->psSystemBuffer);
    
    psDevInfo->ulNumBuffers = 0;
    psDevInfo->sBufferInfo.pixelformat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
    psDevInfo->sBufferInfo.ui32Width = 0;
    psDevInfo->sBufferInfo.ui32Height = 0;
    psDevInfo->sBufferInfo.ui32ByteStride = 0;    
    psDevInfo->sBufferInfo.ui32BufferDeviceID = id;
    psDevInfo->sBufferInfo.ui32Flags = 0;
    psDevInfo->sBufferInfo.ui32BufferCount = (IMG_UINT32)psDevInfo->ulNumBuffers;

    return PVRSRV_OK;
}


static PVRSRV_ERROR BC_Register(id)
{
    BC_CAT_DEVINFO  *psDevInfo;
    
//psDevInfo = GetAnchorPtr();
   psDevInfo = GetAnchorPtr(id);

     if (psDevInfo == NULL)
 { 
    psDevInfo = (BC_CAT_DEVINFO *)BCAllocKernelMem(sizeof(BC_CAT_DEVINFO));

    if (!psDevInfo)
        return PVRSRV_ERROR_OUT_OF_MEMORY;
    
    psDevInfo->ref = 0;
    psDevInfo->ulRefCount = 0;
    SetAnchorPtr(id, (IMG_VOID*)psDevInfo);

    if (BCOpenPVRServices(&psDevInfo->hPVRServices) != PVRSRV_OK)
        return PVRSRV_ERROR_INIT_FAILURE;

    if (BCGetLibFuncAddr(psDevInfo->hPVRServices, "PVRGetBufferClassJTable",
                         &pfnGetPVRJTable) != PVRSRV_OK)
        return PVRSRV_ERROR_INIT_FAILURE;
    
    if (!(*pfnGetPVRJTable)(&psDevInfo->sPVRJTable))
        return PVRSRV_ERROR_INIT_FAILURE;

    psDevInfo->ulNumBuffers = 0;

    psDevInfo->psSystemBuffer = BCAllocKernelMem(sizeof(BC_CAT_BUFFER) * BC_EXAMPLE_NUM_BUFFERS);

    psDevInfo->sBufferInfo.pixelformat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
    psDevInfo->sBufferInfo.ui32Width = 0;
    psDevInfo->sBufferInfo.ui32Height = 0;
    psDevInfo->sBufferInfo.ui32ByteStride = 0;    
    psDevInfo->sBufferInfo.ui32BufferDeviceID = id;
    psDevInfo->sBufferInfo.ui32Flags = 0;
    psDevInfo->sBufferInfo.ui32BufferCount = (IMG_UINT32)psDevInfo->ulNumBuffers;

    psDevInfo->sBCJTable.ui32TableSize = sizeof(PVRSRV_BC_SRV2BUFFER_KMJTABLE);

strncpy(psDevInfo->sBufferInfo.szDeviceName, BUFFERCLASS_DEVICE_NAME, MAX_BUFFER_DEVICE_NAME_SIZE);
#if 0
    psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice;
#else
    if (id == 0) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice0;
    } else if (id == 1) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice1;
    } else if (id == 2) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice2;
    } else if (id == 3) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice3;
    } else if (id == 4) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice4;
    } else if (id == 5) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice5;
    } else if (id == 6) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice6;
    } else if (id == 7) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice7;
    } else if (id == 8) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice8;
    } else if (id == 9) {
        psDevInfo->sBCJTable.pfnOpenBCDevice = OpenBCDevice9;
    } else {
        printk("bad device id: %d\n", id);
        return PVRSRV_ERROR_DEVICE_REGISTER_FAILED;
    }
#endif
    psDevInfo->sBCJTable.pfnCloseBCDevice = CloseBCDevice;
    psDevInfo->sBCJTable.pfnGetBCBuffer = GetBCBuffer;
    psDevInfo->sBCJTable.pfnGetBCInfo = GetBCInfo;
    psDevInfo->sBCJTable.pfnGetBufferAddr = GetBCBufferAddr;
    
    if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterBCDevice(
                &psDevInfo->sBCJTable,
              (IMG_UINT32*)&psDevInfo->ulDeviceID) != PVRSRV_OK)
        return PVRSRV_ERROR_DEVICE_REGISTER_FAILED;
}
    psDevInfo->ulRefCount++;
    
    return PVRSRV_OK;
}


static PVRSRV_ERROR BC_Unregister(int id)
{
    BC_CAT_DEVINFO *psDevInfo;
//    PVRSRV_BC_BUFFER2SRV_KMJTABLE *psJTable;
    
    if ((psDevInfo = GetAnchorPtr(id)) == IMG_NULL)
        return PVRSRV_ERROR_DEVICE_REGISTER_FAILED;
    
    psDevInfo->ulRefCount--;

    if (psDevInfo->ulRefCount)
        return PVRSRV_ERROR_RETRY;


  if (psDevInfo->ulRefCount == 0)
        {

                PVRSRV_BC_BUFFER2SRV_KMJTABLE   *psJTable = &psDevInfo->sPVRJTable;


    if (psJTable->pfnPVRSRVRemoveBCDevice(psDevInfo->ulDeviceID) != PVRSRV_OK)
        //return PVRSRV_ERROR_GENERIC;
        return 1;

    if (BCClosePVRServices(psDevInfo->hPVRServices) != PVRSRV_OK) {
        psDevInfo->hPVRServices = IMG_NULL;
        //return PVRSRV_ERROR_GENERIC;
        return 1;
    }
if (psDevInfo->psSystemBuffer)
                {
                        BCFreeKernelMem(psDevInfo->psSystemBuffer);
                }

    BCFreeKernelMem(psDevInfo);
    SetAnchorPtr(id, IMG_NULL);
}    
    return PVRSRV_OK;
}


static int __init bc_cat_init(void)
{
    struct device *bc_dev;
    int id;

    /* texture buffer width should be multiple of 8 for OMAP3 ES3.x,
     * or 32 for ES2.x */

#ifdef PLAT_TI8168
     width_align = 8;
#else
     width_align = 8;
     //width_align = cpu_is_omap3530() && ( omap_rev() < OMAP3430_REV_ES3_0 ) ? 32 : 8; 
#endif   
 
    major = register_chrdev(0, DEVNAME, &bc_cat_fops);

    if (major <= 0) {
        printk(KERN_ERR DRVNAME ": unable to get major number\n");
        goto ExitDisable;
    }

    bc_class = class_create(THIS_MODULE, DEVNAME);

    if (IS_ERR(bc_class)) {
       printk(KERN_ERR DRVNAME ": upable to create device class\n");
       goto ExitUnregister;
    }

    for (id = 0; id < DEVICE_COUNT; id++) {
        bc_dev = device_create(bc_class, NULL, MKDEV(major, id), NULL,
                               DEVNAME "%d", id);

        if (IS_ERR(bc_dev)) {
           printk(KERN_ERR DRVNAME ": unable to create device %d\n", id);
           goto ExitDestroyClass;
        }

        if (BC_Register(id) != PVRSRV_OK) {
            printk (KERN_ERR DRVNAME ": can't register BC service %d\n", id);
            if (id > 0) {
                /* lets live with the drivers that we were able to create soi
                 * far, even though it isn't as many as we'd like
                 */
                 break;
            }
            goto ExitUnregister;
        }
    }

    return 0;

ExitDestroyClass:
    class_destroy(bc_class);
ExitUnregister:
    unregister_chrdev(major, DEVNAME);
ExitDisable:
    return -EBUSY;
} 

static void __exit bc_cat_cleanup(void)
{    
    int id=0;

    for (id = 0; id < DEVICE_COUNT; id++) {
device_destroy(bc_class, MKDEV(major, id));
}

    class_destroy(bc_class);

    unregister_chrdev(major, DEVNAME);
  
  for (id = 0; id < DEVICE_COUNT; id++) {
        if (BC_DestroyBuffers(id) != PVRSRV_OK) {
            printk(KERN_ERR DRVNAME ": can't free texture buffers\n");
            return;
        }
        if (BC_Unregister(id) != PVRSRV_OK) {
            printk(KERN_ERR DRVNAME ": can't un-register BC service\n");
            return;
        }
    }
} 


static IMG_VOID *BCAllocKernelMem(unsigned long ulSize)
{
    return kmalloc(ulSize, GFP_KERNEL);
}

static IMG_VOID BCFreeKernelMem(IMG_VOID *pvMem)
{
    kfree(pvMem);
}

#if defined(BC_DISCONTIG_BUFFERS)

#define RANGE_TO_PAGES(range) (((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)
#define VMALLOC_TO_PAGE_PHYS(vAddr) page_to_phys(vmalloc_to_page(vAddr))

BCE_ERROR BCAllocDiscontigMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
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
                return BCE_ERROR_OUT_OF_MEMORY;
        }

        pPhysAddr = kmalloc(ulPages * sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
        if (!pPhysAddr)
        {
                vfree(LinAddr);
                return BCE_ERROR_OUT_OF_MEMORY;
        }

        *pLinAddr = LinAddr;

        for (ulPage = 0; ulPage < ulPages; ulPage++)
        {
                pPhysAddr[ulPage].uiAddr = VMALLOC_TO_PAGE_PHYS(LinAddr);

                LinAddr += PAGE_SIZE;
        }

        *ppPhysAddr = pPhysAddr;

        return BCE_OK;
}

void BCFreeDiscontigMemory(unsigned long ulSize,
                         BCE_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr)
{
        kfree(pPhysAddr);

        vfree(LinAddr);
}
#else

BCE_ERROR BCAllocContigMemory(unsigned long ulSize,
                                 IMG_HANDLE unref__ *phMemHandle, 
                                 IMG_CPU_VIRTADDR *pLinAddr, 
                                 IMG_CPU_PHYADDR *pPhysAddr)
{

#if defined(BCE_USE_SET_MEMORY)
        void *pvLinAddr;
        unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
        int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);
        int iError;

        pvLinAddr = kmalloc(ulAlignedSize, GFP_KERNEL);
        BUG_ON(((unsigned long)pvLinAddr)  & ~PAGE_MASK);

        iError = set_memory_wc((unsigned long)pvLinAddr, iPages);
        if (iError != 0)
        {
                printk(KERN_ERR DRVNAME ": BCAllocContigMemory:  set_memory_wc failed (%d)\n", iError);
                return (BCE_ERROR_OUT_OF_MEMORY);
        }

        pPhysAddr->uiAddr = virt_to_phys(pvLinAddr);
        *pLinAddr = pvLinAddr;

        return (BCE_OK);
#else
        dma_addr_t dma;
        void *pvLinAddr;

        pvLinAddr = dma_alloc_coherent(NULL, ulSize, &dma, GFP_KERNEL);
        if (pvLinAddr == NULL)
        {
                return (BCE_ERROR_OUT_OF_MEMORY);
        }

        pPhysAddr->uiAddr = dma;
        *pLinAddr = pvLinAddr;

        return (BCE_OK);
#endif
#if 0
    IMG_VOID *pvLinAddr;
    gfp_t mask = GFP_KERNEL;
    
    pvLinAddr = alloc_pages_exact(ui32Size, mask);
    
    if (pvLinAddr == IMG_NULL)
        return PVRSRV_ERROR_OUT_OF_MEMORY;

    pPhysAddr->uiAddr = virt_to_phys(pvLinAddr);

    *pLinAddr = pvLinAddr;
#endif
 //   return PVRSRV_OK;
}

static IMG_VOID BCFreeContigMemory(unsigned long ulSize,
                        IMG_HANDLE unref__ hMemHandle, 
                        IMG_CPU_VIRTADDR LinAddr, 
                        IMG_CPU_PHYADDR PhysAddr)
{
 //   free_pages_exact(LinAddr, ui32Size);
#if defined(BCE_USE_SET_MEMORY)
        unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
        int iError;
        int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);

        iError = set_memory_wb((unsigned long)LinAddr, iPages);
        if (iError != 0)
        {
                printk(KERN_ERR DRVNAME ": BCFreeContigMemory:  set_memory_wb failed (%d)\n", iError);
        }
        kfree(LinAddr);
#else
        dma_free_coherent(NULL, ulSize, LinAddr, (dma_addr_t)PhysAddr.uiAddr);
#endif
 
}

#endif
static IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(IMG_CPU_PHYADDR cpu_paddr)
{
    IMG_SYS_PHYADDR sys_paddr;
    
    sys_paddr.uiAddr = cpu_paddr.uiAddr;
    return sys_paddr;
}

static IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(IMG_SYS_PHYADDR sys_paddr)
{
    IMG_CPU_PHYADDR cpu_paddr;
    
    cpu_paddr.uiAddr = sys_paddr.uiAddr;
    return cpu_paddr;
}

static PVRSRV_ERROR BCOpenPVRServices (IMG_HANDLE *phPVRServices)
{
    *phPVRServices = 0;
    return PVRSRV_OK;
}


static PVRSRV_ERROR BCClosePVRServices (IMG_HANDLE unref__ hPVRServices)
{
    return PVRSRV_OK;
}

static PVRSRV_ERROR BCGetLibFuncAddr(IMG_HANDLE unref__ hExtDrv,
                              IMG_CHAR *szFunctionName,
                              PFN_BC_GET_PVRJTABLE *ppfnFuncTable)
{
    if (strcmp("PVRGetBufferClassJTable", szFunctionName) != 0)
        return PVRSRV_ERROR_INVALID_PARAMS;

    *ppfnFuncTable = PVRGetBufferClassJTable;
    return PVRSRV_OK;
}


static int bc_open(struct inode *i, struct file *f)
{
    BC_CAT_DEVINFO *devinfo;
    int id = file_to_id(f);

    if ((devinfo = GetAnchorPtr(id)) == IMG_NULL) {
        printk("no device %d\n", id);
        return -ENODEV;
    }

    if (devinfo->ref) {
        printk("device %d busy\n", id);
        return -EBUSY;
    }

    devinfo->ref++;
    return 0;
}


static int bc_release(struct inode *i, struct file *f)
{
    BC_CAT_DEVINFO *devinfo;
    int id = file_to_id(f);

    if ((devinfo = GetAnchorPtr(id)) == IMG_NULL)
        return -ENODEV;

    for (id = 0; id < DEVICE_COUNT; id++) {
        if (BC_DestroyBuffers(id) != PVRSRV_OK) {
            printk(KERN_ERR DRVNAME ": can't free texture buffer \n");
        }
    }

    if (devinfo->ref)
        devinfo->ref--;
    return 0;
}


static int bc_mmap(struct file *filp, struct vm_area_struct *vma)
{
#if defined(DEBUG)
    printk("bc_mmap: vma->vm_start = %#lx\n", vma->vm_start);
    printk("bc_mmap: vma->vm_pgoff = %#lx\n", vma->vm_pgoff);
    printk("bc_mmap: size          = %#lx\n", vma->vm_end - vma->vm_start);
#endif

    /*FIXME check start & size*/
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
                        vma->vm_end - vma->vm_start,
                        vma->vm_page_prot)) {
        printk("bc_mmap: failed remap_pfn_range\n");
        return -EAGAIN;
    }
    return 0;
}

int ReconfigureBuffer(int id, bc_buf_params_t *p, unsigned int *uiSucceed)
{
        BCE_ERROR eError;


        eError = BC_DestroyBuffers(id);

        if (eError != BCE_OK)
        {
            *uiSucceed = 0;
                return -1;
        }





        eError = BC_CreateBuffers(id,p);

        if (eError != BCE_OK)
        {
            *uiSucceed = 0;
                return -1;
        }


        *uiSucceed = 1;
        return 0;
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
static int bc_ioctl(struct inode *inode, struct file *file,
                    unsigned int cmd, unsigned long arg)
#else
static long  bc_ioctl(struct file *file,
                    unsigned int cmd, unsigned long arg)
#endif
{
    BC_CAT_DEVINFO *devinfo;
//PVR_UNREFERENCED_PARAMETER(inode);
    int id = file_to_id (file);

    if ((devinfo = GetAnchorPtr(id)) == IMG_NULL)
        return -ENODEV;

    switch(_IOC_NR(cmd)) {
        case _IOC_NR(BCIOGET_BUFFERCOUNT):
        {    
            BCIO_package *params = (BCIO_package *)arg;

            if (!access_ok(params, sizeof(BCIO_package)))
                return -EFAULT;

            params->output = devinfo->sBufferInfo.ui32BufferCount;
            break;
        }
        case _IOC_NR(BCIOGET_BUFFERPHYADDR):
        {
            int idx;
            BCIO_package *params = (BCIO_package *)arg;

            if (!access_ok(params, sizeof(BCIO_package)))
                return -EFAULT;

            idx = params->input;
            if (idx < 0 || idx > devinfo->ulNumBuffers) {
                printk(KERN_ERR DRVNAME
                        ": BCIOGET_BUFFERADDR - idx out of range\n");
                return -EINVAL;
            }
            params->output = devinfo->psSystemBuffer[idx].sSysAddr.uiAddr;
            break;
        }
        case _IOC_NR(BCIOGET_BUFFERIDX):
        {
            int idx;
            BC_CAT_BUFFER  *buffer;
            BCIO_package *params = (BCIO_package *)arg;

            if (!access_ok(params, sizeof(BCIO_package)))
                return -EFAULT;

            for (idx = 0; idx < devinfo->ulNumBuffers; idx++) {
                buffer = &devinfo->psSystemBuffer[idx];

                if (params->input == (int)buffer->sSysAddr.uiAddr) {
                    params->output = idx;
                    return 0;
                }
            }
            printk(KERN_ERR DRVNAME ": BCIOGET_BUFFERIDX- buffer not found\n");
            return -EINVAL;
            break;
        }
        case _IOC_NR(BCIOREQ_BUFFERS):
        {
            bc_buf_params_t *p = (bc_buf_params_t *) arg;
            
            if (!access_ok(p, sizeof(bc_buf_params_t)))
                return -EFAULT;

            return BC_CreateBuffers(id, p);
            break;
        }
        case _IOC_NR(BCIOSET_BUFFERPHYADDR):
        {
            bc_buf_ptr_t p;
            IMG_CPU_PHYADDR img_pa;

            if (copy_from_user(&p, (void __user *)arg, sizeof(p)))
                return -EFAULT;

            if (p.index >= devinfo->ulNumBuffers || !p.pa)
                return -EINVAL;
            
            /*TODO check buffer size*/

            img_pa.uiAddr = p.pa;

            devinfo->psSystemBuffer[p.index].sCPUVAddr = phys_to_virt(p.pa);
            devinfo->psSystemBuffer[p.index].sSysAddr =
                    CpuPAddrToSysPAddrBC(img_pa);
            devinfo->psSystemBuffer[p.index].sPageAlignSysAddr.uiAddr =
                    devinfo->psSystemBuffer[p.index].sSysAddr.uiAddr &
                    0xFFFFF000;
            break;
        }
	case _IOC_NR(BCIORECONFIGURE_BUFFERS):
	{
		unsigned int outputparam;
		bc_buf_params_t *p = (bc_buf_params_t *) arg;
			if(ReconfigureBuffer(id,p,&outputparam) == -1)
			{
				return -EFAULT; 
			}
			break;
	}
        default:
            return -EFAULT;
    }
    return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
static long bc_ioctl_unlocked(struct file *file, unsigned int cmd, unsigned long arg)
{
        int res;

        mutex_lock(&sBCExampleBridgeMutex);
        res = bc_ioctl(file, cmd, arg);
        mutex_unlock(&sBCExampleBridgeMutex);

        return res;
}
#endif


module_init(bc_cat_init);
module_exit(bc_cat_cleanup);

MODULE_LICENSE("GPL v2");
