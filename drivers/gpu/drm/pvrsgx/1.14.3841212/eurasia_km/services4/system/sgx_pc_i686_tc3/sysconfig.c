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

#include "sgxdefs.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#include "tcfdefs.h"

#ifdef __linux__
#include "mm.h"
#endif

#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
#include "linux/pci.h"
#endif




/* Defines for HW Recovery */
#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100) // 10ms (100hz)
#define SYS_SGX_PDS_TIMER_FREQ				(1000) // 1ms (1000hz)
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(1)

/* top level system data anchor point*/
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
SYS_DATA  gsSysData;

static SYS_SPECIFIC_DATA gsSysSpecificData;



/* SGX structures */
static IMG_UINT32		gui32SGXDeviceID;
static SGX_DEVICE_MAP	gsSGXDeviceMap;
static IMG_CPU_PHYADDR  gsSOCRegsCpuPBase;

#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
/* The following is exported by the Linux module code */
extern struct pci_dev *gpsPVRLDMDev;
#endif

#if defined(NO_HARDWARE)
/* mimic register block with contiguous memory */
static IMG_CPU_VIRTADDR gsSGXRegsCPUVAddr;
#endif

IMG_UINT32 PVRSRV_BridgeDispatchKM( IMG_UINT32  Ioctl,
									IMG_BYTE   *pInBuf,
									IMG_UINT32  InBufLen,
									IMG_BYTE   *pOutBuf,
									IMG_UINT32  OutBufLen,
									IMG_UINT32 *pdwBytesTransferred);

#ifdef __linux__

/*!
******************************************************************************

 @Function		PCIInitDev

 @Description

 Initialise the PCI device for subsequent use.

 @Input		psSysData :	System data

 @Return	PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR PCIInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
	psSysSpecData->hSGXPCI = OSPCISetDev((IMG_VOID *)psSysSpecData->psPCIDev, HOST_PCI_INIT_FLAG_BUS_MASTER);
#else
	/* Old Test chip ID and Emulator board */
	psSysSpecData->hSGXPCI = OSPCIAcquireDev(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV_DEVICE_ID, HOST_PCI_INIT_FLAG_BUS_MASTER);
#if defined (SYS_SGX_DEV1_DEVICE_ID)
	if (!psSysSpecData->hSGXPCI)
	{
		PVR_TRACE(("PCIInitDev: Trying alternative PCI device ID"));
		psSysSpecData->hSGXPCI = OSPCIAcquireDev(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV1_DEVICE_ID, HOST_PCI_INIT_FLAG_BUS_MASTER);
	}
#endif
#endif	/* defined(LDM_PCI) || defined(SUPPORT_DRI_DRM) */
	if (!psSysSpecData->hSGXPCI)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Failed to acquire PCI device"));
		return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
	}
	SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCI_DEV);

	PVR_TRACE(("Atlas Reg PCI memory region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, SYS_ATLAS_REG_PCI_BASENUM), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, SYS_ATLAS_REG_PCI_BASENUM)));

	/* Check the address range is large enough. */
	if (OSPCIAddrRangeLen(psSysSpecData->hSGXPCI, SYS_ATLAS_REG_PCI_BASENUM) < SYS_ATLAS_REG_REGION_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region isn't big enough (atlas reg)"));
		return PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
	}

	/* Reserve the address range */
	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, SYS_ATLAS_REG_PCI_BASENUM) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available (atlas reg)"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}
	SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCI_ATL);

	PVR_TRACE(("SGX Reg/SP PCI memory region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, SYS_SGX_REG_PCI_BASENUM), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, SYS_SGX_REG_PCI_BASENUM)));

	/* Check the address range is large enough. */
	if (OSPCIAddrRangeLen(psSysSpecData->hSGXPCI, SYS_SGX_REG_PCI_BASENUM) < SYS_SGX_REG_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region isn't big enough (SGX reg)"));
		return PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
	}

	/* Reserve the address range */
	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, SYS_SGX_REG_PCI_BASENUM) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available (SGX reg) "));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}
	SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCI_REG);
	PVR_TRACE(("SGX Mem PCI memory region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, SYS_SGX_MEM_PCI_BASENUM), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, SYS_SGX_MEM_PCI_BASENUM)));

	/* Check the address range is large enough. */
	if (OSPCIAddrRangeLen(psSysSpecData->hSGXPCI, SYS_SGX_MEM_PCI_BASENUM) < SYS_SGX_MEM_REGION_SIZE)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region isn't big enough"));
		return PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
	}

	/* Reserve the address range */
	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, SYS_SGX_MEM_PCI_BASENUM) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}

	SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCI_MEM);

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function		PCIDeInitDev

 @Description

 Uninitialise the PCI device when it is no loger required

 @Input		psSysData :	System data

 @Return	none

******************************************************************************/
static IMG_VOID PCIDeInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData;

	psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if(SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCI_MEM))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, SYS_SGX_MEM_PCI_BASENUM);
	}

	if(SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCI_REG))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, SYS_SGX_REG_PCI_BASENUM);
	}

	if(SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCI_ATL))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, SYS_ATLAS_REG_PCI_BASENUM);
	}

	if(SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCI_DEV))
	{
		OSPCIReleaseDev(psSysSpecData->hSGXPCI);
	}
}
#else	/* __linux__ */
/*!
******************************************************************************

 @Function		FindPCIDevice

 @Description	Scans the PCI bus for ui16DevID/ui16VenID and if found,
				enumerates the PCI config registers.

 @Input			ui16VenID
 @Input			ui16DevID
 @Output		psPCISpace

 @Return		PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR FindPCIDevice(IMG_UINT16 ui16VenID, IMG_UINT16 ui16DevID, PCICONFIG_SPACE *psPCISpace)
{
	IMG_UINT32 ui32BusNum;
	IMG_UINT32 ui32DevNum;
	IMG_UINT32 ui32VenDevID;

	/* Check all the busses */
	for (ui32BusNum=0; ui32BusNum < 255; ui32BusNum++)
	{
		/* Check all devices on that bus */
		for (ui32DevNum=0; ui32DevNum < 32; ui32DevNum++)
		{
			/* Get the deviceID and vendor ID */
			ui32VenDevID=OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 0);

			/* Check if it is the required device */
			if (ui32VenDevID == (IMG_UINT32)((ui16DevID<<16)+ui16VenID))
			{
				IMG_UINT32 ui32Idx;

				/* Ensure Access to FB memory is enabled */
				OSPCIWriteDword(ui32BusNum, ui32DevNum, 0, 4, OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 4) | 0x02);

				/* Now copy PCI config space to users buffer */
				for (ui32Idx=0; ui32Idx < 64; ui32Idx++)
				{
					psPCISpace->u.aui32PCISpace[ui32Idx] = OSPCIReadDword(ui32BusNum, ui32DevNum, 0, ui32Idx*4);

					if (ui32Idx < 16)
					{
						PVR_DPF((PVR_DBG_VERBOSE,"%08X\n",psPCISpace->u.aui32PCISpace[ui32Idx]));
					}
				}
				return PVRSRV_OK;
			}

		}/*End loop for each device*/

	}/*End loop for each bus*/

	return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
}

#endif	/* __linux__ */
/*!
******************************************************************************

 @Function	SysLocateDevices

 @Description specifies devices in the systems memory map

 @Input    psSysData - sys data

 @Return   PVRSRV_ERROR  :

******************************************************************************/
static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32AtlasRegBaseAddr, ui32SGXRegBaseAddr, ui32SGXMemBaseAddr;
	IMG_UINT32 ui32IRQ;
	IMG_UINT32 ui32DeviceID = SYS_SGX_DEV_DEVICE_ID;
#if defined(NO_HARDWARE)
	IMG_CPU_PHYADDR sCpuPAddr;
#endif
#ifdef __linux__
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
#endif
#if !defined(__linux__) || defined(NO_HARDWARE)
	PVRSRV_ERROR eError;
#endif

#ifdef	__linux__
	ui32AtlasRegBaseAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, SYS_ATLAS_REG_PCI_BASENUM);

	ui32SGXRegBaseAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, SYS_SGX_REG_PCI_BASENUM);

	ui32SGXMemBaseAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, SYS_SGX_MEM_PCI_BASENUM);

	if (OSPCIIRQ(psSysSpecData->hSGXPCI, &ui32IRQ) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Couldn't get IRQ"));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	PVR_TRACE(("IRQ: %d", ui32IRQ));
#else	/* __linux__ */
	PCICONFIG_SPACE	sPCISpace;

	PVR_UNREFERENCED_PARAMETER(psSysData);

	eError = FindPCIDevice(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV_DEVICE_ID, &sPCISpace);

	if (eError == PVRSRV_OK)
	{
		/* Get the regions from base address register 0, 1, 2 */
		ui32AtlasRegBaseAddr = sPCISpace.u.aui32PCISpace[SYS_ATLAS_REG_PCI_OFFSET];
		ui32SGXRegBaseAddr = sPCISpace.u.aui32PCISpace[SYS_SGX_REG_PCI_OFFSET];
		ui32SGXMemBaseAddr = sPCISpace.u.aui32PCISpace[SYS_SGX_MEM_PCI_OFFSET];
	}
	else
	{
#if defined (SYS_SGX_DEV1_DEVICE_ID)
		ui32DeviceID = SYS_SGX_DEV1_DEVICE_ID;
		eError = FindPCIDevice(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV1_DEVICE_ID, &sPCISpace);
		
		if (eError == PVRSRV_OK)
		{
			/* Get the regions from base address register 0, 1, 2 */
			ui32AtlasRegBaseAddr = sPCISpace.u.aui32PCISpace[SYS_ATLAS_REG_PCI_OFFSET];
			ui32SGXRegBaseAddr = sPCISpace.u.aui32PCISpace[SYS_SGX_REG_PCI_OFFSET];
			ui32SGXMemBaseAddr = sPCISpace.u.aui32PCISpace[SYS_SGX_MEM_PCI_OFFSET];
		}
		else
#endif
		{
			PVR_DPF((PVR_DBG_ERROR,"Couldn't find PCI device"));
			return PVRSRV_ERROR_INVALID_DEVICE;
		}
	}

	PVR_DPF((PVR_DBG_MESSAGE,"Found - DevID %04X Ven Id 0x%04X SSDevId %04X SSVenId %04X",
			sPCISpace.u.aui16PCISpace[SYS_SGX_DEV_ID_16_PCI_OFFSET], 
			sPCISpace.u.aui16PCISpace[SYS_SGX_VEN_ID_16_PCI_OFFSET],
			sPCISpace.u.aui16PCISpace[SYS_SGX_SSDEV_ID_16_PCI_OFFSET], 
			sPCISpace.u.aui16PCISpace[SYS_SGX_SSVEN_ID_16_PCI_OFFSET]));

	ui32IRQ = (IMG_UINT32)sPCISpace.u.aui8PCISpace[SYS_SGX_IRQ_8_PCI_OFFSET];
#endif	/* __linux__ */

	/* Atlas registers */
	gsSOCRegsCpuPBase.uiAddr = ui32AtlasRegBaseAddr;

	/* SGX Device: */
	gsSGXDeviceMap.ui32Flags = 0x0;

#if defined(NO_HARDWARE)
	/* No hardware registers */
	eError = OSBaseAllocContigMemory(SYS_SGX_REG_SIZE,
										&gsSGXRegsCPUVAddr,
										&sCpuPAddr);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_REG_MEM);
	gsSGXDeviceMap.sRegsCpuPBase = sCpuPAddr;

	OSMemSet(gsSGXRegsCPUVAddr, 0, SYS_SGX_REG_SIZE);

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

#else	/* defined(NO_HARDWARE) */
	/* Hardware registers */
	gsSGXDeviceMap.sRegsSysPBase.uiAddr = ui32SGXRegBaseAddr + SYS_SGX_REG_OFFSET;
#endif	/* defined(NO_HARDWARE) */

	/* Common register setup */
	gsSGXDeviceMap.sRegsCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = SYS_SGX_REG_SIZE;

	/* Local Device Memory Region: (present) */
	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr = ui32SGXMemBaseAddr;
	gsSGXDeviceMap.sLocalMemDevPBase = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, gsSGXDeviceMap.sLocalMemSysPBase);
	gsSGXDeviceMap.sLocalMemCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sLocalMemSysPBase);
	
	#if defined (SYS_SGX_DEV1_DEVICE_ID)
	if(ui32DeviceID == SYS_SGX_DEV1_DEVICE_ID)
	{
		gsSGXDeviceMap.ui32LocalMemSize = SYS_SGX_DEV1_LOCALMEM_FOR_SGX_RESERVE_SIZE;
	}
	else
	#endif
	{
		gsSGXDeviceMap.ui32LocalMemSize = SYS_SGX_DEV_LOCALMEM_FOR_SGX_RESERVE_SIZE;
	}
	
#if defined(SGX_FEATURE_HOST_PORT)
	/* HostPort: */
	gsSGXDeviceMap.ui32Flags |= SGX_HOSTPORT_PRESENT;
	gsSGXDeviceMap.sHPSysPBase.uiAddr = ui32SGXMemBaseAddr + 0x10000000;
	gsSGXDeviceMap.sHPCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sHPSysPBase);
	gsSGXDeviceMap.ui32HPSize = SYS_SGX_HP_SIZE;
#endif

	/* device interrupt IRQ */
	gsSGXDeviceMap.ui32IRQ = ui32IRQ;

#if defined(PDUMP)
	{
		/* Initialise memory region name for pdumping.
		 * This could just as well be SGXMEM on the RefPCI Test Chip system
		 * since it uses LMA. But since there will only be 1 device we can
		 * represent it as UMA in the pdump.
		 */
		static IMG_CHAR pszPDumpDevName[] = "SGXMEM";
		gsSGXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif
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
	SGX_TIMING_INFORMATION* psTimingInfo;

	gpsSysData = &gsSysData;
	OSMemSet(gpsSysData, 0, sizeof(SYS_DATA));

	gpsSysData->pvSysSpecificData = &gsSysSpecificData;
	gsSysSpecificData.ui32SysSpecificData = 0;
#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
	/* Save the pci_dev structure pointer from module.c */
	PVR_ASSERT(gpsPVRLDMDev != IMG_NULL);
	gsSysSpecificData.psPCIDev = gpsPVRLDMDev;
#endif

	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to setup env structure"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_ENVDATA);

	/* Set up timing information*/
	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;

#ifdef __linux__
	eError = PCIInitDev(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_PCINIT);
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
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LOCATEDEV);

	/*
		Map the Atlas and PDP registers.
	*/
	gpsSysData->pvSOCRegsBase = OSMapPhysToLin(gsSOCRegsCpuPBase,
											   SYS_ATLAS_REG_REGION_SIZE,
	                                           PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
	                                           IMG_NULL);

	/*
		Do some initial register setup
	*/
	eError = SysInitRegisters();
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitRegisters: Failed to initialise registers"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_INITREG);

#if defined(READ_TCF_HOST_MEM_SIGNATURE)
	SysTestTCFHostMemSig(gsSGXDeviceMap.sRegsCpuPBase,
						 gsSGXDeviceMap.sLocalMemCpuPBase);
#endif

	/*
		Register devices with the system
		This also sets up their memory maps/heaps
	*/
	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice,
								  SysGetSGXInterruptBit(), &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_REGDEV);

	/*
		A given SOC may have 0 to n local device memory blocks.
		It is assumed device local memory blocks will be either
		specific to each device or shared among one or more services
		managed devices.
		Note: no provision is made for a single device having more
		than one local device memory blocks. If required we must
		add new types,
		e.g.PVRSRV_BACKINGSTORE_LOCALMEM_NONCONTIG0
			PVRSRV_BACKINGSTORE_LOCALMEM_NONCONTIG1
	*/

	/*
		Create Local Device Page Managers for each device memory block.
		We'll use an RA to do the management
		(only one for the emulator)
	*/
	gpsSysData->apsLocalDevMemArena[0] = RA_Create ("TestChipDeviceMemory",
													gsSGXDeviceMap.sLocalMemSysPBase.uiAddr,
													gsSGXDeviceMap.ui32LocalMemSize,
													IMG_NULL,
													HOST_PAGESIZE(),
													IMG_NULL,
													IMG_NULL,
													IMG_NULL,
													IMG_NULL);

	if (gpsSysData->apsLocalDevMemArena[0] == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to create local dev mem allocator!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_RA_ARENA);

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

				/* specify the backing store to use for the device's MMU PT/PDs */
				psDeviceNode->psLocalDevMemArena = gpsSysData->apsLocalDevMemArena[0];

				/* useful pointers */
				psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
				psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

				/* specify the backing store for all SGX heaps */
				for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
				{
					/*
						specify the backing store type
						(all local device mem noncontig) for test chip
					*/
					psDeviceMemoryHeap[i].ui32Attribs |= PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG;

					/*
						map the device memory allocator(s) onto
						the device memory heaps as required
					*/
					psDeviceMemoryHeap[i].psLocalDevMemArena = gpsSysData->apsLocalDevMemArena[0];

#ifdef OEM_CUSTOMISE
					/* if required, modify the memory config */
#endif
				}
				break;
			}
			default:
				break;
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
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_INITDEV);

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function	SysFinalise

 @Description Final part of system initialisation.

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
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_MISR);

#if defined(SYS_USING_INTERRUPTS)
	/* install a system ISR */
	eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"OSInstallSystemLISR: Failed to install ISR"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR);
#endif /* defined(SYS_USING_INTERRUPTS) */

#if  defined(SYS_USING_INTERRUPTS)
	SysEnableInterrupts(gpsSysData);
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_IRQ);
#endif

#if defined(__linux__)
	/* Create a human readable version string for this system */
	gpsSysData->pszVersionString = SysCreateVersionString(gsSGXDeviceMap.sRegsCpuPBase);
	if (!gpsSysData->pszVersionString)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: Failed to create a system version string"));
    }
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "SysFinalise: Version string: %s", gpsSysData->pszVersionString));
	}
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

	if (psSysData == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SysDeinitialise: Called with NULL SYS_DATA pointer.  Probably called before."));
		return PVRSRV_OK;
	}

	psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

#if  defined(SYS_USING_INTERRUPTS)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_IRQ))
	{
		SysDisableInterrupts(psSysData);
	}
#endif

#if defined(SYS_USING_INTERRUPTS)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_LISR))
	{
		eError = OSUninstallSystemLISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallSystemLISR failed"));
			return eError;
		}
	}
#endif /* defined(SYS_USING_INTERRUPTS) */

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_MISR))
	{
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_INITDEV))
	{
		/* de-initialise all services managed devices */
		eError = PVRSRVDeinitialiseDevice (gui32SGXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
	}
	if (gpsSysData->pvSOCRegsBase)
	{
		OSUnMapPhysToLin(gpsSysData->pvSOCRegsBase,
						 SYS_ATLAS_REG_SIZE + SYS_PDP_REG_SIZE,
                         PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
						 IMG_NULL);
		gpsSysData->pvSOCRegsBase = IMG_NULL;
	}

	/*
		Destroy the local memory arena.
	*/
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_RA_ARENA))
	{
		RA_Delete(gpsSysData->apsLocalDevMemArena[0]);
		gpsSysData->apsLocalDevMemArena[0] = IMG_NULL;
	}

	SysDeinitialiseCommon(gpsSysData);

#ifdef __linux__
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_PCINIT))
	{
		PCIDeInitDev(psSysData);
	}
#endif
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_ENVDATA))
	{
		eError = OSDeInitEnvData(gpsSysData->pvEnvSpecificData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init env structure"));
			return eError;
		}
	}


#if defined(NO_HARDWARE)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ENABLE_REG_MEM))
	{
		OSBaseFreeContigMemory(SYS_SGX_REG_SIZE, gsSGXRegsCPUVAddr, gsSGXDeviceMap.sRegsCpuPBase);
	}
#endif

	psSysSpecData->ui32SysSpecificData = 0;

	gpsSysData = IMG_NULL;


	return PVRSRV_OK;
}




/*!
******************************************************************************

 @Function      SysGetDeviceMemoryMap

 @Description	returns a device address map for the specified device

 @Input		eDeviceType - device type
 @Input		ppvDeviceMap - void ptr to receive device specific info.

 @Return        PVRSRV_ERROR

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

#if defined(PVR_LMA)
/*!
******************************************************************************
 @Function     SysVerifyCpuPAddrToDevPAddr

 @Description  Verify that it is valid to compute a device physical
		    address from a given cpu physical address.

 @Input        cpu_paddr - cpu physical address.
 @Input        eDeviceType - device type required if DevPAddr
	                     address spaces vary across devices in the same
			     system.

@Return        IMG_TRUE or IMG_FALSE

******************************************************************************/
IMG_BOOL SysVerifyCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
										IMG_CPU_PHYADDR CpuPAddr)
{
	/*
		It is assumed the translation from cpu/system to device
		physical addresses is constant for every 'services managed'
		device in a given system
	*/
	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in
		the adapter)
	*/
	return ((CpuPAddr.uiAddr >= gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr) &&
		(CpuPAddr.uiAddr < gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr + SYS_SGX_MEM_REGION_SIZE))
		 ? IMG_TRUE : IMG_FALSE;
}
#endif

/*!
******************************************************************************
 @Function     SysCpuPAddrToDevPAddr

 @Description  Compute a device physical address from a cpu physical
	            address.

 @Input        cpu_paddr - cpu physical address.
 @Input        eDeviceType - device type required if DevPAddr
	                     address spaces vary across devices in the same
			     system
 @Return       device physical address.

******************************************************************************/
IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
										IMG_CPU_PHYADDR CpuPAddr)
{
	/*
		It is assumed the translation from cpu/system to device
		physical addresses is constant for every 'services managed'
		device in a given system
	*/
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in
		the adapter)
	*/
	DevPAddr.uiAddr = CpuPAddr.uiAddr - gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr;

	return DevPAddr;
}

/*!
******************************************************************************
 @Function     SysSysPAddrToCpuPAddr

 @Description  Compute a cpu physical address from a system physical
	            address.

 @Input        sys_paddr - system physical address.
 @Return       cpu physical address.

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
 @Function     SysCpuPAddrToSysPAddr

 @Description  Compute a system physical address from a cpu physical
	            address.

 @Input        cpu_paddr - cpu physical address.
 @Return       device physical address.

******************************************************************************/
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0,
	   ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;

	return sys_paddr;
}


#if defined(PVR_LMA)
/*!
******************************************************************************
 @Function     SysVerifySysPAddrToDevPAddr

 @Description  Verify that a device physical address can be obtained
		    from a given system physical address.

 @Input        SysPAddr - system physical address.
 @Input        eDeviceType - device type required if DevPAddr
		         address spaces vary across devices in the same system.

 @Return       IMG_TRUE or IMG_FALSE
******************************************************************************/
IMG_BOOL SysVerifySysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
	/*
		It is assumed the translation from cpu/system to device
		physical addresses is constant for every 'services managed'
		device in a given system
	*/
	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in the
		adapter)
	*/

	return ((SysPAddr.uiAddr >= gsSGXDeviceMap.sLocalMemSysPBase.uiAddr) &&
		(SysPAddr.uiAddr < gsSGXDeviceMap.sLocalMemSysPBase.uiAddr + SYS_SGX_MEM_REGION_SIZE))
		  ? IMG_TRUE : IMG_FALSE;
}
#endif

/*!
******************************************************************************
 @Function     SysSysPAddrToDevPAddr

 @Description  Compute a device physical address from a system physical
	            address.

 @Input        SysPAddr - system physical address.
 @Input        eDeviceType - device type required if DevPAddr
			 address spaces vary across devices in the same system.

 @Return       Device physical address.
******************************************************************************/
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
	/*
		It is assumed the translation from cpu/system to device
		physical addresses is constant for every 'services managed'
		device in a given system
	*/
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in the adapter)
	*/
	DevPAddr.uiAddr = SysPAddr.uiAddr - gsSGXDeviceMap.sLocalMemSysPBase.uiAddr;

	return DevPAddr;
}


/*!
******************************************************************************
 @Function     SysDevPAddrToSysPAddr

 @Description  Compute a device physical address from a system physical
	            address.

 @Input        DevPAddr - device physical address.
 @Input        eDeviceType - device type required if DevPAddr
									address spaces vary across devices
									in the same system

 @Return       System physical address.
******************************************************************************/
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR DevPAddr)
{
    IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in the adapter)
	*/
    SysPAddr.uiAddr = DevPAddr.uiAddr + gsSGXDeviceMap.sLocalMemSysPBase.uiAddr;

    return SysPAddr;
}


/*****************************************************************************
 FUNCTION	: SysRegisterExternalDevice

 PURPOSE	: Called when a 3rd party device registers with services

 PARAMETERS: In:  psDeviceNode - the new device node.

***************************************************************************/
IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	psDeviceNode->ui32SOCInterruptBit = SysGetPDPInterruptBit();
}


/*****************************************************************************
 FUNCTION	: SysRemoveExternalDevice

 PURPOSE	: Called when a 3rd party device unregisters from services

 PARAMETERS: In:  psDeviceNode - the device node being removed.

*****************************************************************************/
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}


/*****************************************************************************
 FUNCTION	: SysResetDevice

 PURPOSE	: Reset a device

 PARAMETERS: In:  ui32DeviceIndex - device index.

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR SysResetDevice(IMG_UINT32 ui32DeviceIndex)
{
	if (ui32DeviceIndex == gui32SGXDeviceID)
	{
		SysResetSGX(gpsSysData->pvSOCRegsBase);
	}
	else
	{
		return PVRSRV_ERROR_INVALID_DEVICEID;
	}

	return PVRSRV_OK;
}


/*****************************************************************************
 FUNCTION	: SysOEMFunction

 PURPOSE	: marshalling function for custom OEM functions

 PARAMETERS	: ui32ID  - function ID
			  pvIn - in data
			  pvOut - out data

 RETURNS	: PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR SysOEMFunction (	IMG_UINT32	ui32ID,
								IMG_VOID	*pvIn,
								IMG_UINT32  ulInSize,
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

static IMG_VOID SysSaveRestoreArenaLiveSegments(IMG_BOOL bSave)
{
	IMG_SIZE_T        uiBufferSize;
	static IMG_PVOID  pvBackupBuffer = IMG_NULL;
	static IMG_HANDLE hBlockAlloc;
	static IMG_UINT32 uiWriteBufferSize = 0;
	PVRSRV_ERROR eError;

	uiBufferSize = 0;


	if (gpsSysData->apsLocalDevMemArena[0] != IMG_NULL)
	{

		if (PVRSRVSaveRestoreLiveSegments((IMG_HANDLE)gpsSysData->apsLocalDevMemArena[0], IMG_NULL, &uiBufferSize, bSave) == PVRSRV_OK)
		{
			if (uiBufferSize)
			{
				if (bSave && pvBackupBuffer == IMG_NULL)
				{
					uiWriteBufferSize = uiBufferSize;

					/* Allocate OS memory in which to store the device's local memory. */
					eError = OSAllocPages(PVRSRV_HAP_KERNEL_ONLY | PVRSRV_HAP_CACHED,
										  uiBufferSize,
										  HOST_PAGESIZE(),
										  IMG_NULL,
										  0,
										  IMG_NULL,
										  &pvBackupBuffer,
										  &hBlockAlloc);
					if (eError != PVRSRV_OK)
					{
						PVR_DPF((PVR_DBG_ERROR,
								"SysSaveRestoreArenaLiveSegments: OSAllocPages(0x%" SIZE_T_FMT_LEN "x) failed:%u", uiBufferSize, eError));
						return;
					}
				}
				else
				{
					PVR_ASSERT (uiWriteBufferSize == uiBufferSize);
				}


				PVRSRVSaveRestoreLiveSegments((IMG_HANDLE)gpsSysData->apsLocalDevMemArena[0], pvBackupBuffer, &uiBufferSize, bSave);

				if (!bSave && pvBackupBuffer)
				{
					/* Free the OS memory. */
					eError = OSFreePages(PVRSRV_HAP_KERNEL_ONLY | PVRSRV_HAP_CACHED,
										 uiWriteBufferSize,
										 pvBackupBuffer,
										 hBlockAlloc);
					if (eError != PVRSRV_OK)
					{
						PVR_DPF((PVR_DBG_ERROR,	"SysSaveRestoreArenaLiveSegments: OSFreePages(0x%" SIZE_T_FMT_LEN "x) failed:%u",
								uiBufferSize, eError));
					}

					pvBackupBuffer = IMG_NULL;
				}
			}
		}
	}

}

static PVRSRV_ERROR SysMapInRegisters(IMG_VOID)
{
	PVRSRV_DEVICE_NODE *psDeviceNodeList;

	/*
		Map the Atlas and PDP registers.
	*/
	gpsSysData->pvSOCRegsBase = OSMapPhysToLin(gsSOCRegsCpuPBase,
	                                           SYS_ATLAS_REG_REGION_SIZE,
	                                           PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
	                                           IMG_NULL);

	if (gpsSysData->pvSOCRegsBase == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysPrePowerState: Failed to map SOC Register base"));
		return PVRSRV_ERROR_REGISTER_BASE_NOT_SET;
	}

	psDeviceNodeList = gpsSysData->psDeviceNodeList;

	while (psDeviceNodeList)
	{
		PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNodeList->pvDevice;
		if (psDeviceNodeList->sDevId.eDeviceType == PVRSRV_DEVICE_TYPE_SGX)
		{
#if defined(NO_HARDWARE) && defined(__linux__)
			/*
			 * SysLocateDevices will have reallocated the dummy
			 * registers.
			 */
			PVR_ASSERT(gsSGXRegsCPUVAddr);

			psDevInfo->pvRegsBaseKM = gsSGXRegsCPUVAddr;
#else
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS))
			{
				/* Remap Regs */
				psDevInfo->pvRegsBaseKM = OSMapPhysToLin(gsSGXDeviceMap.sRegsCpuPBase,
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
#endif	/* #if defined(NO_HARDWARE) && defined(__linux__) */

			psDevInfo->ui32RegSize   = gsSGXDeviceMap.ui32RegsSize;
			psDevInfo->sRegsPhysBase = gsSGXDeviceMap.sRegsSysPBase;

#if defined(SGX_FEATURE_HOST_PORT)
			if (gsSGXDeviceMap.ui32Flags & SGX_HOSTPORT_PRESENT)
			{
				if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_HP))
				{
					/* Map Host Port */
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
#endif /* #if defined(SGX_FEATURE_HOST_PORT) */
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
		PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNodeList->pvDevice;
		if (psDeviceNodeList->sDevId.eDeviceType == PVRSRV_DEVICE_TYPE_SGX)
		{

#if !(defined(NO_HARDWARE) && defined(__linux__))
			/* Unmap Regs */
			if (psDevInfo->pvRegsBaseKM)
			{
				OSUnMapPhysToLin(psDevInfo->pvRegsBaseKM,
				                 gsSGXDeviceMap.ui32RegsSize,
				                 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				                 IMG_NULL);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_SGX_REGS);
			}
#endif	/* #if !(defined(NO_HARDWARE) && defined(__linux__)) */

			psDevInfo->pvRegsBaseKM = IMG_NULL;
			psDevInfo->ui32RegSize          = 0;
			psDevInfo->sRegsPhysBase.uiAddr = 0;

#if defined(SGX_FEATURE_HOST_PORT)
			if (gsSGXDeviceMap.ui32Flags & SGX_HOSTPORT_PRESENT)
			{
				/* Unmap Host Port */
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
#endif /* #if defined(SGX_FEATURE_HOST_PORT) */
		}

		psDeviceNodeList = psDeviceNodeList->psNext;
	}

#if defined(NO_HARDWARE)

	PVR_ASSERT(gsSGXRegsCPUVAddr);
	OSBaseFreeContigMemory(SYS_SGX_REG_SIZE, gsSGXRegsCPUVAddr, gsSGXDeviceMap.sRegsCpuPBase);

	SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_REG_MEM);

	gsSGXRegsCPUVAddr = IMG_NULL;
#endif

	/* Unmap the Atlas and PDP registers */
	if (gpsSysData->pvSOCRegsBase)
	{
		OSUnMapPhysToLin(gpsSysData->pvSOCRegsBase,
		                 SYS_ATLAS_REG_REGION_SIZE,
		                 PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
		                 IMG_NULL);

		gpsSysData->pvSOCRegsBase = IMG_NULL;
	}

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysSystemPrePowerState

 @Description	Perform system-level processing required before a system power
 				transition

 @Input	   eNewPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
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
#if defined(SYS_USING_INTERRUPTS)
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_IRQ))
			{
				SysDisableInterrupts(gpsSysData);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_IRQ);

			}

			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR))
			{
				eError = OSUninstallSystemLISR(gpsSysData);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysSystemPrePowerState: OSUninstallSystemLISR failed (%d)", eError));
				}
				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR);
			}
#endif /* defined(SYS_USING_INTERRUPTS) */

			/*
			 * Since this is a local memory architecture system,
			 * any local memory which is currently in use must
			 * be saved before power down.
			 */
			SysSaveRestoreArenaLiveSegments(IMG_TRUE);

			/*
			 * Unmap the system-level registers.
			 */
			SysUnmapRegisters();
#ifdef	__linux__
			eError = OSPCISuspendDev(gsSysSpecificData.hSGXPCI);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPrePowerState: OSPCISuspendDev failed (%d)", eError));
			}
#endif
		}
	}

	return eError;
}

/*!
******************************************************************************

 @Function	SysSystemPostPowerState

 @Description	Perform system-level processing required after a system power
 				transition

 @Input	   eNewPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR SysSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (eNewPowerState != gpsSysData->eCurrentPowerState)
	{
		if ((gpsSysData->eCurrentPowerState == PVRSRV_SYS_POWER_STATE_D3) &&
			(eNewPowerState < PVRSRV_SYS_POWER_STATE_D3))
		{
#ifdef	__linux__
			eError = OSPCIResumeDev(gsSysSpecificData.hSGXPCI);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: OSPCIResumeDev failed (%d)", eError));
			}
#endif
			/*
				Returning from D3 state.
				Find the device again as it may have been remapped.
			*/
			eError = SysLocateDevices(gpsSysData);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to locate devices"));
				return eError;
			}

			/*
			 * Since this is a local memory architecture system, any local memory
			 * which is currently in use must be restored before power up.
			 */
			SysSaveRestoreArenaLiveSegments(IMG_FALSE);

			/*
			 * Map the system-level registers.
			 */
			eError = SysMapInRegisters();
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to map in registers"));
				return eError;
			}

			/* Reset Atlas. */
			eError = SysInitRegisters();
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to Initialise registers"));
				return eError;
			}

#if defined(SYS_USING_INTERRUPTS)
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR))
			{
				eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
				if (eError != PVRSRV_OK)
				{
					PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: OSInstallSystemLISR failed to install ISR (%d)", eError));
				}
				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_LISR);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR);
			}

			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE))
			{
				SysEnableInterrupts(gpsSysData);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_ENABLE_IRQ);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE);
			}
#endif /* defined(SYS_USING_INTERRUPTS) */
		}
	}
	return eError;
}


/*!
******************************************************************************

 @Function	SysDevicePrePowerState

 @Description	Perform system-level processing required before a device power
 				transition

 @Input	   ui32DeviceIndex :
 @Input	   eNewPowerState :
 @Input	   eCurrentPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32				ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE	eNewPowerState,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if (ui32DeviceIndex == gui32SGXDeviceID)
	{
		if ((eNewPowerState != eCurrentPowerState) &&
			(eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF))
		{
			/*
			 * This is the point where power should be removed
			 * from SGX on a non-PCI system.
			 */
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePrePowerState: Remove SGX power"));
		}
	}

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDevicePostPowerState

 @Description	Perform system-level processing required after a device power
 				transition

 @Input	   ui32DeviceIndex :
 @Input	   eNewPowerState :
 @Input	   eCurrentPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32				ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE	eNewPowerState,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if (ui32DeviceIndex == gui32SGXDeviceID)
	{
		if ((eNewPowerState != eCurrentPowerState) &&
			(eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF))
		{
			/*
			 * This is the point where power should be restored to SGX on a
			 * non-PCI system.
			 */
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePostPowerState: Restore SGX power"));
		}
	}

	return PVRSRV_OK;
}

