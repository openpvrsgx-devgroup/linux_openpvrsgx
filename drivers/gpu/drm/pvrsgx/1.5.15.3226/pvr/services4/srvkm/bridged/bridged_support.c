/**********************************************************************
 Copyright (c) Imagination Technologies Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ******************************************************************************/

#include "img_defs.h"
#include "servicesint.h"
#include "bridged_support.h"


PVRSRV_ERROR
PVRSRVLookupOSMemHandle(PVRSRV_HANDLE_BASE *psHandleBase, IMG_HANDLE *phOSMemHandle, IMG_HANDLE hMHandle)
{
	IMG_HANDLE hMHandleInt;
	PVRSRV_HANDLE_TYPE eHandleType;
	PVRSRV_ERROR eError;


	eError = PVRSRVLookupHandleAnyType(psHandleBase, &hMHandleInt,
							  &eHandleType,
							  hMHandle);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}

	switch(eHandleType)
	{
#if defined(PVR_SECURE_HANDLES)
		case PVRSRV_HANDLE_TYPE_MEM_INFO:
		case PVRSRV_HANDLE_TYPE_MEM_INFO_REF:
		case PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO:
		{
			PVRSRV_KERNEL_MEM_INFO *psMemInfo = (PVRSRV_KERNEL_MEM_INFO *)hMHandleInt;

			*phOSMemHandle = psMemInfo->sMemBlk.hOSMemHandle;

			break;
		}
		case PVRSRV_HANDLE_TYPE_SYNC_INFO:
		{
			PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)hMHandleInt;
			PVRSRV_KERNEL_MEM_INFO *psMemInfo = psSyncInfo->psSyncDataMemInfoKM;

			*phOSMemHandle = psMemInfo->sMemBlk.hOSMemHandle;

			break;
		}
		case  PVRSRV_HANDLE_TYPE_SOC_TIMER:
		{
			*phOSMemHandle = (IMG_VOID *)hMHandleInt;
			break;
		}
#else
		case  PVRSRV_HANDLE_TYPE_NONE:
			*phOSMemHandle = (IMG_VOID *)hMHandleInt;
			break;
#endif
		default:
			return PVRSRV_ERROR_BAD_MAPPING;
	}

	return PVRSRV_OK;
}
