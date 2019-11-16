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
#if defined(SUPPORT_DRI_DRM)
#include "drm/drmP.h"
#endif

#include "sgxdefs.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#ifdef SUPPORT_MSVDX
#include "msvdx_defs.h"
#include "msvdxapi.h"
#include "msvdx_infokm.h"
#include "osfunc.h"
#endif
#include "syslocal.h"
#if defined(SUPPORT_DRI_DRM_EXT)
#include "psb_drv.h"
#include "psb_powermgmt.h"
#include "sys_pvr_drm_export.h"
#endif

/* System specific timing defines*/
#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100) // 10ms (100hz)
#define SYS_SGX_PDS_TIMER_FREQ			(1000) // 1ms (1000hz)
#if defined(SUPPORT_DRI_DRM_EXT)
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(50)
#else
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(1)
#endif

/* top level system data anchor point*/
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
SYS_DATA  gsSysData;

static SYS_SPECIFIC_DATA gsSysSpecificData;

/* SGX structures */
static IMG_UINT32 gui32SGXDeviceID;

static SGX_DEVICE_MAP	gsSGXDeviceMap;

#if defined(SUPPORT_DRI_DRM_EXT)
static PVRSRV_DEVICE_NODE *gpsSGXDevNode;
#endif

#if defined(PDUMP)
/* memory space name for device memory */
#if !defined(SYSTEM_PDUMP_NAME_SGXMEM)
#define SYSTEM_PDUMP_NAME "SYSMEM"
#else
#define SYSTEM_PDUMP_NAME "SGXMEM"
#endif
#endif

#ifdef SUPPORT_MSVDX
/* MSVDX structures */
static IMG_UINT32		gui32MSVDXDeviceID;
static MSVDX_DEVICE_MAP	gsMSVDXDeviceMap;
#endif


#if defined(NO_HARDWARE)
/* mimic register block with contiguous memory */
static IMG_CPU_VIRTADDR gsSGXRegsCPUVAddr;
#ifdef SUPPORT_MSVDX
/* and MSVDX */
static IMG_CPU_VIRTADDR gsMSVDXRegsCPUVAddr;
#endif
#endif

#if !defined(NO_HARDWARE)
IMG_CPU_VIRTADDR gsPoulsboRegsCPUVaddr;
IMG_CPU_PHYADDR  gsPoulsboRegsCPUPaddr;
IMG_CPU_VIRTADDR gsPoulsboDisplayRegsCPUVaddr;
#endif

#if defined(SUPPORT_DRI_DRM)
extern struct drm_device *gpsPVRDRMDev;
#endif

#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
/* The following is exported by the Linux module code */
extern struct pci_dev *gpsPVRLDMDev;
#endif

#if !defined(SUPPORT_DRI_DRM_EXT)
IMG_UINT32 PVRSRV_BridgeDispatchKM( IMG_UINT32  Ioctl,
									IMG_BYTE   *pInBuf,
									IMG_UINT32  InBufLen,
									IMG_BYTE   *pOutBuf,
									IMG_UINT32  OutBufLen,
									IMG_UINT32 *pdwBytesTransferred);
#endif

#ifdef __linux__
#define	POULSBO_ADDR_RANGE_INDEX	(MMADR_INDEX - 4)
#define	POULSBO_HP_ADDR_RANGE_INDEX	(GMADR_INDEX - 4)

/*!
******************************************************************************

 @Function		PCIInitDev
 
 @Description

 Initialise the PCI Poulsbo device for subsequent use.
 
 @Input		psSysData :	System data

 @Return	PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR PCIInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	IMG_UINT32 ui32MaxOffset = POULSBO_MAX_OFFSET;

#if defined(SUPPORT_DRI_DRM_EXT)
	if (!IS_POULSBO(psSysSpecData->psDRMDev))
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device not supported"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	psSysSpecData->hSGXPCI = OSPCISetDev((IMG_VOID *)psSysSpecData->psPCIDev, 0);
	
#else	/* defined(SUPPORT_DRI_DRM_EXT) */
#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
	psSysSpecData->hSGXPCI = OSPCISetDev((IMG_VOID *)psSysSpecData->psPCIDev, HOST_PCI_INIT_FLAG_BUS_MASTER);
#else
	psSysSpecData->hSGXPCI = OSPCIAcquireDev(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV_DEVICE_ID, HOST_PCI_INIT_FLAG_BUS_MASTER);
#endif
#endif	/* defined(SUPPORT_DRI_DRM_EXT) */
	if (!psSysSpecData->hSGXPCI)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Failed to acquire PCI device"));
		return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
	}

	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV);

	PVR_TRACE(("PCI memory region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX)));
	PVR_TRACE(("Host Port region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX)));

	/* Check the address range is large enough. */
	if (OSPCIAddrRangeLen(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX) < ui32MaxOffset)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region isn't big enough"));
		return PVRSRV_ERROR_PCI_REGION_TOO_SMALL;
	}

#if !defined(SYS_SHARES_WITH_3PKM)
	/* Reserve the address range */
	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;

	}
	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE);

	/* Reserve the Host Port */
	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Host Port region not available"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}
	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_HOST_PORT_RANGE);
#endif /* defined(SYS_SHARES_WITH_3PKM) */

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function		PCIDeInitDev

 @Description

 Uninitialise the PCI Poulsbo device when it is no loger required

 @Input		psSysData :	System data

 @Return	none

******************************************************************************/
static IMG_VOID PCIDeInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX);
	}

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_HOST_PORT_RANGE))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX);
	}

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV))
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

	PVR_DPF((PVR_DBG_ERROR,"Couldn't find PCI device"));

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
#if !defined(NO_HARDWARE) || defined(__linux__)
	IMG_UINT32 ui32BaseAddr = 0;
	IMG_UINT32 ui32IRQ = 0;
#endif
	IMG_UINT32 ui32HostPortAddr = 0;
#if defined(NO_HARDWARE)
	IMG_CPU_PHYADDR sCpuPAddr;
#endif
#if defined(NO_HARDWARE) || defined(__linux__)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
#endif
#if !defined(__linux__)  || defined(NO_HARDWARE)
	PVRSRV_ERROR eError;
#endif

#ifdef __linux__
	ui32BaseAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, POULSBO_ADDR_RANGE_INDEX);
	ui32HostPortAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, POULSBO_HP_ADDR_RANGE_INDEX);
	if (OSPCIIRQ(psSysSpecData->hSGXPCI, &ui32IRQ) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Couldn't get IRQ"));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	PVR_TRACE(("ui32BaseAddr: %08X", ui32BaseAddr));
	PVR_TRACE(("ui32HostPortAddr: %08X", ui32HostPortAddr));
	PVR_TRACE(("IRQ: %d", ui32IRQ));
#else
	PCICONFIG_SPACE	sPCISpace;

#if !defined (NO_HARDWARE)
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif

	eError = FindPCIDevice(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV_DEVICE_ID, &sPCISpace);
	if (eError == PVRSRV_OK)
	{
		ui32BaseAddr = sPCISpace.u.aui32PCISpace[MMADR_INDEX];
		ui32HostPortAddr = sPCISpace.u.aui32PCISpace[GMADR_INDEX];
	}
	else
	{
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	ui32IRQ = (IMG_UINT32)sPCISpace.u.aui8PCISpace[0x3C];
#endif	/* __linux__ */

	/* SGX Device: */
	gsSGXDeviceMap.ui32Flags = 0x0;

	/* PCI device interrupt IRQ */
#if defined(NO_HARDWARE)
	/* No interrupts available on No HW system */
	gsSGXDeviceMap.ui32IRQ = 0;
#else
	gsSGXDeviceMap.ui32IRQ = ui32IRQ;
#endif

#if defined(NO_HARDWARE)
	/* No hardware registers */
	eError = OSBaseAllocContigMemory(SGX_REG_SIZE,
										&gsSGXRegsCPUVAddr,
										&sCpuPAddr);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS);

	gsSGXDeviceMap.sRegsCpuPBase = sCpuPAddr;

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

#else	/* defined(NO_HARDWARE) */
	gsSGXDeviceMap.sRegsSysPBase.uiAddr = ui32BaseAddr + SGX_REGS_OFFSET;
#endif	/* defined(NO_HARDWARE) */

	/* Registers */
	gsSGXDeviceMap.sRegsCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = SGX_REG_SIZE;

#if defined(SGX_FEATURE_HOST_PORT)
	/* host port */
	gsSGXDeviceMap.ui32Flags = SGX_HOSTPORT_PRESENT;
	gsSGXDeviceMap.sHPSysPBase.uiAddr = ui32HostPortAddr;
	gsSGXDeviceMap.sHPCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sHPSysPBase);
	gsSGXDeviceMap.ui32HPSize = SYS_SGX_HP_SIZE;
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

#if !defined(NO_HARDWARE)
	/* initialise Poulsbo register base */
	{
		IMG_SYS_PHYADDR sPoulsboRegsCpuPBase;

		sPoulsboRegsCpuPBase.uiAddr = ui32BaseAddr + POULSBO_REGS_OFFSET;
		gsPoulsboRegsCPUVaddr = OSMapPhysToLin(SysSysPAddrToCpuPAddr(sPoulsboRegsCpuPBase),
												 POULSBO_REG_SIZE,
												 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
												 IMG_NULL);

		gsPoulsboRegsCPUPaddr = SysSysPAddrToCpuPAddr(sPoulsboRegsCpuPBase);
		sPoulsboRegsCpuPBase.uiAddr = ui32BaseAddr + POULSBO_DISPLAY_REGS_OFFSET;
		gsPoulsboDisplayRegsCPUVaddr = OSMapPhysToLin(gsPoulsboRegsCPUPaddr,
												 POULSBO_DISPLAY_REG_SIZE,
												 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
												 IMG_NULL);
	}
#endif

#if defined(PDUMP)
	{
		/* initialise memory region name for pdumping */
		static IMG_CHAR pszPDumpDevName[] = SYSTEM_PDUMP_NAME;
		gsSGXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif

#ifdef SUPPORT_MSVDX
	/************************************************
		MSVDX Device:
	*************************************************/
#if defined(NO_HARDWARE)
	/* No hardware registers */
	eError = OSBaseAllocContigMemory(MSVDX_REG_SIZE,
										&gsMSVDXRegsCPUVAddr,
										&sCpuPAddr);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_MSVDX_REGS);
	gsMSVDXDeviceMap.sRegsCpuPBase = sCpuPAddr;

	OSMemSet(gsMSVDXRegsCPUVAddr, 0, MSVDX_REG_SIZE);

#if defined(__linux__)
	/* Indicate the registers are already mapped */
	gsMSVDXDeviceMap.pvRegsCpuVBase = gsMSVDXRegsCPUVAddr;
#else
	/*
	 * FIXME: Could we just use the virtual address returned by
	 * OSBaseAllocContigMemory?
	 */
	gsMSVDXDeviceMap.pvRegsCpuVBase = IMG_NULL;
#endif /* __linux__ */
#else	/* NO_HARDWARE */
	gsMSVDXDeviceMap.sRegsCpuPBase.uiAddr = ui32BaseAddr + MSVDX_REGS_OFFSET;
#endif /* NO_HARDWARE */
	gsMSVDXDeviceMap.sRegsSysPBase		  = SysCpuPAddrToSysPAddr(gsMSVDXDeviceMap.sRegsCpuPBase);
	gsMSVDXDeviceMap.ui32RegsSize		  = MSVDX_REG_SIZE;

	/*
		Local Device Memory Region: (not present)
		Note: the device doesn't need to know about its memory
		but keep info here for now
	*/
	gsMSVDXDeviceMap.sLocalMemSysPBase.uiAddr = 0;
	gsMSVDXDeviceMap.sLocalMemDevPBase.uiAddr = 0;
	gsMSVDXDeviceMap.sLocalMemCpuPBase.uiAddr = 0;
	gsMSVDXDeviceMap.ui32LocalMemSize		  = 0;

	/*
		device interrupt IRQ
	*/
	gsMSVDXDeviceMap.ui32IRQ = ui32IRQ;

#if defined(PDUMP)
	{
		/* initialise memory region name for pdumping */
		static IMG_CHAR pszPDumpDevName[] = SYSTEM_PDUMP_NAME;
		gsMSVDXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif
#endif // SUPPORT_MSVDX

	/************************************************
		add other devices here:
	*************************************************/

	return PVRSRV_OK;
}

#ifdef SUPPORT_MSVDX_FPGA
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
	IMG_UINT32  ui32BusNum;
	IMG_UINT32  ui32DevNum;
	IMG_UINT32  ui32VenDevID;
	IMG_UINT32	ui32BarIndex;

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

				/* Ensure access to memory space is enabled */
				OSPCIWriteDword(ui32BusNum, ui32DevNum, 0, 4, OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 4) | 0x02);

				psPCISpace->ui32BusNum  = ui32BusNum;	/* Save the device's address	*/
				psPCISpace->ui32DevNum  = ui32DevNum;
				psPCISpace->ui32FuncNum = 0;

				/* Now copy PCI config space to users buffer */
				for (ui32Idx=0; ui32Idx < 64; ui32Idx++)
				{
					psPCISpace->u.aui32PCISpace[ui32Idx] = OSPCIReadDword(ui32BusNum, ui32DevNum, 0, ui32Idx*4);

					if (ui32Idx < 16)
					{
						PVR_DPF((PVR_DBG_VERBOSE,"%08X\n",psPCISpace->u.aui32PCISpace[ui32Idx]));
					}
				}
						 										/* Get the size of the BARs		*/
				for (ui32BarIndex = 0; ui32BarIndex < 6; ui32BarIndex++)
				{
					GetPCIMemSpaceSize (ui32BusNum, ui32DevNum, ui32BarIndex, &psPCISpace->aui32PCIMemSpaceSize[ui32BarIndex]);
				}
				return PVRSRV_OK;
			}

		}/*End loop for each device*/

	}/*End loop for each bus*/

	PVR_DPF((PVR_DBG_ERROR,"Couldn't find PCI device"));

	return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
}

/*!
******************************************************************************

 @Function		GetPCIMemSpaceSize

 @Description	Identify the size for each PCI memory region.

 @Input			ui32BusNum
 @Input			ui32DevNum
 @Input			ui32BarIndex
 @Output		pui32PCIMemSpaceSize

 @Return		PVRSRV_ERROR

******************************************************************************/
static IMG_UINT32 GetPCIMemSpaceSize (IMG_UINT32 ui32BusNum, IMG_UINT32 ui32DevNum, IMG_UINT32 ui32BarIndex, IMG_UINT32* pui32PCIMemSpaceSize)
{

	IMG_UINT32	ui32AddressRange;
	IMG_UINT32	ui32BarSave;

	ui32BarSave = OSPCIReadDword (ui32BusNum, ui32DevNum, 0, ((4 + ui32BarIndex) * sizeof (IMG_UINT32)));

	OSPCIWriteDword (ui32BusNum, ui32DevNum, 0, ((4 + ui32BarIndex) * sizeof (IMG_UINT32)), 0xFFFFFFFF);

	ui32AddressRange = OSPCIReadDword (ui32BusNum, ui32DevNum, 0, ((4 + ui32BarIndex) * sizeof (IMG_UINT32)));

	OSPCIWriteDword (ui32BusNum, ui32DevNum, 0, ((4 + ui32BarIndex) * sizeof (IMG_UINT32)), ui32BarSave);

	*pui32PCIMemSpaceSize = (~(ui32AddressRange & 0xFFFFFFF0)) + 1;
	return PVRSRV_OK;
}
#endif


#ifdef __linux__
/*!
******************************************************************************

 @Function		SysCreateVersionString

 @Description

 Generate a device version string

 @Input		psSysData :	System data

 @Return	PVRSRV_ERROR

******************************************************************************/
#define VERSION_STR_MAX_LEN_TEMPLATE "SGX revision = 000.000.000"
static PVRSRV_ERROR SysCreateVersionString(SYS_DATA *psSysData)
{
    IMG_UINT32 ui32MaxStrLen;
    PVRSRV_ERROR eError;
    IMG_INT32 i32Count;
    IMG_CHAR *pszVersionString;
    IMG_UINT32 ui32SGXRevision = 0;


    ui32SGXRevision = SGX_CORE_REV;
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
		    "SGX revision = %u",
		    (IMG_UINT)(ui32SGXRevision));

    if(i32Count == -1)
    {
        OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
                    ui32MaxStrLen + 1,
                    pszVersionString,
                    IMG_NULL);
		/*not nulling pointer, out of scope*/
		return PVRSRV_ERROR_INVALID_PARAMS;
    }

    psSysData->pszVersionString = pszVersionString;

    return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function		SysFreeVersionString
 
 @Description

 Free the device version string
 
 @Input		psSysData :	System data

 @Return	none

******************************************************************************/
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
#endif /* __linux__ */
/*!
******************************************************************************

 @Function	SysInitialise

 @Description Initialises kernel services at 'driver load' time

 @Return   PVRSRV_ERROR  :

******************************************************************************/
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
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

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

	SysPowerMaybeInit(gpsSysData);

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
	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice,
								  DEVICE_SGX_INTERRUPT, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

#ifdef SUPPORT_MSVDX
	eError = PVRSRVRegisterDevice(gpsSysData, MSVDXRegisterDevice,
								  DEVICE_MSVDX_INTERRUPT, &gui32MSVDXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
#endif

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

				/* specify the backing store to use for the devices MMU PT/PDs */
				psDeviceNode->psLocalDevMemArena = IMG_NULL;

				/* useful pointers */
				psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
				psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

				/* specify the backing store for all SGX heaps */
				for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
				{
					psDeviceMemoryHeap[i].ui32Attribs |= PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;
#ifdef OEM_CUSTOMISE
					/* if required, modify the memory config */
#endif
				}
#if defined(SUPPORT_DRI_DRM_EXT)
				gpsSGXDevNode = psDeviceNode;
#endif
				break;
			}
#ifdef SUPPORT_MSVDX
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
					psDeviceMemoryHeap[i].ui32Attribs |= PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG;
#ifdef OEM_CUSTOMISE
					/* if required, modify the memory config */
#endif
				}
				break;
			}
#endif
			default:
			{
				PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to find SGX device node!"));
				return PVRSRV_ERROR_INIT_FAILURE;
			}
		}

		/* advance to next device */
		psDeviceNode = psDeviceNode->psNext;
	}

	/* Initialise all devices 'managed' by services: */
	eError = PVRSRVInitialiseDevice (gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_SGX_INITIALISED);

#ifdef SUPPORT_MSVDX
	eError = PVRSRVInitialiseDevice (gui32MSVDXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_MSVDX_INITIALISED);
#endif

	return PVRSRV_OK;
}


#if !defined(SUPPORT_DRI_DRM_EXT)
static IMG_VOID SysEnableInterrupts(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	IMG_UINT32 ui32RegData;
	IMG_UINT32 ui32Mask;

#ifdef SUPPORT_MSVDX
	ui32Mask = POULSBO_THALIA_MASK | POULSBO_MSVDX_MASK;
#else
	ui32Mask = POULSBO_THALIA_MASK;
#endif

	/* clear any spurious SGX interrupts */
	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_IDENTITY_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_IDENTITY_REG, ui32RegData | ui32Mask);

	/* unmask SGX bit in IMR */
	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_MASK_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_MASK_REG, ui32RegData & (~ui32Mask));

	/* Enable SGX bit in IER */
	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_ENABLE_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_ENABLE_REG, ui32RegData | ui32Mask);

	PVR_DPF((PVR_DBG_MESSAGE, "SysEnableInterrupts: Interrupts enabled"));
#endif
	PVR_UNREFERENCED_PARAMETER(psSysData);
}
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) */

#if !defined(SUPPORT_DRI_DRM_EXT)
static IMG_VOID SysDisableInterrupts(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	IMG_UINT32 ui32RegData;
	IMG_UINT32 ui32Mask;

#if defined (SUPPORT_MSVDX)
	ui32Mask = POULSBO_THALIA_MASK | POULSBO_MSVDX_MASK;
#else
	ui32Mask = POULSBO_THALIA_MASK;
#endif

	/* Disable SGX bit in IER */
	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_ENABLE_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_ENABLE_REG, ui32RegData & (~ui32Mask));

	/* Mask SGX bit in IMR */
	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_MASK_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_MASK_REG, ui32RegData | ui32Mask);

	PVR_TRACE(("SysDisableInterrupts: Interrupts disabled"));
#endif
	PVR_UNREFERENCED_PARAMETER(psSysData);
}
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) */




/*!
******************************************************************************

 @Function	SysFinalise
 
 @Description Final part of initialisation
 

 @Return   PVRSRV_ERROR  : 

******************************************************************************/
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

#if defined(SYS_USING_INTERRUPTS) && !defined(SUPPORT_DRI_DRM_EXT)
	eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: OSInstallSystemLISR failed"));
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED);
#endif /* defined(SYS_USING_INTERRUPTS) && !defined(SUPPORT_DRI_DRM_EXT) */

#if  defined(SYS_USING_INTERRUPTS) && !defined(SUPPORT_DRI_DRM_EXT)
	SysEnableInterrupts(gpsSysData);
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
#endif

#ifdef	__linux__
	/* Create a human readable version string for this system */
	eError = SysCreateVersionString(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: Failed to create a system version string"));
	}
	else
	{
	    PVR_DPF((PVR_DBG_WARNING, "SysFinalise: Version string: %s", gpsSysData->pszVersionString));
	}
#endif

	return eError;
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

	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

#if  defined(SYS_USING_INTERRUPTS) && !defined(SUPPORT_DRI_DRM_EXT)
	if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED))
	{
		SysDisableInterrupts(psSysData);
	}
#endif

#if defined(SYS_USING_INTERRUPTS) && !defined(SUPPORT_DRI_DRM_EXT)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_LISR_INSTALLED))
	{
		eError = OSUninstallSystemLISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallSystemLISR failed"));
			return eError;
		}
	}
#endif	/* defined(SYS_USING_INTERRUPTS) && !defined(SUPPORT_DRI_DRM_EXT) */

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_MISR_INSTALLED))
	{
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
	}

#if defined(SUPPORT_MSVDX)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_MSVDX_INITIALISED))
	{
		/* de-initialise all services managed devices */
		eError = PVRSRVDeinitialiseDevice(gui32MSVDXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
	}
#endif

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_SGX_INITIALISED))
	{
		/* de-initialise all services managed devices */
		eError = PVRSRVDeinitialiseDevice(gui32SGXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
	}

#ifdef __linux__
	SysFreeVersionString(psSysData);

	PCIDeInitDev(psSysData);
#endif

	eError = OSDeInitEnvData(psSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init env structure"));
		return eError;
	}

	SysDeinitialiseCommon(gpsSysData);

#if defined(NO_HARDWARE)
#ifdef SUPPORT_MSVDX
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_MSVDX_REGS))
	{
		OSBaseFreeContigMemory(MSVDX_REG_SIZE, gsMSVDXRegsCPUVAddr, gsMSVDXDeviceMap.sRegsCpuPBase);
	}
#endif

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS))
	{
		OSBaseFreeContigMemory(SGX_REG_SIZE, gsSGXRegsCPUVAddr, gsSGXDeviceMap.sRegsCpuPBase);
	}
#endif

#if !defined(NO_HARDWARE)
	/* Deinitialise Poulsbo register base */
	OSUnMapPhysToLin(gsPoulsboRegsCPUVaddr,
											 POULSBO_REG_SIZE,
											 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											 IMG_NULL);

	OSUnMapPhysToLin(gsPoulsboDisplayRegsCPUVaddr,
											 POULSBO_DISPLAY_REG_SIZE,
											 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											 IMG_NULL);
#endif
	gpsSysData = IMG_NULL;

	return PVRSRV_OK;
}


/*****************************************************************************
 @Function        SysGetInterruptSource

 @Description     Returns System specific information about the device(s) that 
				generated the interrupt in the system

 @Input           psSysData
 @Input           psDeviceNode

 @Return          System specific information indicating which device(s) 
		  generated the interrupt

******************************************************************************/
IMG_UINT32 SysGetInterruptSource(SYS_DATA* psSysData,
								 PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if !defined(SUPPORT_DRI_DRM_EXT) && !defined(NO_HARDWARE)
	IMG_UINT32 ui32Devices = 0;
	IMG_UINT32 ui32Data, ui32DIMMask;

	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	/* check if there is an interrupt to process */
	ui32Data = OSReadHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_IDENTITY_REG);

	if (ui32Data & POULSBO_THALIA_MASK)
	{
		ui32Devices |= DEVICE_SGX_INTERRUPT;
	}

	if (ui32Data & POULSBO_MSVDX_MASK)
	{
		ui32Devices |= DEVICE_MSVDX_INTERRUPT;
	}

	/* any other interrupts are for the DIM */
	ui32DIMMask = OSReadHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_ENABLE_REG);
	ui32DIMMask &= ~(POULSBO_THALIA_MASK | POULSBO_MSVDX_MASK);

	/* test for anything relating to the display, vsyncs, hot-plug etc */
	if (ui32Data & ui32DIMMask)
	{
		ui32Devices |= DEVICE_DISP_INTERRUPT;
	}

	return (ui32Devices);
#else	/* !defined(SUPPORT_DRI_DRM_EXT) && !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return 0;
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) && !defined(NO_HARDWARE) */
}

/*****************************************************************************
 @Function        SysClearInterrupts

 @Description     Clears specified system interrupts

 @Input           psSysData
 @Input           ui32ClearBits

******************************************************************************/
IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32ClearBits)
{
#if !defined(SUPPORT_DRI_DRM_EXT) && !defined(NO_HARDWARE)
	IMG_UINT32 ui32Data;
	IMG_UINT32 ui32Mask = 0;

	PVR_UNREFERENCED_PARAMETER(psSysData);

	ui32Data = OSReadHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_IDENTITY_REG);

	if ((ui32ClearBits & DEVICE_SGX_INTERRUPT) &&
		(ui32Data & POULSBO_THALIA_MASK))
	{
		ui32Mask |= POULSBO_THALIA_MASK;
	}

	if ((ui32ClearBits & DEVICE_MSVDX_INTERRUPT) &&
		(ui32Data & POULSBO_MSVDX_MASK))
	{
		ui32Mask |= POULSBO_MSVDX_MASK;
	}

	if ((ui32ClearBits & DEVICE_DISP_INTERRUPT) &&
		(ui32Data & POULSBO_VSYNC_PIPEA_VBLANK_MASK))
	{
	  ui32Mask |= POULSBO_VSYNC_PIPEA_VBLANK_MASK;
	}

	if (ui32Mask)
	{
		OSWriteHWReg(gsPoulsboRegsCPUVaddr, POULSBO_INTERRUPT_IDENTITY_REG, ui32Mask);
	}
#else	/* !defined(SUPPORT_DRI_DRM_EXT) && !defined(NO_HARDWARE)*/
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32ClearBits);
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) && !defined(NO_HARDWARE) */
}


/*!
******************************************************************************

 @Function	SysGetDeviceMemoryMap

 @Description 	returns a device address map for the specified device

 @Input		eDeviceType - device type
 @Input		ppvDeviceMap - void ptr to receive device specific info.

 @Return   	PVRSRV_ERROR

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
#ifdef SUPPORT_MSVDX
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


/*****************************************************************************
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

	/* Note: for this UMA system we assume DevP == CpuP */
	DevPAddr.uiAddr = CpuPAddr.uiAddr;

	return DevPAddr;
}


/*****************************************************************************
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

/*****************************************************************************
 @Function        SysCpuPAddrToSysPAddr

 @Description     Compute a system physical address from a cpu physical
	            address.

 @Input           cpu_paddr - cpu physical address.
 @Return          device physical address.

******************************************************************************/
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0, 
	   ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}


/*****************************************************************************
 @Function        SysSysPAddrToDevPAddr

 @Description     Compute a device physical address from a system physical
	            address.
         	
 @Input           SysPAddr - system physical address.
 @Input           eDeviceType - device type required if DevPAddr
				address spaces vary across devices 
				in the same system

 @Return          Device physical address.

******************************************************************************/
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
    IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/* Note: for this UMA system we assume DevP == CpuP */
    DevPAddr.uiAddr = SysPAddr.uiAddr;

    return DevPAddr;
}


/*****************************************************************************
 @Function        SysDevPAddrToSysPAddr

 @Description     Compute a device physical address from a system physical
	            address.

 @Input           DevPAddr - device physical address.
 @Input           eDeviceType - device type required if DevPAddr
				address spaces vary across devices 
				in the same system

 @Return          System physical address.

******************************************************************************/
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR DevPAddr)
{
    IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

    /* Note: for this UMA system we assume DevP == CpuP */
    SysPAddr.uiAddr = DevPAddr.uiAddr;

    return SysPAddr;
}


/*****************************************************************************
 @Function       SysRegisterExternalDevice

 @Description    Called when a 3rd party device registers with services

 @Input           psDeviceNode - the new device node.
*****************************************************************************/
IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
  // Services System LISR uses this bit to find what device raised the ISR
  psDeviceNode->ui32SOCInterruptBit = DEVICE_DISP_INTERRUPT;   
}


/*****************************************************************************
 @Function       SysRemoveExternalDevice

 @Description    Called when a 3rd party device unregisters from services

 @Input           psDeviceNode - the device node being removed.
*****************************************************************************/
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}

/*****************************************************************************
 @Function       SysOEMFunction

 @Description    marshalling function for custom OEM functions

 @Input           ui32ID  - function ID
 @Input           in data
 @Output          out data
 
 @Return          PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR SysOEMFunction (	IMG_UINT32	ui32ID,
								IMG_VOID	*pvIn,
								IMG_UINT32  ulInSize,
								IMG_VOID	*pvOut,
								IMG_UINT32	ulOutSize)
{
#if !defined(SUPPORT_DRI_DRM_EXT)
	PVR_UNREFERENCED_PARAMETER(ulInSize);
	PVR_UNREFERENCED_PARAMETER(pvIn);

	if ((ui32ID == OEM_GET_EXT_FUNCS) &&
		(ulOutSize == sizeof(PVRSRV_DC_OEM_JTABLE)))
	{
		PVRSRV_DC_OEM_JTABLE *psOEMJTable = (PVRSRV_DC_OEM_JTABLE*)pvOut;
		psOEMJTable->pfnOEMBridgeDispatch = &PVRSRV_BridgeDispatchKM;
#ifdef	__linux__
		psOEMJTable->pfnOEMReadRegistryString  = IMG_NULL;
		psOEMJTable->pfnOEMWriteRegistryString = IMG_NULL;
#else
		psOEMJTable->pfnOEMReadRegistryString  = IMG_NULL;//&PVRSRVReadRegistryString;
		psOEMJTable->pfnOEMWriteRegistryString = IMG_NULL;//&PVRSRVWriteRegistryString;
#endif

		return PVRSRV_OK;
	}
#endif

	return PVRSRV_ERROR_INVALID_PARAMS;
}


#if !defined(SUPPORT_DRI_DRM_EXT)
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
#if defined(NO_HARDWARE) && defined(__linux__)
			/*
			 * SysLocateDevices will have reallocated the dummy
			 * registers.
			 */
			PVR_ASSERT(gsSGXRegsCPUVAddr);

			psDevInfo->pvRegsBaseKM = gsSGXRegsCPUVAddr;
#else	/* defined(NO_HARDWARE) && defined(__linux__) */
			/* Remap SGX Regs */
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
			break;
		}
#ifdef SUPPORT_MSVDX
		case PVRSRV_DEVICE_TYPE_MSVDX:
		{
			PVRSRV_MSVDXDEV_INFO *psDevInfo = (PVRSRV_MSVDXDEV_INFO *)psDeviceNodeList->pvDevice;
#if defined(NO_HARDWARE) && defined(__linux__)
			/*
			 * SysLocate will have reallocated the
			 * dummy registers
			 */
			PVR_ASSERT(gsMSVDXRegsCPUVAddr);
			psDevInfo->pvRegsBaseKM = gsMSVDXRegsCPUVAddr;
#else	/* defined(NO_HARDWARE) && defined(__linux__) */
			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_MSVDX_REGS))
			{
				/* Remap registers */
				psDevInfo->pvRegsBaseKM = OSMapPhysToLin (
					gsMSVDXDeviceMap.sRegsCpuPBase,
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
#endif	/* defined(NO_HARDWARE) && defined(__linux__) */
			psDevInfo->ui32RegSize = gsMSVDXDeviceMap.ui32RegsSize;
			psDevInfo->sRegsPhysBase = gsMSVDXDeviceMap.sRegsSysPBase;
			break;
		}
#endif	/* SUPPORT_MSVDX */
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
			break;
		}
#ifdef SUPPORT_MSVDX
		case PVRSRV_DEVICE_TYPE_MSVDX:
		{
			PVRSRV_MSVDXDEV_INFO *psDevInfo = (PVRSRV_MSVDXDEV_INFO *)psDeviceNodeList->pvDevice;
#if !(defined(NO_HARDWARE) && defined(__linux__))
			if (psDevInfo->pvRegsBaseKM)
			{
				OSUnMapPhysToLin(psDevInfo->pvRegsBaseKM,
					psDevInfo->ui32RegSize,
					PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
					IMG_NULL);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_UNMAP_MSVDX_REGS);
			}
#endif	/* !(defined(NO_HARDWARE) && defined(__linux__)) */
			psDevInfo->pvRegsBaseKM = IMG_NULL;
			psDevInfo->ui32RegSize = 0;
			psDevInfo->sRegsPhysBase.uiAddr = 0;
			break;
		}
#endif	/* SUPPORT_MSVDX */
		default:
			break;
		}
		psDeviceNodeList = psDeviceNodeList->psNext;
	}

#if !defined(NO_HARDWARE)
	/* Unmap Poulsbo register base */
	OSUnMapPhysToLin(gsPoulsboRegsCPUVaddr,
				POULSBO_REG_SIZE,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				IMG_NULL);

	/* Unmap Poulsbo display registers */
	OSUnMapPhysToLin(gsPoulsboDisplayRegsCPUVaddr,
				POULSBO_DISPLAY_REG_SIZE,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				IMG_NULL);

#endif	/* #if !defined(NO_HARDWARE) */

#if defined(NO_HARDWARE)
#ifdef SUPPORT_MSVDX
	if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_MSVDX_REGS))
	{
		PVR_ASSERT(gsMSVDXRegsCPUVAddr)

		OSBaseFreeContigMemory(MSVDX_REG_SIZE, gsMSVDXRegsCPUVAddr, gsMSVDXDeviceMap.sRegsCpuPBase);

		gsMSVDXRegsCPUVAddr = IMG_NULL;

		SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_MSVDX_REGS);
	}
#endif

	if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS))
	{
		PVR_ASSERT(gsSGXRegsCPUVAddr);

		OSBaseFreeContigMemory(SGX_REG_SIZE, gsSGXRegsCPUVAddr, gsSGXDeviceMap.sRegsCpuPBase);
		gsSGXRegsCPUVAddr = IMG_NULL;

		SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_ALLOC_DUMMY_SGX_REGS);
	}
#endif /* defined(NO_HARDWARE) */

	return PVRSRV_OK;
}
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) */


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
#if !defined(SUPPORT_DRI_DRM_EXT)
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

#if  defined (SYS_USING_INTERRUPTS)
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
#endif /* !defined(UNDER_VISTA) && defined (SYS_USING_INTERRUPTS) */

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
#else	/* !defined(SUPPORT_DRI_DRM_EXT) */
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);

	return PVRSRV_OK;
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) */
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
#if !defined(SUPPORT_DRI_DRM_EXT)
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
				return eError;
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
			 * Map the system-level registers.
			 */
			eError = SysMapInRegisters();
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SysSystemPostPowerState: Failed to map in registers"));
				return eError;
			}

#if  defined (SYS_USING_INTERRUPTS)
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
#endif /* !defined(UNDER_VISTA) && defined (SYS_USING_INTERRUPTS) */

			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE))
			{
				SysEnableInterrupts(gpsSysData);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE);
			}
		}
	}
	return eError;
#else	/* !defined(SUPPORT_DRI_DRM_EXT) */
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);

	return PVRSRV_OK;
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) */
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
#if defined(SUPPORT_DRI_DRM_EXT)
			ospm_power_using_hw_end(OSPM_GRAPHICS_ISLAND);

			ospm_power_using_hw_end(OSPM_DISPLAY_ISLAND);
#endif
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
#if defined(SUPPORT_DRI_DRM_EXT)
			if (!ospm_power_using_hw_begin(OSPM_DISPLAY_ISLAND, true))
			{
				return PVRSRV_ERROR_DEVICE_POWER_CHANGE_FAILURE;
			}

			if (!ospm_power_using_hw_begin(OSPM_GRAPHICS_ISLAND, true))
			{
				ospm_power_using_hw_end(OSPM_DISPLAY_ISLAND);

				return PVRSRV_ERROR_DEVICE_POWER_CHANGE_FAILURE;
			}
#endif
		}
	}

	return PVRSRV_OK;
}

#if defined(SUPPORT_DRI_DRM_EXT)
/*
 * INTEGRATION_POINT:
 * Need an indirect way of calling the Device LISR (e.g. something like
 * OSCallDeviceISR or PVRSRVCallDeviceISR).
 */
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
#endif
