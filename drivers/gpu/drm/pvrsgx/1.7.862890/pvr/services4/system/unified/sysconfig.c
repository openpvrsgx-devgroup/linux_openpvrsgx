/**********************************************************************
 *
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

#include "linux/pci.h"

#include "sgxdefs.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "pdump_km.h"
#include "syslocal.h"
#include "env_data.h"
#include "psb_drv.h"
#include "psb_powermgmt.h"
#include "sys_pvr_drm_export.h"
#include "msvdx_power.h"


/* Graphics MSI address and data region in PCIx */
#define MRST_PCIx_MSI_ADDR_LOC		0x94
#define MRST_PCIx_MSI_DATA_LOC		0x98

#define SYS_SGX_CLOCK_SPEED			(400000000)
#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100)
#define SYS_SGX_PDS_TIMER_FREQ			(1000)
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(5)

#define	DRI_DRM_STATIC
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
SYS_DATA  gsSysData;

static SYS_SPECIFIC_DATA gsSysSpecificData;

IMG_UINT32 gui32SGXDeviceID;
extern IMG_UINT32 	gui32MRSTDisplayDeviceID;
IMG_UINT32		gui32MRSTMSVDXDeviceID;
IMG_UINT32		gui32MRSTTOPAZDeviceID;

static SGX_DEVICE_MAP	gsSGXDeviceMap;
extern struct drm_device *gpDrmDevice;

static PVRSRV_DEVICE_NODE *gpsSGXDevNode;

IMG_CPU_VIRTADDR gsPoulsboRegsCPUVaddr;

IMG_CPU_VIRTADDR gsPoulsboDisplayRegsCPUVaddr;

extern struct pci_dev *gpsPVRLDMDev;

bool need_sample_cache_workaround;

#define	POULSBO_ADDR_RANGE_INDEX	(MMADR_INDEX - 4)
#define	POULSBO_HP_ADDR_RANGE_INDEX	(GMADR_INDEX - 4)
static PVRSRV_ERROR PCIInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	IMG_UINT32 ui32MaxOffset = POULSBO_MAX_OFFSET;
	struct pci_dev *mch_dev;

	if (!IS_CDV(gpDrmDevice))
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device not supported"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	mch_dev = pci_get_bus_and_slot(0, PCI_DEVFN(0,0));
	if (!mch_dev)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Failed to get (0,0,0) device"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}
	if (mch_dev->revision < 4) {
		need_sample_cache_workaround = true;
		printk(KERN_INFO "Cedartrail: apply sample cache workaround\n");
	} else
		need_sample_cache_workaround = false;

	pci_dev_put(mch_dev);

	psSysSpecData->hSGXPCI = OSPCISetDev((IMG_VOID *)psSysSpecData->psPCIDev, 0);
	ui32MaxOffset = PSB_POULSBO_MAX_OFFSET;

	if (!psSysSpecData->hSGXPCI)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Failed to acquire PCI device"));
		return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
	}

	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV);

	PVR_TRACE(("PCI memory region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX)));
#if defined(SGX_FEATURE_HOST_PORT)
	PVR_TRACE(("Host Port region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX)));
#endif

	if (OSPCIAddrRangeLen(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX) < ui32MaxOffset)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region isn't big enough"));
		return PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
	}


	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;

	}
	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE);

#if defined(SGX_FEATURE_HOST_PORT)

	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Host Port region not available"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}
	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_HOST_PORT_RANGE);
#endif
	return PVRSRV_OK;
}

static IMG_VOID PCIDeInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX);
	}
#if defined(SGX_FEATURE_HOST_PORT)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_HOST_PORT_RANGE))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX);
	}
#endif
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV))
	{
		OSPCIReleaseDev(psSysSpecData->hSGXPCI);
	}
}
static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32BaseAddr = 0;
	IMG_UINT32 ui32IRQ = 0;

#if defined(SGX_FEATURE_HOST_PORT)
	IMG_UINT32 ui32HostPortAddr = 0;
#endif
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
   struct drm_psb_private *dev_priv = (struct drm_psb_private *) gpDrmDevice->dev_private;
#endif

	ui32BaseAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX);
#if defined(SGX_FEATURE_HOST_PORT)
	ui32HostPortAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX);
#endif
	if (OSPCIIRQ(psSysSpecData->hSGXPCI, &ui32IRQ) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Couldn't get IRQ"));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	PVR_TRACE(("ui32BaseAddr: %08X", ui32BaseAddr));
#if defined(SGX_FEATURE_HOST_PORT)
	PVR_TRACE(("ui32HostPortAddr: %08X", ui32HostPortAddr));
#endif
	PVR_TRACE(("IRQ: %d", ui32IRQ));


	gsSGXDeviceMap.ui32Flags = 0x0;
	gsSGXDeviceMap.ui32IRQ = ui32IRQ;


	gsSGXDeviceMap.sRegsSysPBase.uiAddr = ui32BaseAddr + (IS_CDV(gpDrmDevice) ? SGX_REGS_OFFSET : PSB_SGX_REGS_OFFSET);
	gsSGXDeviceMap.sRegsCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = SGX_REG_SIZE;

#if defined(SGX_FEATURE_HOST_PORT)

	gsSGXDeviceMap.ui32Flags = SGX_HOSTPORT_PRESENT;
	gsSGXDeviceMap.sHPSysPBase.uiAddr = ui32HostPortAddr;
	gsSGXDeviceMap.sHPCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sHPSysPBase);
	if (IS_CDV(gpDrmDevice) )
		gsSGXDeviceMap.ui32HPSize = SYS_SGX_HP_SIZE;
	else
		gsSGXDeviceMap.ui32HPSize = PSB_SYS_SGX_HP_SIZE;
#endif

#if defined(MRST_SLAVEPORT)

	gsSGXDeviceMap.sSPSysPBase.uiAddr = ui32BaseAddr + MRST_SGX_SP_OFFSET;
	gsSGXDeviceMap.sSPCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sSPSysPBase);
	gsSGXDeviceMap.ui32SPSize = SGX_SP_SIZE;
#endif



	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemDevPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr = 0;
	gsSGXDeviceMap.ui32LocalMemSize = 0;


	{
		IMG_SYS_PHYADDR sPoulsboRegsCpuPBase;
		sPoulsboRegsCpuPBase.uiAddr = ui32BaseAddr + POULSBO_REGS_OFFSET;
		gsPoulsboRegsCPUVaddr = OSMapPhysToLin(SysSysPAddrToCpuPAddr(sPoulsboRegsCpuPBase),
												 POULSBO_REG_SIZE,
												 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
												 IMG_NULL);

		sPoulsboRegsCpuPBase.uiAddr = ui32BaseAddr + POULSBO_DISPLAY_REGS_OFFSET;
		gsPoulsboDisplayRegsCPUVaddr = OSMapPhysToLin(SysSysPAddrToCpuPAddr(sPoulsboRegsCpuPBase),
												 POULSBO_DISPLAY_REG_SIZE,
												 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
												 IMG_NULL);
	}

#if defined(PDUMP)
	{

		static IMG_CHAR pszPDumpDevName[] = "SGXMEM";
		gsSGXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif

	return PVRSRV_OK;
}


#define VERSION_STR_MAX_LEN_TEMPLATE "SGX revision = 000.000.000"
static PVRSRV_ERROR SysCreateVersionString(SYS_DATA *psSysData)
{
    IMG_UINT32 ui32MaxStrLen;
    PVRSRV_ERROR eError;
    IMG_INT32 i32Count;
    IMG_CHAR *pszVersionString;
    IMG_UINT32 ui32SGXRevision = 0;
	IMG_VOID *pvSGXRegs;

	pvSGXRegs = OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
											 gsSGXDeviceMap.ui32RegsSize,
											 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											 IMG_NULL);

	if (pvSGXRegs != IMG_NULL)
	{
            ui32SGXRevision = OSReadHWReg(pvSGXRegs, EUR_CR_CORE_REVISION);
	     OSUnMapPhysToLin(pvSGXRegs,
												gsSGXDeviceMap.ui32RegsSize,
												PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
												IMG_NULL);
	}
	else
	{
	     PVR_DPF((PVR_DBG_ERROR,"SysCreateVersionString: Couldn't map SGX registers"));
	}

    ui32MaxStrLen = OSStringLength(VERSION_STR_MAX_LEN_TEMPLATE);
    eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
                          ui32MaxStrLen + 1,
                          (IMG_PVOID *)&pszVersionString,
                          IMG_NULL,
			  "Version String");
    if(eError != PVRSRV_OK)
    {
		return eError;
    }

    i32Count = OSSNPrintf(pszVersionString, ui32MaxStrLen + 1,
                           "SGX revision = %u.%u.%u",
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MAJOR_MASK)
                            >> EUR_CR_CORE_REVISION_MAJOR_SHIFT),
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MINOR_MASK)
                            >> EUR_CR_CORE_REVISION_MINOR_SHIFT),
                           (IMG_UINT)((ui32SGXRevision & EUR_CR_CORE_REVISION_MAINTENANCE_MASK)
                            >> EUR_CR_CORE_REVISION_MAINTENANCE_SHIFT)
                           );
    if(i32Count == -1)
    {
        ui32MaxStrLen = OSStringLength(VERSION_STR_MAX_LEN_TEMPLATE);
        OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
                    ui32MaxStrLen + 1,
                    pszVersionString,
                    IMG_NULL);

		return PVRSRV_ERROR_INVALID_PARAMS;
    }

    psSysData->pszVersionString = pszVersionString;

    return PVRSRV_OK;
}

static IMG_VOID SysFreeVersionString(SYS_DATA *psSysData)
{
    if(psSysData->pszVersionString)
    {
        IMG_UINT32 ui32MaxStrLen;
        ui32MaxStrLen = OSStringLength(VERSION_STR_MAX_LEN_TEMPLATE);
        OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
                    ui32MaxStrLen+1,
                    psSysData->pszVersionString,
                    IMG_NULL);
		psSysData->pszVersionString = IMG_NULL;
    }
}

PVRSRV_ERROR SysInitialise(IMG_VOID)
{
	IMG_UINT32			i			  = 0;
	PVRSRV_ERROR 		eError;
	PVRSRV_DEVICE_NODE	*psDeviceNode;
	SGX_TIMING_INFORMATION*	psTimingInfo;

	gpsSysData = &gsSysData;
	OSMemSet(gpsSysData, 0, sizeof(SYS_DATA));

	gpsSysData->pvSysSpecificData = &gsSysSpecificData;
	gsSysSpecificData.ui32SysSpecificData = 0;

	PVR_ASSERT(gpsPVRLDMDev != IMG_NULL);
	gsSysSpecificData.psPCIDev = gpsPVRLDMDev;

	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to setup env structure"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}


	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = (drm_psb_ospm != 0);
	/*printk(KERN_ERR "SGX APM is %s\n", (drm_psb_ospm != 0)? "enabled":"disabled"); */
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;

	eError = PCIInitDev(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	gpsSysData->ui32NumDevices = SYS_DEVICE_COUNT;


	for(i=0; i<SYS_DEVICE_COUNT; i++)
	{
		gpsSysData->sDeviceID[i].uiID = i;
		gpsSysData->sDeviceID[i].bInUse = IMG_FALSE;
	}

	gpsSysData->psDeviceNodeList = IMG_NULL;
	gpsSysData->psQueueList = IMG_NULL;

	eError = SysInitialiseCommon(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed in SysInitialiseCommon"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}





	eError = SysLocateDevices(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to locate devices"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}




	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice,
								  DEVICE_SGX_INTERRUPT, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}


	/* register MSVDX, with 0 interrupt bit, no interrupt will be served */
	eError = PVRSRVRegisterDevice(gpsSysData, MSVDXRegisterDevice,
				      DEVICE_MSVDX_INTERRUPT, &gui32MRSTMSVDXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register MSVDXdevice!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	psDeviceNode = gpsSysData->psDeviceNodeList;

	while(psDeviceNode)
	{

		switch(psDeviceNode->sDevId.eDeviceType)
		{
			case PVRSRV_DEVICE_TYPE_SGX:
			{
				DEVICE_MEMORY_INFO *psDevMemoryInfo;
				DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;


				psDeviceNode->psLocalDevMemArena = IMG_NULL;


				psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
				psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;


				for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
				{
					psDeviceMemoryHeap[i].ui32Attribs |= PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;
#ifdef OEM_CUSTOMISE

#endif
				}

				gpsSGXDevNode = psDeviceNode;
				break;
			}
			case PVRSRV_DEVICE_TYPE_MSVDX:
			/* nothing need to do here */
			break;
			case PVRSRV_DEVICE_TYPE_TOPAZ:
			break;
			default:
			{
				PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to find SGX device node!"));
				return PVRSRV_ERROR_INIT_FAILURE;
			}
		}


		psDeviceNode = psDeviceNode->psNext;
	}

	PDUMPINIT();
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PDUMP_INIT);


	eError = PVRSRVInitialiseDevice (gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_SGX_INITIALISED);

	eError = PVRSRVInitialiseDevice (gui32MRSTMSVDXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	if (0)
	{
		eError = PVRSRVInitialiseDevice (gui32MRSTTOPAZDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
			SysDeinitialise(gpsSysData);
			gpsSysData = IMG_NULL;
			return eError;
		}
	}

	return PVRSRV_OK;
}


PVRSRV_ERROR SysFinalise(IMG_VOID)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: OSInstallMISR failed"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_MISR_INSTALLED);

	eError = SysCreateVersionString(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to create a system version string"));
	}
	else
	{
	    PVR_DPF((PVR_DBG_WARNING, "SysFinalise: Version string: %s", gpsSysData->pszVersionString));
	}

	return eError;
}

PVRSRV_ERROR SysDeinitialise (SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError;

	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_MISR_INSTALLED))
	{
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_SGX_INITIALISED))
	{

		eError = PVRSRVDeinitialiseDevice(gui32SGXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
	}

	SysFreeVersionString(psSysData);

	PCIDeInitDev(psSysData);

	eError = OSDeInitEnvData(psSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init env structure"));
		return eError;
	}

	SysDeinitialiseCommon(gpsSysData);


	OSUnMapPhysToLin(gsPoulsboRegsCPUVaddr,
											 POULSBO_REG_SIZE,
											 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											 IMG_NULL);

	OSUnMapPhysToLin(gsPoulsboDisplayRegsCPUVaddr,
											 POULSBO_DISPLAY_REG_SIZE,
											 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											 IMG_NULL);

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PDUMP_INIT))
	{
		PDUMPDEINIT();
	}

	gpsSysData = IMG_NULL;

	return PVRSRV_OK;
}

IMG_UINT32 SysGetInterruptSource(SYS_DATA* psSysData,
		PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return 0;
}

IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32ClearBits)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32ClearBits);
}


PVRSRV_ERROR SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_VOID **ppvDeviceMap)
{
	switch(eDeviceType)
	{
		case PVRSRV_DEVICE_TYPE_SGX:
		{

			*ppvDeviceMap = (IMG_VOID*)&gsSGXDeviceMap;
			break;
		}
		default:
		{
			PVR_DPF((PVR_DBG_ERROR,"SysGetDeviceMemoryMap: unsupported device type"));
		}
	}
	return PVRSRV_OK;
}


IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
										IMG_CPU_PHYADDR CpuPAddr)
{
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);


	DevPAddr.uiAddr = CpuPAddr.uiAddr;

	return DevPAddr;
}


IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr (IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;


	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;


	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}


IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
    IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);


    DevPAddr.uiAddr = SysPAddr.uiAddr;

    return DevPAddr;
}


IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR DevPAddr)
{
    IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);


    SysPAddr.uiAddr = DevPAddr.uiAddr;

    return SysPAddr;
}


IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{

  psDeviceNode->ui32SOCInterruptBit = DEVICE_DISP_INTERRUPT;
}


IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}

PVRSRV_ERROR SysOEMFunction (	IMG_UINT32	ui32ID,
								IMG_VOID	*pvIn,
								IMG_UINT32  ulInSize,
								IMG_VOID	*pvOut,
								IMG_UINT32	ulOutSize)
{
	PVR_UNREFERENCED_PARAMETER(ulInSize);
	PVR_UNREFERENCED_PARAMETER(pvIn);

	return PVRSRV_ERROR_INVALID_PARAMS;
}


static PVRSRV_ERROR SysMapInRegisters(IMG_VOID)
{
	PVRSRV_DEVICE_NODE *psDeviceNodeList;

	psDeviceNodeList = gpsSysData->psDeviceNodeList;

	while (psDeviceNodeList)
	{
		switch(psDeviceNodeList->sDevId.eDeviceType)
		{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNodeList->pvDevice;

			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS))
			{
				psDevInfo->pvRegsBaseKM = OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
									 gsSGXDeviceMap.ui32RegsSize,
									 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
									 IMG_NULL);

				if (!psDevInfo->pvRegsBaseKM)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysMapInRegisters : Failed to map in SGX registers\n"));
					return PVRSRV_ERROR_BAD_MAPPING;
				}
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS);
			}
			psDevInfo->ui32RegSize   = gsSGXDeviceMap.ui32RegsSize;
			psDevInfo->sRegsPhysBase = gsSGXDeviceMap.sRegsSysPBase;

#if defined(SGX_FEATURE_HOST_PORT)
			if (gsSGXDeviceMap.ui32Flags & SGX_HOSTPORT_PRESENT)
			{
				if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_HP))
				{

					psDevInfo->pvHostPortBaseKM = OSMapPhysToLin(gsSGXDeviceMap.sHPCpuPBase,
														     gsSGXDeviceMap.ui32HPSize,
														     PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
														     IMG_NULL);
					if (!psDevInfo->pvHostPortBaseKM)
					{
						PVR_DPF((PVR_DBG_ERROR,"SysMapInRegisters : Failed to map in host port\n"));
						return PVRSRV_ERROR_BAD_MAPPING;
					}
					SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_HP);
				}
				psDevInfo->ui32HPSize  = gsSGXDeviceMap.ui32HPSize;
				psDevInfo->sHPSysPAddr = gsSGXDeviceMap.sHPSysPBase;
			}
#endif
			break;
		}
		default:
			break;
		}
		psDeviceNodeList = psDeviceNodeList->psNext;
	}

	return PVRSRV_OK;
}


static PVRSRV_ERROR SysUnmapRegisters(IMG_VOID)
{
	PVRSRV_DEVICE_NODE *psDeviceNodeList;

	psDeviceNodeList = gpsSysData->psDeviceNodeList;

	while (psDeviceNodeList)
	{
		switch (psDeviceNodeList->sDevId.eDeviceType)
		{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNodeList->pvDevice;

			if (psDevInfo->pvRegsBaseKM)
			{
				OSUnMapPhysToLin(psDevInfo->pvRegsBaseKM,
				                 gsSGXDeviceMap.ui32RegsSize,
				                 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				                 IMG_NULL);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS);
			}

			psDevInfo->pvRegsBaseKM = IMG_NULL;
			psDevInfo->ui32RegSize          = 0;
			psDevInfo->sRegsPhysBase.uiAddr = 0;

#if defined(SGX_FEATURE_HOST_PORT)
			if (gsSGXDeviceMap.ui32Flags & SGX_HOSTPORT_PRESENT)
			{

				if (psDevInfo->pvHostPortBaseKM)
				{
					OSUnMapPhysToLin(psDevInfo->pvHostPortBaseKM,
					                 gsSGXDeviceMap.ui32HPSize,
					                 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
					                 IMG_NULL);

					SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_HP);

					psDevInfo->pvHostPortBaseKM = IMG_NULL;
				}

				psDevInfo->ui32HPSize  = 0;
				psDevInfo->sHPSysPAddr.uiAddr = 0;
			}
#endif
			break;
		}
		default:
			break;
		}
		psDeviceNodeList = psDeviceNodeList->psNext;
	}

	OSUnMapPhysToLin(gsPoulsboRegsCPUVaddr,
				POULSBO_REG_SIZE,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				IMG_NULL);


	OSUnMapPhysToLin(gsPoulsboDisplayRegsCPUVaddr,
				POULSBO_DISPLAY_REG_SIZE,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				IMG_NULL);

	return PVRSRV_OK;
}


PVRSRV_ERROR SysSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError= PVRSRV_OK;
	PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)(gsSysSpecificData.hSGXPCI);

	if (eNewPowerState != gpsSysData->eCurrentPowerState)
	{
		if ((eNewPowerState == PVRSRV_SYS_POWER_STATE_D3) &&
			(gpsSysData->eCurrentPowerState < PVRSRV_SYS_POWER_STATE_D3))
		{
			SysUnmapRegisters();

			//Save some pci state that won't get saved properly by pci_save_state()
			pci_read_config_dword(psPVRPCI->psPCIDev, 0x5C, &gsSysSpecificData.saveBSM);
			pci_read_config_dword(psPVRPCI->psPCIDev, 0xFC, &gsSysSpecificData.saveVBT);
			pci_read_config_dword(psPVRPCI->psPCIDev, MRST_PCIx_MSI_ADDR_LOC, &gsSysSpecificData.msi_addr);
			pci_read_config_dword(psPVRPCI->psPCIDev, MRST_PCIx_MSI_DATA_LOC, &gsSysSpecificData.msi_data);
		}
	}

	return eError;
}

PVRSRV_ERROR SysSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVR_PCI_DEV *psPVRPCI = (PVR_PCI_DEV *)(gsSysSpecificData.hSGXPCI);

	if (eNewPowerState != gpsSysData->eCurrentPowerState)
	{
		if ((gpsSysData->eCurrentPowerState == PVRSRV_SYS_POWER_STATE_D3) &&
			(eNewPowerState < PVRSRV_SYS_POWER_STATE_D3))
		{
			//Restore some pci state that will not have gotten restored properly by pci_restore_state()
			pci_write_config_dword(psPVRPCI->psPCIDev, 0x5c, gsSysSpecificData.saveBSM);
			pci_write_config_dword(psPVRPCI->psPCIDev, 0xFC, gsSysSpecificData.saveVBT);
			pci_write_config_dword(psPVRPCI->psPCIDev, MRST_PCIx_MSI_ADDR_LOC, gsSysSpecificData.msi_addr);
			pci_write_config_dword(psPVRPCI->psPCIDev, MRST_PCIx_MSI_DATA_LOC, gsSysSpecificData.msi_data);

			eError = SysLocateDevices(gpsSysData);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to locate devices"));
				return eError;
			}

			eError = SysMapInRegisters();
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to map in registers"));
				return eError;
			}
		}
	}
	return eError;
}


PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32			ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE	eNewPowerState,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF))
	{
		if (ui32DeviceIndex == gui32SGXDeviceID)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePrePowerState: Remove SGX power"));
			psb_irq_uninstall_islands(gpDrmDevice, OSPM_GRAPHICS_ISLAND);
			/* XXX don't power off SGX on Cedartrail, windows driver guy said don't
 			 * need that. -- zhenyu */
			/* ospm_power_island_down(OSPM_GRAPHICS_ISLAND); */
		}
		else if (ui32DeviceIndex == gui32MRSTMSVDXDeviceID)
		{
			/* XXX don't power off MSVDX on Cedartrail, windows driver guy said don't
 			 * need that. -- zhenyu */
			/* ospm_power_island_down(OSPM_VIDEO_DEC_ISLAND); */
#if 0
			ospm_power_using_hw_end(OSPM_VIDEO_DEC_ISLAND);
			ospm_power_using_hw_end(OSPM_DISPLAY_ISLAND);
#endif
		}
		else if (ui32DeviceIndex == gui32MRSTTOPAZDeviceID)
		{
			ospm_power_island_down(OSPM_VIDEO_ENC_ISLAND);
		}
#if 0
			ospm_power_using_hw_end(OSPM_VIDEO_ENC_ISLAND);
			ospm_power_using_hw_end(OSPM_DISPLAY_ISLAND);
#endif
	}

	return PVRSRV_OK;
}


PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32			ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE	eNewPowerState,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF))
	{
		if (ui32DeviceIndex == gui32SGXDeviceID)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePrePowerState: Restore SGX power"));
			/* XXX we don't power off SGX on Cedartrail */
			/* ospm_power_island_up(OSPM_GRAPHICS_ISLAND); */
		}
		else if (ui32DeviceIndex == gui32MRSTMSVDXDeviceID)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePrePowerState: Restore MSVDX power"));
			/* XXX we don't power off MSVDX on Cedartrail */
			/* ospm_power_island_up(OSPM_VIDEO_DEC_ISLAND); */
#if 0
			if (!ospm_power_using_hw_begin(OSPM_DISPLAY_ISLAND, true))
			{
				return PVRSRV_ERROR_GENERIC;
			}

			if (!ospm_power_using_hw_begin(OSPM_VIDEO_DEC_ISLAND, true))
			{
				ospm_power_using_hw_end(OSPM_DISPLAY_ISLAND);

				return PVRSRV_ERROR_GENERIC;
			}
#endif
		}
		else if (ui32DeviceIndex == gui32MRSTTOPAZDeviceID)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePrePowerState: Restore TOPAZ power"));

			ospm_power_island_up(OSPM_VIDEO_ENC_ISLAND);
#if 0
			if (!ospm_power_using_hw_begin(OSPM_DISPLAY_ISLAND, true))
			{
				return PVRSRV_ERROR_GENERIC;
			}

			if (!ospm_power_using_hw_begin(OSPM_VIDEO_ENC_ISLAND, true))
			{
				ospm_power_using_hw_end(OSPM_DISPLAY_ISLAND);

				return PVRSRV_ERROR_GENERIC;
			}
#endif
		}
	}

	return PVRSRV_OK;
}

int SYSPVRServiceSGXInterrupt(struct drm_device *dev)
{
	IMG_BOOL bStatus = IMG_FALSE;

	PVR_UNREFERENCED_PARAMETER(dev);

	if (gpsSGXDevNode != IMG_NULL)
	{
		bStatus = (*gpsSGXDevNode->pfnDeviceISR)(gpsSGXDevNode->pvISRData);
		if (bStatus)
		{
			OSScheduleMISR((IMG_VOID *)gpsSGXDevNode->psSysData);
		}
	}

	return bStatus ? 1 : 0;
}
