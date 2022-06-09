/***************************************************************************
 *
 * Copyright © 2010 Intel Corporation
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

#define MODULE_NAME hal.buf_class

#if defined(__linux__)
#include <linux/string.h>
#else
#include <string.h>
#endif
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#if 0
#if defined(LMA)
#include <linux/pci.h>
#else
#include <linux/dma-mapping.h>
#endif
#endif
#include <drm/drmP.h>
#include <drm/drm.h>
#include "emgd_drv.h"
#include "io.h"

#include "pvrmodule.h"
#include "emgd_bc.h"
/* 
** Device Name Space is for device name, steam tag and device state.
** Format
** [Name]...[Video ID][Device Status]
** From Device Name Space tail, 
** Device Status - one Byte
** Video ID - four Byte
** Device Name - variation according to device name set.
*/
#define TSBUFFERCLASS_DEVICE_NAME "BC Texture Stream Device"

unsigned int bc_video_id[BUFCLASS_DEVICE_MAX_ID];
unsigned int bc_video_id_usage[BUFCLASS_DEVICE_MAX_ID];

void *gp_bc_Anchor[BUFCLASS_DEVICE_MAX_ID];

static PFN_BC_GET_PVRJTABLE pfnGetPVRJTable = IMG_NULL;

BC_DEVINFO *GetAnchorPtr (int id){
    BC_DEVINFO *AnchorPtr = NULL;
    if (id < BUFCLASS_DEVICE_MAX_ID)
        AnchorPtr = gp_bc_Anchor[id];
    return AnchorPtr;
}

static void SetAnchorPtr(BC_DEVINFO *psDevInfo, int id) {
    if (id < BUFCLASS_DEVICE_MAX_ID) {
        gp_bc_Anchor[id] = (void *) psDevInfo;
    }    
}

static PVRSRV_ERROR OpenBCDevice (IMG_UINT32 uDeviceID, IMG_HANDLE * phDevice){
    BC_DEVINFO *psDevInfo = NULL;
	int i = 0;
	
	EMGD_TRACE_ENTER;
	if (NULL == phDevice) {
		EMGD_ERROR("Invliad Input parameter");
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	*phDevice = NULL;
	
	EMGD_DEBUG("Try to Open Device ID - %lu", uDeviceID);

	for (i = 0; i < BUFCLASS_DEVICE_MAX_ID; i++) {
		psDevInfo = GetAnchorPtr(i);
		if (NULL != psDevInfo && uDeviceID == psDevInfo->Device_ID) {
			*phDevice = (IMG_HANDLE)psDevInfo;
			EMGD_DEBUG("psDevInfo - 0x%lx", (unsigned long)psDevInfo);
			break;
		}
	}
	
    if (NULL == *phDevice) {
		EMGD_ERROR("Invliad Device ID - %lu", uDeviceID);
		return PVRSRV_ERROR_INVALID_DEVICE;
    }

	EMGD_TRACE_EXIT;
    return (PVRSRV_OK);
}

static PVRSRV_ERROR CloseBCDevice(IMG_UINT32 uDeviceID, IMG_HANDLE hDevice){
    UNREFERENCED_PARAMETER (uDeviceID);
    UNREFERENCED_PARAMETER (hDevice);

    return (PVRSRV_OK);
}

static PVRSRV_ERROR GetBCBuffer(IMG_HANDLE hDevice,
        IMG_UINT32 ui32BufferNumber,
        PVRSRV_SYNC_DATA * psSyncData, IMG_HANDLE * phBuffer){

    BC_DEVINFO *psDevInfo = NULL;
	PVRSRV_ERROR ret = PVRSRV_ERROR_INVALID_PARAMS;

	EMGD_TRACE_ENTER;

    if (NULL == hDevice || NULL == phBuffer){
		return (PVRSRV_ERROR_INVALID_PARAMS);
    }

    psDevInfo = (BC_DEVINFO *) hDevice;

	EMGD_DEBUG("Device - 0x%lx, Device ID - %lu, Buffer ID - %lu\n", 
		(unsigned long)psDevInfo, 
		psDevInfo->Device_ID,
		ui32BufferNumber);
	
    if (ui32BufferNumber < psDevInfo->sBufferInfo.ui32BufferCount) {
        psDevInfo->psSystemBuffer[ui32BufferNumber].psSyncData = psSyncData;
        *phBuffer = (IMG_HANDLE) &psDevInfo->psSystemBuffer[ui32BufferNumber];
		EMGD_DEBUG("Assign System Buffer address - 0x%lx to phBuffer", (unsigned long)(*phBuffer));
		ret = PVRSRV_OK;
    } else {
    	EMGD_ERROR("PVRSRV_ERROR_INVALID_PARAMS\n");
        ret= PVRSRV_ERROR_INVALID_PARAMS;
    }

	EMGD_TRACE_EXIT;

    return ret;
}

static PVRSRV_ERROR GetBCInfo(IMG_HANDLE hDevice, BUFFER_INFO *psBCInfo){
    BC_DEVINFO *psDevInfo = NULL;

	EMGD_TRACE_ENTER;

    if (!hDevice || !psBCInfo){
        return (PVRSRV_ERROR_INVALID_PARAMS);
    }

    psDevInfo = (BC_DEVINFO *)hDevice;

	EMGD_DEBUG("psDevInfo - 0x%lx, Device ID - %lu", (unsigned long)psDevInfo, psDevInfo->Device_ID);
	
    memcpy((void *)psBCInfo, (void *)&psDevInfo->sBufferInfo, sizeof(BUFFER_INFO));

	EMGD_TRACE_EXIT;

    return (PVRSRV_OK);
}

static PVRSRV_ERROR
GetBCBufferAddr (IMG_HANDLE hDevice,
        IMG_HANDLE hBuffer,
        IMG_SYS_PHYADDR ** ppsSysAddr,
        IMG_UINT32 * pui32ByteSize,
        IMG_VOID ** ppvCpuVAddr,
        IMG_HANDLE * phOSMapInfo,
        IMG_BOOL * pbIsContiguous, IMG_BOOL *pbMapped){
    BC_BUFFER *psBuffer;

	EMGD_TRACE_ENTER;

    if (NULL == hDevice || NULL == hBuffer){
        return (PVRSRV_ERROR_INVALID_PARAMS);
    }
        psBuffer = (BC_BUFFER *)hBuffer;
        
	if (IMG_NULL != pbMapped) {

		if(psBuffer->mapped)
			*pbMapped = IMG_TRUE;
		else
			*pbMapped = IMG_FALSE;
	}	

    
	EMGD_DEBUG("Buffer 0x%lx", (IMG_UINT32)psBuffer);
	if (NULL != ppsSysAddr) {
		*ppsSysAddr = psBuffer->psSysAddr;
	}
	if (NULL != pui32ByteSize) {
		*pui32ByteSize = (IMG_UINT32)psBuffer->ulSize;
	}
	if (NULL != ppvCpuVAddr) {
    	*ppvCpuVAddr = psBuffer->sCPUVAddr;
	}
	
	if (NULL != phOSMapInfo) {
    	*phOSMapInfo = IMG_NULL;
	}

    if (NULL != pbIsContiguous) {
    	*pbIsContiguous = psBuffer->is_conti_addr;
    }
	    
	EMGD_TRACE_EXIT;

    return (PVRSRV_OK);
}

static PVRSRV_ERROR GetBCBufferIdFromTag(IMG_HANDLE hDevice, IMG_UINT32 tag, IMG_HANDLE idx){
    BC_DEVINFO *psDevInfo = NULL;
	IMG_UINT32 i = 0;
	
	EMGD_TRACE_ENTER;

    if (NULL == hDevice || NULL == idx){
        return (PVRSRV_ERROR_INVALID_PARAMS);
    }

    psDevInfo = (BC_DEVINFO *)hDevice;

	EMGD_DEBUG("hDevice - 0x%lx, Device ID - %lu", (unsigned long)psDevInfo, psDevInfo->Device_ID);

	for (i = 0; i < psDevInfo->sBufferInfo.ui32BufferCount; i++) {
		if (psDevInfo->psSystemBuffer[i].tag == tag) {
			EMGD_DEBUG("Found the Buffer: Id - %lu, Tag - %lu", i, tag);
			*((IMG_UINT32 *) idx) = i;
			break;	
		}
	}

	EMGD_TRACE_EXIT;

    return (PVRSRV_OK);
}

emgd_error_t bc_ts_init(IMG_UINT32 id) {
    BC_DEVINFO *psDevInfo = NULL;

	EMGD_TRACE_ENTER;

    psDevInfo = GetAnchorPtr(id);

    if (psDevInfo == NULL){
    	EMGD_DEBUG("To Allocate Device with Index - %lu", id);
        psDevInfo = (BC_DEVINFO *)BCAllocKernelMem(sizeof(BC_DEVINFO));
		EMGD_DEBUG("psDevInfo - %lx", (unsigned long)psDevInfo);
        if (NULL == psDevInfo){
            return (EMGD_ERROR_OUT_OF_MEMORY);
        }
		memset((void *)psDevInfo, 0, sizeof(BC_DEVINFO));

		psDevInfo->Device_ID = id; /* device index amond device list */
		psDevInfo->sBufferInfo.ui32BufferDeviceID = id;
		
        SetAnchorPtr((void *)psDevInfo, id);

        if (BCOpenPVRServices(&psDevInfo->hPVRServices) != EMGD_OK){
            return (EMGD_ERROR_INIT_FAILURE);
        }
		/* Initialize PVR JTable Function */
        if (BCGetLibFuncAddr(psDevInfo->hPVRServices, "PVRGetBufferClassJTable",
                 &pfnGetPVRJTable) != EMGD_OK) {
            return (EMGD_ERROR_INIT_FAILURE);
        }

        if (!(*pfnGetPVRJTable)(&psDevInfo->sPVRJTable)){
            return (EMGD_ERROR_INIT_FAILURE);
        }
		if (TSBUFFERCLASS_DEV_NAME_LEN < 0) {
			EMGD_ERROR("BufferClass Device Name Space is too small!");
			return EMGD_ERROR_INIT_FAILURE;
		}
        strncpy(psDevInfo->sBufferInfo.szDeviceName, TSBUFFERCLASS_DEVICE_NAME, TSBUFFERCLASS_DEV_NAME_LEN);
		/* Initialize BC JTable */
        psDevInfo->sBCJTable.ui32TableSize = sizeof (PVRSRV_BC_SRV2BUFFER_KMJTABLE);

        psDevInfo->sBCJTable.pfnOpenBCDevice       = (PFN_OPEN_BC_DEVICE)OpenBCDevice;
        psDevInfo->sBCJTable.pfnCloseBCDevice      = (PFN_CLOSE_BC_DEVICE)CloseBCDevice;
        psDevInfo->sBCJTable.pfnGetBCBuffer        = (PFN_GET_BC_BUFFER)GetBCBuffer;
        psDevInfo->sBCJTable.pfnGetBCInfo          = (PFN_GET_BC_INFO)GetBCInfo;
        psDevInfo->sBCJTable.pfnGetBufferAddr      = (PFN_GET_BUFFER_ADDR)GetBCBufferAddr;
	psDevInfo->sBCJTable.pfnGetBufferIdFromTag = (PFN_GET_BUFFER_ID_FROM_TAG)GetBCBufferIdFromTag;

        if (PVRSRV_OK != psDevInfo->sPVRJTable.pfnPVRSRVRegisterBCDevice(&psDevInfo->sBCJTable,
                    &(psDevInfo->Device_ID))){
            return (EMGD_ERROR_DEVICE_REGISTER_FAILED);
        } else {
			EMGD_DEBUG("Got Device ID - %lu", psDevInfo->Device_ID);
		}
    } else {
		EMGD_ERROR("Duplicate register Device - %lu", psDevInfo->Device_ID);
    }

	EMGD_TRACE_EXIT;

    return (EMGD_OK);
}
emgd_error_t bc_ts_uninit(IMG_UINT32 id) {
    BC_DEVINFO *psDevInfo = NULL;
	int i = 0;

	EMGD_TRACE_ENTER;

    psDevInfo = (BC_DEVINFO *)GetAnchorPtr(id);

    if (psDevInfo == NULL){
        return (EMGD_ERROR_GENERIC);
    }
  	EMGD_DEBUG("To Unregister the Device: ID - %lu, RefCount - %lu, idx - %lu", 
  		psDevInfo->Device_ID,
  		psDevInfo->ulRefCount,
  		psDevInfo->sBufferInfo.ui32BufferDeviceID);

	if (id != psDevInfo->sBufferInfo.ui32BufferDeviceID) {
		EMGD_ERROR("Index mis-matcheh input idx vs. device idx: %lu vs. %lu", id, psDevInfo->sBufferInfo.ui32BufferDeviceID);
	}
	
	if (0 != psDevInfo->ulRefCount) {
    	psDevInfo->ulRefCount--;
	}

    if (1 /*psDevInfo->ulRefCount == 0*/){
        PVRSRV_BC_BUFFER2SRV_KMJTABLE *psJTable = &psDevInfo->sPVRJTable;

        if (psJTable->pfnPVRSRVRemoveBCDevice(psDevInfo->Device_ID) !=
                PVRSRV_OK) {
            return (EMGD_ERROR_GENERIC);
        }

        if (BCClosePVRServices(psDevInfo->hPVRServices) != EMGD_OK){
            psDevInfo->hPVRServices = NULL;
            return (EMGD_ERROR_GENERIC);
        }
		if (NULL != psDevInfo->psSystemBuffer) {
        	for (i = 0; i < BUFCLASS_BUFFER_MAX; i++) {
				bc_ts_free_bcbuffer(&(psDevInfo->psSystemBuffer[i]));
        	}
			BCFreeKernelMem(psDevInfo->psSystemBuffer);
		}
        BCFreeKernelMem(psDevInfo);

        SetAnchorPtr(NULL, id);
        bc_video_id_usage[id] = 0;
    }
	EMGD_TRACE_EXIT;

    return (EMGD_OK);
}
void bc_ts_free_bcbuffer(BC_BUFFER *bc_buf) {
	if (NULL == bc_buf) {
		return;
	}

	BCFreeKernelMem(bc_buf->psSysAddr);
	bc_buf->psSysAddr = NULL;
#if 0 /* Cause Kernel Error */	
	BCFreeKernelMem(bc_buf->psSyncData);
	bc_buf->psSyncData = NULL;
#endif	
	return;
}

void *BCAllocKernelMem (unsigned long ulSize){
    return vmalloc(ulSize); /* kmalloc*/
}

void BCFreeKernelMem(void *pvMem){
	if (NULL != pvMem) { /* kfree -> vfree*/
    	vfree(pvMem);
	}
}

