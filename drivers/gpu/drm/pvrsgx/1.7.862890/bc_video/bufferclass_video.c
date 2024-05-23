/***************************************************************************
 *
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ******************************************************************************/

#if defined(__linux__)
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "bufferclass_video.h"
#include "bufferclass_video_linux.h"

#define VBUFFERCLASS_DEVICE_NAME "Video Bufferclass Device"
#define CBUFFERCLASS_DEVICE_NAME "Camera Bufferclass Device"

static void *gpvAnchorVideo[BC_VIDEO_DEVICE_MAX_ID];

static void *gpcAnchor;
static PFN_BC_GET_PVRJTABLE pfnGetPVRJTable = IMG_NULL;

BC_VIDEO_DEVINFO *
GetAnchorPtr (int id)
{
    BC_VIDEO_DEVINFO *AnchorPtr = NULL;
    if (id < BC_VIDEO_DEVICE_MAX_ID)
        AnchorPtr = gpvAnchorVideo[id];
    else if (id == BC_CAMERA_DEVICEID)
        AnchorPtr = gpcAnchor;
    return AnchorPtr;
}

static void
SetAnchorPtr (BC_VIDEO_DEVINFO * psDevInfo, int id)
{
    if (id < BC_VIDEO_DEVICE_MAX_ID)
        gpvAnchorVideo[id] = (void *) psDevInfo;
    else if (id == BC_CAMERA_DEVICEID)
        gpcAnchor = (void *) psDevInfo;
}

static PVRSRV_ERROR
OpenVBCDevice (IMG_UINT32 uDeviceID, IMG_HANDLE * phDevice)
{
    BC_VIDEO_DEVINFO *psDevInfo;
    int id;
    *phDevice = NULL;
    for(id = 0; id < BC_VIDEO_DEVICE_MAX_ID; id++){
        psDevInfo = GetAnchorPtr(id);
        if(psDevInfo != NULL && psDevInfo->ulDeviceID == uDeviceID){
            *phDevice = (IMG_HANDLE) psDevInfo;
            break;
        }
    }

    return (PVRSRV_OK);
}

static PVRSRV_ERROR OpenCBCDevice(IMG_UINT32 uDeviceID, IMG_HANDLE *phDevice)
{
    BC_VIDEO_DEVINFO *psDevInfo;

    UNREFERENCED_PARAMETER(uDeviceID);
    psDevInfo = GetAnchorPtr(BC_CAMERA_DEVICEID);

    *phDevice = (IMG_HANDLE)psDevInfo;

    return (PVRSRV_OK);
}

static PVRSRV_ERROR
CloseBCDevice (IMG_UINT32 uDeviceID, IMG_HANDLE hDevice)
{
    UNREFERENCED_PARAMETER (uDeviceID);
    UNREFERENCED_PARAMETER (hDevice);

    return (PVRSRV_OK);
}

static PVRSRV_ERROR
GetBCBuffer (IMG_HANDLE hDevice,
        IMG_UINT32 ui32BufferNumber,
        PVRSRV_SYNC_DATA * psSyncData, IMG_HANDLE * phBuffer)
{
    BC_VIDEO_DEVINFO *psDevInfo;

    if (!hDevice || !phBuffer)
    {
        return (PVRSRV_ERROR_INVALID_PARAMS);
    }

    psDevInfo = (BC_VIDEO_DEVINFO *) hDevice;

    if (ui32BufferNumber < psDevInfo->sBufferInfo.ui32BufferCount)
    {
        psDevInfo->psSystemBuffer[ui32BufferNumber].psSyncData = psSyncData;
        *phBuffer = (IMG_HANDLE) & psDevInfo->psSystemBuffer[ui32BufferNumber];
    }
    else
    {
        return (PVRSRV_ERROR_INVALID_PARAMS);
    }

    return (PVRSRV_OK);
}

static PVRSRV_ERROR
GetBCInfo (IMG_HANDLE hDevice, BUFFER_INFO * psBCInfo)
{
    BC_VIDEO_DEVINFO *psDevInfo;

    if (!hDevice || !psBCInfo)
    {
        return (PVRSRV_ERROR_INVALID_PARAMS);
    }

    psDevInfo = (BC_VIDEO_DEVINFO *) hDevice;

    *psBCInfo = psDevInfo->sBufferInfo;

    return (PVRSRV_OK);
}

static PVRSRV_ERROR
GetBCBufferAddr (IMG_HANDLE hDevice,
        IMG_HANDLE hBuffer,
        IMG_SYS_PHYADDR ** ppsSysAddr,
        IMG_UINT32 * pui32ByteSize,
        IMG_VOID ** ppvCpuVAddr,
        IMG_HANDLE * phOSMapInfo,
        IMG_BOOL * pbIsContiguous, IMG_UINT32 * pui32TilingStride)
{
    BC_VIDEO_BUFFER *psBuffer;

    UNREFERENCED_PARAMETER (pui32TilingStride);
    if (!hDevice || !hBuffer || !ppsSysAddr || !pui32ByteSize)
    {
        return (PVRSRV_ERROR_INVALID_PARAMS);
    }

    psBuffer = (BC_VIDEO_BUFFER *) hBuffer;

    *ppvCpuVAddr = psBuffer->sCPUVAddr;

    *phOSMapInfo = IMG_NULL;
    *pui32ByteSize = (IMG_UINT32) psBuffer->ulSize;

    *ppsSysAddr = psBuffer->psSysAddr;
    *pbIsContiguous = psBuffer->is_conti_addr;

    return (PVRSRV_OK);
}


BCE_ERROR
BC_Video_Register (int id)
{
    BC_VIDEO_DEVINFO *psDevInfo;

    psDevInfo = GetAnchorPtr (id);

    if (psDevInfo == NULL)
    {
        psDevInfo =
            (BC_VIDEO_DEVINFO *) BCAllocKernelMem (sizeof (BC_VIDEO_DEVINFO));

        if (!psDevInfo)
        {
            return (BCE_ERROR_OUT_OF_MEMORY);
        }

        SetAnchorPtr ((void *) psDevInfo, id);

        psDevInfo->ulRefCount = 0;

        if (BCOpenPVRServices (&psDevInfo->hPVRServices) != BCE_OK)
        {
            return (BCE_ERROR_INIT_FAILURE);
        }
        if (BCGetLibFuncAddr
                (psDevInfo->hPVRServices, "PVRGetBufferClassJTable",
                 &pfnGetPVRJTable) != BCE_OK)
        {
            return (BCE_ERROR_INIT_FAILURE);
        }

        if (!(*pfnGetPVRJTable) (&psDevInfo->sPVRJTable))
        {
            return (BCE_ERROR_INIT_FAILURE);
        }

        psDevInfo->ulNumBuffers = 0;

        psDevInfo->sBufferInfo.pixelformat = PVRSRV_PIXEL_FORMAT_UNKNOWN;
        psDevInfo->sBufferInfo.ui32Width = 0;
        psDevInfo->sBufferInfo.ui32Height = 0;
        psDevInfo->sBufferInfo.ui32ByteStride = 0;
        psDevInfo->sBufferInfo.ui32BufferDeviceID = id;
        psDevInfo->sBufferInfo.ui32Flags = 0;
        psDevInfo->sBufferInfo.ui32BufferCount =
            (IMG_UINT32) psDevInfo->ulNumBuffers;

        if (id < BC_VIDEO_DEVICE_MAX_ID)
        {
            strncpy (psDevInfo->sBufferInfo.szDeviceName,
                    VBUFFERCLASS_DEVICE_NAME, MAX_BUFFER_DEVICE_NAME_SIZE);
            psDevInfo->sBCJTable.pfnOpenBCDevice = OpenVBCDevice;
        }
        else if (id == BC_CAMERA_DEVICEID)
        {
            strncpy (psDevInfo->sBufferInfo.szDeviceName,
                    CBUFFERCLASS_DEVICE_NAME, MAX_BUFFER_DEVICE_NAME_SIZE);
            psDevInfo->sBCJTable.pfnOpenBCDevice = OpenCBCDevice;
        }

        psDevInfo->sBCJTable.ui32TableSize =
            sizeof (PVRSRV_BC_SRV2BUFFER_KMJTABLE);
        psDevInfo->sBCJTable.pfnCloseBCDevice = CloseBCDevice;
        psDevInfo->sBCJTable.pfnGetBCBuffer = GetBCBuffer;
        psDevInfo->sBCJTable.pfnGetBCInfo = GetBCInfo;
        psDevInfo->sBCJTable.pfnGetBufferAddr = GetBCBufferAddr;

        if (psDevInfo->sPVRJTable.
                pfnPVRSRVRegisterBCDevice (&psDevInfo->sBCJTable,
                    &psDevInfo->ulDeviceID) != PVRSRV_OK)
        {
            return (BCE_ERROR_DEVICE_REGISTER_FAILED);
        }
    }

    psDevInfo->ulRefCount++;

    return (BCE_OK);
}

BCE_ERROR
BC_Video_Unregister (int id)
{
    BC_VIDEO_DEVINFO *psDevInfo;

    psDevInfo = GetAnchorPtr (id);

    if (psDevInfo == NULL)
    {
        return (BCE_ERROR_GENERIC);
    }

    psDevInfo->ulRefCount--;

    if (psDevInfo->ulRefCount == 0)
    {
        PVRSRV_BC_BUFFER2SRV_KMJTABLE *psJTable = &psDevInfo->sPVRJTable;

        if (psJTable->pfnPVRSRVRemoveBCDevice (psDevInfo->ulDeviceID) !=
                PVRSRV_OK)
        {
            return (BCE_ERROR_GENERIC);
        }

        if (BCClosePVRServices (psDevInfo->hPVRServices) != BCE_OK)
        {
            psDevInfo->hPVRServices = NULL;
            return (BCE_ERROR_GENERIC);
        }

        if (psDevInfo->psSystemBuffer)
        {
            BCFreeKernelMem (psDevInfo->psSystemBuffer);
        }

        BCFreeKernelMem (psDevInfo);

        SetAnchorPtr (NULL, id);
    }
    return (BCE_OK);
}

BCE_ERROR
BC_Video_Init (int id)
{
    BCE_ERROR eError;

    eError = BC_Video_Register (id);
    if (eError != BCE_OK)
    {
        return eError;
    }

    return (BCE_OK);
}

BCE_ERROR
BC_Video_Deinit (int id)
{
    BCE_ERROR eError;

    BC_DestroyBuffers (id);

    eError = BC_Video_Unregister (id);
    if (eError != BCE_OK)
    {
        return eError;
    }

    return (BCE_OK);
}
