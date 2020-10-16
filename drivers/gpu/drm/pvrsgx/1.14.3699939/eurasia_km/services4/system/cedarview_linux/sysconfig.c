/*************************************************************************/ /*!
@Title          System Description
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Description functions
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

#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
#include "linux/pci.h"
#endif
#include <linux/version.h>
#if defined(SUPPORT_DRI_DRM) && (LINUX_VERSION_CODE < KERNEL_VERSION(5,5,0))
#include "drm/drmP.h"
#endif

#include "sgxdefs.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "pdump_km.h"
#include "sgxinfokm.h"

#include "sysinfo.h"
#include "sysconfig.h"
#include "sysutils.h"
#ifdef SUPPORT_MSVDX
#include "msvdx_defs.h"
#include "msvdxapi.h"
#include "msvdx_infokm.h"
#include "osfunc.h"
#endif

#define SUPPORT_PRIMARY_SURFACE_SEGMENT

/*
 * This file implements the SOC-dependant portion of Services.  The defined
 * interface for what this implements can be found in
 * $(WORKROOT)/services4/system/include/syscommon.h
 *
 * In addition to this file, the three headers are used.
 * - sysinfo.h specifies overall properties of the system
 *   - e.g. clock speeds, Vendor ID, Device ID, etc.
 * - sysconfig.h specifies device-specific details
 *   - e.g. Register offsets for SGX, SOC register bits, etc.
 * - sysutils.h specify any software utilities needed
 *   - e.g. Reported aperture size, how to read PCI space
 *
 */

/**
 * TOP level system data
 */
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
SYS_DATA  gsSysData;

static SYS_SPECIFIC_DATA gsSysSpecificData;

/**
 * SOC structures
 */
static CDV_DEVICE_MAP gsSOCDeviceMap;

/**
 * SGX structures
 */
static IMG_UINT32		gui32SGXDeviceID;
static SGX_DEVICE_MAP	gsSGXDeviceMap;

/**
 * VXD structures
 */
#if defined(SUPPORT_MSVDX)
static IMG_UINT32		gui32MSVDXDeviceID;
static MSVDX_DEVICE_MAP	gsMSVDXDeviceMap;
#endif

#if defined(SUPPORT_DRI_DRM)
extern struct drm_device *gpsPVRDRMDev;
#endif

#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
/* The following is exported by the Linux module code */
extern struct pci_dev *gpsPVRLDMDev;
#endif

/***********************************************************************//**
 * Locate and describe our devices on the PCI bus
 *
 * Fills out the device map for all devices we know aout and control
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32IRQ = 0;
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
#else
	PVRSRV_ERROR eError;
#endif
	/************************
	 *       SOC Setup      *
	 ************************/


#if !defined(NO_HARDWARE)
	/* Get the regions from the base address register */
	gsSOCDeviceMap.sRegsSysPBase.uiAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, CEDARVIEW_ADDR_RANGE_INDEX);

	PVR_TRACE(("uiBaseAddr: " SYSPADDR_FMT, gsSOCDeviceMap.sRegsSysPBase.uiAddr));

	/* Convert it to a CPU physical address */
	gsSOCDeviceMap.sRegsCpuPBase =
		SysSysPAddrToCpuPAddr(gsSOCDeviceMap.sRegsSysPBase);

	/*
	 * And map in the system registers.
	 */
	gsSOCDeviceMap.sRegsCpuVBase = OSMapPhysToLin(gsSOCDeviceMap.sRegsCpuPBase,
												  SYS_SOC_REG_SIZE,
												  PVRSRV_HAP_KERNEL_ONLY | PVRSRV_HAP_UNCACHED,
												  IMG_NULL);

	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_SOC_REGS_MAPPED);
#endif

	/************************
	 *       SGX Setup      *
	 ************************/

#if !defined(NO_HARDWARE)
	gsSGXDeviceMap.sRegsSysPBase.uiAddr =
		gsSOCDeviceMap.sRegsSysPBase.uiAddr + SYS_SGX_REG_OFFSET;

	gsSGXDeviceMap.sRegsCpuPBase =
		SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);

	/* device interrupt IRQ */
	if (OSPCIIRQ(psSysSpecData->hSGXPCI, &ui32IRQ) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Couldn't get IRQ"));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	PVR_TRACE(("IRQ: %d", ui32IRQ));

#else /* !defined(NO_HARDWARE) */

	/*
	 * With no hardware, allocate contiguous memory to emulate the registers
	 */
	eError = OSBaseAllocContigMemory(SYS_SGX_REG_SIZE,
	                                 &(gsSGXDeviceMap.pvRegsCpuVBase),
	                                 &(gsSGXDeviceMap.sRegsCpuPBase));
	if(eError != PVRSRV_OK)
	{
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_DUMMY_SGX_REGS);

	OSMemSet(gsSGXDeviceMap.pvRegsCpuVBase, 0, SYS_SGX_REG_SIZE);

	gsSGXDeviceMap.sRegsSysPBase =
		SysCpuPAddrToSysPAddr(gsSGXDeviceMap.sRegsCpuPBase);

#endif	/* !defined(NO_HARDWARE) */

	/*
	 * Other setup
	 */
	gsSGXDeviceMap.ui32Flags = 0x0;
	gsSGXDeviceMap.ui32RegsSize = SYS_SGX_REG_SIZE;
	gsSGXDeviceMap.ui32IRQ = ui32IRQ;

	/*
	 * Local Device Memory Region is never present
	 */
	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemDevPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr = 0;
	gsSGXDeviceMap.ui32LocalMemSize = 0;

#if defined(PDUMP)
	{
		/* initialise memory region name for pdumping */
		static IMG_CHAR pszPDumpDevName[] = SYSTEM_PDUMP_NAME;
		gsSGXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif

	/************************
	 *       VXD Setup      *
	 ************************/
#if defined(SUPPORT_MSVDX)

#if !defined(NO_HARDWARE)
	gsMSVDXDeviceMap.sRegsSysPBase.uiAddr =
		gsSOCDeviceMap.sRegsSysPBase.uiAddr + SYS_MSVDX_REG_OFFSET;

	gsMSVDXDeviceMap.sRegsCpuPBase =
		SysSysPAddrToCpuPAddr(gsMSVDXDeviceMap.sRegsSysPBase);

#else
	/* No hardware registers */
	eError = OSBaseAllocContigMemory(MSVDX_REG_SIZE,
									 &(gsMSVDXDeviceMap.sRegsCpuVBase),
									 &(gsMSVDXDeviceMap.sRegsCpuPBase));
	if(eError != PVRSRV_OK)
	{
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_DUMMY_MSVDX_REGS);

	OSMemSet(gsMSVDXDeviceMap.sRegsCpuVBase, 0, MSVDX_REG_SIZE);
	gsMSVDXDeviceMap.sRegsSysPBase =
		SysCpuPAddrToSysPAddr(gsMSVDXDeviceMap.sRegsCpuPBase);

#endif /* NO_HARDWARE */

	/* Common setup */
	gsMSVDXDeviceMap.ui32RegsSize = MSVDX_REG_SIZE;

	/*
	 * No local device memory region
	 */
	gsMSVDXDeviceMap.sLocalMemSysPBase.uiAddr = 0;
	gsMSVDXDeviceMap.sLocalMemDevPBase.uiAddr = 0;
	gsMSVDXDeviceMap.sLocalMemCpuPBase.uiAddr = 0;
	gsMSVDXDeviceMap.ui32LocalMemSize		  = 0;

	/*
	 * device interrupt IRQ
	 */
	gsMSVDXDeviceMap.ui32IRQ = ui32IRQ;

#if defined(PDUMP)
	{
		/* initialise memory region name for pdumping */
		static IMG_CHAR pszPDumpDevName[] = SYSTEM_PDUMP_NAME;
		gsMSVDXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif /* defined(PDUMP) */

#endif /* defined(SUPPORT_MSVDX) */

	PVR_DPF((PVR_DBG_MESSAGE,
			 "SGX registers base physical address: 0x" SYSPADDR_FMT,
			 gsSGXDeviceMap.sRegsSysPBase.uiAddr));
#if defined(SUPPORT_MSVDX)
	PVR_DPF((PVR_DBG_MESSAGE,
			 "VXD registers base physical address: 0x" SYSPADDR_FMT,
			 gsMSVDXDeviceMap.sRegsSysPBase.uiAddr));
#endif

	return PVRSRV_OK;
}

/***********************************************************************//**
 * Perform hardware initialisation at driver load time
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysInitialise(IMG_VOID)
{
	IMG_UINT32 i;
	PVRSRV_ERROR eError;
	PVRSRV_DEVICE_NODE *psDeviceNode;
	SGX_TIMING_INFORMATION *psTimingInfo;

	gpsSysData = &gsSysData;
	OSMemSet(gpsSysData, 0, sizeof(SYS_DATA));

	gpsSysData->pvSysSpecificData = &gsSysSpecificData;
	gsSysSpecificData.ui32SysSpecificData = 0;
#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
	/* Save the pci_dev structure pointer from module.c */
	PVR_ASSERT(gpsPVRLDMDev != IMG_NULL);
	gsSysSpecificData.psPCIDev = gpsPVRLDMDev;
#endif
#if defined(SUPPORT_DRI_DRM)
	/* Save the DRM device structure pointer */
	PVR_ASSERT(gpsPVRDRMDev != IMG_NULL);
	gsSysSpecificData.psDRMDev = gpsPVRDRMDev;
#endif
	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to setup env structure"));
		goto errorExit;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENVDATA);

	/* Set up timing information*/
	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;

	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else /* defined(SUPPORT_ACTIVE_POWER_MANAGEMENT) */
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif /* defined(SUPPORT_ACTIVE_POWER_MANAGEMENT) */

	eError = PCIInitDev(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	gpsSysData->ui32NumDevices = SYS_DEVICE_COUNT;

	/* init device ID's */
	for(i=0; i<SYS_DEVICE_COUNT; i++)
	{
		gpsSysData->sDeviceID[i].uiID = i;
		gpsSysData->sDeviceID[i].bInUse = IMG_FALSE;
	}

	gpsSysData->psDeviceNodeList = IMG_NULL;
	gpsSysData->psQueueList = IMG_NULL;

	SysPowerMaybeInit(gpsSysData);

	eError = SysInitialiseCommon(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed in SysInitialiseCommon"));
		goto errorExit;
	}

	/*
	 * Locate the devices within the system and get the
	 * physical addresses of each
	 */
	eError = SysLocateDevices(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to locate devices"));
		goto errorExit;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_LOCATEDEV);

	/*
	 * Register devices with the system
	 * This also sets up their memory maps/heaps
	 */
	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice,
								  DEVICE_SGX_INTERRUPT, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		goto errorExit;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_REGSGXDEV);

#if defined(SUPPORT_MSVDX)
	eError = PVRSRVRegisterDevice(gpsSysData, MSVDXRegisterDevice,
								  DEVICE_MSVDX_INTERRUPT, &gui32MSVDXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register VXD device!"));
		goto errorExit;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_REGVXDDEV);
#endif

	/*
	 * Once all devices are registered, specify the backing store
	 * and, if required, customise the memory heap config
	 */
	psDeviceNode = gpsSysData->psDeviceNodeList;
	while(psDeviceNode)
	{
		/* perform any OEM SOC address space customisations here */
		switch(psDeviceNode->sDevId.eDeviceType)
		{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			DEVICE_MEMORY_INFO *psDevMemoryInfo;
			DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;

			/*
			 * Specify the backing store to use for the devices MMU PT/PDs
			 */
			psDeviceNode->psLocalDevMemArena = IMG_NULL;

			/* useful pointers */
			psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
			psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

			/* specify the backing store for all SGX heaps */
			for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
			{
				psDeviceMemoryHeap[i].ui32Attribs |=
					PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;
#if defined(OEM_CUSTOMISE)
				/* if required, modify the memory config */
#endif
			}
			break;
		}
#if defined(SUPPORT_MSVDX)
		case PVRSRV_DEVICE_TYPE_MSVDX:
		{
			DEVICE_MEMORY_INFO *psDevMemoryInfo;
			DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;

			/* specify the backing store to use for the devices MMU PT/PDs */
			psDeviceNode->psLocalDevMemArena = IMG_NULL;

			/* useful pointers */
			psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
			psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

			/* specify the backing store for all SGX heaps */
			for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
			{
				psDeviceMemoryHeap[i].ui32Attribs |=
					PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;
#if defined(OEM_CUSTOMISE)
				/* if required, modify the memory config */
#endif
			}
			break;
		}
#endif
		default:
			break;
		}

		/* advance to next device */
		psDeviceNode = psDeviceNode->psNext;
	}

	/*
	 * Initialise all devices managed by us
	 */
	eError = PVRSRVInitialiseDevice(gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise SGX!"));
		goto errorExit;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_INITSGXDEV);

#ifdef SUPPORT_MSVDX
	eError = PVRSRVInitialiseDevice(gui32MSVDXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise VXD!"));
		goto errorExit;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_INITVXDDEV);
#endif

	/* All fine! */
	return eError;

errorExit:
	/* Error occurred - shut down everything and return */
	SysDeinitialise(gpsSysData);
	gpsSysData = IMG_NULL;
	return eError;
}

IMG_VOID SysEnableInterrupts(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	IMG_UINT32 ui32RegData;
	IMG_UINT32 ui32Mask;

#ifdef SUPPORT_MSVDX
	ui32Mask = CDV_SGX_MASK | CDV_MSVDX_MASK;
#else
	ui32Mask = CDV_SGX_MASK;
#endif

	ui32RegData = OSReadHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_IDENTITY_REG);
	OSWriteHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_IDENTITY_REG, ui32RegData | ui32Mask);
	/* unmask SGX bit in IMR */
	ui32RegData = OSReadHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_MASK_REG);
	OSWriteHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_MASK_REG, ui32RegData & (~ui32Mask));

	/* Enable SGX bit in IER */
	ui32RegData = OSReadHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_ENABLE_REG);
	OSWriteHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_ENABLE_REG, ui32RegData | ui32Mask);

	PVR_DPF((PVR_DBG_MESSAGE, "SysEnableInterrupts: Interrupts enabled"));
#endif
	PVR_UNREFERENCED_PARAMETER(psSysData);

}

IMG_VOID SysDisableInterrupts(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	IMG_UINT32 ui32RegData;
	IMG_UINT32 ui32Mask;

#if defined (SUPPORT_MSVDX)
	ui32Mask = CDV_SGX_MASK | CDV_MSVDX_MASK;
#else
	ui32Mask = CDV_SGX_MASK;
#endif

	/* Disable SGX bit in IER */
	ui32RegData = OSReadHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_ENABLE_REG);
	OSWriteHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_ENABLE_REG, ui32RegData & (~ui32Mask));

	/* Mask SGX bit in IMR */
	ui32RegData = OSReadHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_MASK_REG);
	OSWriteHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_MASK_REG, ui32RegData | ui32Mask);

	PVR_TRACE(("SysDisableInterrupts: Interrupts disabled"));
#endif
	PVR_UNREFERENCED_PARAMETER(psSysData);
}

/***********************************************************************//**
 * Final part of initialisation.  After this, the system is ready to
 * begin work
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysFinalise(IMG_VOID)
{
	PVRSRV_ERROR eError;
#if defined(SYS_USING_INTERRUPTS)

	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"OSInstallMISR: Failed to install MISR"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_MISR_INSTALLED);

	/* install a system ISR */
	eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"OSInstallSystemLISR: Failed to install ISR"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED);

	SysEnableInterrupts(gpsSysData);
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
#endif /* defined(SYS_USING_INTERRUPTS) */

#if !defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	SysPowerOnSGX();
#endif

	eError = SysCreateVersionString(gpsSysData, &gsSGXDeviceMap);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: Failed to create a system version string"));
	}
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "SysFinalise: Version string: %s", gpsSysData->pszVersionString));
	}

	return PVRSRV_OK;
}

/***********************************************************************//**
 * Deinitialise the system for unloading the driver
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysDeinitialise (SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA * psSysSpecData;
	PVRSRV_ERROR eError;

	if (psSysData == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SysDeinitialise: NULL SYS_DATA pointer."));
		return PVRSRV_OK;
	}

	psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

#if defined(SYS_USING_INTERRUPTS)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_IRQ_ENABLED))
	{
		SysDisableInterrupts(psSysData);
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
	}
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_LISR_INSTALLED))
	{
		eError = OSUninstallSystemLISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallSystemLISR failed"));
			return eError;
		}
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, SYS_SPECIFIC_DATA_LISR_INSTALLED);
	}
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_MISR_INSTALLED))
	{
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, SYS_SPECIFIC_DATA_MISR_INSTALLED);
	}
#endif /* #if defined(SYS_USING_INTERRUPTS) */

#if defined(SUPPORT_MSVDX)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_INITVXDDEV))
	{
		/* de-initialise all services managed devices */
		eError = PVRSRVDeinitialiseDevice(gui32MSVDXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, SYS_SPECIFIC_DATA_INITVXDDEV);
	}
#endif

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_INITSGXDEV))
	{
		/* de-initialise all services managed devices */
		eError = PVRSRVDeinitialiseDevice(gui32SGXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, SYS_SPECIFIC_DATA_INITSGXDEV);
	}

	SysFreeVersionString(psSysData);

	PCIDeInitDev(psSysData);

	SysDeinitialiseCommon(gpsSysData);

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENVDATA))
	{
		eError = OSDeInitEnvData(gpsSysData->pvEnvSpecificData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init env structure"));
			return eError;
		}
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, SYS_SPECIFIC_DATA_ENVDATA);
	}

#if defined(NO_HARDWARE)
#if defined(SUPPORT_MSVDX)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_DUMMY_MSVDX_REGS))
	{
		OSBaseFreeContigMemory(MSVDX_REG_SIZE,
							   gsMSVDXDeviceMap.sRegsCpuVBase,
							   gsMSVDXDeviceMap.sRegsCpuPBase);
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData,
								SYS_SPECIFIC_DATA_DUMMY_MSVDX_REGS);
	}
#endif

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_DUMMY_SGX_REGS))
	{
		OSBaseFreeContigMemory(SYS_SGX_REG_SIZE,
							   gsSGXDeviceMap.pvRegsCpuVBase,
							   gsSGXDeviceMap.sRegsCpuPBase);
		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData,
								SYS_SPECIFIC_DATA_DUMMY_SGX_REGS);
	}
#endif

#if !defined(NO_HARDWARE)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_SOC_REGS_MAPPED))
	{
		OSUnMapPhysToLin(gsSOCDeviceMap.sRegsCpuVBase,
													  SYS_SOC_REG_SIZE,
													  PVRSRV_HAP_KERNEL_ONLY | PVRSRV_HAP_UNCACHED,
													  IMG_NULL);

		SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, SYS_SPECIFIC_DATA_SOC_REGS_MAPPED);
	}
#endif
	psSysSpecData->ui32SysSpecificData = 0;
	gpsSysData = IMG_NULL;

	return PVRSRV_OK;
}

/***********************************************************************//**
 * Get the device map for the specified device
 *
 * @param	eDeviceType		Device type
 * @param[out]	ppvDeviceMap	Device map
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_VOID **ppvDeviceMap)
{
	switch(eDeviceType)
	{
	case PVRSRV_DEVICE_TYPE_SGX:
	{
		/* just return a pointer to the structure */
		*ppvDeviceMap = (IMG_VOID*)&gsSGXDeviceMap;
		break;
	}
#if defined(SUPPORT_MSVDX)
	case PVRSRV_DEVICE_TYPE_MSVDX:
	{
		/* just return a pointer to the structure */
		*ppvDeviceMap = (IMG_VOID*)&gsMSVDXDeviceMap;
		break;
	}
#endif
	default:
	{
		PVR_DPF((PVR_DBG_ERROR,"SysGetDeviceMemoryMap: unsupported device type"));
	}
	}
	return PVRSRV_OK;
}

/***********************************************************************//**
 * Translate a CPU physical address to a device physical address
 *
 * as we're a UMA system, the device physical address with be the same,
 * so all we need to do is return the CPU physical address as-is.
 *
 * @param	eDeviceType		Device type
 * @param	CpuPAddr		CPU physical address to translate
 *
 * @returns Equivalent device physical address
 **************************************************************************/
IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
									   IMG_CPU_PHYADDR sCpuPAddr)
{
	IMG_DEV_PHYADDR sDevPAddr;
	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	sDevPAddr.uiAddr = sCpuPAddr.uiAddr;

	return sDevPAddr;
}

/***********************************************************************//**
 * Translate a device physical address to a CPU physical address
 *
 * as we're a UMA system, the device physical address with be the same,
 * so all we need to do is return the CPU physical address as-is.
 *
 * @param	sSysPAddr		system physical address to translate
 *
 * @returns Equivalent CPU physical address
 **************************************************************************/
IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr (IMG_SYS_PHYADDR sSysPAddr)
{
	IMG_CPU_PHYADDR sCpuPAddr;

	sCpuPAddr.uiAddr = sSysPAddr.uiAddr;

	return sCpuPAddr;
}

/***********************************************************************//**
 * Translate a CPU physical address to a system physical address
 *
 * as we're a UMA system, the device physical address with be the same,
 * so all we need to do is return the CPU physical address as-is.
 *
 * @param	sCpuPAddr		CPU physical address to translate
 *
 * @returns Equivalent system physical address
 **************************************************************************/
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR sCpuPAddr)
{
	IMG_SYS_PHYADDR sSysPAddr;

	sSysPAddr.uiAddr = sCpuPAddr.uiAddr;

	return sSysPAddr;
}

/***********************************************************************//**
 * Translate a system physical address to a CPU physical address
 *
 * as we're a UMA system, the device physical address with be the same,
 * so all we need to do is return the CPU physical address as-is.
 *
 * @param	eDeviceType		Device type
 * @param	sSysPAddr		system physical address to translate
 *
 * @returns Equivalent device physical address
 **************************************************************************/
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
									   IMG_SYS_PHYADDR sSysPAddr)
{
	IMG_DEV_PHYADDR sDevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);
    sDevPAddr.uiAddr = sSysPAddr.uiAddr;

	return sDevPAddr;
}

/***********************************************************************//**
 * Translate a device physical address to a system physical address
 *
 * as we're a UMA system, the device physical address with be the same,
 * so all we need to do is return the CPU physical address as-is.
 *
 * @param	eDeviceType		Device type
 * @param	sDevPAddr		system physical address to translate
 *
 * @returns Equivalent device physical address
 **************************************************************************/
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
									   IMG_DEV_PHYADDR sDevPAddr)
{
	IMG_SYS_PHYADDR sSysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* INTEGRATION_POINT: Is this used?  Is it correct? */
	PVR_DBG_BREAK;

	sSysPAddr.uiAddr = sDevPAddr.uiAddr;

	return sSysPAddr;
}

/***********************************************************************//**
 * Register an external device with services
 *
 * @param	psDeviceNode	   	Device node
 **************************************************************************/
IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	/*
	 * Register to ensure we correctly handle the interrupts from
	 * the external device
	 */
	psDeviceNode->ui32SOCInterruptBit = DEVICE_DISP_INTERRUPT;
}

/***********************************************************************//**
 * Remove an external device from the system
 *
 * @param	psDeviceNode	   	Device node
 **************************************************************************/
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	/* Do we need to do anything (remove the interrupt bit?) */
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}

/***********************************************************************//**
 * Reset a device
 *
 * @param	ui32DeviceIndex	   	Device index to reset
 **************************************************************************/
PVRSRV_ERROR SysResetDevice(IMG_UINT32 ui32DeviceIndex)
{
	if (ui32DeviceIndex == gui32SGXDeviceID)
	{
		/* INTEGRATION_POINT: What do we need for SGX resets? */
	}
#if defined(SUPPORT_MSVDX)
	else if (ui32DeviceIndex == gui32MSVDXDeviceID)
	{
		/* INTEGRATION_POINT: What do we need for VXD resets? */
	}
#endif
	else
	{
		/* INTEGRATION_POINT: Display? */
		return PVRSRV_ERROR_INVALID_DEVICEID;
	}

	return PVRSRV_OK;
}

/***********************************************************************//**
 * Marshalling function for custom OEM functions
 *
 * @param	ui32ID	   	Function ID
 * @param	pvIn		Input data
 * @param	ui32InSize	Size of input data
 * @param	pvOut		Output data
 * @param	ui32OutSize	Size of output data
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysOEMFunction (IMG_UINT32	ui32ID,
							 IMG_PVOID pvIn,
							 IMG_UINT32 ui32InSize,
							 IMG_PVOID pvOut,
							 IMG_UINT32 ui32OutSize)
{
	PVR_UNREFERENCED_PARAMETER(ui32ID);
	PVR_UNREFERENCED_PARAMETER(pvIn);
	PVR_UNREFERENCED_PARAMETER(ui32InSize);
	PVR_UNREFERENCED_PARAMETER(pvOut);
	PVR_UNREFERENCED_PARAMETER(ui32OutSize);

	return PVRSRV_ERROR_INVALID_PARAMS;
}

/***********************************************************************//**
 * Map the CPU physical backing for the registers to CPU virtual memory
 * so we can write to it from the host
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
static PVRSRV_ERROR SysMapInRegisters(void)
{
	PVRSRV_DEVICE_NODE *psDeviceNodeList;

	psDeviceNodeList = gpsSysData->psDeviceNodeList;

	while (psDeviceNodeList)
	{
		switch (psDeviceNodeList->sDevId.eDeviceType)
		{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)
				psDeviceNodeList->pvDevice;
#if !defined(NO_HARDWARE)
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS))
			{
				/* Remap Regs */
				psDevInfo->pvRegsBaseKM =
					OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
								   gsSGXDeviceMap.ui32RegsSize,
								   PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
								   IMG_NULL);

				if (!psDevInfo->pvRegsBaseKM)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysMapInRegisters : Failed to map in regs\n"));
					return PVRSRV_ERROR_BAD_MAPPING;
				}
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS);
			}
#else	/* !defined(NO_HARDWARE) */
			/*
			 * SysLocateDevices will have reallocated the dummy
			 * registers.
			 */
			psDevInfo->pvRegsBaseKM = gsSGXDeviceMap.pvRegsCpuVBase;
#endif	/* !defined(NO_HARDWARE) */
			psDevInfo->ui32RegSize   = gsSGXDeviceMap.ui32RegsSize;
			psDevInfo->sRegsPhysBase = gsSGXDeviceMap.sRegsSysPBase;
			break;
		}
#if defined(SUPPORT_MSVDX)
		case PVRSRV_DEVICE_TYPE_MSVDX:
		{
			PVRSRV_MSVDXDEV_INFO *psDevInfo = (PVRSRV_MSVDXDEV_INFO *)
				psDeviceNodeList->pvDevice;
#if !defined(NO_HARDWARE)
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_MSVDX_REGS))
			{
				/* Remap registers */
				psDevInfo->pvRegsBaseKM =
					OSMapPhysToLin(gsMSVDXDeviceMap.sRegsCpuPBase,
								   gsMSVDXDeviceMap.ui32RegsSize,
								   PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
								   IMG_NULL);
				if (!psDevInfo->pvRegsBaseKM)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysMapInRegisters : Failed to map MSVDX registers\n"));
					return PVRSRV_ERROR_BAD_MAPPING;
				}
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_MSVDX_REGS);
			}
#else	/* !defined(NO_HARDWARE) */
			/*
			 * SysLocateDevices will have reallocated the dummy
			 * registers.
			 */
			psDevInfo->pvRegsBaseKM = gsMSVDXDeviceMap.sRegsCpuVBase;
#endif	/* !defined(NO_HARDWARE) */
			psDevInfo->ui32RegSize = gsMSVDXDeviceMap.ui32RegsSize;
			psDevInfo->sRegsPhysBase = gsMSVDXDeviceMap.sRegsSysPBase;
			break;
		}
#endif	/* SUPPORT_MSVDX */
		default:
			/* Ignore any other (unknown) devices */
			break;
		}

		psDeviceNodeList = psDeviceNodeList->psNext;
	}

	return PVRSRV_OK;
}

/***********************************************************************//**
 * Unmap the CPU virtual memory from the registers
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
static PVRSRV_ERROR SysUnmapRegisters(void)
{
	PVRSRV_DEVICE_NODE *psDeviceNodeList;

	psDeviceNodeList = gpsSysData->psDeviceNodeList;

	while (psDeviceNodeList)
	{
		switch (psDeviceNodeList->sDevId.eDeviceType)
		{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)
				psDeviceNodeList->pvDevice;
#if !defined(NO_HARDWARE)
			/* Unmap Regs */
			if (psDevInfo->pvRegsBaseKM)
			{
				OSUnMapPhysToLin(psDevInfo->pvRegsBaseKM,
				                 gsSGXDeviceMap.ui32RegsSize,
				                 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				                 IMG_NULL);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS);
			}
#endif
			psDevInfo->pvRegsBaseKM = IMG_NULL;
			psDevInfo->ui32RegSize          = 0;
			psDevInfo->sRegsPhysBase.uiAddr = 0;
			break;
		}
#ifdef SUPPORT_MSVDX
		case PVRSRV_DEVICE_TYPE_MSVDX:
		{
			PVRSRV_MSVDXDEV_INFO *psDevInfo = (PVRSRV_MSVDXDEV_INFO *)
				psDeviceNodeList->pvDevice;

#if !defined(NO_HARDWARE)
			if (psDevInfo->pvRegsBaseKM)
			{
				OSUnMapPhysToLin(psDevInfo->pvRegsBaseKM,
								 psDevInfo->ui32RegSize,
								 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
								 IMG_NULL);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_MSVDX_REGS);
			}
#endif
			psDevInfo->pvRegsBaseKM = IMG_NULL;
			psDevInfo->ui32RegSize = 0;
			psDevInfo->sRegsPhysBase.uiAddr = 0;
			break;
		}
#endif	/* SUPPORT_MSVDX */
		default:
			/* Ignore unknowns */
			break;
		}
		psDeviceNodeList = psDeviceNodeList->psNext;
	}
#if defined(NO_HARDWARE)
#if defined(SUPPORT_MSVDX)
	if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_DUMMY_MSVDX_REGS))
	{
		PVR_ASSERT(gsMSVDXDeviceMap);

		OSBaseFreeContigMemory(MSVDX_REG_SIZE,
							   gsMSVDXDeviceMap.sRegsCpuVBase,
							   gsMSVDXDeviceMap.sRegsCpuPBase);

		SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_DUMMY_MSVDX_REGS);
	}
#endif /* defined(SUPPORT_MSVDX) */

	if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_DUMMY_SGX_REGS))
	{
		OSBaseFreeContigMemory(SYS_SGX_REG_SIZE,
							   gsSGXDeviceMap.pvRegsCpuVBase,
							   gsSGXDeviceMap.sRegsCpuPBase);

		SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_DUMMY_SGX_REGS);
	}

#endif /* defined(NO_HARDWARE) */

#if !defined(NO_HARDWARE)
	if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_SOC_REGS_MAPPED))
	{
		OSUnMapPhysToLin(gsSOCDeviceMap.sRegsCpuVBase,
													  SYS_SOC_REG_SIZE,
													  PVRSRV_HAP_KERNEL_ONLY | PVRSRV_HAP_UNCACHED,
													  IMG_NULL);

		SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_SOC_REGS_MAPPED);
	}
#endif
	return PVRSRV_OK;
}

/***********************************************************************//**
 * Return a mask of known devices that generated the system interrupt
 *
 * @param 	psSysData		System data
 * @param	psDeviceNode	Device node
 *
 * @returns Mask of devices with outstanding interrupts
 **************************************************************************/
IMG_UINT32 SysGetInterruptSource(SYS_DATA			*psSysData,
								 PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if defined(NO_HARDWARE)
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
	/* With no hardware, we always*/

	return 0;
#else
	IMG_UINT32 ui32Devices = 0;
	IMG_UINT32 ui32Data, ui32DIMMask;

	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	/*
	 * INTEGRATION_POINT: Check with SOC registers to see if any interrupts are
	 * outstanding
	 */

	/* Skip if registers are not initialised beforehand. */
	if (gsSOCDeviceMap.sRegsCpuVBase == 0)
	{
		return 0;
	}

	/* check if there is an interrupt to process */
	ui32Data = OSReadHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_IDENTITY_REG);

	if (ui32Data & CDV_SGX_MASK)
	{
		ui32Devices |= DEVICE_SGX_INTERRUPT;
	}

	if (ui32Data & CDV_MSVDX_MASK)
	{
		ui32Devices |= DEVICE_MSVDX_INTERRUPT;
	}

	/* any other interrupts are for the DIM */
	ui32DIMMask = OSReadHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_ENABLE_REG);
	ui32DIMMask &= ~(CDV_SGX_MASK | CDV_MSVDX_MASK);

	/* test for anything relating to the display, vsyncs, hot-plug etc */
	if (ui32Data & ui32DIMMask)
	{
		ui32Devices |= DEVICE_DISP_INTERRUPT;
	}

	return ui32Devices;
#endif
}

/***********************************************************************//**
 * Clear the interrupts from the given device mask
 *
 * @param 	psSysData		System data
 * @param	ui32DeviceMask	Mask of devices which we should clear the interrupt
 *							from
 **************************************************************************/
IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32DeviceMask)
{
#if defined(NO_HARDWARE)
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32DeviceMask);
#else
IMG_UINT32 ui32Data;
	IMG_UINT32 ui32Mask = 0;

	PVR_UNREFERENCED_PARAMETER(psSysData);

	ui32Data = OSReadHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_IDENTITY_REG);

	if ((ui32DeviceMask & DEVICE_SGX_INTERRUPT) &&
		(ui32Data & CDV_SGX_MASK))
	{
		ui32Mask |= CDV_SGX_MASK;
	}

	if ((ui32DeviceMask & DEVICE_MSVDX_INTERRUPT) &&
		(ui32Data & CDV_MSVDX_MASK))
	{
		ui32Mask |= CDV_MSVDX_MASK;
	}

	if ((ui32DeviceMask & DEVICE_DISP_INTERRUPT) &&
		(ui32Data & CDV_VSYNC_PIPEA_VBLANK_MASK))
	{
	  ui32Mask |= CDV_VSYNC_PIPEA_VBLANK_MASK;
	}

	if (ui32Mask)
	{
		OSWriteHWReg(gsSOCDeviceMap.sRegsCpuVBase, CDV_INTERRUPT_IDENTITY_REG, ui32Mask);
	}

	/* INTEGRATION_POINT: Clear SOC registers for devices */
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32DeviceMask);
#endif
}

/***********************************************************************//**
 * Perform system-level processing required before a system power transition
 *
 * @param eNewPowerState		Power state we're switching to
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError= PVRSRV_OK;

	if (eNewPowerState != gpsSysData->eCurrentPowerState)
	{
		if ((eNewPowerState == PVRSRV_SYS_POWER_STATE_D3) &&
			(gpsSysData->eCurrentPowerState < PVRSRV_SYS_POWER_STATE_D3))
		{
			/*
			 * About to enter D3 state.
			 */
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED))
			{
				SysDisableInterrupts(gpsSysData);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
			}

#if defined (SYS_USING_INTERRUPTS)
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED))
			{
				eError = OSUninstallSystemLISR(gpsSysData);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysSystemPrePowerState: OSUninstallSystemLISR failed (%d)", eError));
				}
				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);

				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED);
			}
#endif /* defined (SYS_USING_INTERRUPTS) */

			/*
			 * Unmap the system-level registers.
			 */
			SysUnmapRegisters();
		}
	}
	return eError;
}

/***********************************************************************//**
 * Perform system-level processing required after a system power transition
 *
 * @param eNewPowerState		Power state we're switching to
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (eNewPowerState != gpsSysData->eCurrentPowerState)
	{
		if ((gpsSysData->eCurrentPowerState == PVRSRV_SYS_POWER_STATE_D3) &&
			(eNewPowerState < PVRSRV_SYS_POWER_STATE_D3))
		{
			/*
			 * Returning from D3 state.
			 * Find the device again as it may have been remapped.
			 */
			eError = SysLocateDevices(gpsSysData);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to locate devices"));
				return eError;
			}

			/*
			 * Remap the system-level registers.
			 */
			eError = SysMapInRegisters();
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to map in registers"));
				return eError;
			}

#if defined (SYS_USING_INTERRUPTS)
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR))
			{
				eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: OSInstallSystemLISR failed to install ISR (%d)", eError));
				}
				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
			}
#endif /* defined (SYS_USING_INTERRUPTS) */

			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE))
			{
				SysEnableInterrupts(gpsSysData);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE);
			}
		}
	}
	return eError;
}


/***********************************************************************//**
 * Perform system-level processing required before a device power transition
 *
 * @param ui32DeviceIndex		Device being switched
 * @param eNewPowerState		Power state we're switching to
 * @param eCurrentPowerState	Power state we're currently in
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32 ui32DeviceIndex,
                                    PVRSRV_DEV_POWER_STATE eNewPowerState,
                                    PVRSRV_DEV_POWER_STATE eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(ui32DeviceIndex);
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	return PVRSRV_OK;
}

/***********************************************************************//**
 * Perform system-level processing required after a device power transition
 *
 * @param ui32DeviceIndex		Device being switched
 * @param eNewPowerState		Power state we're switching to
 * @param eCurrentPowerState	Power state we're currently in
 *
 * @returns PVRSRV_OK for success, or failure code
 **************************************************************************/
PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32 ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE eNewPowerState,
									 PVRSRV_DEV_POWER_STATE eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(ui32DeviceIndex);
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	return PVRSRV_OK;
}

/*****************************************************************************
 * Turn SGX on.  This function is intended for use during system
 * initialisation/deinitialisation, to allow access to SGX registers.
******************************************************************************/
IMG_VOID SysPowerOnSGX(IMG_VOID)
{
}

/*****************************************************************************
 * Turn SGX Off. This function is intended for use during system
 * initialisation/deinitialisation, to turn SGX off after calling
 * PowerOnSGX.
******************************************************************************/
IMG_VOID SysPowerOffSGX(IMG_VOID)
{
}

