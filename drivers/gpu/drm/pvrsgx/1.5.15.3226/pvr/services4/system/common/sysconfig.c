/* -*- syscommon-c -*-
 *-----------------------------------------------------------------------------
 * Filename: syscommon.c
 * $Revision: 1.11 $
 *-----------------------------------------------------------------------------
 * Copyright (c) 2002-2010, Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *-----------------------------------------------------------------------------
 * Description: platform detection, and sharing of correct platform interface.
 *
 *-----------------------------------------------------------------------------
 */

#include <linux/pci.h>
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
#include "pdump_km.h"
#include "syslocal.h"
#include "sysconfig.h"
#include "pvr_debug.h"
#include "msvdx_pvr.h"

/* --------------------------------------------------------------------------*/
/**
* @Synopsis Pointer to "System Data" used by all platform blocks, exported
* in "syscommon.h" header file.
*/
/* --------------------------------------------------------------------------*/
SYS_DATA *gpsSysData = (SYS_DATA*)IMG_NULL;

/* --------------------------------------------------------------------------*/
/**
* @Synopsis Pointer to found platform interface. It is IMG_NULL when no
* correct platform can be found.
*/
/* --------------------------------------------------------------------------*/
SYS_PLATFORM_INTERFACE *gpsSysPlatformInterface = (SYS_PLATFORM_INTERFACE*)IMG_NULL;


/* --------------------------------------------------------------------------*/
/**
* @Synopsis Poulsbo platform interface.
*/
/* --------------------------------------------------------------------------*/
extern SYS_PLATFORM_INTERFACE gpsSysPlatformInterfacePlb;

/* --------------------------------------------------------------------------*/
/**
* @Synopsis Atom E6xx platform interface.
*/
/* --------------------------------------------------------------------------*/
extern SYS_PLATFORM_INTERFACE gpsSysPlatformInterfaceTnc;

/* --------------------------------------------------------------------------*/
/**
* @Synopsis Table to available platforms interfaces.
* The last element must be set to IMG_NULL.
*
* @Param &gpsSysPlatformInterfacePlb Poulsbo interface
* @Param &gpsSysPlatformInterfaceTnc Atom E6xx interface
*/
/* --------------------------------------------------------------------------*/
SYS_PLATFORM_INTERFACE* gpsSysPlatformInterfacesTab[] = {	&gpsSysPlatformInterfacePlb,
								&gpsSysPlatformInterfaceTnc,
								(SYS_PLATFORM_INTERFACE*)IMG_NULL};


/* --------------------------------------------------------------------------*/
/**
* @Synopsis Function to detect platform and set "gpsSysPlatformInterface" to
* one member of "gpsSysPlatformInterfacesTab".
*
* @Param IMG_VOID
*
* @Returns
*/
/* --------------------------------------------------------------------------*/
IMG_VOID SysPlatformDetect(IMG_VOID)
{
	SYS_PLATFORM_INTERFACE** psSysPlatformInterfacesTab = gpsSysPlatformInterfacesTab;

	while(*psSysPlatformInterfacesTab != IMG_NULL) {

		if(pci_get_device(SYS_SGX_DEV_VENDOR_ID, (unsigned short)((*psSysPlatformInterfacesTab)->uiSgxDevDeviceID), NULL)) {
			gpsSysPlatformInterface = *psSysPlatformInterfacesTab;
			PVR_DPF((PVR_DBG_MESSAGE,"Platform detected: %s", gpsSysPlatformInterface->sProductName));
			return;
		}
		PVR_DPF((PVR_DBG_MESSAGE,"Platform search: %s", (*psSysPlatformInterfacesTab)->sProductName));
		psSysPlatformInterfacesTab++;
	}
	PVR_DPF((PVR_DBG_ERROR,"Platform detected Failed"));
}


/* --------------------------------------------------------------------------*/
/**
* @Synopsis  Interface for Atom E6xx device
*/
/* ----------------------------------------------------------------------------*/

static SYS_DATA  gsSysData;

static SYS_SPECIFIC_DATA gsSysSpecificData;

static IMG_UINT32		gui32SGXDeviceID;
static SGX_DEVICE_MAP	gsSGXDeviceMap;
static IMG_UINT32		gui32MSVDXDeviceID;

#ifdef SUPPORT_MSVDX
static IMG_UINT32		gui32MSVDXDeviceID;
static MSVDX_DEVICE_MAP	gsMSVDXDeviceMap;
#endif


#if defined(NO_HARDWARE)
static IMG_CPU_VIRTADDR gsSGXRegsCPUVAddr;
#ifdef SUPPORT_MSVDX
static IMG_CPU_VIRTADDR gsMSVDXRegsCPUVAddr;
#endif
#endif

#if !defined(NO_HARDWARE)
static IMG_CPU_VIRTADDR gsPoulsboRegsCPUVaddr;

#if defined(MAP_UNUSED_MAPPINGS)
static IMG_CPU_VIRTADDR gsPoulsboDisplayRegsCPUVaddr;
#endif

#endif

#ifdef	LDM_PCI
extern struct pci_dev *gpsPVRLDMDev;
#endif

IMG_UINT32 PVRSRV_BridgeDispatchKM( IMG_UINT32  Ioctl,
									IMG_BYTE   *pInBuf,
									IMG_UINT32  InBufLen,
									IMG_BYTE   *pOutBuf,
									IMG_UINT32  OutBufLen,
									IMG_UINT32 *pdwBytesTransferred);

#ifdef __linux__
#define	ADDR_RANGE_INDEX	(MMADR_INDEX - 4)
#if defined(SGX_FEATURE_HOST_PORT)
#define	HP_ADDR_RANGE_INDEX	(GMADR_INDEX - 4)
#endif
static PVRSRV_ERROR PCIInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

#ifdef	LDM_PCI
	psSysSpecData->hSGXPCI = OSPCISetDev((IMG_VOID *)psSysSpecData->psPCIDev, HOST_PCI_INIT_FLAG_BUS_MASTER);
#else
	psSysSpecData->hSGXPCI = OSPCIAcquireDev(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV_DEVICE_ID, HOST_PCI_INIT_FLAG_BUS_MASTER);
#endif
	if (!psSysSpecData->hSGXPCI)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Failed to acquire PCI device"));
		return PVRSRV_ERROR_GENERIC;
	}

	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV);

	PVR_TRACE(("PCI memory region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, ADDR_RANGE_INDEX), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, ADDR_RANGE_INDEX)));
#if defined(SGX_FEATURE_HOST_PORT)
	PVR_TRACE(("Host Port region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, HP_ADDR_RANGE_INDEX), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, HP_ADDR_RANGE_INDEX)));
#endif

	if (OSPCIAddrRangeLen(psSysSpecData->hSGXPCI, ADDR_RANGE_INDEX) < MAX_OFFSET)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region isn't big enough"));
		return PVRSRV_ERROR_GENERIC;
	}


	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, ADDR_RANGE_INDEX) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available"));
		return PVRSRV_ERROR_GENERIC;
	}
	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE);


#if defined(SGX_FEATURE_HOST_PORT)
	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, HP_ADDR_RANGE_INDEX) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Host Port region not available"));
		return PVRSRV_ERROR_GENERIC;
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
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, ADDR_RANGE_INDEX);
	}

#if defined(SGX_FEATURE_HOST_PORT)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_HOST_PORT_RANGE))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, HP_ADDR_RANGE_INDEX);
	}
#endif

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV))
	{
		OSPCIReleaseDev(psSysSpecData->hSGXPCI);
	}
}
#else
static PVRSRV_ERROR FindPCIDevice(IMG_UINT16 ui16VenID, IMG_UINT16 ui16DevID, PCICONFIG_SPACE *psPCISpace)
{
	IMG_UINT32 ui32BusNum;
	IMG_UINT32 ui32DevNum;
	IMG_UINT32 ui32VenDevID;


	for (ui32BusNum=0; ui32BusNum < 255; ui32BusNum++)
	{

		for (ui32DevNum=0; ui32DevNum < 32; ui32DevNum++)
		{

			ui32VenDevID=OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 0);


			if (ui32VenDevID == (IMG_UINT32)((ui16DevID<<16)+ui16VenID))
			{
				IMG_UINT32 ui32Idx;


				OSPCIWriteDword(ui32BusNum, ui32DevNum, 0, 4, OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 4) | 0x02);


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

		}

	}

	PVR_DPF((PVR_DBG_ERROR,"Couldn't find PCI device"));

	return PVRSRV_ERROR_GENERIC;
}
#endif

static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE) || defined(__linux__)
	IMG_UINT32 ui32BaseAddr;
	IMG_UINT32 ui32IRQ;
#endif
#if defined(SGX_FEATURE_HOST_PORT)
	IMG_UINT32 ui32HostPortAddr = 0UL;
#endif
#if defined(NO_HARDWARE)
	IMG_CPU_PHYADDR sCpuPAddr;
#endif
#if defined(NO_HARDWARE) || defined(__linux__)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
#endif
#if (!defined(__linux__) || defined(NO_HARDWARE))
	PVRSRV_ERROR eError;
#endif

#ifdef __linux__
	ui32BaseAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, ADDR_RANGE_INDEX);
#if defined(SGX_FEATURE_HOST_PORT)
	ui32HostPortAddr = OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, HP_ADDR_RANGE_INDEX);
#endif
	if (OSPCIIRQ(psSysSpecData->hSGXPCI, &ui32IRQ) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Couldn't get IRQ"));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	PVR_TRACE(("IRQ: %d", ui32IRQ));
#else
	PVRSRV_ERROR eError;
	PCICONFIG_SPACE	sPCISpace;

	eError = FindPCIDevice(VENDOR_ID, DEVICE_ID, &sPCISpace);
	if (eError == PVRSRV_OK)
	{
		ui32BaseAddr = sPCISpace.u.aui32PCISpace[MMADR_INDEX];
#if defined(SGX_FEATURE_HOST_PORT)
		ui32HostPortAddr = sPCISpace.u.aui32PCISpace[GMADR_INDEX];
#endif
	}
	else
	{
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	ui32IRQ = (IMG_UINT32)sPCISpace.u.aui8PCISpace[0x3C];
#endif


	gsSGXDeviceMap.ui32Flags = 0x0;


#if defined(NO_HARDWARE)

	gsSGXDeviceMap.ui32IRQ = 0;
#else
	gsSGXDeviceMap.ui32IRQ = ui32IRQ;
#endif

#if defined(NO_HARDWARE)

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

	gsSGXDeviceMap.pvRegsCpuVBase = gsSGXRegsCPUVAddr;
#else

	gsSGXDeviceMap.pvRegsCpuVBase = IMG_NULL;
#endif

#else
	gsSGXDeviceMap.sRegsSysPBase.uiAddr = ui32BaseAddr + SGX_REGS_OFFSET;
#endif


	gsSGXDeviceMap.sRegsCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = SGX_REG_SIZE;

#if defined(SGX_FEATURE_HOST_PORT)

	gsSGXDeviceMap.ui32Flags = SGX_HOSTPORT_PRESENT;
	gsSGXDeviceMap.sHPSysPBase.uiAddr = ui32HostPortAddr;
	gsSGXDeviceMap.sHPCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sHPSysPBase);
	gsSGXDeviceMap.ui32HPSize = SYS_SGX_HP_SIZE;
#endif




	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemDevPBase.uiAddr = 0;
	gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr = 0;
	gsSGXDeviceMap.ui32LocalMemSize = 0;

#if !defined(NO_HARDWARE)

	{
		IMG_SYS_PHYADDR sPoulsboRegsCpuPBase;

		sPoulsboRegsCpuPBase.uiAddr = ui32BaseAddr + REGS_OFFSET;
		gsPoulsboRegsCPUVaddr = OSMapPhysToLin(SysSysPAddrToCpuPAddr(sPoulsboRegsCpuPBase),
												 REG_SIZE,
												 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
												 IMG_NULL);

#if defined(MAP_UNUSED_MAPPINGS)
		sPoulsboRegsCpuPBase.uiAddr = ui32BaseAddr + DISPLAY_REGS_OFFSET;
		gsPoulsboDisplayRegsCPUVaddr = OSMapPhysToLin(SysSysPAddrToCpuPAddr(sPoulsboRegsCpuPBase),
												 DISPLAY_REG_SIZE,
												 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
												 IMG_NULL);
#endif
	}
#endif

#ifdef SUPPORT_MSVDX


#if defined(NO_HARDWARE)

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

	gsMSVDXDeviceMap.pvRegsCpuVBase = gsMSVDXRegsCPUVAddr;
#else

	gsMSVDXDeviceMap.pvRegsCpuVBase = IMG_NULL;
#endif
#else
	gsMSVDXDeviceMap.sRegsCpuPBase.uiAddr = ui32BaseAddr + MSVDX_REGS_OFFSET;
#endif
	gsMSVDXDeviceMap.sRegsSysPBase		  = SysCpuPAddrToSysPAddr(gsMSVDXDeviceMap.sRegsCpuPBase);
	gsMSVDXDeviceMap.ui32RegsSize		  = MSVDX_REG_SIZE;





	gsMSVDXDeviceMap.sLocalMemSysPBase.uiAddr = 0;
	gsMSVDXDeviceMap.sLocalMemDevPBase.uiAddr = 0;
	gsMSVDXDeviceMap.sLocalMemCpuPBase.uiAddr = 0;
	gsMSVDXDeviceMap.ui32LocalMemSize		  = 0;



	gsMSVDXDeviceMap.ui32IRQ = ui32IRQ;

#endif



	return PVRSRV_OK;
}

#ifdef SUPPORT_MSVDX_FPGA
static PVRSRV_ERROR FindPCIDevice(IMG_UINT16 ui16VenID, IMG_UINT16 ui16DevID, PCICONFIG_SPACE *psPCISpace)
{
	IMG_UINT32  ui32BusNum;
	IMG_UINT32  ui32DevNum;
	IMG_UINT32  ui32VenDevID;
	IMG_UINT32	ui32BarIndex;


	for (ui32BusNum=0; ui32BusNum < 255; ui32BusNum++)
	{

		for (ui32DevNum=0; ui32DevNum < 32; ui32DevNum++)
		{

			ui32VenDevID=OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 0);


			if (ui32VenDevID == (IMG_UINT32)((ui16DevID<<16)+ui16VenID))
			{
				IMG_UINT32 ui32Idx;


				OSPCIWriteDword(ui32BusNum, ui32DevNum, 0, 4, OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 4) | 0x02);

				psPCISpace->ui32BusNum  = ui32BusNum;
				psPCISpace->ui32DevNum  = ui32DevNum;
				psPCISpace->ui32FuncNum = 0;


				for (ui32Idx=0; ui32Idx < 64; ui32Idx++)
				{
					psPCISpace->u.aui32PCISpace[ui32Idx] = OSPCIReadDword(ui32BusNum, ui32DevNum, 0, ui32Idx*4);

					if (ui32Idx < 16)
					{
						PVR_DPF((PVR_DBG_VERBOSE,"%08X\n",psPCISpace->u.aui32PCISpace[ui32Idx]));
					}
				}

				for (ui32BarIndex = 0; ui32BarIndex < 6; ui32BarIndex++)
				{
					GetPCIMemSpaceSize (ui32BusNum, ui32DevNum, ui32BarIndex, &psPCISpace->aui32PCIMemSpaceSize[ui32BarIndex]);
				}
				return PVRSRV_OK;
			}

		}

	}

	PVR_DPF((PVR_DBG_ERROR,"Couldn't find PCI device"));

	return PVRSRV_ERROR_GENERIC;
}

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
#define VERSION_STR_MAX_LEN_TEMPLATE "SGX revision = 000.000.000"
static PVRSRV_ERROR SysCreateVersionString(SYS_DATA *psSysData)
{
    IMG_UINT32 ui32SGXRevision = 0;
    IMG_UINT32 ui32MaxStrLen;
    PVRSRV_ERROR eError;
    IMG_INT32 i32Count;
    IMG_CHAR *pszVersionString;

#if !defined(NO_HARDWARE)

    {
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
    }
#endif

    ui32MaxStrLen = OSStringLength(VERSION_STR_MAX_LEN_TEMPLATE);
    eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
                          ui32MaxStrLen + 1,
                          (IMG_PVOID *)&pszVersionString,
                          IMG_NULL,
						  "Version String");
    if(eError != PVRSRV_OK)
    {
		return PVRSRV_ERROR_GENERIC;
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

		return PVRSRV_ERROR_GENERIC;
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
		psSysData->pszVersionString = NULL;
    }
}
#endif
PVRSRV_ERROR SysInitialise(IMG_VOID)
{
	IMG_UINT32			i;
	PVRSRV_ERROR 		eError;
	PVRSRV_DEVICE_NODE	*psDeviceNode;
	SGX_TIMING_INFORMATION*	psTimingInfo;

	PVR_DPF((PVR_DBG_MESSAGE,"SysInitialise"));

	SysPlatformDetect();

	gpsSysData = &gsSysData;
	OSMemSet(gpsSysData, 0, sizeof(SYS_DATA));

	gpsSysData->pvSysSpecificData = &gsSysSpecificData;
	gsSysSpecificData.ui32SysSpecificData = 0;
#ifdef	LDM_PCI

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


	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif
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

//#ifdef SUPPORT_MSVDX
	eError = PVRSRVRegisterDevice(gpsSysData, MSVDXRegisterDevice,
								  DEVICE_MSVDX_INTERRUPT, &gui32MSVDXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
//#endif


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

				break;
			}

//#ifdef SUPPORT_MSVDX
			case PVRSRV_DEVICE_TYPE_MSVDX:
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
				break;
			}
//#endif
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

//#ifdef SUPPORT_MSVDX
	eError = PVRSRVInitialiseDevice (gui32MSVDXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_MSVDX_INITIALISED);
//#endif

	return PVRSRV_OK;
}


static IMG_VOID SysEnableInterrupts(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	IMG_UINT32 ui32RegData;
	IMG_UINT32 ui32Mask;

	ui32Mask = THALIA_MASK | MSVDX_MASK;

	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_IDENTITY_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_IDENTITY_REG, ui32RegData | ui32Mask);


	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_MASK_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_MASK_REG, ui32RegData & (~ui32Mask));


	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_ENABLE_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_ENABLE_REG, ui32RegData | ui32Mask);

	PVR_TRACE(("SysEnableInterrupts: Interrupts enabled"));
#endif
	PVR_UNREFERENCED_PARAMETER(psSysData);
}

static IMG_VOID SysDisableInterrupts(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	IMG_UINT32 ui32RegData;
	IMG_UINT32 ui32Mask;


	ui32Mask = THALIA_MASK | MSVDX_MASK;

	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_ENABLE_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_ENABLE_REG, ui32RegData & (~ui32Mask));


	ui32RegData = OSReadHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_MASK_REG);
	OSWriteHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_MASK_REG, ui32RegData | ui32Mask);

	PVR_TRACE(("SysDisableInterrupts: Interrupts disabled"));
#endif
	PVR_UNREFERENCED_PARAMETER(psSysData);
}

IMG_VOID SysReEnableInterrupts(IMG_VOID)
{
	SysEnableInterrupts(gpsSysData);
}

PVRSRV_ERROR SysFinalise(IMG_VOID)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

#if defined(SYS_USING_INTERRUPTS)
	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: OSInstallMISR failed"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_MISR_INSTALLED);

	eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysFinalise: OSInstallSystemLISR failed"));
		SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_LISR_INSTALLED);
#endif

	SysEnableInterrupts(gpsSysData);
	SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED);

#ifdef	__linux__

	eError = SysCreateVersionString(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to create a system version string"));
	}
#endif

	return eError;
}

PVRSRV_ERROR SysDeinitialise (SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError;

	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED))
	{
		SysDisableInterrupts(psSysData);
	}

#if defined(SYS_USING_INTERRUPTS)
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_LISR_INSTALLED))
	{
		eError = OSUninstallSystemLISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallSystemLISR failed"));
			return eError;
		}
	}

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_MISR_INSTALLED))
	{
		eError = OSUninstallMISR(psSysData);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
			return eError;
		}
	}
#endif
/* Commenting this out to clean up allocation made in SysInitialise */
/*#if defined(SUPPORT_MSVDX)*/
	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_MSVDX_INITIALISED))
	{

		eError = PVRSRVDeinitialiseDevice(gui32MSVDXDeviceID);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
			return eError;
		}
	}
/*#endif*/

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_SGX_INITIALISED))
	{

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

	OSUnMapPhysToLin(gsPoulsboRegsCPUVaddr,
											 REG_SIZE,
											 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											 IMG_NULL);

#if defined(MAP_UNUSED_MAPPINGS)
	OSUnMapPhysToLin(gsPoulsboDisplayRegsCPUVaddr,
											 DISPLAY_REG_SIZE,
											 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											 IMG_NULL);
#endif

#endif
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
#if !defined(NO_HARDWARE)
	IMG_UINT32 ui32Devices = 0;
	IMG_UINT32 ui32Data, ui32DIMMask;

	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);


	ui32Data = OSReadHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_IDENTITY_REG);

	if (ui32Data & THALIA_MASK)
	{
		ui32Devices |= DEVICE_SGX_INTERRUPT;
	}

	if (ui32Data & MSVDX_MASK)
	{
		ui32Devices |= DEVICE_MSVDX_INTERRUPT;
	}


	ui32DIMMask = OSReadHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_ENABLE_REG);
	ui32DIMMask &= ~(THALIA_MASK | MSVDX_MASK);


	if (ui32Data & ui32DIMMask)
	{
		ui32Devices |= DEVICE_DISP_INTERRUPT;
	}

	return (ui32Devices);
#else
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return 0;
#endif
}

IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32ClearBits)
{
#if !defined(NO_HARDWARE)
	IMG_UINT32 ui32Data;
	IMG_UINT32 ui32Mask = 0;

	PVR_UNREFERENCED_PARAMETER(psSysData);

	ui32Data = OSReadHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_STATUS_REG);

	if ((ui32ClearBits & DEVICE_SGX_INTERRUPT) &&
		((ui32Data & THALIA_MASK) == 0))
	{
		ui32Mask |= THALIA_MASK;
	}

	if ((ui32ClearBits & DEVICE_MSVDX_INTERRUPT) &&
		((ui32Data & MSVDX_MASK) == 0))
	{
		ui32Mask |= MSVDX_MASK;
	}

	if (ui32Mask)
	{
		OSWriteHWReg(gsPoulsboRegsCPUVaddr, INTERRUPT_IDENTITY_REG, ui32Mask);
	}
#else
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32ClearBits);
#endif
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
#ifdef SUPPORT_MSVDX
		case PVRSRV_DEVICE_TYPE_MSVDX:
		{

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
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
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
	if (ulInSize || pvIn);

	if ((ui32ID == OEM_GET_EXT_FUNCS) &&
		(ulOutSize == sizeof(PVRSRV_DC_OEM_JTABLE)))
	{
		PVRSRV_DC_OEM_JTABLE *psOEMJTable = (PVRSRV_DC_OEM_JTABLE*)pvOut;
		psOEMJTable->pfnOEMBridgeDispatch = &PVRSRV_BridgeDispatchKM;
#ifdef	__linux__
		psOEMJTable->pfnOEMReadRegistryString  = IMG_NULL;
		psOEMJTable->pfnOEMWriteRegistryString = IMG_NULL;
#else
		psOEMJTable->pfnOEMReadRegistryString  = IMG_NULL;
		psOEMJTable->pfnOEMWriteRegistryString = IMG_NULL;
#endif
		return PVRSRV_OK;
	}

	return PVRSRV_ERROR_INVALID_PARAMS;
}



PVRSRV_ERROR SysSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVRSRV_ERROR eError= PVRSRV_OK;

	if (eNewPowerState != gpsSysData->eCurrentPowerState)
	{
		if ((eNewPowerState == PVRSRV_SYS_POWER_STATE_D3) &&
			(gpsSysData->eCurrentPowerState < PVRSRV_SYS_POWER_STATE_D3))
		{

			if (SYS_SPECIFIC_DATA_TEST(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED))
			{
				SysDisableInterrupts(gpsSysData);

				SYS_SPECIFIC_DATA_SET(&gsSysSpecificData, SYS_SPECIFIC_DATA_PM_IRQ_DISABLE);
				SYS_SPECIFIC_DATA_CLEAR(&gsSysSpecificData, SYS_SPECIFIC_DATA_IRQ_ENABLED);
			}

#if defined(SYS_USING_INTERRUPTS)
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
#endif


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


#if defined(SYS_USING_INTERRUPTS)
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
#endif

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


PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32				ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE	eNewPowerState,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if (ui32DeviceIndex == gui32SGXDeviceID)
	{
		if ((eNewPowerState != eCurrentPowerState) &&
			(eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF))
		{
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePrePowerState: Remove SGX power"));
		}
	}

	return PVRSRV_OK;
}


PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32				ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE	eNewPowerState,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if (ui32DeviceIndex == gui32SGXDeviceID)
	{
		if ((eNewPowerState != eCurrentPowerState) &&
			(eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF))
		{
			PVR_DPF((PVR_DBG_MESSAGE,"SysDevicePostPowerState: Restore SGX power"));
		}
	}

	return PVRSRV_OK;
}




