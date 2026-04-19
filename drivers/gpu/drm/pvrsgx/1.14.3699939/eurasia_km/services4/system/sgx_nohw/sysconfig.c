/*************************************************************************/ /*!
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Configuration functions
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

#include "sgxdefs.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"


/* Defines for HW Recovery */
#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100) // 10ms (100hz)
#define SYS_SGX_PDS_TIMER_FREQ				(1000) // 1ms (1000hz)

static IMG_BOOL bInstalledMISR;

/* top level system data anchor point*/
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
SYS_DATA  gsSysData;

/* SGX structures */
static IMG_UINT32		gui32SGXDeviceID;
static SGX_DEVICE_MAP	gsSGXDeviceMap;

/* mimic register block with contiguous memory */
IMG_CPU_VIRTADDR gsSGXRegsCPUVAddr;

IMG_UINT32   PVRSRV_BridgeDispatchKM( IMG_UINT32  Ioctl,
									IMG_BYTE   *pInBuf,
									IMG_UINT32  InBufLen,
									IMG_BYTE   *pOutBuf,
									IMG_UINT32  OutBufLen,
									IMG_UINT32 *pdwBytesTransferred);

/*!
******************************************************************************

 @Function	SysLocateDevices

 @Description specifies devices in the systems memory map

 @Input    psSysData - sys data

 @Return   PVRSRV_ERROR  :

******************************************************************************/
static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError;
	IMG_CPU_PHYADDR sCpuPAddr;

	PVR_UNREFERENCED_PARAMETER(psSysData);

	/* SGX Device: */
	gsSGXDeviceMap.ui32Flags = 0x0;

	/*
		in the case of NoHW we allocate some contiguous
		memory for the register block
	*/

	/* Registers */
	eError = OSBaseAllocContigMemory(SGX_REG_SIZE,
										&gsSGXRegsCPUVAddr,
										&sCpuPAddr);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}
	gsSGXDeviceMap.sRegsCpuPBase = sCpuPAddr;
	gsSGXDeviceMap.sRegsSysPBase = SysCpuPAddrToSysPAddr(gsSGXDeviceMap.sRegsCpuPBase);;
	gsSGXDeviceMap.ui32RegsSize = SGX_REG_SIZE;
	OSMemSet(gsSGXRegsCPUVAddr, 0, SGX_REG_SIZE);

#if defined(__linux__)
	/* Indicate the registers are already mapped */
	gsSGXDeviceMap.pvRegsCpuVBase = gsSGXRegsCPUVAddr;
#else
	/*
	 * FIXME: Could we just use the virtual address returned by
	 * OSBaseAllocContigMemory?
	 */
	gsSGXDeviceMap.pvRegsCpuVBase = IMG_NULL;
#endif

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

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	/*
		external system cache details
		- TO BE SPECIFIED IN SYSTEM PORT!
	*/
	gsSGXDeviceMap.sExtSysCacheRegsDevPBase.uiAddr = SYS_EXT_SYS_CACHE_GBL_INV_REG_OFFSET;
	gsSGXDeviceMap.ui32ExtSysCacheRegsSize = SGX_EXT_SYSTEM_CACHE_REGS_SIZE;
#endif /* SUPPORT_EXTERNAL_SYSTEM_CACHE */

	/*
		device interrupt IRQ
		Note: no interrupts available on No HW system
	*/
	gsSGXDeviceMap.ui32IRQ = 0;

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

	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to setup env structure"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/* Set up timing information*/
	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
	psTimingInfo->bEnableActivePM = IMG_FALSE;
	psTimingInfo->ui32ActivePowManLatencyms = 0;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;

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
		(IMG_VOID)SysDeinitialise(gpsSysData);
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
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		Register devices with the system
		This also sets up their memory maps/heaps
	*/
	eError = PVRSRVRegisterDevice(gpsSysData, &SGXRegisterDevice, 0, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
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
					/* Registry not supported in kernel */
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
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

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

	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"OSInstallMISR: Failed to install MISR"));
		return eError;
	}
	bInstalledMISR = IMG_TRUE;

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
    PVRSRV_ERROR eError;

    if (psSysData == IMG_NULL)
    {
        PVR_DPF((PVR_DBG_ERROR, "SysDeinitialise: Called with NULL SYS_DATA pointer.  Probably called before."));
        return PVRSRV_OK;
    }

	if (bInstalledMISR)
	{
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
		bInstalledMISR = IMG_FALSE;
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

	/* Destroy the local memory arena. */
	if (gpsSysData->apsLocalDevMemArena[0])
	{
		RA_Delete(gpsSysData->apsLocalDevMemArena[0]);
		gpsSysData->apsLocalDevMemArena[0] = IMG_NULL;
	}

	SysDeinitialiseCommon(gpsSysData);

	/* Free hardware resources. */
	OSBaseFreeContigMemory(SGX_REG_SIZE, gsSGXRegsCPUVAddr, gsSGXDeviceMap.sRegsCpuPBase);

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


/*!
******************************************************************************
 @Function        SysCpuPAddrToDevPAddr

 @Description     Compute a device physical address from a cpu physical
	            address. Relevant when

 @Input           cpu_paddr - cpu physical address.
 @Input           eDeviceType - device type required if DevPAddr
		address spaces vary across devices
		in the same system
 @Return          device physical address.
******************************************************************************/
IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
										IMG_CPU_PHYADDR CpuPAddr)
{
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for no HW UMA system we assume DevP == CpuP */
	DevPAddr.uiAddr = CpuPAddr.uiAddr;

	return DevPAddr;
}

/*!
******************************************************************************
 @Function        SysSysPAddrToCpuPAddr

 @Description     Compute a cpu physical address from a system physical
	            address.

 @Input           sys_paddr - system physical address.
 @Return          cpu physical address.
******************************************************************************/
IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr (IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0,
	   ie. multi CPU system */
	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

/*!
******************************************************************************
 @Function        SysCpuPAddrToSysPAddr

 @Description     Compute a system physical address from a cpu physical
	            address.

 @Input           cpu_paddr - cpu physical address.
 @Return        device physical address.
******************************************************************************/
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0,
	   ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}


/*!
******************************************************************************
 @Function        SysSysPAddrToDevPAddr

 @Description     Compute a device physical address from a system physical
	            address.

 @Input           SysPAddr - system physical address.
 @Input           eDeviceType - device type required if DevPAddr
			address spaces vary across devices
			in the same system

 @Return        Device physical address.
******************************************************************************/
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for no HW UMA system we assume DevP == CpuP */
	DevPAddr.uiAddr = SysPAddr.uiAddr;

	return DevPAddr;
}


/*!
******************************************************************************
 @Function        SysDevPAddrToSysPAddr

 @Description     Compute a device physical address from a system physical
	            address.

 @Input           DevPAddr - device physical address.
 @Input           eDeviceType - device type required if DevPAddr
			address spaces vary across devices
			in the same system

 @Return        System physical address.
******************************************************************************/
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR DevPAddr)
{
	IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for no HW UMA system we assume DevP == SysP */
	SysPAddr.uiAddr = DevPAddr.uiAddr;

	return SysPAddr;
}


/*****************************************************************************
 @Function        SysRegisterExternalDevice

 @Description     Called when a 3rd party device registers with services

 @Input           psDeviceNode - the new device node.

 @Return        IMG_VOID
*****************************************************************************/
IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}


/*****************************************************************************
 @Function        SysRemoveExternalDevice

 @Description     Called when a 3rd party device unregisters from services

 @Input           psDeviceNode - the device node being removed.

 @Return        IMG_VOID
*****************************************************************************/
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}


/*!
******************************************************************************
 @Function        SysGetInterruptSource

 @Description     Returns System specific information about the device(s) that
				generated the interrupt in the system

 @Input           psSysData
 @Input           psDeviceNode

 @Return        System specific information indicating which device(s)
				generated the interrupt
******************************************************************************/
IMG_UINT32 SysGetInterruptSource(SYS_DATA* psSysData,
								 PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	/* no interrupts in no_hw system just return all bits */
	return 0xFFFFFFFF;
}


/*!
******************************************************************************
 @Function        SysGetInterruptSource

 @Description     Clears specified system interrupts

 @Input           psSysData
 @Input           ui32ClearBits

 @Return        IMG_VOID
******************************************************************************/
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
PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32				ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE	eNewPowerState,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(ui32DeviceIndex);
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

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
PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32				ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE	eNewPowerState,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(ui32DeviceIndex);
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	return PVRSRV_OK;
}


/*****************************************************************************
 @Function        SysOEMFunction

 @Description     marshalling function for custom OEM functions

 @Input           ui32ID  - function ID
 @Input           pvIn - in data
 @Output          pvOut - out data

 @Return        PVRSRV_ERROR
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
#if  defined (__QNXNTO__)
		psOEMJTable->pfnOEMBridgeDispatch = IMG_NULL;
#else
		psOEMJTable->pfnOEMBridgeDispatch = &PVRSRV_BridgeDispatchKM;
#endif

		return PVRSRV_OK;
	}

	return PVRSRV_ERROR_INVALID_PARAMS;
}
/******************************************************************************
 End of file (sysconfig.c)
******************************************************************************/
