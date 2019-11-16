/*************************************************************************/ /*!
@Title          Shared system dependent utilities
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Provides system-specific functions
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
#include "sgxinfokm.h"

#include "sysinfo.h"
#include "sysconfig.h"
#include "sysutils.h"

/* SYSTEM-SPECIFIC UNTILITY FUNCTIONS */

/*******************************************************************************
 * Initialise the device for subsequent use.
 *
 * @Input	psSysData :	System data
 *
 * @Return	PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR PCIInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

#if defined(LDM_PCI) || defined(SUPPORT_DRI_DRM)
	psSysSpecData->hSGXPCI = OSPCISetDev((IMG_VOID *)psSysSpecData->psPCIDev, HOST_PCI_INIT_FLAG_BUS_MASTER);
#else
	psSysSpecData->hSGXPCI = OSPCIAcquireDev(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV_DEVICE_ID, HOST_PCI_INIT_FLAG_BUS_MASTER);
#if defined(SYS_SGX_DEV1_DEVICE_ID)
	if (psSysSpecData->hSGXPCI == NULL)
	{
		psSysSpecData->hSGXPCI = OSPCIAcquireDev(SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV1_DEVICE_ID, HOST_PCI_INIT_FLAG_BUS_MASTER);
	}
#endif
#endif
	if (psSysSpecData->hSGXPCI == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Failed to acquire PCI device"));
		return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
	}

	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV);

	PVR_TRACE(("PCI memory region: %x to %x", OSPCIAddrRangeStart(psSysSpecData->hSGXPCI, CEDARVIEW_ADDR_RANGE_INDEX), OSPCIAddrRangeEnd(psSysSpecData->hSGXPCI, CEDARVIEW_ADDR_RANGE_INDEX)));

	/* Reserve the address range */
	if (OSPCIRequestAddrRange(psSysSpecData->hSGXPCI, CEDARVIEW_ADDR_RANGE_INDEX) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;

	}
	 SYS_SPECIFIC_DATA_SET(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE);

	return PVRSRV_OK;
}

/*******************************************************************************
 * Uninitialise the PCI Poulsbo device when it is no loger required
 *
 * @Input	psSysData :	System data
 *
 * @Return	none
******************************************************************************/
IMG_VOID PCIDeInitDev(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_REQUEST_SGX_ADDR_RANGE))
	{
		OSPCIReleaseAddrRange(psSysSpecData->hSGXPCI, CEDARVIEW_ADDR_RANGE_INDEX);
	}

	if (SYS_SPECIFIC_DATA_TEST(psSysSpecData, SYS_SPECIFIC_DATA_PCI_ACQUIRE_DEV))
	{
		OSPCIReleaseDev(psSysSpecData->hSGXPCI);
	}
}

/*******************************************************************************
 * Generate a device version string
 *
 * @Input	psSysData :	System data
 *
 * @Return	PVRSRV_ERROR
******************************************************************************/
#define VERSION_STR_MAX_LEN_TEMPLATE "SGX revision = 000.000.000"
PVRSRV_ERROR SysCreateVersionString(SYS_DATA *psSysData, SGX_DEVICE_MAP *psSGXDeviceMap)
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

/*******************************************************************************
 * Free the device version string
 *
 * @Input	psSysData :	System data
 *
 * @Return	none
******************************************************************************/
IMG_VOID SysFreeVersionString(SYS_DATA *psSysData)
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

#if defined(PVR_LINUX_USING_WORKQUEUES)

static PVRSRV_ERROR PowerLockWrap(SYS_SPECIFIC_DATA *psSysSpecData, IMG_BOOL bTryLock)
{
	if (!in_interrupt())
	{
		if (bTryLock)
		{
			int locked = mutex_trylock(&psSysSpecData->sPowerLock);
			if (locked == 0)
			{
				return PVRSRV_ERROR_RETRY;
			}
		}
		else
		{
			mutex_lock(&psSysSpecData->sPowerLock);
		}
	}

	return PVRSRV_OK;
}

static IMG_VOID PowerLockUnwrap(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (!in_interrupt())
	{
		mutex_unlock(&psSysSpecData->sPowerLock);
	}
}

PVRSRV_ERROR SysPowerLockWrap(IMG_BOOL bTryLock)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	return PowerLockWrap(psSysData->pvSysSpecificData, bTryLock);
}

IMG_VOID SysPowerLockUnwrap(IMG_VOID)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	PowerLockUnwrap(psSysData->pvSysSpecificData);
}

IMG_VOID SysPowerMaybeInit(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	mutex_init(&psSysSpecData->sPowerLock);
}

#endif /* defined(PVR_LINUX_USING_WORKQUEUES) */
