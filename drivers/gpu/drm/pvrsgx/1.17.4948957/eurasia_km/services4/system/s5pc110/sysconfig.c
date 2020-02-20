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

#include "sgxdefs.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/err.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/pm_qos.h>

#define REAL_HARDWARE 1

#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100) // 10ms (100hz)
#define SYS_SGX_PDS_TIMER_FREQ				(1000) // 1ms (1000hz)
#ifndef SYS_SGX_ACTIVE_POWER_LATENCY_MS
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(500)
#endif

typedef struct _SYS_SPECIFIC_DATA_TAG_
{
	IMG_UINT32 ui32SysSpecificData;

} SYS_SPECIFIC_DATA;

#define SYS_SPECIFIC_DATA_ENABLE_IRQ		0x00000001UL
#define SYS_SPECIFIC_DATA_ENABLE_LISR		0x00000002UL
#define SYS_SPECIFIC_DATA_ENABLE_MISR		0x00000004UL

SYS_SPECIFIC_DATA gsSysSpecificData;

/* top level system data anchor point*/
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
SYS_DATA  gsSysData;

/* SGX structures */
static IMG_UINT32		gui32SGXDeviceID;
static SGX_DEVICE_MAP	gsSGXDeviceMap;

/* mimic slaveport and register block with contiguous memory */
IMG_CPU_VIRTADDR gsSGXRegsCPUVAddr;
IMG_CPU_VIRTADDR gsSGXSPCPUVAddr;

static char gszVersionString[] = "SGX540 S5PC110";

IMG_UINT32   PVRSRV_BridgeDispatchKM( IMG_UINT32  Ioctl,
									IMG_BYTE   *pInBuf,
									IMG_UINT32  InBufLen, 
									IMG_BYTE   *pOutBuf,
									IMG_UINT32  OutBufLen,
									IMG_UINT32 *pdwBytesTransferred);

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
/*
 * We need to keep the memory bus speed up when the GPU is active.
 * On the  S5PV210, it is bound to the CPU freq.
 * The bus speed is only lowered when the
 * CPU freq is below 200MHz.
 */
#define MIN_CPU_KHZ_FREQ 100000
#define MIN_G3D_CPU_KHZ_FREQ 200000

static struct clk *g3d_clock;
static bool running;

struct dev_pm_qos_request qos_req;

static PVRSRV_ERROR EnableSGXClocks(void)
{
	running = true;
	clk_prepare_enable(g3d_clock);

	dev_pm_qos_update_request(&qos_req, MIN_G3D_CPU_KHZ_FREQ);

	return PVRSRV_OK;
}

static PVRSRV_ERROR DisableSGXClocks(void)
{
	clk_disable_unprepare(g3d_clock);
	running = false;

	dev_pm_qos_update_request(&qos_req, MIN_CPU_KHZ_FREQ);

	return PVRSRV_OK;
}

#endif /* defined(SUPPORT_ACTIVE_POWER_MANAGEMENT) */

/*!
******************************************************************************

 @Function	SysLocateDevices

 @Description specifies devices in the systems memory map

 @Input    psSysData - sys data

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
	extern struct platform_device *gpsPVRLDMDev;
	struct resource res;
	int ret;

	PVR_UNREFERENCED_PARAMETER(psSysData);

	ret = of_address_to_resource(gpsPVRLDMDev->dev.of_node, 0, &res);
	if (ret)
		return PVRSRV_ERROR_INIT_FAILURE;

	gsSGXDeviceMap.sRegsSysPBase.uiAddr = res.start;
	gsSGXDeviceMap.sRegsCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = resource_size(&res);

#if defined(SGX_FEATURE_HOST_PORT)
	/* HostPort: */
	gsSGXDeviceMap.sHPSysPBase.uiAddr = 0;
	gsSGXDeviceMap.sHPCpuPBase.uiAddr = 0;
	gsSGXDeviceMap.ui32HPSize = 0;
#endif

	/* 
		Local Device Memory Region: (not present)
		Note: the device doesn't need to know about its memory 
		but keep info here for now
	*/
	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemDevPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr = 0;
	gsSGXDeviceMap.ui32LocalMemSize = 0;

	/* 
		device interrupt IRQ
		Note: no interrupts available on No HW system
	*/
	gsSGXDeviceMap.ui32IRQ = of_irq_get(gpsPVRLDMDev->dev.of_node, 0);

#if defined(PDUMP)
	{
		/* initialise memory region name for pdumping */
		static IMG_CHAR pszPDumpDevName[] = "SGXMEM";
		gsSGXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif

	/* add other devices here: */

	return PVRSRV_OK;
}



/*!
******************************************************************************

 @Function	SysInitialise
 
 @Description Initialises kernel services at 'driver load' time
 
 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR SysInitialise(IMG_VOID)
{
	IMG_UINT32			i;
	PVRSRV_ERROR 		eError;
	PVRSRV_DEVICE_NODE	*psDeviceNode;
	SGX_TIMING_INFORMATION*	psTimingInfo;

	gpsSysData = &gsSysData;
	OSMemSet(gpsSysData, 0, sizeof(SYS_DATA));

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	{
		extern struct platform_device *gpsPVRLDMDev;
		struct device *dev;
		int ret;

		g3d_clock = devm_clk_get(&gpsPVRLDMDev->dev, "sclk");
		if (IS_ERR(g3d_clock))
		{
			PVR_DPF((PVR_DBG_ERROR, "G3D failed to find g3d clock source-enable"));
			return PVRSRV_ERROR_INIT_FAILURE;
		}

		dev = get_cpu_device(0);
		if (unlikely(!dev)) {
			pr_warn("No cpu device!\n");
			return PVRSRV_ERROR_INIT_FAILURE;
		}

		ret = dev_pm_qos_add_request(dev, &qos_req,
					     DEV_PM_QOS_MIN_FREQUENCY,
					     MIN_CPU_KHZ_FREQ);
		if (ret < 0) {
			pr_err("%s: Failed to add freq constraint (%d)\n", __func__,
			       ret);
			return PVRSRV_ERROR_INIT_FAILURE;
		}

		EnableSGXClocks();
	}
#endif

	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to setup env structure"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	gpsSysData->pvSysSpecificData = (IMG_PVOID)&gsSysSpecificData;
	OSMemSet(&gsSGXDeviceMap, 0, sizeof(SGX_DEVICE_MAP));
	
	/* Set up timing information*/
	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ; 
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS; 
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ; 

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else  
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif

	gpsSysData->ui32NumDevices = SYS_DEVICE_COUNT;

	/* init device ID's */
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

	/*
		Locate the devices within the system, specifying 
		the physical addresses of each devices components 
		(regs, mem, ports etc.)
	*/
	eError = SysLocateDevices(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to locate devices"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/* 
		Register devices with the system
		This also sets up their memory maps/heaps
	*/
	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice, 1, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		Once all devices are registered, specify the backing store
		and, if required, customise the memory heap config
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
				IMG_UINT32 ui32MemConfig;

				if(gpsSysData->apsLocalDevMemArena[0] != IMG_NULL)
				{
					/* specify the backing store to use for the device's MMU PT/PDs */
					psDeviceNode->psLocalDevMemArena = gpsSysData->apsLocalDevMemArena[0];
					ui32MemConfig = PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG;
				}
				else
				{
					/*
						specify the backing store to use for the devices MMU PT/PDs
						- the PT/PDs are always UMA in this system
					*/
					psDeviceNode->psLocalDevMemArena = IMG_NULL;
					ui32MemConfig = PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;
				}

				/* useful pointers */
				psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
				psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

				/* specify the backing store for all SGX heaps */
				for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
				{
#if defined(SGX_FEATURE_VARIABLE_MMU_PAGE_SIZE)
					IMG_CHAR *pStr;
								
					switch(psDeviceMemoryHeap[i].ui32HeapID)
					{
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_GENERAL_HEAP_ID):
						{
							pStr = "GeneralHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_GENERAL_MAPPING_HEAP_ID):
						{
							pStr = "GeneralMappingHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_TADATA_HEAP_ID):
						{
							pStr = "TADataHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_KERNEL_CODE_HEAP_ID):
						{
							pStr = "KernelCodeHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_KERNEL_DATA_HEAP_ID):
						{
							pStr = "KernelDataHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_PIXELSHADER_HEAP_ID):
						{
							pStr = "PixelShaderHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_VERTEXSHADER_HEAP_ID):
						{
							pStr = "VertexShaderHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_PDSPIXEL_CODEDATA_HEAP_ID):
						{
							pStr = "PDSPixelHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_PDSVERTEX_CODEDATA_HEAP_ID):
						{
							pStr = "PDSVertexHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_SYNCINFO_HEAP_ID):
						{
							pStr = "SyncInfoHeapPageSize";
							break;
						}
						case HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_3DPARAMETERS_HEAP_ID):
						{
							pStr = "3DParametersHeapPageSize";
							break;
						}
						default:
						{
							/* not interested in other heaps */
							pStr = IMG_NULL;
							break;	
						}
					}
					if (pStr 
					&&	OSReadRegistryDWORDFromString(0,
														PVRSRV_REGISTRY_ROOT,
														pStr,
														&psDeviceMemoryHeap[i].ui32DataPageSize) == IMG_TRUE)
					{
						PVR_DPF((PVR_DBG_VERBOSE,"SysInitialise: set Heap %s page size to %d", pStr, psDeviceMemoryHeap[i].ui32DataPageSize));
					}
#endif
					/*
						map the device memory allocator(s) onto
						the device memory heaps as required
					*/
					psDeviceMemoryHeap[i].psLocalDevMemArena = gpsSysData->apsLocalDevMemArena[0];

					/* set the memory config (uma | non-uma) */
					psDeviceMemoryHeap[i].ui32Attribs |= ui32MemConfig;
				}

				break;
			}
			default:
				PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to find SGX device node!"));
				return PVRSRV_ERROR_INIT_FAILURE;
		}

		/* advance to next device */
		psDeviceNode = psDeviceNode->psNext;
	}

	/*
		Initialise all devices 'managed' by services:
	*/
	eError = PVRSRVInitialiseDevice (gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	DisableSGXClocks();
#endif

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysFinalise
 
 @Description Final part of initialisation
 

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR SysFinalise(IMG_VOID)
{
	PVRSRV_ERROR eError;    

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	eError = EnableSGXClocks();
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to Enable SGX clocks (%d)", eError));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
#endif

	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"OSInstallMISR: Failed to install MISR"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	gsSysSpecificData.ui32SysSpecificData |= SYS_SPECIFIC_DATA_ENABLE_MISR;

#if defined(SYS_USING_INTERRUPTS)
	/* install a system ISR */
	eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"OSInstallSystemLISR: Failed to install ISR"));
		OSUninstallMISR(gpsSysData);
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	gsSysSpecificData.ui32SysSpecificData |= SYS_SPECIFIC_DATA_ENABLE_LISR;
	gsSysSpecificData.ui32SysSpecificData |= SYS_SPECIFIC_DATA_ENABLE_IRQ;
#endif /* defined(SYS_USING_INTERRUPTS) */

	/* Create a human readable version string for this system */
	gpsSysData->pszVersionString = gszVersionString;

	if (!gpsSysData->pszVersionString)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: Failed to create a system version string"));
    }
	else
	{
		PVR_DPF((PVR_DBG_WARNING, "SysFinalise: Version string: %s", gpsSysData->pszVersionString));
	}

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	DisableSGXClocks();
#endif 

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDeinitialise

 @Description De-initialises kernel services at 'driver unload' time

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
PVRSRV_ERROR SysDeinitialise (SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA * psSysSpecData;
	PVRSRV_ERROR eError;

	if (psSysData == IMG_NULL) {
		PVR_DPF((PVR_DBG_ERROR, "SysDeinitialise: Called with NULL SYS_DATA pointer.  Probably called before."));
		return PVRSRV_OK;
	}

	psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	/* TODO: regulator and clk put. */
	dev_pm_qos_remove_request(&qos_req);
#endif

#if defined(SYS_USING_INTERRUPTS)
	if (psSysSpecData->ui32SysSpecificData & SYS_SPECIFIC_DATA_ENABLE_LISR)
	{	
		eError = OSUninstallSystemLISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallSystemLISR failed"));
			return eError;
		}
	}
#endif

	if (psSysSpecData->ui32SysSpecificData & SYS_SPECIFIC_DATA_ENABLE_MISR)
	{
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
	}

	/* de-initialise all services managed devices */
	eError = PVRSRVDeinitialiseDevice (gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
		return eError;
	}

	eError = OSDeInitEnvData(gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init env structure"));
		return eError;
	}

	SysDeinitialiseCommon(gpsSysData);

	gpsSysData = IMG_NULL;

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function		SysGetDeviceMemoryMap

 @Description	returns a device address map for the specified device

 @Input			eDeviceType - device type
 @Input			ppvDeviceMap - void ptr to receive device specific info.

 @Return   		PVRSRV_ERROR

******************************************************************************/
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
		default:
		{
			PVR_DPF((PVR_DBG_ERROR,"SysGetDeviceMemoryMap: unsupported device type"));
		}
	}
	return PVRSRV_OK;
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysCpuPAddrToDevPAddr

	PURPOSE:    Compute a device physical address from a cpu physical
	            address. Relevant when 

	PARAMETERS:	In:  cpu_paddr - cpu physical address.
				In:  eDeviceType - device type required if DevPAddr 
									address spaces vary across devices 
									in the same system
	RETURNS:	device physical address.

</function>
------------------------------------------------------------------------------*/
IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, 
										IMG_CPU_PHYADDR CpuPAddr)
{
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for no HW UMA system we assume DevP == CpuP */
	DevPAddr.uiAddr = CpuPAddr.uiAddr;
	
	return DevPAddr;
}

/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysSysPAddrToCpuPAddr

	PURPOSE:    Compute a cpu physical address from a system physical
	            address.

	PARAMETERS:	In:  sys_paddr - system physical address.
	RETURNS:	cpu physical address.

</function>
------------------------------------------------------------------------------*/
IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr (IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0, 
	   ie. multi CPU system */
	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysCpuPAddrToSysPAddr

	PURPOSE:    Compute a system physical address from a cpu physical
	            address.

	PARAMETERS:	In:  cpu_paddr - cpu physical address.
	RETURNS:	device physical address.

</function>
------------------------------------------------------------------------------*/
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0, 
	   ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysSysPAddrToDevPAddr

	PURPOSE:    Compute a device physical address from a system physical
	            address.

	PARAMETERS: In:  SysPAddr - system physical address.
				In:  eDeviceType - device type required if DevPAddr 
									address spaces vary across devices 
									in the same system

	RETURNS:    Device physical address.

</function>
-----------------------------------------------------------------------------*/
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for no HW UMA system we assume DevP == CpuP */
	DevPAddr.uiAddr = SysPAddr.uiAddr;

	return DevPAddr;
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysDevPAddrToSysPAddr

	PURPOSE:    Compute a device physical address from a system physical
	            address.

	PARAMETERS: In:  DevPAddr - device physical address.
				In:  eDeviceType - device type required if DevPAddr 
									address spaces vary across devices 
									in the same system

	RETURNS:    System physical address.

</function>
-----------------------------------------------------------------------------*/
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR DevPAddr)
{
	IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for no HW UMA system we assume DevP == SysP */
	SysPAddr.uiAddr = DevPAddr.uiAddr;

	return SysPAddr;
}


/*****************************************************************************
 FUNCTION	: SysRegisterExternalDevice

 PURPOSE	: Called when a 3rd party device registers with services

 PARAMETERS: In:  psDeviceNode - the new device node.

 RETURNS	: IMG_VOID
*****************************************************************************/
IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}


/*****************************************************************************
 FUNCTION	: SysRemoveExternalDevice

 PURPOSE	: Called when a 3rd party device unregisters from services

 PARAMETERS: In:  psDeviceNode - the device node being removed.

 RETURNS	: IMG_VOID
*****************************************************************************/
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:   SysGetInterruptSource

	PURPOSE:    Returns System specific information about the device(s) that 
				generated the interrupt in the system

	PARAMETERS: In:  psSysData
				In:  psDeviceNode

	RETURNS:    System specific information indicating which device(s) 
				generated the interrupt

</function>
-----------------------------------------------------------------------------*/
IMG_UINT32 SysGetInterruptSource(SYS_DATA* psSysData,
								 PVRSRV_DEVICE_NODE *psDeviceNode)
{	
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

#if defined(NO_HARDWARE)
	/* no interrupts in no_hw system just return all bits */
	return 0xFFFFFFFF;
#else
	return 0x1;
#endif
}


/*----------------------------------------------------------------------------
<function>
	FUNCTION:	SysGetInterruptSource

	PURPOSE:	Clears specified system interrupts

	PARAMETERS: psSysData
				ui32ClearBits

	RETURNS:	IMG_VOID

</function>
-----------------------------------------------------------------------------*/
IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32ClearBits)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32ClearBits);

	/* no interrupts in no_hw system, nothing to do */
}


/*!
******************************************************************************

 @Function	SysSystemPrePowerState

 @Description	Perform system-level processing required before a power transition

 @Input	   eNewPowerState : 

 @Return   PVRSRV_ERROR : 

******************************************************************************/
PVRSRV_ERROR SysSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	SysSystemPostPowerState

 @Description	Perform system-level processing required after a power transition

 @Input	   eNewPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR SysSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDevicePrePowerState

 @Description	Perform system-level processing required before a device power
 				transition

 @Input		ui32DeviceIndex :
 @Input		eNewPowerState :
 @Input		eCurrentPowerState :

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32			ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE		eNewPowerState,
									PVRSRV_DEV_POWER_STATE		eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	if (ui32DeviceIndex != gui32SGXDeviceID)
	{
		return PVRSRV_OK;
	}
 
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF)
	{
		PVRSRVSetDCState(DC_STATE_FLUSH_COMMANDS);
		PVR_DPF((PVR_DBG_MESSAGE, "SysDevicePrePowerState: SGX Entering state D3"));
		DisableSGXClocks();
		PVRSRVSetDCState(DC_STATE_NO_FLUSH_COMMANDS);
	}
#else
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
#endif

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDevicePostPowerState

 @Description	Perform system-level processing required after a device power
 				transition

 @Input		ui32DeviceIndex :
 @Input		eNewPowerState :
 @Input		eCurrentPowerState :

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32			ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE	eNewPowerState,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	if (ui32DeviceIndex != gui32SGXDeviceID)
	{
		return eError;
	}

#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_ON)
	{
		PVR_DPF((PVR_DBG_MESSAGE, "SysDevicePostPowerState: SGX Leaving state D3"));
		eError = EnableSGXClocks();
	}
#else
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
#endif   

	return eError;
}


/*****************************************************************************
 FUNCTION	: SysOEMFunction

 PURPOSE	: marshalling function for custom OEM functions

 PARAMETERS	: ui32ID  - function ID
			  pvIn - in data
			  pvOut - out data

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR SysOEMFunction(IMG_UINT32	ui32ID, 
							IMG_VOID	*pvIn,
							IMG_UINT32	ulInSize,
							IMG_VOID	*pvOut,
							IMG_UINT32	ulOutSize)
{
	PVR_UNREFERENCED_PARAMETER(ulInSize);
	PVR_UNREFERENCED_PARAMETER(pvIn);

	if ((ui32ID == OEM_GET_EXT_FUNCS) &&
		(ulOutSize == sizeof(PVRSRV_DC_OEM_JTABLE)))
	{
		PVRSRV_DC_OEM_JTABLE *psOEMJTable = (PVRSRV_DC_OEM_JTABLE*)pvOut;
		psOEMJTable->pfnOEMBridgeDispatch = &PVRSRV_BridgeDispatchKM;

		return PVRSRV_OK;
	}

	return PVRSRV_ERROR_INVALID_PARAMS;
}

PVRSRV_ERROR SysPowerLockWrap(IMG_BOOL bTryLock)
{
	/* FIXME: This should not be empty */
	PVR_UNREFERENCED_PARAMETER(bTryLock);
	return PVRSRV_OK;
}

IMG_VOID SysPowerLockUnwrap(IMG_VOID)
{
	/* FIXME: This should not be empty */
}

IMG_VOID SysSGXIdleEntered(IMG_VOID)
{
	/* FIXME: This should not be empty */
}

IMG_VOID SysSGXCommandPending(IMG_BOOL bSGXIdle)
{
	/* FIXME: This should not be empty */
}

/******************************************************************************
 End of file (sysconfig.c)
******************************************************************************/
