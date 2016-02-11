/*************************************************************************/ /*!
@Title          bufferclass_example kernel driver
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the IMG POWERVR
 Services driver with a 3rd Party 'buffer device'.  It is NOT a specification for
 a 'buffer device' driver, rather a specification to extend the API for a
 pre-existing driver for the 'buffer device' hardware.

 The 3rd party driver interface provides IMG POWERVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying buffer device hardware, 
 allowing the client drivers to indirectly control the buffer device hardware
 and access its associated memory.
 
 Functions of the API include
 - query buffer device surface attributes (width, height, stride, pixel format, 
 CPU physical and virtual address)

 Note: having queried surface attributes the client drivers are able to map the
 buffer device memory to any IMG POWERVR Services device by calling
 PVRSRVMapDeviceClassMemory with the buffer device surface handle.

 This code is intended to be an example of how a pre-existing buffer device
 driver may be extended to support the 3rd Party buffer device interface to 
 POWERVR Services
 - IMG is not providing a buffer device driver implementation.
 **************************************************************************/
#if defined(__linux__)
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "bufferclass_example.h"



#define BUFFERCLASS_DEVICE_NAME "Example Bufferclass Device (SW)"

/* top level 'hook ptr' */
static void *gpvAnchor = NULL;
static PFN_BC_GET_PVRJTABLE pfnGetPVRJTable = IMG_NULL;

/*
	Kernel services is a kernel module and must be loaded first.
	This driver is also a kernel module and must be loaded after the pvr services module.
	This driver should be able to retrieve the
	address of the services PVRGetBufferClassJTable from (the already loaded)
	kernel services module.
*/

/* returns anchor pointer */
BC_EXAMPLE_DEVINFO * GetAnchorPtr(void)
{
	return (BC_EXAMPLE_DEVINFO *)gpvAnchor;
}

/* sets anchor pointer */
static void SetAnchorPtr(BC_EXAMPLE_DEVINFO *psDevInfo)
{
	gpvAnchor = (void *)psDevInfo;
}


/* Open device function, called from services */
static PVRSRV_ERROR OpenBCDevice(IMG_UINT32 ui32DeviceID, IMG_HANDLE *phDevice)
{
	BC_EXAMPLE_DEVINFO *psDevInfo;
	
	/*
		bufferclass_example manages only one BufferClass device
		therefore there is no need to track ID numbers.
	*/
	UNREFERENCED_PARAMETER(ui32DeviceID);

	psDevInfo = GetAnchorPtr();

	/* return handle to the devinfo */
	*phDevice = (IMG_HANDLE)psDevInfo;

	return (PVRSRV_OK);
}


/* Close device function, called from services */
static PVRSRV_ERROR CloseBCDevice(IMG_UINT32 ui32DeviceID, IMG_HANDLE hDevice)
{
	UNREFERENCED_PARAMETER(hDevice);

	return (PVRSRV_OK);
}

/* Passes in the sync data for a buffer, and returns the handle */
/* called from services */
static PVRSRV_ERROR GetBCBuffer(IMG_HANDLE          hDevice,
                                IMG_UINT32          ui32BufferNumber,
                                PVRSRV_SYNC_DATA   *psSyncData,
                                IMG_HANDLE         *phBuffer)
{
	BC_EXAMPLE_DEVINFO	*psDevInfo;

	if(!hDevice || !phBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (BC_EXAMPLE_DEVINFO*)hDevice;

	if( ui32BufferNumber < psDevInfo->sBufferInfo.ui32BufferCount )
	{
		psDevInfo->psSystemBuffer[ui32BufferNumber].psSyncData = psSyncData;
		*phBuffer = (IMG_HANDLE)&psDevInfo->psSystemBuffer[ui32BufferNumber];
	}
	else
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	return (PVRSRV_OK);
}


/* get buffer info function, called from services */
static PVRSRV_ERROR GetBCInfo(IMG_HANDLE hDevice, BUFFER_INFO *psBCInfo)
{
	BC_EXAMPLE_DEVINFO	*psDevInfo;

	if(!hDevice || !psBCInfo)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (BC_EXAMPLE_DEVINFO*)hDevice;

	*psBCInfo = psDevInfo->sBufferInfo;

	return (PVRSRV_OK);
}


/* get buffer address function, called from services */
static PVRSRV_ERROR GetBCBufferAddr(IMG_HANDLE      hDevice,
                                    IMG_HANDLE      hBuffer,
                                    IMG_SYS_PHYADDR **ppsSysAddr,
                                    IMG_UINT32      *pui32ByteSize,
                                    IMG_VOID        **ppvCpuVAddr,
                                    IMG_HANDLE      *phOSMapInfo,
                                    IMG_BOOL        *pbIsContiguous,
                                    IMG_UINT32      *pui32TilingStride)
{
	BC_EXAMPLE_BUFFER *psBuffer;

	PVR_UNREFERENCED_PARAMETER(pui32TilingStride);

	if(!hDevice || !hBuffer || !ppsSysAddr || !pui32ByteSize)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psBuffer = (BC_EXAMPLE_BUFFER *) hBuffer;

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

	return (PVRSRV_OK);
}


/* 
 *  Register the device with services module srvkm.
 *  This should only be done once at boot time.
 */
BCE_ERROR BC_Example_Register(void)
{
	BC_EXAMPLE_DEVINFO	*psDevInfo;

	/*
		- connect to services
		- register with services
		- allocate and setup private data structure
	*/


	/*
		in kernel driver, data structures must be anchored to something for subsequent retrieval
		this may be a single global pointer or TLS or something else - up to you
		call API to retrieve this ptr
	*/

	/*
		get the anchor pointer 
	*/
	psDevInfo = GetAnchorPtr();

	if (psDevInfo == NULL)
	{
		/* allocate device info. structure */
		psDevInfo = (BC_EXAMPLE_DEVINFO *)BCAllocKernelMem(sizeof(BC_EXAMPLE_DEVINFO));

		if(!psDevInfo)
		{
			return (BCE_ERROR_OUT_OF_MEMORY);/* failure */
		}

		/* set the top-level anchor */
		SetAnchorPtr((void*)psDevInfo);

		/* set ref count */
		psDevInfo->ulRefCount = 0;

	
		if(BCOpenPVRServices(&psDevInfo->hPVRServices) != BCE_OK)
		{
			return (BCE_ERROR_INIT_FAILURE);/* failure */
		}
		if(BCGetLibFuncAddr (psDevInfo->hPVRServices, "PVRGetBufferClassJTable", &pfnGetPVRJTable) != BCE_OK)
		{
			return (BCE_ERROR_INIT_FAILURE);/* failure */
		}

		/* got the kernel services function table */
		if(!(*pfnGetPVRJTable)(&psDevInfo->sPVRJTable))
		{
			return (BCE_ERROR_INIT_FAILURE);/* failure */
		}

		/*
			Setup the devinfo
		*/

		psDevInfo->ulNumBuffers = 0;

		psDevInfo->psSystemBuffer = BCAllocKernelMem(sizeof(BC_EXAMPLE_BUFFER) * BC_EXAMPLE_NUM_BUFFERS);

		if(!psDevInfo->psSystemBuffer)
		{
			return (BCE_ERROR_OUT_OF_MEMORY);/* failure */
		}

		/* Setup Buffer Info */
		psDevInfo->sBufferInfo.pixelformat        = PVRSRV_PIXEL_FORMAT_UNKNOWN;
		psDevInfo->sBufferInfo.ui32Width          = 0;
		psDevInfo->sBufferInfo.ui32Height         = 0;
		psDevInfo->sBufferInfo.ui32ByteStride     = 0;
		psDevInfo->sBufferInfo.ui32BufferDeviceID = BC_EXAMPLE_DEVICEID;
		psDevInfo->sBufferInfo.ui32Flags          = 0;
		psDevInfo->sBufferInfo.ui32BufferCount    = (IMG_UINT32)psDevInfo->ulNumBuffers;
		
		strncpy(psDevInfo->sBufferInfo.szDeviceName, BUFFERCLASS_DEVICE_NAME, MAX_BUFFER_DEVICE_NAME_SIZE);

		/*
			Bsetup the BC Jtable so SRVKM can call into this driver
		*/
		psDevInfo->sBCJTable.ui32TableSize    = sizeof(PVRSRV_BC_SRV2BUFFER_KMJTABLE);
		psDevInfo->sBCJTable.pfnOpenBCDevice  = OpenBCDevice;
		psDevInfo->sBCJTable.pfnCloseBCDevice = CloseBCDevice;
		psDevInfo->sBCJTable.pfnGetBCBuffer   = GetBCBuffer;
		psDevInfo->sBCJTable.pfnGetBCInfo     = GetBCInfo;
		psDevInfo->sBCJTable.pfnGetBufferAddr = GetBCBufferAddr;


		/* register device with services and retrieve device index */
		/* This example only registers 1 device, but for multiple buffer streams, register more devices */
		if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterBCDevice (&psDevInfo->sBCJTable,
															(IMG_UINT32*)&psDevInfo->ulDeviceID ) != PVRSRV_OK)
		{
			return (BCE_ERROR_DEVICE_REGISTER_FAILED);/* failure */
		}
	}

	/* increment the ref count */
	psDevInfo->ulRefCount++;

	/* return success */
	return (BCE_OK);
}

/* 
 *  Unregister the device with services module srvkm.
 */
BCE_ERROR BC_Example_Unregister(void)
{
	BC_EXAMPLE_DEVINFO *psDevInfo;

	psDevInfo = GetAnchorPtr();

	/* check DevInfo has been setup */
	if (psDevInfo == NULL)
	{
		return (BCE_ERROR_GENERIC);/* failure */
	}
	/* decrement ref count */
	psDevInfo->ulRefCount--;

	if (psDevInfo->ulRefCount == 0)
	{
		/* all references gone - de-init device information */
		PVRSRV_BC_BUFFER2SRV_KMJTABLE	*psJTable = &psDevInfo->sPVRJTable;


		/* Remove the device from kernel services device register */
		if (psJTable->pfnPVRSRVRemoveBCDevice(psDevInfo->ulDeviceID) != PVRSRV_OK)
		{
			return (BCE_ERROR_GENERIC);/* failure */
		}

		if (BCClosePVRServices(psDevInfo->hPVRServices) != BCE_OK)
		{
			psDevInfo->hPVRServices = NULL;
			return (BCE_ERROR_GENERIC);/* failure */
		}

		if (psDevInfo->psSystemBuffer)
		{
			BCFreeKernelMem(psDevInfo->psSystemBuffer);
		}

		/* de-allocate data structure */
		BCFreeKernelMem(psDevInfo);

		/* clear the top-level anchor */
		SetAnchorPtr(NULL);
	}

	/* return success */
	return (BCE_OK);
}


/* 
 * Create shared buffers. 
 */
BCE_ERROR BC_Example_Buffers_Create(void)
{
	BC_EXAMPLE_DEVINFO  *psDevInfo;
	unsigned long        i;
#if !defined(BC_DISCONTIG_BUFFERS)
	IMG_CPU_PHYADDR      sSystemBufferCPUPAddr;
#endif
	PVRSRV_PIXEL_FORMAT  pixelformat    = BC_EXAMPLE_PIXELFORMAT;
	static IMG_UINT32    ui32Width      = BC_EXAMPLE_WIDTH;
	static IMG_UINT32    ui32Height     = BC_EXAMPLE_HEIGHT;
	static IMG_UINT32    ui32ByteStride = BC_EXAMPLE_STRIDE;

	IMG_UINT32 ui32MaxWidth = 320 * 4;

	/*
		get the anchor pointer 
	*/
	psDevInfo = GetAnchorPtr();
	if (psDevInfo == NULL)
	{
		/* 
		 * This device was not correctly registered/created. 
		 */
		return (BCE_ERROR_DEVICE_REGISTER_FAILED);
	}
	if (psDevInfo->ulNumBuffers)
	{
		/* Buffers already allocated */
		return (BCE_ERROR_GENERIC);
	}
		
	/* Setup Buffer Info */
	psDevInfo->sBufferInfo.pixelformat        = BC_EXAMPLE_PIXELFORMAT;
	psDevInfo->sBufferInfo.ui32Width          = ui32Width;
	psDevInfo->sBufferInfo.ui32Height         = ui32Height;
	psDevInfo->sBufferInfo.ui32ByteStride     = ui32ByteStride;
	psDevInfo->sBufferInfo.ui32BufferDeviceID = BC_EXAMPLE_DEVICEID;
	psDevInfo->sBufferInfo.ui32Flags          = PVRSRV_BC_FLAGS_YUVCSC_FULL_RANGE | PVRSRV_BC_FLAGS_YUVCSC_BT601;

	for(i=psDevInfo->ulNumBuffers; i < BC_EXAMPLE_NUM_BUFFERS; i++)
	{
		unsigned long ulSize = (unsigned long)(ui32Height * ui32ByteStride);

		if(psDevInfo->sBufferInfo.pixelformat == PVRSRV_PIXEL_FORMAT_NV12)
		{
			/* Second plane is quarter size, but 2bytes per pixel */
			ulSize += ((ui32ByteStride >> 1) * (ui32Height >> 1) << 1);
		}
		else if(psDevInfo->sBufferInfo.pixelformat == PVRSRV_PIXEL_FORMAT_YV12)
		{
			/* Second plane is quarter size, but 1byte per pixel */
			ulSize += (ui32ByteStride >> 1) * (ui32Height >> 1);
			
			/* third plane is quarter size, but 1byte per pixel */
			ulSize += (ui32ByteStride >> 1) * (ui32Height >> 1);
		}

#if defined(BC_DISCONTIG_BUFFERS)
		if (BCAllocDiscontigMemory(ulSize,
								   &psDevInfo->psSystemBuffer[i].hMemHandle,
								   &psDevInfo->psSystemBuffer[i].sCPUVAddr,
								   &psDevInfo->psSystemBuffer[i].psSysAddr) != BCE_OK)
		{
			break;
		}
#else
		/* Setup system buffer */
		if (BCAllocContigMemory(ulSize,
		                        &psDevInfo->psSystemBuffer[i].hMemHandle,
		                        &psDevInfo->psSystemBuffer[i].sCPUVAddr,
		                        &sSystemBufferCPUPAddr) != BCE_OK)
		{
			break;
		}
		psDevInfo->psSystemBuffer[i].sSysAddr = CpuPAddrToSysPAddrBC(sSystemBufferCPUPAddr);
		psDevInfo->psSystemBuffer[i].sPageAlignSysAddr.uiAddr = (psDevInfo->psSystemBuffer[i].sSysAddr.uiAddr & 0xFFFFF000);
#endif

		psDevInfo->ulNumBuffers++;

		psDevInfo->psSystemBuffer[i].ulSize = ulSize;
		psDevInfo->psSystemBuffer[i].psSyncData = NULL;
	}

	psDevInfo->sBufferInfo.ui32BufferCount = (IMG_UINT32)psDevInfo->ulNumBuffers;

	/*
		Bsetup the BC Jtable so SRVKM can call into this driver
	*/
	psDevInfo->sBCJTable.ui32TableSize    = sizeof(PVRSRV_BC_SRV2BUFFER_KMJTABLE);
	psDevInfo->sBCJTable.pfnOpenBCDevice  = OpenBCDevice;
	psDevInfo->sBCJTable.pfnCloseBCDevice = CloseBCDevice;
	psDevInfo->sBCJTable.pfnGetBCBuffer   = GetBCBuffer;
	psDevInfo->sBCJTable.pfnGetBCInfo     = GetBCInfo;
	psDevInfo->sBCJTable.pfnGetBufferAddr = GetBCBufferAddr;
	


	/* Update buffer's parameters for reconfiguration next time */
	if (ui32Width < ui32MaxWidth)
	{
		switch(pixelformat)
		{
		    case PVRSRV_PIXEL_FORMAT_NV12:
		    case PVRSRV_PIXEL_FORMAT_YV12:
			{
			    ui32Width += 320;
				ui32Height += 160;
				ui32ByteStride = ui32Width;
				break;
			}
		    case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_VYUY:
		    case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY:
		    case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV:
		    case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YVYU:
			{
			    ui32Width += 320;
				ui32Height += 160;
				ui32ByteStride = ui32Width*2;
				break;
			}
		    case PVRSRV_PIXEL_FORMAT_RGB565:
			{
			    ui32Width += 320;
				ui32Height += 160;
				ui32ByteStride = ui32Width*2;
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
		ui32Width      = BC_EXAMPLE_WIDTH;
		ui32Height     = BC_EXAMPLE_HEIGHT;
		ui32ByteStride = BC_EXAMPLE_STRIDE;
	}

	/* return success */
	return (BCE_OK);
}


/*
 * Destroy shared buffers.
 */
BCE_ERROR BC_Example_Buffers_Destroy(void)
{
	BC_EXAMPLE_DEVINFO *psDevInfo;
	unsigned long       i;

	psDevInfo = GetAnchorPtr();

	/* check DevInfo has been setup */
	if (psDevInfo == NULL)
	{
		/* 
		   This device was not correctly registered/created. 
		 */
		return (BCE_ERROR_DEVICE_REGISTER_FAILED);
	}

	/* 
	   Free all allocated surfaces 
	*/
	for(i = 0; i < psDevInfo->ulNumBuffers; i++)
	{
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
	psDevInfo->ulNumBuffers = 0;

	/* Reset buffer info */
	psDevInfo->sBufferInfo.pixelformat        = PVRSRV_PIXEL_FORMAT_UNKNOWN;
	psDevInfo->sBufferInfo.ui32Width          = 0;
	psDevInfo->sBufferInfo.ui32Height         = 0;
	psDevInfo->sBufferInfo.ui32ByteStride     = 0;
	psDevInfo->sBufferInfo.ui32BufferDeviceID = BC_EXAMPLE_DEVICEID;
	psDevInfo->sBufferInfo.ui32Flags          = 0;
	psDevInfo->sBufferInfo.ui32BufferCount    = (IMG_UINT32)psDevInfo->ulNumBuffers;

	/* return success */
	return (BCE_OK);
}


/* 
 * This function does both registration and buffer allocation at
 * boot time.
 */
BCE_ERROR  BC_Example_Init(void)
{
	BCE_ERROR eError;

	eError = BC_Example_Register();
	if (eError != BCE_OK)
	{
		return eError;
	}

	eError = BC_Example_Buffers_Create();
	if (eError != BCE_OK)
	{
		return eError;
	}

	return (BCE_OK);
}

/* 
 *	Destroy buffers and unregister device.
 */
BCE_ERROR BC_Example_Deinit(void)
{
	BCE_ERROR eError;

	eError = BC_Example_Buffers_Destroy();
	if (eError != BCE_OK)
	{
		return eError;
	}

	eError = BC_Example_Unregister();
	if (eError != BCE_OK)
	{
		return eError;
	}

	return (BCE_OK);
}

/******************************************************************************
 End of file (bufferclass_example.c)
******************************************************************************/
