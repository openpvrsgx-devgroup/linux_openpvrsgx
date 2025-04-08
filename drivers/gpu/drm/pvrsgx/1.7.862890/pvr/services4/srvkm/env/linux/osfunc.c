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

#include <asm/io.h>
#include <asm/page.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)) && (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0))
#include <asm/system.h>
#endif
#include <asm/cacheflush.h>
#include <linux/mm.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,8,0))
#else
#define mmap_sem mmap_lock	// has been renamed by v5.8-rc1
#endif

#include <linux/pagemap.h>
#include <linux/hugetlb.h> 
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <linux/string.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/hardirq.h>
#include <linux/timer.h>
#include <linux/capability.h>
#include <asm/uaccess.h>
#include <linux/spinlock.h>
#if defined(PVR_LINUX_MISR_USING_WORKQUEUE) || \
	defined(PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE) || \
	defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || \
	defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE) || \
	defined(PVR_LINUX_USING_WORKQUEUES)
#include <linux/workqueue.h>
#endif

#include "img_types.h"
#include "services_headers.h"
#include "mm.h"
#include "pvrmmap.h"
#include "mmap.h"
#include "env_data.h"
#include "proc.h"
#include "mutex.h"
#include "event.h"
#include "linkage.h"

#define PVRSRV_MODNAME "PowerVR"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
#define ON_EACH_CPU(func, info, wait) on_each_cpu(func, info, wait)
#else
#define ON_EACH_CPU(func, info, wait) on_each_cpu(func, info, 0, wait)
#endif

#if defined(PVR_LINUX_USING_WORKQUEUES) && !defined(CONFIG_PREEMPT)
#error "A preemptible Linux kernel is required when using workqueues"
#endif

#if defined(EMULATOR)
#define EVENT_OBJECT_TIMEOUT_MS		(2000)
#else
#define EVENT_OBJECT_TIMEOUT_MS		(100)
#endif 

#if !defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
PVRSRV_ERROR OSAllocMem_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T uiSize, IMG_PVOID *ppvCpuVAddr, IMG_HANDLE *phBlockAlloc)
#else
PVRSRV_ERROR OSAllocMem_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T uiSize, IMG_PVOID *ppvCpuVAddr, IMG_HANDLE *phBlockAlloc, IMG_CHAR *pszFilename, IMG_UINT32 ui32Line)
#endif
{
    PVR_UNREFERENCED_PARAMETER(ui32Flags);
    PVR_UNREFERENCED_PARAMETER(phBlockAlloc);

    if (uiSize > PAGE_SIZE)
    {
        
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
        *ppvCpuVAddr = _VMallocWrapper(uiSize, PVRSRV_HAP_CACHED, pszFilename, ui32Line);
#else
        *ppvCpuVAddr = VMallocWrapper(uiSize, PVRSRV_HAP_CACHED);
#endif
        if (*ppvCpuVAddr)
        {
            return PVRSRV_OK;
        }
    }

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    *ppvCpuVAddr = _KMallocWrapper(uiSize, GFP_KERNEL | __GFP_NOWARN, pszFilename, ui32Line);
#else
    *ppvCpuVAddr = KMallocWrapper(uiSize, GFP_KERNEL | __GFP_NOWARN);
#endif
    if (!*ppvCpuVAddr)
    {
        return PVRSRV_ERROR_OUT_OF_MEMORY;
    }

    return PVRSRV_OK;
}

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24))

static inline int is_vmalloc_addr(const void *pvCpuVAddr)
{
	unsigned long lAddr = (unsigned long)pvCpuVAddr;
	return lAddr >= VMALLOC_START && lAddr < VMALLOC_END;
}

#endif 

#if !defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
PVRSRV_ERROR OSFreeMem_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T uiSize, IMG_PVOID pvCpuVAddr, IMG_HANDLE hBlockAlloc)
#else
PVRSRV_ERROR OSFreeMem_Impl(IMG_UINT32 ui32Flags, IMG_SIZE_T uiSize, IMG_PVOID pvCpuVAddr, IMG_HANDLE hBlockAlloc, IMG_CHAR *pszFilename, IMG_UINT32 ui32Line)
#endif
{	
    PVR_UNREFERENCED_PARAMETER(ui32Flags);
    PVR_UNREFERENCED_PARAMETER(uiSize);
    PVR_UNREFERENCED_PARAMETER(hBlockAlloc);

    if (is_vmalloc_addr(pvCpuVAddr))
    {
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
        _VFreeWrapper(pvCpuVAddr, pszFilename, ui32Line);
#else
        VFreeWrapper(pvCpuVAddr);
#endif
    }
    else
    {
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
        _KFreeWrapper(pvCpuVAddr, pszFilename, ui32Line);
#else
        KFreeWrapper(pvCpuVAddr);
#endif
    }

    return PVRSRV_OK;
}


PVRSRV_ERROR
OSAllocPages_Impl(IMG_UINT32 ui32AllocFlags,
				  IMG_UINT32 ui32Size,
				  IMG_UINT32 ui32PageSize,
				  IMG_VOID **ppvCpuVAddr,
				  IMG_HANDLE *phOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea;

    PVR_UNREFERENCED_PARAMETER(ui32PageSize);

#if 0
    
    if(ui32AllocFlags & PVRSRV_HAP_SINGLE_PROCESS)
    {
        ui32AllocFlags &= ~PVRSRV_HAP_SINGLE_PROCESS;
        ui32AllocFlags |= PVRSRV_HAP_MULTI_PROCESS;
    }
#endif

    switch(ui32AllocFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
        {
            psLinuxMemArea = NewVMallocLinuxMemArea(ui32Size, ui32AllocFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_OUT_OF_MEMORY;
            }
            break;
        }
        case PVRSRV_HAP_SINGLE_PROCESS:
        {
            
            
            psLinuxMemArea = NewAllocPagesLinuxMemArea(ui32Size, ui32AllocFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_OUT_OF_MEMORY;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }

        case PVRSRV_HAP_MULTI_PROCESS:
        {
            
#if defined(VIVT_CACHE) || defined(__sh__)
            
            ui32AllocFlags &= ~PVRSRV_HAP_CACHED;
#endif
            psLinuxMemArea = NewVMallocLinuxMemArea(ui32Size, ui32AllocFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_OUT_OF_MEMORY;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        default:
            PVR_DPF((PVR_DBG_ERROR, "OSAllocPages: invalid flags 0x%x\n", ui32AllocFlags));
            *ppvCpuVAddr = NULL;
            *phOSMemHandle = (IMG_HANDLE)0;
            return PVRSRV_ERROR_INVALID_PARAMS;
    }

    *ppvCpuVAddr = LinuxMemAreaToCpuVAddr(psLinuxMemArea);
    *phOSMemHandle = psLinuxMemArea;
    
    LinuxMemAreaRegister(psLinuxMemArea);

    return PVRSRV_OK;
}


PVRSRV_ERROR
OSFreePages(IMG_UINT32 ui32AllocFlags, IMG_UINT32 ui32Bytes, IMG_VOID *pvCpuVAddr, IMG_HANDLE hOSMemHandle)
{   
    LinuxMemArea *psLinuxMemArea;
    PVRSRV_ERROR eError;
    
    PVR_UNREFERENCED_PARAMETER(ui32Bytes);
    PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
    
    psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;

    switch(ui32AllocFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
            break;
        case PVRSRV_HAP_SINGLE_PROCESS:
        case PVRSRV_HAP_MULTI_PROCESS:
            eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
            if (eError != PVRSRV_OK)
            {
                PVR_DPF((PVR_DBG_ERROR,
                         "OSFreePages(ui32AllocFlags=0x%08X, ui32Bytes=%d, "
                                        "pvCpuVAddr=%p, hOSMemHandle=%p) FAILED!",
                         ui32AllocFlags, ui32Bytes, pvCpuVAddr, hOSMemHandle));
                return eError;
            }
            break;
        default:
            PVR_DPF((PVR_DBG_ERROR,"%s: invalid flags 0x%x\n",
                    __FUNCTION__, ui32AllocFlags));
            return PVRSRV_ERROR_INVALID_PARAMS;
    }

    LinuxMemAreaDeepFree(psLinuxMemArea);

    return PVRSRV_OK;
}


PVRSRV_ERROR
OSGetSubMemHandle(IMG_HANDLE hOSMemHandle,
                  IMG_UINT32 ui32ByteOffset,
                  IMG_UINT32 ui32Bytes,
                  IMG_UINT32 ui32Flags,
                  IMG_HANDLE *phOSMemHandleRet)
{
    LinuxMemArea *psParentLinuxMemArea, *psLinuxMemArea;
    PVRSRV_ERROR eError;

    psParentLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
    
    psLinuxMemArea = NewSubLinuxMemArea(psParentLinuxMemArea, ui32ByteOffset, ui32Bytes);
    if(!psLinuxMemArea)
    {
        *phOSMemHandleRet = NULL;
        return PVRSRV_ERROR_OUT_OF_MEMORY;
    }
    *phOSMemHandleRet = psLinuxMemArea;

    
    if(ui32Flags & PVRSRV_HAP_KERNEL_ONLY)
    {
        return PVRSRV_OK;
    }

    eError = PVRMMapRegisterArea(psLinuxMemArea);
    if(eError != PVRSRV_OK)
    {
        goto failed_register_area;
    }

    return PVRSRV_OK;

failed_register_area:
    *phOSMemHandleRet = NULL;
    LinuxMemAreaDeepFree(psLinuxMemArea);
    return eError;
}

PVRSRV_ERROR
OSReleaseSubMemHandle(IMG_VOID *hOSMemHandle, IMG_UINT32 ui32Flags)
{
    LinuxMemArea *psLinuxMemArea;
    PVRSRV_ERROR eError;
    
    psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
    PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC);
    
    if((ui32Flags & PVRSRV_HAP_KERNEL_ONLY) == 0)
    {
        eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
        if(eError != PVRSRV_OK)
        {
            return eError;
        }
    }
    LinuxMemAreaDeepFree(psLinuxMemArea);

    return PVRSRV_OK;
}


IMG_CPU_PHYADDR
OSMemHandleToCpuPAddr(IMG_VOID *hOSMemHandle, IMG_UINT32 ui32ByteOffset)
{
    PVR_ASSERT(hOSMemHandle);

    return LinuxMemAreaToCpuPAddr(hOSMemHandle, ui32ByteOffset);
}



IMG_VOID OSMemCopy(IMG_VOID *pvDst, IMG_VOID *pvSrc, IMG_UINT32 ui32Size)
{
#if defined(USE_UNOPTIMISED_MEMCPY)
    IMG_UINT8 *Src,*Dst;
    IMG_INT i;

    Src=(IMG_UINT8 *)pvSrc;
    Dst=(IMG_UINT8 *)pvDst;
    for(i=0;i<ui32Size;i++)
    {
        Dst[i]=Src[i];
    }
#else
    memcpy(pvDst, pvSrc, ui32Size);
#endif
}


IMG_VOID OSMemSet(IMG_VOID *pvDest, IMG_UINT8 ui8Value, IMG_UINT32 ui32Size)
{
#if defined(USE_UNOPTIMISED_MEMSET)
    IMG_UINT8 *Buff;
    IMG_INT i;

    Buff=(IMG_UINT8 *)pvDest;
    for(i=0;i<ui32Size;i++)
    {
        Buff[i]=ui8Value;
    }
#else
    memset(pvDest, (IMG_INT) ui8Value, (size_t) ui32Size);
#endif
}


IMG_CHAR *OSStringCopy(IMG_CHAR *pszDest, const IMG_CHAR *pszSrc)
{
    return (strcpy(pszDest, pszSrc));
}

IMG_INT32 OSSNPrintf(IMG_CHAR *pStr, IMG_UINT32 ui32Size, const IMG_CHAR *pszFormat, ...)
{
    va_list argList;
    IMG_INT32 iCount;

    va_start(argList, pszFormat);
    iCount = vsnprintf(pStr, (size_t)ui32Size, pszFormat, argList);
    va_end(argList);

    return iCount;
}

IMG_VOID OSBreakResourceLock (PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID)
{
    volatile IMG_UINT32 *pui32Access = (volatile IMG_UINT32 *)&psResource->ui32Lock;

    if(*pui32Access)
    {
        if(psResource->ui32ID == ui32ID)
        {
            psResource->ui32ID = 0;
            *pui32Access = 0;
        }
        else
        {
            PVR_DPF((PVR_DBG_MESSAGE,"OSBreakResourceLock: Resource is not locked for this process.")); 
        }
    }
    else
    {
        PVR_DPF((PVR_DBG_MESSAGE,"OSBreakResourceLock: Resource is not locked"));
    }
}


PVRSRV_ERROR OSCreateResource(PVRSRV_RESOURCE *psResource)
{
    psResource->ui32ID = 0;
    psResource->ui32Lock = 0;

    return PVRSRV_OK;
}


PVRSRV_ERROR OSDestroyResource (PVRSRV_RESOURCE *psResource)
{
    OSBreakResourceLock (psResource, psResource->ui32ID);

    return PVRSRV_OK;
}


PVRSRV_ERROR OSInitEnvData(IMG_PVOID *ppvEnvSpecificData)
{
    ENV_DATA		*psEnvData;
    PVRSRV_ERROR	eError;
    
    
    eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(ENV_DATA), (IMG_VOID **)&psEnvData, IMG_NULL,
        "Environment Data");
    if (eError != PVRSRV_OK)
    {
        return eError;
    }

    eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, PVRSRV_MAX_BRIDGE_IN_SIZE + PVRSRV_MAX_BRIDGE_OUT_SIZE, 
                    &psEnvData->pvBridgeData, IMG_NULL,
                    "Bridge Data");
    if (eError != PVRSRV_OK)
    {
        OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(ENV_DATA), psEnvData, IMG_NULL);
		
        return eError;
    }


    
    psEnvData->bMISRInstalled = IMG_FALSE;
    psEnvData->bLISRInstalled = IMG_FALSE;

    
    *ppvEnvSpecificData = psEnvData;

    return PVRSRV_OK;
}


PVRSRV_ERROR OSDeInitEnvData(IMG_PVOID pvEnvSpecificData)
{
    ENV_DATA		*psEnvData = (ENV_DATA*)pvEnvSpecificData;

    PVR_ASSERT(!psEnvData->bMISRInstalled);
    PVR_ASSERT(!psEnvData->bLISRInstalled);

    OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, PVRSRV_MAX_BRIDGE_IN_SIZE + PVRSRV_MAX_BRIDGE_OUT_SIZE, psEnvData->pvBridgeData, IMG_NULL);
    psEnvData->pvBridgeData = IMG_NULL;

    OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(ENV_DATA), pvEnvSpecificData, IMG_NULL);
	

    return PVRSRV_OK;
}


 
IMG_VOID OSReleaseThreadQuanta(IMG_VOID)
{
    schedule();
}


 
IMG_UINT32 OSClockus(IMG_VOID)
{
    IMG_UINT32 time, j = jiffies;

    time = j * (1000000 / HZ);

    return time;
}


IMG_VOID OSWaitus(IMG_UINT32 ui32Timeus)
{
    udelay(ui32Timeus);
}


IMG_VOID OSSleepms(IMG_UINT32 ui32Timems)
{
    msleep(ui32Timems);
}


 
IMG_HANDLE OSFuncHighResTimerCreate(IMG_VOID)
{
	
	return (IMG_HANDLE) 1;
}

 
IMG_UINT32 OSFuncHighResTimerGetus(IMG_HANDLE hTimer)
{
	return (IMG_UINT32) jiffies_to_usecs(jiffies);
}

 
IMG_VOID OSFuncHighResTimerDestroy(IMG_HANDLE hTimer)
{
	PVR_UNREFERENCED_PARAMETER(hTimer);
}

IMG_UINT32 OSGetCurrentProcessIDKM(IMG_VOID)
{
    if (in_interrupt())
    {
        return KERNEL_ID;
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
    return (IMG_UINT32)current->pgrp;
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
    return (IMG_UINT32)task_tgid_nr(current);
#else
    return (IMG_UINT32)current->tgid;
#endif
#endif
}


IMG_UINT32 OSGetPageSize(IMG_VOID)
{
#if defined(__sh__)
    IMG_UINT32 ui32ReturnValue = PAGE_SIZE;

    return (ui32ReturnValue);
#else
    return PAGE_SIZE;
#endif
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
static irqreturn_t DeviceISRWrapper(int irq, void *dev_id
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
        , struct pt_regs *regs
#endif
        )
{
    PVRSRV_DEVICE_NODE *psDeviceNode;
    IMG_BOOL bStatus = IMG_FALSE;

    PVR_UNREFERENCED_PARAMETER(irq);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
    PVR_UNREFERENCED_PARAMETER(regs);
#endif	
    psDeviceNode = (PVRSRV_DEVICE_NODE*)dev_id;
    if(!psDeviceNode)
    {
        PVR_DPF((PVR_DBG_ERROR, "DeviceISRWrapper: invalid params\n"));
        goto out;
    }

    bStatus = PVRSRVDeviceLISR(psDeviceNode);

    if (bStatus)
    {
		OSScheduleMISR((IMG_VOID *)psDeviceNode->psSysData);
    }

out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
    return bStatus ? IRQ_HANDLED : IRQ_NONE;
#endif
}



static irqreturn_t SystemISRWrapper(int irq, void *dev_id
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
        , struct pt_regs *regs
#endif
        )
{
    SYS_DATA *psSysData;
    IMG_BOOL bStatus = IMG_FALSE;

    PVR_UNREFERENCED_PARAMETER(irq);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19))
    PVR_UNREFERENCED_PARAMETER(regs);
#endif
    psSysData = (SYS_DATA *)dev_id;
    if(!psSysData)
    {
        PVR_DPF((PVR_DBG_ERROR, "SystemISRWrapper: invalid params\n"));
        goto out;
    }

    bStatus = PVRSRVSystemLISR(psSysData);

    if (bStatus)
    {
        OSScheduleMISR((IMG_VOID *)psSysData);
    }

out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
    return bStatus ? IRQ_HANDLED : IRQ_NONE;
#endif
}
PVRSRV_ERROR OSInstallDeviceLISR(IMG_VOID *pvSysData,
                                    IMG_UINT32 ui32Irq,
                                    IMG_CHAR *pszISRName,
                                    IMG_VOID *pvDeviceNode)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (psEnvData->bLISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSInstallDeviceLISR: An ISR has already been installed: IRQ %d cookie %p", psEnvData->ui32IRQ, psEnvData->pvISRCookie));
        return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
    }

    PVR_TRACE(("Installing device LISR %s on IRQ %d with cookie %p", pszISRName, ui32Irq, pvDeviceNode));

    if(request_irq(ui32Irq, DeviceISRWrapper,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
        SA_SHIRQ
#else
        IRQF_SHARED
#endif
        , pszISRName, pvDeviceNode))
    {
        PVR_DPF((PVR_DBG_ERROR,"OSInstallDeviceLISR: Couldn't install device LISR on IRQ %d", ui32Irq));

        return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
    }

    psEnvData->ui32IRQ = ui32Irq;
    psEnvData->pvISRCookie = pvDeviceNode;
    psEnvData->bLISRInstalled = IMG_TRUE;

    return PVRSRV_OK;	
}

PVRSRV_ERROR OSUninstallDeviceLISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (!psEnvData->bLISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSUninstallDeviceLISR: No LISR has been installed"));
        return PVRSRV_ERROR_ISR_NOT_INSTALLED;
    }
        
    PVR_TRACE(("Uninstalling device LISR on IRQ %d with cookie %p", psEnvData->ui32IRQ,  psEnvData->pvISRCookie));

    free_irq(psEnvData->ui32IRQ, psEnvData->pvISRCookie);

    psEnvData->bLISRInstalled = IMG_FALSE;

    return PVRSRV_OK;
}


PVRSRV_ERROR OSInstallSystemLISR(IMG_VOID *pvSysData, IMG_UINT32 ui32Irq)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (psEnvData->bLISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSInstallSystemLISR: An LISR has already been installed: IRQ %d cookie %p", psEnvData->ui32IRQ, psEnvData->pvISRCookie));
        return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
    }

    PVR_TRACE(("Installing system LISR on IRQ %d with cookie %p", ui32Irq, pvSysData));

    if(request_irq(ui32Irq, SystemISRWrapper,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
        SA_SHIRQ
#else
        IRQF_SHARED
#endif
        , PVRSRV_MODNAME, pvSysData))
    {
        PVR_DPF((PVR_DBG_ERROR,"OSInstallSystemLISR: Couldn't install system LISR on IRQ %d", ui32Irq));

        return PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
    }

    psEnvData->ui32IRQ = ui32Irq;
    psEnvData->pvISRCookie = pvSysData;
    psEnvData->bLISRInstalled = IMG_TRUE;

    return PVRSRV_OK;	
}


PVRSRV_ERROR OSUninstallSystemLISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (!psEnvData->bLISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSUninstallSystemLISR: No LISR has been installed"));
        return PVRSRV_ERROR_ISR_NOT_INSTALLED;
    }

    PVR_TRACE(("Uninstalling system LISR on IRQ %d with cookie %p", psEnvData->ui32IRQ, psEnvData->pvISRCookie));

    free_irq(psEnvData->ui32IRQ, psEnvData->pvISRCookie);

    psEnvData->bLISRInstalled = IMG_FALSE;

    return PVRSRV_OK;
}

#if defined(PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE)
static void MISRWrapper(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
			void *data
#else
			struct work_struct *data
#endif
)
{
	ENV_DATA *psEnvData = container_of(data, ENV_DATA, sMISRWork);
	SYS_DATA *psSysData  = (SYS_DATA *)psEnvData->pvMISRData;

	PVRSRVMISR(psSysData);
}


PVRSRV_ERROR OSInstallMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

	if (psEnvData->bMISRInstalled)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: An MISR has already been installed"));
		return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
	}

	PVR_TRACE(("Installing MISR with cookie %p", pvSysData));

	psEnvData->psWorkQueue = create_singlethread_workqueue("pvr_workqueue");

	if (psEnvData->psWorkQueue == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: create_singlethreaded_workqueue failed"));
		return PVRSRV_ERROR_UNABLE_TO_CREATE_THREAD;
	}

	INIT_WORK(&psEnvData->sMISRWork, MISRWrapper
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		, (void *)&psEnvData->sMISRWork
#endif
				);

	psEnvData->pvMISRData = pvSysData;
	psEnvData->bMISRInstalled = IMG_TRUE;

	return PVRSRV_OK;
}


PVRSRV_ERROR OSUninstallMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

	if (!psEnvData->bMISRInstalled)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSUninstallMISR: No MISR has been installed"));
		return PVRSRV_ERROR_ISR_NOT_INSTALLED;
	}

	PVR_TRACE(("Uninstalling MISR"));

	destroy_workqueue(psEnvData->psWorkQueue);

	psEnvData->bMISRInstalled = IMG_FALSE;

	return PVRSRV_OK;
}


PVRSRV_ERROR OSScheduleMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA*)psSysData->pvEnvSpecificData;

	if (psEnvData->bMISRInstalled)
	{
		queue_work(psEnvData->psWorkQueue, &psEnvData->sMISRWork);
	}

	return PVRSRV_OK;	
}
#else	
#if defined(PVR_LINUX_MISR_USING_WORKQUEUE)
static void MISRWrapper(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
			void *data
#else
			struct work_struct *data
#endif
)
{
	ENV_DATA *psEnvData = container_of(data, ENV_DATA, sMISRWork);
	SYS_DATA *psSysData  = (SYS_DATA *)psEnvData->pvMISRData;

	PVRSRVMISR(psSysData);
}


PVRSRV_ERROR OSInstallMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

	if (psEnvData->bMISRInstalled)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: An MISR has already been installed"));
		return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
	}

	PVR_TRACE(("Installing MISR with cookie %p", pvSysData));

	INIT_WORK(&psEnvData->sMISRWork, MISRWrapper
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20))
		, (void *)&psEnvData->sMISRWork
#endif
				);

	psEnvData->pvMISRData = pvSysData;
	psEnvData->bMISRInstalled = IMG_TRUE;

	return PVRSRV_OK;
}


PVRSRV_ERROR OSUninstallMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

	if (!psEnvData->bMISRInstalled)
	{
		PVR_DPF((PVR_DBG_ERROR, "OSUninstallMISR: No MISR has been installed"));
		return PVRSRV_ERROR_ISR_NOT_INSTALLED;
	}

	PVR_TRACE(("Uninstalling MISR"));

	flush_scheduled_work();

	psEnvData->bMISRInstalled = IMG_FALSE;

	return PVRSRV_OK;
}


PVRSRV_ERROR OSScheduleMISR(IMG_VOID *pvSysData)
{
	SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
	ENV_DATA *psEnvData = (ENV_DATA*)psSysData->pvEnvSpecificData;

	if (psEnvData->bMISRInstalled)
	{
		schedule_work(&psEnvData->sMISRWork);
	}

	return PVRSRV_OK;	
}

#else	


static void MISRWrapper(unsigned long data)
{
    SYS_DATA *psSysData;

    psSysData = (SYS_DATA *)data;
    
    PVRSRVMISR(psSysData);
}


PVRSRV_ERROR OSInstallMISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (psEnvData->bMISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSInstallMISR: An MISR has already been installed"));
        return PVRSRV_ERROR_ISR_ALREADY_INSTALLED;
    }

    PVR_TRACE(("Installing MISR with cookie %p", pvSysData));

    tasklet_init(&psEnvData->sMISRTasklet, MISRWrapper, (unsigned long)pvSysData);

    psEnvData->bMISRInstalled = IMG_TRUE;

    return PVRSRV_OK;
}


PVRSRV_ERROR OSUninstallMISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA *)psSysData->pvEnvSpecificData;

    if (!psEnvData->bMISRInstalled)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSUninstallMISR: No MISR has been installed"));
        return PVRSRV_ERROR_ISR_NOT_INSTALLED;
    }

    PVR_TRACE(("Uninstalling MISR"));

    tasklet_kill(&psEnvData->sMISRTasklet);

    psEnvData->bMISRInstalled = IMG_FALSE;

    return PVRSRV_OK;
}

PVRSRV_ERROR OSScheduleMISR(IMG_VOID *pvSysData)
{
    SYS_DATA *psSysData = (SYS_DATA*)pvSysData;
    ENV_DATA *psEnvData = (ENV_DATA*)psSysData->pvEnvSpecificData;

    if (psEnvData->bMISRInstalled)
    {
        tasklet_schedule(&psEnvData->sMISRTasklet);
    }

    return PVRSRV_OK;	
}

#endif 
#endif 

#endif 

IMG_VOID OSPanic(IMG_VOID)
{
	BUG();
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
#define	OS_TAS(p)	xchg((p), 1)
#else
#define	OS_TAS(p)	tas(p)
#endif
PVRSRV_ERROR OSLockResource ( PVRSRV_RESOURCE 	*psResource,
                                IMG_UINT32 			ui32ID)

{
    PVRSRV_ERROR eError = PVRSRV_OK;

    if(!OS_TAS(&psResource->ui32Lock))
        psResource->ui32ID = ui32ID;
    else
        eError = PVRSRV_ERROR_UNABLE_TO_LOCK_RESOURCE;

    return eError;
}


PVRSRV_ERROR OSUnlockResource (PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID)
{
    volatile IMG_UINT32 *pui32Access = (volatile IMG_UINT32 *)&psResource->ui32Lock;
    PVRSRV_ERROR eError = PVRSRV_OK;

    if(*pui32Access)
    {
        if(psResource->ui32ID == ui32ID)
        {
            psResource->ui32ID = 0;
            smp_mb();
            *pui32Access = 0;
        }
        else
        {
            PVR_DPF((PVR_DBG_ERROR,"OSUnlockResource: Resource %p is not locked with expected value.", psResource)); 
            PVR_DPF((PVR_DBG_MESSAGE,"Should be %x is actually %x", ui32ID, psResource->ui32ID));
            eError = PVRSRV_ERROR_INVALID_LOCK_ID;
        }
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR,"OSUnlockResource: Resource %p is not locked", psResource));
        eError = PVRSRV_ERROR_RESOURCE_NOT_LOCKED;
    }
    
    return eError;
}


IMG_BOOL OSIsResourceLocked (PVRSRV_RESOURCE *psResource, IMG_UINT32 ui32ID)
{
    volatile IMG_UINT32 *pui32Access = (volatile IMG_UINT32 *)&psResource->ui32Lock;

    return 	(*(volatile IMG_UINT32 *)pui32Access == 1) && (psResource->ui32ID == ui32ID)
            ?	IMG_TRUE
            :	IMG_FALSE;
}


#if !defined(SYS_CUSTOM_POWERLOCK_WRAP)
PVRSRV_ERROR OSPowerLockWrap(IMG_BOOL bTryLock)
{
	PVR_UNREFERENCED_PARAMETER(bTryLock);	

	return PVRSRV_OK;
}

IMG_VOID OSPowerLockUnwrap (IMG_VOID)
{
}
#endif 


IMG_CPU_PHYADDR OSMapLinToCPUPhys(IMG_HANDLE hOSMemHandle,
								  IMG_VOID *pvLinAddr)
{
    IMG_CPU_PHYADDR CpuPAddr;
    LinuxMemArea *psLinuxMemArea;
	IMG_UINTPTR_T uiByteOffset;
	IMG_UINT32 ui32ByteOffset;

	PVR_ASSERT(hOSMemHandle != IMG_NULL);

	

	psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;

	uiByteOffset = (IMG_UINTPTR_T)pvLinAddr - (IMG_UINTPTR_T)LinuxMemAreaToCpuVAddr(psLinuxMemArea);
	ui32ByteOffset = (IMG_UINT32)uiByteOffset;

	CpuPAddr = LinuxMemAreaToCpuPAddr(hOSMemHandle, ui32ByteOffset);

	return CpuPAddr;
}


IMG_VOID *
OSMapPhysToLin(IMG_CPU_PHYADDR BasePAddr,
               IMG_UINT32 ui32Bytes,
               IMG_UINT32 ui32MappingFlags,
               IMG_HANDLE *phOSMemHandle)
{
    if(ui32MappingFlags & PVRSRV_HAP_KERNEL_ONLY)
    {
	
	if(phOSMemHandle == IMG_NULL)
	{
		IMG_VOID *pvIORemapCookie;
		pvIORemapCookie = IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags);
		if(pvIORemapCookie == IMG_NULL)
		{
		    return IMG_NULL;
		}
		return pvIORemapCookie;
	}
	else
	{
		LinuxMemArea *psLinuxMemArea = NewIORemapLinuxMemArea(BasePAddr, ui32Bytes, ui32MappingFlags);

		if(psLinuxMemArea == IMG_NULL)
		{
		    return IMG_NULL;
		}

		*phOSMemHandle = (IMG_HANDLE)psLinuxMemArea;
		return LinuxMemAreaToCpuVAddr(psLinuxMemArea);
	}
    }

    PVR_DPF((PVR_DBG_ERROR,
             "OSMapPhysToLin should only be used with PVRSRV_HAP_KERNEL_ONLY "
             " (Use OSReservePhys otherwise)"));

    return IMG_NULL;
}

IMG_BOOL
OSUnMapPhysToLin(IMG_VOID *pvLinAddr, IMG_UINT32 ui32Bytes, IMG_UINT32 ui32MappingFlags, IMG_HANDLE hOSMemHandle)
{
    PVR_TRACE(("%s: unmapping %d bytes from %p", __FUNCTION__, ui32Bytes, pvLinAddr));

    PVR_UNREFERENCED_PARAMETER(ui32Bytes);	

    if(ui32MappingFlags & PVRSRV_HAP_KERNEL_ONLY)
    {
        if (hOSMemHandle == IMG_NULL)
	{
		IOUnmapWrapper(pvLinAddr);
	}
	else
	{
		LinuxMemArea *psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;

		PVR_ASSERT(LinuxMemAreaToCpuVAddr(psLinuxMemArea) == pvLinAddr);
		
		FreeIORemapLinuxMemArea(psLinuxMemArea);
	}

        return IMG_TRUE;
    }

    PVR_DPF((PVR_DBG_ERROR,
                 "OSUnMapPhysToLin should only be used with PVRSRV_HAP_KERNEL_ONLY "
                 " (Use OSUnReservePhys otherwise)"));
    return IMG_FALSE;
}

static PVRSRV_ERROR
RegisterExternalMem(IMG_SYS_PHYADDR *pBasePAddr,
          IMG_VOID *pvCPUVAddr,
              IMG_UINT32 ui32Bytes,
          IMG_BOOL bPhysContig,
              IMG_UINT32 ui32MappingFlags,
              IMG_HANDLE *phOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea;

    switch(ui32MappingFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
        {
        psLinuxMemArea = NewExternalKVLinuxMemArea(pBasePAddr, pvCPUVAddr, ui32Bytes, bPhysContig, ui32MappingFlags);
        
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            break;
        }
        case PVRSRV_HAP_SINGLE_PROCESS:
        {
        psLinuxMemArea = NewExternalKVLinuxMemArea(pBasePAddr, pvCPUVAddr, ui32Bytes, bPhysContig, ui32MappingFlags);

            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        case PVRSRV_HAP_MULTI_PROCESS:
        {
            
#if defined(VIVT_CACHE) || defined(__sh__)
            
            ui32MappingFlags &= ~PVRSRV_HAP_CACHED;
#endif
        psLinuxMemArea = NewExternalKVLinuxMemArea(pBasePAddr, pvCPUVAddr, ui32Bytes, bPhysContig, ui32MappingFlags);

            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        default:
            PVR_DPF((PVR_DBG_ERROR,"OSRegisterMem : invalid flags 0x%x\n", ui32MappingFlags));
            *phOSMemHandle = (IMG_HANDLE)0;
            return PVRSRV_ERROR_INVALID_FLAGS;
    }
    
    *phOSMemHandle = (IMG_HANDLE)psLinuxMemArea;

    LinuxMemAreaRegister(psLinuxMemArea);

    return PVRSRV_OK;
}


PVRSRV_ERROR
OSRegisterMem(IMG_CPU_PHYADDR BasePAddr,
              IMG_VOID *pvCPUVAddr,
              IMG_UINT32 ui32Bytes,
              IMG_UINT32 ui32MappingFlags,
              IMG_HANDLE *phOSMemHandle)
{
    IMG_SYS_PHYADDR SysPAddr = SysCpuPAddrToSysPAddr(BasePAddr);

    return RegisterExternalMem(&SysPAddr, pvCPUVAddr, ui32Bytes, IMG_TRUE, ui32MappingFlags, phOSMemHandle);
}


PVRSRV_ERROR OSRegisterDiscontigMem(IMG_SYS_PHYADDR *pBasePAddr, IMG_VOID *pvCPUVAddr, IMG_UINT32 ui32Bytes, IMG_UINT32 ui32MappingFlags, IMG_HANDLE *phOSMemHandle)
{
    return RegisterExternalMem(pBasePAddr, pvCPUVAddr, ui32Bytes, IMG_FALSE, ui32MappingFlags, phOSMemHandle);
}


PVRSRV_ERROR
OSUnRegisterMem (IMG_VOID *pvCpuVAddr,
                IMG_UINT32 ui32Bytes,
                IMG_UINT32 ui32MappingFlags,
                IMG_HANDLE hOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
    PVRSRV_ERROR eError;

    PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
    PVR_UNREFERENCED_PARAMETER(ui32Bytes);

    switch(ui32MappingFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
            break;
        case PVRSRV_HAP_SINGLE_PROCESS:
        case PVRSRV_HAP_MULTI_PROCESS:
        {
            eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
            if (eError != PVRSRV_OK)
            {
                 PVR_DPF((PVR_DBG_ERROR, "%s(%p, %d, 0x%08X, %p) FAILED!",
                          __FUNCTION__, pvCpuVAddr, ui32Bytes,
                          ui32MappingFlags, hOSMemHandle));
                return eError;
            }
            break;
        }
        default:
        {
            PVR_DPF((PVR_DBG_ERROR, "OSUnRegisterMem : invalid flags 0x%x", ui32MappingFlags));
            return PVRSRV_ERROR_INVALID_PARAMS;
        }
    }

    LinuxMemAreaDeepFree(psLinuxMemArea);

    return PVRSRV_OK;
}

PVRSRV_ERROR OSUnRegisterDiscontigMem(IMG_VOID *pvCpuVAddr, IMG_UINT32 ui32Bytes, IMG_UINT32 ui32Flags, IMG_HANDLE hOSMemHandle)
{
    return OSUnRegisterMem(pvCpuVAddr, ui32Bytes, ui32Flags, hOSMemHandle);
}

PVRSRV_ERROR
OSReservePhys(IMG_CPU_PHYADDR BasePAddr,
              IMG_UINT32 ui32Bytes,
              IMG_UINT32 ui32MappingFlags,
              IMG_VOID **ppvCpuVAddr,
              IMG_HANDLE *phOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea;

#if 0
    
    if(ui32MappingFlags & PVRSRV_HAP_SINGLE_PROCESS)
    {
        ui32MappingFlags &= ~PVRSRV_HAP_SINGLE_PROCESS;
        ui32MappingFlags |= PVRSRV_HAP_MULTI_PROCESS;
    }
#endif

    switch(ui32MappingFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
        {
            
            psLinuxMemArea = NewIORemapLinuxMemArea(BasePAddr, ui32Bytes, ui32MappingFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            break;
        }
        case PVRSRV_HAP_SINGLE_PROCESS:
        {
            
            psLinuxMemArea = NewIOLinuxMemArea(BasePAddr, ui32Bytes, ui32MappingFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        case PVRSRV_HAP_MULTI_PROCESS:
        {
            
#if defined(VIVT_CACHE) || defined(__sh__)
            
            ui32MappingFlags &= ~PVRSRV_HAP_CACHED;
#endif
            psLinuxMemArea = NewIORemapLinuxMemArea(BasePAddr, ui32Bytes, ui32MappingFlags);
            if(!psLinuxMemArea)
            {
                return PVRSRV_ERROR_BAD_MAPPING;
            }
            PVRMMapRegisterArea(psLinuxMemArea);
            break;
        }
        default:
            PVR_DPF((PVR_DBG_ERROR,"OSMapPhysToLin : invalid flags 0x%x\n", ui32MappingFlags));
            *ppvCpuVAddr = NULL;
            *phOSMemHandle = (IMG_HANDLE)0;
            return PVRSRV_ERROR_INVALID_FLAGS;
    }

    *phOSMemHandle = (IMG_HANDLE)psLinuxMemArea;
    *ppvCpuVAddr = LinuxMemAreaToCpuVAddr(psLinuxMemArea);

    LinuxMemAreaRegister(psLinuxMemArea);

    return PVRSRV_OK;
}

PVRSRV_ERROR
OSUnReservePhys(IMG_VOID *pvCpuVAddr,
                IMG_UINT32 ui32Bytes,
                IMG_UINT32 ui32MappingFlags,
                IMG_HANDLE hOSMemHandle)
{
    LinuxMemArea *psLinuxMemArea;
    PVRSRV_ERROR eError;

    PVR_UNREFERENCED_PARAMETER(pvCpuVAddr);
    PVR_UNREFERENCED_PARAMETER(ui32Bytes);

    psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
    
    switch(ui32MappingFlags & PVRSRV_HAP_MAPTYPE_MASK)
    {
        case PVRSRV_HAP_KERNEL_ONLY:
            break;
        case PVRSRV_HAP_SINGLE_PROCESS:
        case PVRSRV_HAP_MULTI_PROCESS:
        {
            eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
            if (eError != PVRSRV_OK)
            {
                 PVR_DPF((PVR_DBG_ERROR, "%s(%p, %d, 0x%08X, %p) FAILED!",
                          __FUNCTION__, pvCpuVAddr, ui32Bytes,
                          ui32MappingFlags, hOSMemHandle));
                return eError;
            }
            break;
        }
        default:
        {
            PVR_DPF((PVR_DBG_ERROR, "OSUnMapPhysToLin : invalid flags 0x%x", ui32MappingFlags));
            return PVRSRV_ERROR_INVALID_PARAMS;
        }
    }
    
    LinuxMemAreaDeepFree(psLinuxMemArea);

    return PVRSRV_OK;
}


PVRSRV_ERROR OSBaseAllocContigMemory(IMG_UINT32 ui32Size, IMG_CPU_VIRTADDR *pvLinAddr, IMG_CPU_PHYADDR *psPhysAddr)
{
#if !defined(NO_HARDWARE)
    PVR_UNREFERENCED_PARAMETER(ui32Size);
    PVR_UNREFERENCED_PARAMETER(pvLinAddr);
    PVR_UNREFERENCED_PARAMETER(psPhysAddr);
    PVR_DPF((PVR_DBG_ERROR, "%s: Not available", __FUNCTION__));

    return PVRSRV_ERROR_OUT_OF_MEMORY;
#else
    IMG_VOID *pvKernLinAddr;

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    pvKernLinAddr = _KMallocWrapper(ui32Size, GFP_KERNEL, __FILE__, __LINE__);
#else
    pvKernLinAddr = KMallocWrapper(ui32Size, GFP_KERNEL);
#endif
    if (!pvKernLinAddr)
    {
    return PVRSRV_ERROR_OUT_OF_MEMORY;
    }

    *pvLinAddr = pvKernLinAddr;

    psPhysAddr->uiAddr = virt_to_phys(pvKernLinAddr);

    return PVRSRV_OK;
#endif	
}


PVRSRV_ERROR OSBaseFreeContigMemory(IMG_UINT32 ui32Size, IMG_CPU_VIRTADDR pvLinAddr, IMG_CPU_PHYADDR psPhysAddr)
{
#if !defined(NO_HARDWARE)
    PVR_UNREFERENCED_PARAMETER(ui32Size);
    PVR_UNREFERENCED_PARAMETER(pvLinAddr);
    PVR_UNREFERENCED_PARAMETER(psPhysAddr.uiAddr);

    PVR_DPF((PVR_DBG_WARNING, "%s: Not available", __FUNCTION__));
#else
    PVR_UNREFERENCED_PARAMETER(ui32Size);
    PVR_UNREFERENCED_PARAMETER(psPhysAddr.uiAddr);

    KFreeWrapper(pvLinAddr);
#endif
    return PVRSRV_OK;
}

IMG_UINT32 OSReadHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset)
{
#if !defined(NO_HARDWARE)
    return (IMG_UINT32) readl((IMG_PBYTE)pvLinRegBaseAddr+ui32Offset);
#else
    return *(IMG_UINT32 *)((IMG_PBYTE)pvLinRegBaseAddr+ui32Offset);
#endif
}

IMG_VOID OSWriteHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset, IMG_UINT32 ui32Value)
{
#if !defined(NO_HARDWARE)
    writel(ui32Value, (IMG_PBYTE)pvLinRegBaseAddr+ui32Offset);
#else
    *(IMG_UINT32 *)((IMG_PBYTE)pvLinRegBaseAddr+ui32Offset) = ui32Value;
#endif
}

#if defined(CONFIG_PCI) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))

PVRSRV_PCI_DEV_HANDLE OSPCISetDev(IMG_VOID *pvPCICookie, HOST_PCI_INIT_FLAGS eFlags)
{
    int err;
    IMG_UINT32 i;
    PVR_PCI_DEV *psPVRPCI;

    PVR_TRACE(("OSPCISetDev"));

    if(OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psPVRPCI), (IMG_VOID **)&psPVRPCI, IMG_NULL,
        "PCI Device") != PVRSRV_OK)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCISetDev: Couldn't allocate PVR PCI structure"));
        return IMG_NULL;
    }

    psPVRPCI->psPCIDev = (struct pci_dev *)pvPCICookie;
    psPVRPCI->ePCIFlags = eFlags;

    err = pci_enable_device(psPVRPCI->psPCIDev);
    if (err != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCISetDev: Couldn't enable device (%d)", err));
        return IMG_NULL;
    }

    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	 
    {
        pci_set_master(psPVRPCI->psPCIDev);
    }

    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_MSI)		 
    {
#if defined(CONFIG_PCI_MSI)
        err = pci_enable_msi(psPVRPCI->psPCIDev);
        if (err != 0)
        {
            PVR_DPF((PVR_DBG_WARNING, "OSPCISetDev: Couldn't enable MSI (%d)", err));
            psPVRPCI->ePCIFlags &= ~HOST_PCI_INIT_FLAG_MSI;	 
        }
#else
        PVR_DPF((PVR_DBG_WARNING, "OSPCISetDev: MSI support not enabled in the kernel"));
#endif
    }

    
    for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
        psPVRPCI->abPCIResourceInUse[i] = IMG_FALSE;
    }

    return (PVRSRV_PCI_DEV_HANDLE)psPVRPCI;
}

PVRSRV_PCI_DEV_HANDLE OSPCIAcquireDev(IMG_UINT16 ui16VendorID, IMG_UINT16 ui16DeviceID, HOST_PCI_INIT_FLAGS eFlags)
{
    struct pci_dev *psPCIDev;

    psPCIDev = pci_get_device(ui16VendorID, ui16DeviceID, NULL);
    if (psPCIDev == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCIAcquireDev: Couldn't acquire device"));
        return IMG_NULL;
    }

    return OSPCISetDev((IMG_VOID *)psPCIDev, eFlags);
}

PVRSRV_ERROR OSPCIIRQ(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 *pui32IRQ)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;

    *pui32IRQ = psPVRPCI->psPCIDev->irq;

    return PVRSRV_OK;
}

enum HOST_PCI_ADDR_RANGE_FUNC
{
    HOST_PCI_ADDR_RANGE_FUNC_LEN,
    HOST_PCI_ADDR_RANGE_FUNC_START,
    HOST_PCI_ADDR_RANGE_FUNC_END,
    HOST_PCI_ADDR_RANGE_FUNC_REQUEST,
    HOST_PCI_ADDR_RANGE_FUNC_RELEASE
};

static IMG_UINT32 OSPCIAddrRangeFunc(enum HOST_PCI_ADDR_RANGE_FUNC eFunc,
                                     PVRSRV_PCI_DEV_HANDLE hPVRPCI,
                                     IMG_UINT32 ui32Index)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;

    if (ui32Index >= DEVICE_COUNT_RESOURCE)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCIAddrRangeFunc: Index out of range"));
        return 0;

    }

    switch (eFunc)
    {
        case HOST_PCI_ADDR_RANGE_FUNC_LEN:
            return pci_resource_len(psPVRPCI->psPCIDev, ui32Index);
        case HOST_PCI_ADDR_RANGE_FUNC_START:
            return pci_resource_start(psPVRPCI->psPCIDev, ui32Index);
        case HOST_PCI_ADDR_RANGE_FUNC_END:
            return pci_resource_end(psPVRPCI->psPCIDev, ui32Index);
        case HOST_PCI_ADDR_RANGE_FUNC_REQUEST:
        {
            int err;

            err = pci_request_region(psPVRPCI->psPCIDev, (IMG_INT)ui32Index, PVRSRV_MODNAME);
            if (err != 0)
            {
                PVR_DPF((PVR_DBG_ERROR, "OSPCIAddrRangeFunc: pci_request_region_failed (%d)", err));
                return 0;
            }
            psPVRPCI->abPCIResourceInUse[ui32Index] = IMG_TRUE;
            return 1;
        }
        case HOST_PCI_ADDR_RANGE_FUNC_RELEASE:
            if (psPVRPCI->abPCIResourceInUse[ui32Index])
            {
                pci_release_region(psPVRPCI->psPCIDev, (IMG_INT)ui32Index);
                psPVRPCI->abPCIResourceInUse[ui32Index] = IMG_FALSE;
            }
            return 1;
        default:
            PVR_DPF((PVR_DBG_ERROR, "OSPCIAddrRangeFunc: Unknown function"));
            break;
    }

    return 0;
}

IMG_UINT32 OSPCIAddrRangeLen(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_LEN, hPVRPCI, ui32Index); 
}

IMG_UINT32 OSPCIAddrRangeStart(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_START, hPVRPCI, ui32Index); 
}

IMG_UINT32 OSPCIAddrRangeEnd(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_END, hPVRPCI, ui32Index); 
}

PVRSRV_ERROR OSPCIRequestAddrRange(PVRSRV_PCI_DEV_HANDLE hPVRPCI,
                                   IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_REQUEST, hPVRPCI, ui32Index) == 0 ? PVRSRV_ERROR_PCI_CALL_FAILED : PVRSRV_OK;
}

PVRSRV_ERROR OSPCIReleaseAddrRange(PVRSRV_PCI_DEV_HANDLE hPVRPCI, IMG_UINT32 ui32Index)
{
    return OSPCIAddrRangeFunc(HOST_PCI_ADDR_RANGE_FUNC_RELEASE, hPVRPCI, ui32Index) == 0 ? PVRSRV_ERROR_PCI_CALL_FAILED : PVRSRV_OK;
}

PVRSRV_ERROR OSPCIReleaseDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;
    int i;

    PVR_TRACE(("OSPCIReleaseDev"));

    
    for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
        if (psPVRPCI->abPCIResourceInUse[i])
        {
            PVR_TRACE(("OSPCIReleaseDev: Releasing Address range %d", i));
            pci_release_region(psPVRPCI->psPCIDev, i);
            psPVRPCI->abPCIResourceInUse[i] = IMG_FALSE;
        }
    }

#if defined(CONFIG_PCI_MSI)
    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_MSI)		 
    {
        pci_disable_msi(psPVRPCI->psPCIDev);
    }
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	 
    {
        pci_clear_master(psPVRPCI->psPCIDev);
    }
#endif
    pci_disable_device(psPVRPCI->psPCIDev);

    OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psPVRPCI), (IMG_VOID *)psPVRPCI, IMG_NULL);
	

    return PVRSRV_OK;
}

PVRSRV_ERROR OSPCISuspendDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;
    int i;
    int err;

    PVR_TRACE(("OSPCISuspendDev"));

    
    for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
        if (psPVRPCI->abPCIResourceInUse[i])
        {
            pci_release_region(psPVRPCI->psPCIDev, i);
        }
    }

    err = pci_save_state(psPVRPCI->psPCIDev);
    if (err != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCISuspendDev: pci_save_state_failed (%d)", err));
        return PVRSRV_ERROR_PCI_CALL_FAILED;
    }

    pci_disable_device(psPVRPCI->psPCIDev);

    err = pci_set_power_state(psPVRPCI->psPCIDev, pci_choose_state(psPVRPCI->psPCIDev, PMSG_SUSPEND));
    switch(err)
    {
        case 0:
            break;
        case -EIO:
            PVR_DPF((PVR_DBG_WARNING, "OSPCISuspendDev: device doesn't support PCI PM"));
            break;
        case -EINVAL:
            PVR_DPF((PVR_DBG_ERROR, "OSPCISuspendDev: can't enter requested power state"));
            break;
        default:
            PVR_DPF((PVR_DBG_ERROR, "OSPCISuspendDev: pci_set_power_state failed (%d)", err));
            break;
    }

    return PVRSRV_OK;
}

PVRSRV_ERROR OSPCIResumeDev(PVRSRV_PCI_DEV_HANDLE hPVRPCI)
{
    PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)hPVRPCI;
    int err;
    int i;

    PVR_TRACE(("OSPCIResumeDev"));

    err = pci_set_power_state(psPVRPCI->psPCIDev, pci_choose_state(psPVRPCI->psPCIDev, PMSG_ON));
    switch(err)
    {
        case 0:
            break;
        case -EIO:
            PVR_DPF((PVR_DBG_WARNING, "OSPCIResumeDev: device doesn't support PCI PM"));
            break;
        case -EINVAL:
            PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: can't enter requested power state"));
            return PVRSRV_ERROR_UNKNOWN_POWER_STATE;
        default:
            PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: pci_set_power_state failed (%d)", err));
            return PVRSRV_ERROR_UNKNOWN_POWER_STATE;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
    pci_restore_state(psPVRPCI->psPCIDev);
#else
    err = pci_restore_state(psPVRPCI->psPCIDev);
    if (err != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: pci_restore_state failed (%d)", err));
        return PVRSRV_ERROR_PCI_CALL_FAILED;
    }
#endif

    err = pci_enable_device(psPVRPCI->psPCIDev);
    if (err != 0)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: Couldn't enable device (%d)", err));
        return PVRSRV_ERROR_PCI_CALL_FAILED;
    }

    if (psPVRPCI->ePCIFlags & HOST_PCI_INIT_FLAG_BUS_MASTER)	 
        pci_set_master(psPVRPCI->psPCIDev);

    
    for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
    {
        if (psPVRPCI->abPCIResourceInUse[i])
        {
            err = pci_request_region(psPVRPCI->psPCIDev, i, PVRSRV_MODNAME);
            if (err != 0)
            {
                PVR_DPF((PVR_DBG_ERROR, "OSPCIResumeDev: pci_request_region_failed (region %d, error %d)", i, err));
            }
        }

    }

    return PVRSRV_OK;
}

#endif 

#define	OS_MAX_TIMERS	8

/* Timer callback strucure used by OSAddTimer */
typedef struct TIMER_CALLBACK_DATA_TAG
{
    IMG_BOOL			bInUse;
    PFN_TIMER_FUNC		pfnTimerFunc;
    IMG_VOID 			*pvData;
    struct timer_list		sTimer;
    IMG_UINT32			ui32Delay;
    IMG_BOOL			bActive;
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    struct work_struct		sWork;
#endif
}TIMER_CALLBACK_DATA;

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
static struct workqueue_struct	*psTimerWorkQueue;
#endif

static TIMER_CALLBACK_DATA sTimers[OS_MAX_TIMERS];

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
DEFINE_MUTEX(sTimerStructLock);
#else
 
static spinlock_t sTimerStructLock = __SPIN_LOCK_UNLOCKED(sTimerStructLock);
#endif

static void OSTimerCallbackBody(TIMER_CALLBACK_DATA *psTimerCBData)
{
    if (!psTimerCBData->bActive)
        return;

    
    psTimerCBData->pfnTimerFunc(psTimerCBData->pvData);
    
    
    mod_timer(&psTimerCBData->sTimer, psTimerCBData->ui32Delay + jiffies);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
 /*!
 ******************************************************************************

 @Function      OSTimerCallbackWrapper

 @Description   OS specific timer callback wrapper function

 @Input         psTimer    Timer list structure

*/ /**************************************************************************/
static void OSTimerCallbackWrapper(struct timer_list *psTimer)
{
	TIMER_CALLBACK_DATA *psTimerCBData = from_timer(psTimerCBData, psTimer, sTimer);
#else
/*!
******************************************************************************

 @Function	OSTimerCallbackWrapper

 @Description 	OS specific timer callback wrapper function

 @Input    ui32Data : timer callback data

 @Return   NONE

******************************************************************************/
static IMG_VOID OSTimerCallbackWrapper(IMG_UINTPTR_T uiData)
{
    TIMER_CALLBACK_DATA	*psTimerCBData = (TIMER_CALLBACK_DATA*)uiData;

#endif

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    int res;

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    res = queue_work(psTimerWorkQueue, &psTimerCBData->sWork);
#else
    res = schedule_work(&psTimerCBData->sWork);
#endif
    if (res == 0)
    {
        PVR_DPF((PVR_DBG_WARNING, "OSTimerCallbackWrapper: work already queued"));		
    }
#else
    OSTimerCallbackBody(psTimerCBData);
#endif
}


#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
static void OSTimerWorkQueueCallBack(struct work_struct *psWork)
{
    TIMER_CALLBACK_DATA *psTimerCBData = container_of(psWork, TIMER_CALLBACK_DATA, sWork);

    OSTimerCallbackBody(psTimerCBData);
}
#endif

IMG_HANDLE OSAddTimer(PFN_TIMER_FUNC pfnTimerFunc, IMG_VOID *pvData, IMG_UINT32 ui32MsTimeout)
{
    TIMER_CALLBACK_DATA	*psTimerCBData;
    IMG_UINTPTR_T		ui;
#if !(defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE))
    unsigned long		ulLockFlags;
#endif

    /* check callback */
    if(!pfnTimerFunc)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSAddTimer: passed invalid callback"));
        return IMG_NULL;
    }

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    mutex_lock(&sTimerStructLock);
#else
    spin_lock_irqsave(&sTimerStructLock, ulLockFlags);
#endif
    for (ui = 0; ui < OS_MAX_TIMERS; ui++)
    {
        psTimerCBData = &sTimers[ui];
        if (!psTimerCBData->bInUse)
        {
            psTimerCBData->bInUse = IMG_TRUE;
            break;
        }
    }
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    mutex_unlock(&sTimerStructLock);
#else
    spin_unlock_irqrestore(&sTimerStructLock, ulLockFlags);
#endif
    if (ui >= OS_MAX_TIMERS)
    {
        PVR_DPF((PVR_DBG_ERROR, "OSAddTimer: all timers are in use"));
        return IMG_NULL;
    }

    psTimerCBData->pfnTimerFunc = pfnTimerFunc;
    psTimerCBData->pvData = pvData;
    psTimerCBData->bActive = IMG_FALSE;

    /*
        HZ = ticks per second
        ui32MsTimeout = required ms delay
        ticks = (Hz * ui32MsTimeout) / 1000
    */
    psTimerCBData->ui32Delay = ((HZ * ui32MsTimeout) < 1000)
                                ?	1
                                :	((HZ * ui32MsTimeout) / 1000);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0))
    timer_setup(&psTimerCBData->sTimer, OSTimerCallbackWrapper, 0);
#else
    /* initialise object */
    init_timer(&psTimerCBData->sTimer);

    /* setup timer object */
    /* PRQA S 0307,0563 1 */ /* ignore warning about inconpartible ptr casting */
    psTimerCBData->sTimer.function = (IMG_VOID *)OSTimerCallbackWrapper;
    psTimerCBData->sTimer.data = (IMG_UINT32)psTimerCBData;
#endif
    return (IMG_HANDLE)(ui + 1);
}


static inline TIMER_CALLBACK_DATA *GetTimerStructure(IMG_HANDLE hTimer)
{
    IMG_UINT32 ui32i = ((IMG_UINT32)hTimer) - 1;

    PVR_ASSERT(ui32i < OS_MAX_TIMERS);

    return &sTimers[ui32i];
}

PVRSRV_ERROR OSRemoveTimer (IMG_HANDLE hTimer)
{
    TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

    PVR_ASSERT(psTimerCBData->bInUse);
    PVR_ASSERT(!psTimerCBData->bActive);

    
    psTimerCBData->bInUse = IMG_FALSE;
    
    return PVRSRV_OK;
}


PVRSRV_ERROR OSEnableTimer (IMG_HANDLE hTimer)
{
    TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

    PVR_ASSERT(psTimerCBData->bInUse);
    PVR_ASSERT(!psTimerCBData->bActive);

    
    psTimerCBData->bActive = IMG_TRUE;

    
    psTimerCBData->sTimer.expires = psTimerCBData->ui32Delay + jiffies;

    
    add_timer(&psTimerCBData->sTimer);
    
    return PVRSRV_OK;
}


PVRSRV_ERROR OSDisableTimer (IMG_HANDLE hTimer)
{
    TIMER_CALLBACK_DATA *psTimerCBData = GetTimerStructure(hTimer);

    PVR_ASSERT(psTimerCBData->bInUse);
    PVR_ASSERT(psTimerCBData->bActive);

    
    psTimerCBData->bActive = IMG_FALSE;
    smp_mb();

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    flush_workqueue(psTimerWorkQueue);
#endif
#if defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    flush_scheduled_work();
#endif

    
    timer_delete_sync(&psTimerCBData->sTimer);	
    
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    
    flush_workqueue(psTimerWorkQueue);
#endif
#if defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    flush_scheduled_work();
#endif

    return PVRSRV_OK;
}


#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR OSEventObjectCreateKM(const IMG_CHAR *pszName, PVRSRV_EVENTOBJECT_KM *psEventObject)
#else
PVRSRV_ERROR OSEventObjectCreateKM(const IMG_CHAR *pszName, PVRSRV_EVENTOBJECT *psEventObject)
#endif
{

    PVRSRV_ERROR eError = PVRSRV_OK;
    
    if(psEventObject)
    {
        if(pszName)
        {
            
            strncpy(psEventObject->szName, pszName, EVENTOBJNAME_MAXLENGTH);
        }
        else
        {
            	
            static IMG_UINT16 ui16NameIndex = 0;			
#if defined (SUPPORT_SID_INTERFACE)
            snprintf(psEventObject->szName, EVENTOBJNAME_MAXLENGTH, "PVRSRV_EVENTOBJECT_KM_%d", ui16NameIndex++);
#else
            snprintf(psEventObject->szName, EVENTOBJNAME_MAXLENGTH, "PVRSRV_EVENTOBJECT_%d", ui16NameIndex++);
#endif
        }
        
        if(LinuxEventObjectListCreate(&psEventObject->hOSEventKM) != PVRSRV_OK)
        {
             eError = PVRSRV_ERROR_OUT_OF_MEMORY;	
        }

    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectCreateKM: psEventObject is not a valid pointer"));
        eError = PVRSRV_ERROR_UNABLE_TO_CREATE_EVENT;	
    }
    
    return eError;

}


#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR OSEventObjectDestroyKM(PVRSRV_EVENTOBJECT_KM *psEventObject)
#else
PVRSRV_ERROR OSEventObjectDestroyKM(PVRSRV_EVENTOBJECT *psEventObject)
#endif
{
    PVRSRV_ERROR eError = PVRSRV_OK;

    if(psEventObject)
    {
        if(psEventObject->hOSEventKM)
        {
            LinuxEventObjectListDestroy(psEventObject->hOSEventKM);
        }
        else
        {
            PVR_DPF((PVR_DBG_ERROR, "OSEventObjectDestroyKM: hOSEventKM is not a valid pointer"));
            eError = PVRSRV_ERROR_INVALID_PARAMS;
        }
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectDestroyKM: psEventObject is not a valid pointer"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
}

PVRSRV_ERROR OSEventObjectWaitKM(IMG_HANDLE hOSEventKM)
{
    PVRSRV_ERROR eError;
    
    if(hOSEventKM)
    {
        eError = LinuxEventObjectWait(hOSEventKM, EVENT_OBJECT_TIMEOUT_MS);
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectWaitKM: hOSEventKM is not a valid handle"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
}

PVRSRV_ERROR OSEventObjectOpenKM(
#if defined (SUPPORT_SID_INTERFACE)
				 PVRSRV_EVENTOBJECT_KM *psEventObject,
#else
				 PVRSRV_EVENTOBJECT *psEventObject,
#endif
                                 IMG_HANDLE *phOSEvent)
{
    PVRSRV_ERROR eError = PVRSRV_OK;
    
    if(psEventObject)
    {
        if(LinuxEventObjectAdd(psEventObject->hOSEventKM, phOSEvent) != PVRSRV_OK)
        {
            PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectAdd: failed"));
            eError = PVRSRV_ERROR_INVALID_PARAMS;
        }

    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectCreateKM: psEventObject is not a valid pointer"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
}

PVRSRV_ERROR OSEventObjectCloseKM(
#if defined (SUPPORT_SID_INTERFACE)
				  PVRSRV_EVENTOBJECT_KM *psEventObject,
#else
				  PVRSRV_EVENTOBJECT *psEventObject,
#endif
                                  IMG_HANDLE hOSEventKM)
{
    PVRSRV_ERROR eError = PVRSRV_OK;

    if(psEventObject)
    {
        if(LinuxEventObjectDelete(psEventObject->hOSEventKM, hOSEventKM) != PVRSRV_OK)
        {
            PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectDelete: failed"));
            eError = PVRSRV_ERROR_INVALID_PARAMS;
        }

    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectDestroyKM: psEventObject is not a valid pointer"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
    
}

PVRSRV_ERROR OSEventObjectSignalKM(IMG_HANDLE hOSEventKM)
{
    PVRSRV_ERROR eError;
    
    if(hOSEventKM)
    {
        eError = LinuxEventObjectSignal(hOSEventKM);
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR, "OSEventObjectSignalKM: hOSEventKM is not a valid handle"));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
    }
    
    return eError;
}

IMG_BOOL OSProcHasPrivSrvInit(IMG_VOID)
{
	/* Return true here so that we can run X as a non-root user. */
	return IMG_TRUE;
}

PVRSRV_ERROR OSCopyToUser(IMG_PVOID pvProcess, 
                          IMG_VOID *pvDest, 
                          IMG_VOID *pvSrc, 
                          IMG_UINT32 ui32Bytes)
{
    PVR_UNREFERENCED_PARAMETER(pvProcess);

    if (copy_to_user(pvDest, pvSrc, ui32Bytes) == 0)
        return PVRSRV_OK;
    else
        return PVRSRV_ERROR_FAILED_TO_COPY_VIRT_MEMORY;
}

PVRSRV_ERROR OSCopyFromUser( IMG_PVOID pvProcess, 
                             IMG_VOID *pvDest, 
                             IMG_VOID *pvSrc, 
                             IMG_UINT32 ui32Bytes)
{
    PVR_UNREFERENCED_PARAMETER(pvProcess);

    if (copy_from_user(pvDest, pvSrc, ui32Bytes) == 0)
        return PVRSRV_OK;
    else
        return PVRSRV_ERROR_FAILED_TO_COPY_VIRT_MEMORY;
}

IMG_BOOL OSAccessOK(IMG_VERIFY_TEST eVerification, IMG_VOID *pvUserPtr, IMG_SIZE_T uiBytes)
{
    (void)eVerification; /* unused */
    return access_ok(pvUserPtr, uiBytes);
}

typedef enum _eWrapMemType_
{
    WRAP_TYPE_NULL = 0,
    WRAP_TYPE_GET_USER_PAGES,
    WRAP_TYPE_FIND_VMA
} eWrapMemType;

typedef struct _sWrapMemInfo_
{
    eWrapMemType eType;
    IMG_INT iNumPages;
    IMG_INT iNumPagesMapped;
    struct page **ppsPages;
    IMG_SYS_PHYADDR *psPhysAddr;
    IMG_INT iPageOffset;
#if defined(DEBUG)
    IMG_UINT32 ulStartAddr;
    IMG_UINT32 ulBeyondEndAddr;
    struct vm_area_struct *psVMArea;
#endif
} sWrapMemInfo;


static IMG_BOOL CPUVAddrToPFN(struct vm_area_struct *psVMArea, IMG_UINTPTR_T uCPUVAddr, IMG_UINT32 *pui32PFN, struct page **ppsPage)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0))
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,10))
    pgd_t *psPGD;
    pud_t *psPUD;
    pmd_t *psPMD;
    pte_t *psPTE;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,12,0))
    p4d_t *psP4D;
#endif
    struct mm_struct *psMM = psVMArea->vm_mm;
    spinlock_t *psPTLock;
    IMG_BOOL bRet = IMG_FALSE;

    *pulPFN = 0;
    *ppsPage = NULL;

    psPGD = pgd_offset(psMM, uCPUVAddr);
    if (pgd_none(*psPGD) || pgd_bad(*psPGD))
        return bRet;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(4,12,0))
    psP4D = p4d_offset(psPGD, uCPUVAddr);
    if (p4d_none(*psP4D) || unlikely(p4d_bad(*psP4D)))
        return bRet;

    psPUD = pud_offset(psP4D, uCPUVAddr);
#else
    psPUD = pud_offset(psPGD, ulCPUVAddr);
#endif
    if (pud_none(*psPUD) || pud_bad(*psPUD))
        return bRet;

    psPMD = pmd_offset(psPUD, ulCPUVAddr);
    if (pmd_none(*psPMD) || pmd_bad(*psPMD))
        return bRet;

    psPTE = (pte_t *)pte_offset_map_lock(psMM, psPMD, ulCPUVAddr, &psPTLock);

    if ((pte_none(*psPTE) == 0) && (pte_present(*psPTE) != 0) && (pte_write(*psPTE) != 0))
    {
        *pulPFN = pte_pfn(*psPTE);
	bRet = IMG_TRUE;

        if (pfn_valid(*pulPFN))
        {
            *ppsPage = pfn_to_page(*pulPFN);

            get_page(*ppsPage);
        }
    }

    pte_unmap_unlock(psPTE, psPTLock);

    return bRet;
#else
    return IMG_FALSE;
#endif
#else	/* #if (LINUX_VERSION_CODE < KERNEL_VERSION(6,6,0)) */
	spinlock_t *ptl;
	pte_t *ptep;
	int ret;

	if (!(psVMArea->vm_flags & (VM_IO | VM_PFNMAP)))
		return IMG_FALSE;

	ret = follow_pte(psVMArea->vm_mm, uCPUVAddr, &ptep, &ptl);
	if (ret < 0)
		return IMG_FALSE;

	*pui32PFN = pte_pfn(ptep_get(ptep));
	if (!pfn_valid(*pui32PFN))
		return IMG_FALSE;

	*ppsPage = pfn_to_page(*pui32PFN);
	get_page(*ppsPage);
	pte_unmap_unlock(ptep, ptl);

	return IMG_TRUE;
#endif
}

PVRSRV_ERROR OSReleasePhysPageAddr(IMG_HANDLE hOSWrapMem)
{
    sWrapMemInfo *psInfo = (sWrapMemInfo *)hOSWrapMem;
    IMG_INT i;

    if (psInfo == IMG_NULL)
    {
        PVR_DPF((PVR_DBG_WARNING,
            "OSReleasePhysPageAddr: called with null wrap handle"));
	return PVRSRV_OK;
    }

    switch (psInfo->eType)
    {
	case WRAP_TYPE_NULL:
	{
            PVR_DPF((PVR_DBG_WARNING,
                "OSReleasePhysPageAddr: called with wrap type WRAP_TYPE_NULL"));
	    break;
	}
        case WRAP_TYPE_GET_USER_PAGES:
        {
            for (i = 0; i < psInfo->iNumPagesMapped; i++)
            {
                struct page *psPage = psInfo->ppsPages[i];

		PVR_ASSERT(psPage != NULL);

                /*
		 * If the number of pages mapped is not the same as
		 * the number of pages in the address range, then
		 * get_user_pages must have failed, so we are cleaning
		 * up after failure, and the pages can't be dirty.
                 */
		if (psInfo->iNumPagesMapped == psInfo->iNumPages)
		{
                    if (!PageReserved(psPage))
                    {
                        SetPageDirty(psPage);
                    }
	        }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
                page_cache_release(psPage);
#else
                put_page(psPage);
#endif
	    }
            break;
        }
        case WRAP_TYPE_FIND_VMA:
        {
            for (i = 0; i < psInfo->iNumPages; i++)
            {
		if (psInfo->ppsPages[i] != IMG_NULL)
		{
                    put_page(psInfo->ppsPages[i]);
		}
            }
            break;
        }
        default:
        {
            PVR_DPF((PVR_DBG_ERROR,
                "OSReleasePhysPageAddr: Unknown wrap type (%d)", psInfo->eType));
            return PVRSRV_ERROR_INVALID_WRAP_TYPE;
        }
    }

    if (psInfo->ppsPages != IMG_NULL)
    {
        kfree(psInfo->ppsPages);
    }

    if (psInfo->psPhysAddr != IMG_NULL)
    {
        kfree(psInfo->psPhysAddr);
    }

    kfree(psInfo);

    return PVRSRV_OK;
}

PVRSRV_ERROR OSAcquirePhysPageAddr(IMG_VOID *pvCPUVAddr, 
                                    IMG_SIZE_T uiBytes,
                                    IMG_SYS_PHYADDR *psSysPAddr,
                                    IMG_HANDLE *phOSWrapMem)
{
    IMG_UINTPTR_T uStartAddrOrig = (IMG_UINTPTR_T) pvCPUVAddr;
    IMG_SIZE_T uAddrRangeOrig = uiBytes;
    IMG_UINTPTR_T uBeyondEndAddrOrig = uStartAddrOrig + uAddrRangeOrig;
    IMG_UINTPTR_T uStartAddr;
    IMG_SIZE_T uAddrRange;
    IMG_UINTPTR_T uBeyondEndAddr;
    IMG_UINTPTR_T uAddr;
    IMG_INT i;
    struct vm_area_struct *psVMArea;
    sWrapMemInfo *psInfo = NULL;
    IMG_BOOL bHavePageStructs = IMG_FALSE;
    IMG_BOOL bHaveNoPageStructs = IMG_FALSE;
    IMG_BOOL bPFNMismatch = IMG_FALSE;
    IMG_BOOL bMMapSemHeld = IMG_FALSE;
    PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;

    /* Align start and end addresses to page boundaries */
    uStartAddr = uStartAddrOrig & PAGE_MASK;
    uBeyondEndAddr = PAGE_ALIGN(uBeyondEndAddrOrig);
    uAddrRange = uBeyondEndAddr - uStartAddr;

    /*
     * Check for address range calculation overflow, and attempts to wrap
     * zero bytes.
     */
    if (uBeyondEndAddr <= uStartAddr)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Invalid address range (start %x, length %x)",
		uStartAddrOrig, uAddrRangeOrig));
        goto error;
    }

    /* Allocate information structure */
    psInfo = kmalloc(sizeof(*psInfo), GFP_KERNEL);
    if (psInfo == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Couldn't allocate information structure"));
        goto error;
    }
    memset(psInfo, 0, sizeof(*psInfo));

#if defined(DEBUG)
    psInfo->uStartAddr = uStartAddrOrig;
    psInfo->uBeyondEndAddr = uBeyondEndAddrOrig;
#endif

    psInfo->iNumPages = (IMG_INT)(uAddrRange >> PAGE_SHIFT);
    psInfo->iPageOffset = (IMG_INT)(uStartAddrOrig & ~PAGE_MASK);

    /* Allocate physical address array */
    psInfo->psPhysAddr = kmalloc((size_t)psInfo->iNumPages * sizeof(*psInfo->psPhysAddr), GFP_KERNEL);
    if (psInfo->psPhysAddr == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Couldn't allocate page array"));
        goto error;
    }
    memset(psInfo->psPhysAddr, 0, (size_t)psInfo->iNumPages * sizeof(*psInfo->psPhysAddr));

    /* Allocate page array */
    psInfo->ppsPages = kmalloc((size_t)psInfo->iNumPages * sizeof(*psInfo->ppsPages),  GFP_KERNEL);
    if (psInfo->ppsPages == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Couldn't allocate page array"));
        goto error;
    }
    memset(psInfo->ppsPages, 0, (size_t)psInfo->iNumPages * sizeof(*psInfo->ppsPages));

    /* Default error code from now on */
    eError = PVRSRV_ERROR_BAD_MAPPING;

    /* Set the mapping type to aid clean up */
    psInfo->eType = WRAP_TYPE_GET_USER_PAGES;

     /* Lock down user memory */
    down_read(&current->mm->mmap_sem);
    bMMapSemHeld = IMG_TRUE;

    /* Get page list */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0))
    psInfo->iNumPagesMapped = get_user_pages(
		current, current->mm,
		uStartAddr, psInfo->iNumPages, 1, 0, psInfo->ppsPages, NULL);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0))
    psInfo->iNumPagesMapped = get_user_pages(
		uStartAddr, psInfo->iNumPages, 1, 0, psInfo->ppsPages, NULL);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(6,5,0))
    psInfo->iNumPagesMapped = get_user_pages(
		uStartAddr, psInfo->iNumPages, 1, psInfo->ppsPages, NULL);
#else
    psInfo->iNumPagesMapped = get_user_pages(
		uStartAddr, psInfo->iNumPages, 1, psInfo->ppsPages);
#endif

    if (psInfo->iNumPagesMapped >= 0)
    {
        /* See if we got all the pages we wanted */
        if (psInfo->iNumPagesMapped != psInfo->iNumPages)
        {
            PVR_TRACE(("OSAcquirePhysPageAddr: Couldn't map all the pages needed (wanted: %d, got %d)", psInfo->iNumPages, psInfo->iNumPagesMapped));

            goto error;
        }

        /* Build list of physical page addresses */
        for (i = 0; i < psInfo->iNumPages; i++)
        {
            IMG_CPU_PHYADDR CPUPhysAddr;
	    IMG_UINT32 ui32PFN;

            ui32PFN = page_to_pfn(psInfo->ppsPages[i]);
            CPUPhysAddr.uiAddr = ui32PFN << PAGE_SHIFT;
	    if ((CPUPhysAddr.uiAddr >> PAGE_SHIFT) != ui32PFN)
	    {
                PVR_DPF((PVR_DBG_ERROR,
		    "OSAcquirePhysPageAddr: Page frame number out of range (%x)", ui32PFN));

		    goto error;
	    }
            psInfo->psPhysAddr[i] = SysCpuPAddrToSysPAddr(CPUPhysAddr);
            psSysPAddr[i] = psInfo->psPhysAddr[i];
            
        }

        goto exit;
    }

    PVR_DPF((PVR_DBG_MESSAGE, "OSAcquirePhysPageAddr: get_user_pages failed (%d), using CPU page table", psInfo->iNumPagesMapped));

    /* Reset some fields */
    psInfo->eType = WRAP_TYPE_NULL;
    psInfo->iNumPagesMapped = 0;
    memset(psInfo->ppsPages, 0, (size_t)psInfo->iNumPages * sizeof(*psInfo->ppsPages));


    /* Set the mapping type to aid clean up */
    psInfo->eType = WRAP_TYPE_FIND_VMA;

    psVMArea = find_vma(current->mm, uStartAddrOrig);
    if (psVMArea == NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Couldn't find memory region containing start address %x", uStartAddrOrig));
        
        goto error;
    }
#if defined(DEBUG)
    psInfo->psVMArea = psVMArea;
#endif

    /*
     * find_vma locates a region with an end point past a given
     * virtual address.  So check the address is actually in the region.
     */
    if (uStartAddrOrig < psVMArea->vm_start)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Start address %x is outside of the region returned by find_vma", uStartAddrOrig));
        goto error;
    }

    /* Now check the end address is in range */
    if (uBeyondEndAddrOrig > psVMArea->vm_end)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: End address %x is outside of the region returned by find_vma", uBeyondEndAddrOrig));
        goto error;
    }

    /* Does the region represent memory mapped I/O? */
    if (!(psVMArea->vm_flags & VM_IO))
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Memory region does not represent memory mapped I/O (VMA flags: 0x%lx)", psVMArea->vm_flags));
        goto error;
    }

    /* We require read and write access */
    if ((psVMArea->vm_flags & (VM_READ | VM_WRITE)) != (VM_READ | VM_WRITE))
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: No read/write access to memory region (VMA flags: 0x%lx)", psVMArea->vm_flags));
        goto error;
    }

    for (uAddr = uStartAddrOrig, i = 0; uAddr < uBeyondEndAddrOrig; uAddr += PAGE_SIZE, i++)
    {
	IMG_CPU_PHYADDR CPUPhysAddr;
	IMG_UINT32 ui32PFN = 0;

	PVR_ASSERT(i < psInfo->iNumPages);

	if (!CPUVAddrToPFN(psVMArea, uAddr, &ui32PFN, &psInfo->ppsPages[i]))
	{
            PVR_DPF((PVR_DBG_ERROR,
	       "OSAcquirePhysPageAddr: Invalid CPU virtual address"));

	    goto error;
	}
	if (psInfo->ppsPages[i] == NULL)
	{

	    bHaveNoPageStructs = IMG_TRUE;

#if defined(VM_PFNMAP)
	    if ((psVMArea->vm_flags & VM_PFNMAP) != 0)
	    {
	        IMG_UINT32 ulPFNRaw = ((uAddr - psVMArea->vm_start) >> PAGE_SHIFT) + psVMArea->vm_pgoff;

	        if (ulPFNRaw != ui32PFN)
	        {
			bPFNMismatch = IMG_TRUE;
	        }
	    }
#endif
	}
	else
	{
	    bHavePageStructs = IMG_TRUE;

	    psInfo->iNumPagesMapped++;

	    PVR_ASSERT(ui32PFN == page_to_pfn(psInfo->ppsPages[i]));
	}

        CPUPhysAddr.uiAddr = ui32PFN << PAGE_SHIFT;
	if ((CPUPhysAddr.uiAddr >> PAGE_SHIFT) != ui32PFN)
	{
                PVR_DPF((PVR_DBG_ERROR,
		    "OSAcquirePhysPageAddr: Page frame number out of range (%x)", ulPFN));

		    goto error;
	}

	psInfo->psPhysAddr[i] = SysCpuPAddrToSysPAddr(CPUPhysAddr);
	psSysPAddr[i] = psInfo->psPhysAddr[i];
    }
    PVR_ASSERT(i ==  psInfo->iNumPages);

#if defined(VM_MIXEDMAP)
    if ((psVMArea->vm_flags & VM_MIXEDMAP) != 0)
    {
        goto exit;
    }
#endif

    if (bHavePageStructs && bHaveNoPageStructs)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Region is VM_MIXEDMAP, but isn't marked as such"));
	goto error;
    }

    if (!bHaveNoPageStructs)
    {
	
	goto exit;
    }

#if defined(VM_PFNMAP)
    if ((psVMArea->vm_flags & VM_PFNMAP) == 0)
#endif
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: Region is VM_PFNMAP, but isn't marked as such"));
	goto error;
    }

    if (bPFNMismatch)
    {
        PVR_DPF((PVR_DBG_ERROR,
            "OSAcquirePhysPageAddr: PFN calculation mismatch for VM_PFNMAP region"));
	goto error;
    }

exit:
    PVR_ASSERT(bMMapSemHeld);
    up_read(&current->mm->mmap_sem);

    
    *phOSWrapMem = (IMG_HANDLE)psInfo;

    if (bHaveNoPageStructs)
    {
        PVR_DPF((PVR_DBG_WARNING,
            "OSAcquirePhysPageAddr: Region contains pages which can't be locked down (no page structures)"));
    }

    PVR_ASSERT(psInfo->eType != 0);

#if 0
    
    
    OSCleanCPUCacheRangeKM(pvCPUVAddr, (IMG_VOID *)((IMG_CHAR *)pvCPUVAddr + ui32Bytes));
#endif

    return PVRSRV_OK;

error:
    if (bMMapSemHeld)
    {
        up_read(&current->mm->mmap_sem);
    }
    OSReleasePhysPageAddr((IMG_HANDLE)psInfo);

    PVR_ASSERT(eError != PVRSRV_OK);

    return eError;
}

typedef void (*InnerCacheOp_t)(const void *pvStart, const void *pvEnd);
typedef void (*OuterCacheOp_t)(unsigned long ulStart, unsigned long ulEnd);

#if defined(CONFIG_OUTER_CACHE)

typedef unsigned long (*MemAreaToPhys_t)(LinuxMemArea *psLinuxMemArea,
										 IMG_VOID *pvRangeAddrStart,
										 IMG_UINT32 ui32PageNumOffset,
										 IMG_UINT32 ui32PageNum);

static unsigned long VMallocAreaToPhys(LinuxMemArea *psLinuxMemArea,
									   IMG_VOID *pvRangeAddrStart,
									   IMG_UINT32 ui32PageNumOffset,
									   IMG_UINT32 ui32PageNum)
{
	return vmalloc_to_pfn(pvRangeAddrStart + ui32PageNum * PAGE_SIZE) << PAGE_SHIFT;
}

static unsigned long ExternalKVAreaToPhys(LinuxMemArea *psLinuxMemArea,
										  IMG_VOID *pvRangeAddrStart,
										  IMG_UINT32 ui32PageNumOffset,
										  IMG_UINT32 ui32PageNum)
{
	IMG_SYS_PHYADDR SysPAddr;
	IMG_CPU_PHYADDR CpuPAddr;
	SysPAddr = psLinuxMemArea->uData.sExternalKV.uPhysAddr.pSysPhysAddr[ui32PageNumOffset + ui32PageNum];
	CpuPAddr = SysSysPAddrToCpuPAddr(SysPAddr);
	return CpuPAddr.uiAddr;
}

static unsigned long AllocPagesAreaToPhys(LinuxMemArea *psLinuxMemArea,
										  IMG_VOID *pvRangeAddrStart,
										  IMG_UINT32 ui32PageNumOffset,
										  IMG_UINT32 ui32PageNum)
{
	struct page *pPage;
	pPage = psLinuxMemArea->uData.sPageList.pvPageList[ui32PageNumOffset + ui32PageNum];
	return page_to_pfn(pPage) << PAGE_SHIFT;
}

#endif 

#ifndef __mips__
static
IMG_VOID *FindMMapBaseVAddr(struct list_head *psMMapOffsetStructList,
							IMG_VOID *pvRangeAddrStart, IMG_UINT32 ui32Length)
{
	PKV_OFFSET_STRUCT psOffsetStruct;
	IMG_VOID *pvMinVAddr;

	
	list_for_each_entry(psOffsetStruct, psMMapOffsetStructList, sAreaItem)
	{
		if(OSGetCurrentProcessIDKM() != psOffsetStruct->ui32PID)
			continue;

		pvMinVAddr = (IMG_VOID *)psOffsetStruct->uiUserVAddr;

		
		if(pvRangeAddrStart >= pvMinVAddr &&
		   ui32Length <= psOffsetStruct->uiRealByteSize)
			return pvMinVAddr;
	}

	return IMG_NULL;
}

static
IMG_BOOL CheckExecuteCacheOp(IMG_HANDLE hOSMemHandle,
							 IMG_VOID *pvRangeAddrStart,
							 IMG_SIZE_T uiLength,
							 InnerCacheOp_t pfnInnerCacheOp,
							 OuterCacheOp_t pfnOuterCacheOp)
{
	LinuxMemArea *psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
	IMG_SIZE_T uiAreaLength;
        IMG_UINTPTR_T uiAreaOffset = 0;
	struct list_head *psMMapOffsetStructList;
	IMG_VOID *pvMinVAddr;

#if defined(CONFIG_OUTER_CACHE)
	MemAreaToPhys_t pfnMemAreaToPhys = IMG_NULL;
	IMG_UINTPTR_T uiPageNumOffset = 0;
#endif

	PVR_ASSERT(psLinuxMemArea != IMG_NULL);

	uiAreaLength = psLinuxMemArea->uiByteSize;
	psMMapOffsetStructList = &psLinuxMemArea->sMMapOffsetStructList;

	PVR_ASSERT(uiLength <= uiAreaLength);

	if(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC)
	{
		uiAreaOffset = psLinuxMemArea->uData.sSubAlloc.uiByteOffset;
		psLinuxMemArea = psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea;
	}

	/* Recursion surely isn't possible? */
	PVR_ASSERT(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC);

	switch(psLinuxMemArea->eAreaType)
	{
		case LINUX_MEM_AREA_VMALLOC:
		{
			if(is_vmalloc_addr(pvRangeAddrStart))
			{
				pvMinVAddr = psLinuxMemArea->uData.sVmalloc.pvVmallocAddress + uiAreaOffset;

				/* Outside permissible range */
				if(pvRangeAddrStart < pvMinVAddr)
					goto err_blocked;

				pfnInnerCacheOp(pvRangeAddrStart, pvRangeAddrStart + uiLength);
			}
			else
			{
				/* If this isn't a vmalloc address, assume we're flushing by
				 * user-virtual. Compute the mmap base vaddr and use this to
				 * compute the offset in vmalloc space.
				 */

				pvMinVAddr = FindMMapBaseVAddr(psMMapOffsetStructList,
											   pvRangeAddrStart, uiLength);
				if(!pvMinVAddr)
					goto err_blocked;

				pfnInnerCacheOp(pvRangeAddrStart, pvRangeAddrStart + uiLength);

#if defined(CONFIG_OUTER_CACHE)
				
				pvRangeAddrStart = psLinuxMemArea->uData.sVmalloc.pvVmallocAddress +
								   (uiAreaOffset & PAGE_MASK) + (pvRangeAddrStart - pvMinVAddr);
			}

			pfnMemAreaToPhys = VMallocAreaToPhys;
#else 
			}
#endif 
			break;
		}

		case LINUX_MEM_AREA_EXTERNAL_KV:
		{
			
			if (psLinuxMemArea->uData.sExternalKV.bPhysContig == IMG_TRUE)
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Attempt to flush contiguous external memory", __func__));

				goto err_blocked;
			}

			
			if (psLinuxMemArea->uData.sExternalKV.pvExternalKV != IMG_NULL)
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Attempt to flush external memory with a kernel virtual address", __func__));

				goto err_blocked;
			}

			

			pvMinVAddr = FindMMapBaseVAddr(psMMapOffsetStructList,
										   pvRangeAddrStart, uiLength);
			if(!pvMinVAddr)
				goto err_blocked;

			pfnInnerCacheOp(pvRangeAddrStart, pvRangeAddrStart + uiLength);

#if defined(CONFIG_OUTER_CACHE)
			uiPageNumOffset = ((uiAreaOffset & PAGE_MASK) + (pvRangeAddrStart - pvMinVAddr)) >> PAGE_SHIFT;
			pfnMemAreaToPhys = ExternalKVAreaToPhys;
#endif
			break;
		}

		case LINUX_MEM_AREA_ALLOC_PAGES:
		{
			pvMinVAddr = FindMMapBaseVAddr(psMMapOffsetStructList,
										   pvRangeAddrStart, uiLength);
			if(!pvMinVAddr)
				goto err_blocked;

			pfnInnerCacheOp(pvRangeAddrStart, pvRangeAddrStart + uiLength);

#if defined(CONFIG_OUTER_CACHE)
			uiPageNumOffset = ((uiAreaOffset & PAGE_MASK) + (pvRangeAddrStart - pvMinVAddr)) >> PAGE_SHIFT;
			pfnMemAreaToPhys = AllocPagesAreaToPhys;
#endif
			break;
		}

		default:
			PVR_DBG_BREAK;
	}

#if defined(CONFIG_OUTER_CACHE)
	PVR_ASSERT(pfnMemAreaToPhys != IMG_NULL);

	
	{
		unsigned long ulStart, ulEnd, ulLength, ulStartOffset, ulEndOffset;
		IMG_UINT32 i, ui32NumPages;

		
		ulLength = (unsigned long)ui32Length;
		ulStartOffset = ((unsigned long)pvRangeAddrStart) & (PAGE_SIZE - 1);
		ulEndOffset = ((unsigned long)pvRangeAddrStart + ulLength) & (PAGE_SIZE - 1);

		
		ui32NumPages = (ulStartOffset + ulLength + PAGE_SIZE - 1) >> PAGE_SHIFT;

		for(i = 0; i < ui32NumPages; i++)
		{
			ulStart = pfnMemAreaToPhys(psLinuxMemArea, pvRangeAddrStart,
									   ui32PageNumOffset, i);
			ulEnd = ulStart + PAGE_SIZE;

			if(i == ui32NumPages - 1 && ulEndOffset != 0)
				ulEnd = ulStart + ulEndOffset;

			if(i == 0)
				ulStart += ulStartOffset;

			pfnOuterCacheOp(ulStart, ulEnd);
		}
	}
#endif

	return IMG_TRUE;

err_blocked:
	PVR_DPF((PVR_DBG_WARNING, "%s: Blocked cache op on virtual range "
							  "%p-%p (type %d)", __func__,
			 pvRangeAddrStart, pvRangeAddrStart + ui32Length,
			 psLinuxMemArea->eAreaType));
	return IMG_FALSE;
}

#endif

#if defined(__i386__)

#define ROUND_UP(x,a) (((x) + (a) - 1) & ~((a) - 1))

static void per_cpu_cache_flush(void *arg)
{
    PVR_UNREFERENCED_PARAMETER(arg);
    wbinvd();
}

static void x86_flush_cache_range(const void *pvStart, const void *pvEnd)
{
	IMG_BYTE *pbStart = (IMG_BYTE *)pvStart;
	IMG_BYTE *pbEnd = (IMG_BYTE *)pvEnd;
	IMG_BYTE *pbBase;

	pbEnd = (IMG_BYTE *)ROUND_UP((IMG_UINTPTR_T)pbEnd,
								 boot_cpu_data.x86_clflush_size);

	mb();
	for(pbBase = pbStart; pbBase < pbEnd; pbBase += boot_cpu_data.x86_clflush_size)
		clflush(pbBase);
	mb();
}

IMG_VOID OSCleanCPUCacheKM(IMG_VOID)
{
	
	ON_EACH_CPU(per_cpu_cache_flush, NULL, 1);
}

IMG_VOID OSFlushCPUCacheKM(IMG_VOID)
{
	ON_EACH_CPU(per_cpu_cache_flush, NULL, 1);
}

IMG_BOOL OSFlushCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	
	return CheckExecuteCacheOp(hOSMemHandle, pvRangeAddrStart, ui32Length,
							   x86_flush_cache_range, IMG_NULL);
}

IMG_BOOL OSCleanCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	
	return CheckExecuteCacheOp(hOSMemHandle, pvRangeAddrStart, ui32Length,
							   x86_flush_cache_range, IMG_NULL);
}

IMG_BOOL OSInvalidateCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32Length)
{
	
	return CheckExecuteCacheOp(hOSMemHandle, pvRangeAddrStart, ui32Length,
							   x86_flush_cache_range, IMG_NULL);
}

#else 

#if defined(__arm__)

static void per_cpu_cache_flush(void *arg)
{
	PVR_UNREFERENCED_PARAMETER(arg);
	flush_cache_all();
}

IMG_VOID OSCleanCPUCacheKM(IMG_VOID)
{
	
	ON_EACH_CPU(per_cpu_cache_flush, NULL, 1);
#if defined(CONFIG_OUTER_CACHE) && !defined(PVR_NO_FULL_CACHE_OPS)
	outer_clean_all();
#endif
}

IMG_VOID OSFlushCPUCacheKM(IMG_VOID)
{
	ON_EACH_CPU(per_cpu_cache_flush, NULL, 1);
#if defined(CONFIG_OUTER_CACHE) && !defined(PVR_NO_FULL_CACHE_OPS)
	outer_flush_all();
#endif
}

static inline size_t pvr_dmac_range_len(const void *pvStart, const void *pvEnd)
{
	return (size_t)((char *)pvEnd - (char *)pvStart);
}

static void pvr_dmac_inv_range(const void *pvStart, const void *pvEnd)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
	dmac_inv_range(pvStart, pvEnd);
#else
	dmac_map_area(pvStart, pvr_dmac_range_len(pvStart, pvEnd), DMA_FROM_DEVICE);
#endif
}

static void pvr_dmac_clean_range(const void *pvStart, const void *pvEnd)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
	dmac_clean_range(pvStart, pvEnd);
#else
	dmac_map_area(pvStart, pvr_dmac_range_len(pvStart, pvEnd), DMA_TO_DEVICE);
#endif
}

IMG_BOOL OSFlushCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, pvRangeAddrStart, ui32Length,
							   dmac_flush_range, outer_flush_range);
}

IMG_BOOL OSCleanCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, pvRangeAddrStart, ui32Length,
							   pvr_dmac_clean_range, outer_clean_range);
}

IMG_BOOL OSInvalidateCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32Length)
{
	return CheckExecuteCacheOp(hOSMemHandle, pvRangeAddrStart, ui32Length,
							   pvr_dmac_inv_range, outer_inv_range);
}

#else 

#if defined(__mips__)
IMG_VOID OSCleanCPUCacheKM(IMG_VOID)
{
	
	dma_cache_wback(0, 0x100000);
}

IMG_VOID OSFlushCPUCacheKM(IMG_VOID)
{
	
	dma_cache_wback_inv(0, 0x100000);
}

IMG_BOOL OSFlushCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	if (ui32Length)
		dma_cache_wback_inv((IMG_UINTPTR_T)pvRangeAddrStart, ui32Length);	
	return IMG_TRUE;
}

IMG_BOOL OSCleanCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
								IMG_VOID *pvRangeAddrStart,
								IMG_UINT32 ui32Length)
{
	if (ui32Length)
		dma_cache_wback((IMG_UINTPTR_T)pvRangeAddrStart, ui32Length);
	return IMG_TRUE;
}

IMG_BOOL OSInvalidateCPUCacheRangeKM(IMG_HANDLE hOSMemHandle,
									 IMG_VOID *pvRangeAddrStart,
									 IMG_UINT32 ui32Length)
{
	if (ui32Length)
		dma_cache_inv((IMG_UINTPTR_T)pvRangeAddrStart, ui32Length);
	return IMG_TRUE;
}

#else 

#error "Implement CPU cache flush/clean/invalidate primitives for this CPU!"

#endif 

#endif 

#endif 

PVRSRV_ERROR PVROSFuncInit(IMG_VOID)
{
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    {
        psTimerWorkQueue = create_workqueue("pvr_timer");
        if (psTimerWorkQueue == NULL)
        {
	    PVR_DPF((PVR_DBG_ERROR, "%s: couldn't create timer workqueue", __FUNCTION__));		
	    return PVRSRV_ERROR_UNABLE_TO_CREATE_THREAD;

        }
    }
#endif

#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES) || defined(PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE)
    {
	IMG_UINT32 ui32i;

        for (ui32i = 0; ui32i < OS_MAX_TIMERS; ui32i++)
        {
            TIMER_CALLBACK_DATA *psTimerCBData = &sTimers[ui32i];

	    INIT_WORK(&psTimerCBData->sWork, OSTimerWorkQueueCallBack);
        }
    }
#endif
    return PVRSRV_OK;
}

IMG_VOID PVROSFuncDeInit(IMG_VOID)
{
#if defined(PVR_LINUX_TIMERS_USING_WORKQUEUES)
    if (psTimerWorkQueue != NULL)
    {
	destroy_workqueue(psTimerWorkQueue);
    }
#endif
}
