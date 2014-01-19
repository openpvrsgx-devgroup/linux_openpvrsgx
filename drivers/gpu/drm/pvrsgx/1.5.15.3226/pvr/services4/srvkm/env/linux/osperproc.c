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

#include "services_headers.h"
#include "osperproc.h"

#include "env_perproc.h"
#include "proc.h"

extern IMG_UINT32 gui32ReleasePID;

PVRSRV_ERROR OSPerProcessPrivateDataInit(IMG_HANDLE *phOsPrivateData)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE hBlockAlloc;
	PVRSRV_ENV_PER_PROCESS_DATA *psEnvPerProc;

	eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				sizeof(PVRSRV_ENV_PER_PROCESS_DATA),
				phOsPrivateData,
				&hBlockAlloc,
				"Environment per Process Data");

	if (eError != PVRSRV_OK)
	{
		*phOsPrivateData = IMG_NULL;

		PVR_DPF((PVR_DBG_ERROR, "%s: OSAllocMem failed (%d)", __FUNCTION__, eError));
		return eError;
	}

	psEnvPerProc = (PVRSRV_ENV_PER_PROCESS_DATA *)*phOsPrivateData;
	OSMemSet(psEnvPerProc, 0, sizeof(*psEnvPerProc));

	psEnvPerProc->hBlockAlloc = hBlockAlloc;


	LinuxMMapPerProcessConnect(psEnvPerProc);

#if defined(SUPPORT_DRI_DRM) && defined(PVR_SECURE_DRM_AUTH_EXPORT)

	INIT_LIST_HEAD(&psEnvPerProc->sDRMAuthListHead);
#endif

	return PVRSRV_OK;
}

PVRSRV_ERROR OSPerProcessPrivateDataDeInit(IMG_HANDLE hOsPrivateData)
{
	PVRSRV_ERROR eError;
	PVRSRV_ENV_PER_PROCESS_DATA *psEnvPerProc;

	if (hOsPrivateData == IMG_NULL)
	{
		return PVRSRV_OK;
	}

	psEnvPerProc = (PVRSRV_ENV_PER_PROCESS_DATA *)hOsPrivateData;


	LinuxMMapPerProcessDisconnect(psEnvPerProc);


	RemovePerProcessProcDir(psEnvPerProc);

	eError = OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				sizeof(PVRSRV_ENV_PER_PROCESS_DATA),
				hOsPrivateData,
				psEnvPerProc->hBlockAlloc);


	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: OSFreeMem failed (%d)", __FUNCTION__, eError));
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR OSPerProcessSetHandleOptions(PVRSRV_HANDLE_BASE *psHandleBase)
{
	return LinuxMMapPerProcessHandleOptions(psHandleBase);
}

IMG_HANDLE LinuxTerminatingProcessPrivateData(IMG_VOID)
{
	if(!gui32ReleasePID)
		return NULL;
	return PVRSRVPerProcessPrivateData(gui32ReleasePID);
}
