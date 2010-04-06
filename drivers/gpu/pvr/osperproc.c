/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful but, except
 * as otherwise stated in writing, without any warranty; without even the
 * implied warranty of merchantability or fitness for a particular purpose.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK
 *
 ******************************************************************************/

#include "services_headers.h"
#include "osperproc.h"

#include "env_perproc.h"
#include "proc.h"

enum PVRSRV_ERROR OSPerProcessPrivateDataInit(void **phOsPrivateData)
{
	enum PVRSRV_ERROR eError;
	void *hBlockAlloc;
	struct PVRSRV_ENV_PER_PROCESS_DATA *psEnvPerProc;

	eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			    sizeof(struct PVRSRV_ENV_PER_PROCESS_DATA),
			    phOsPrivateData, &hBlockAlloc);

	if (eError != PVRSRV_OK) {
		*phOsPrivateData = NULL;

		PVR_DPF(PVR_DBG_ERROR, "%s: OSAllocMem failed (%d)", __func__,
			 eError);
		return eError;
	}

	psEnvPerProc = (struct PVRSRV_ENV_PER_PROCESS_DATA *)*phOsPrivateData;
	OSMemSet(psEnvPerProc, 0, sizeof(*psEnvPerProc));

	psEnvPerProc->hBlockAlloc = hBlockAlloc;

	LinuxMMapPerProcessConnect(psEnvPerProc);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR OSPerProcessPrivateDataDeInit(void *hOsPrivateData)
{
	struct PVRSRV_ENV_PER_PROCESS_DATA *psEnvPerProc;

	if (hOsPrivateData == NULL)
		return PVRSRV_OK;

	psEnvPerProc = (struct PVRSRV_ENV_PER_PROCESS_DATA *)hOsPrivateData;

	LinuxMMapPerProcessDisconnect(psEnvPerProc);

	RemovePerProcessProcDir(psEnvPerProc);

	OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			   sizeof(struct PVRSRV_ENV_PER_PROCESS_DATA),
			   hOsPrivateData, psEnvPerProc->hBlockAlloc);
	return PVRSRV_OK;
}

enum PVRSRV_ERROR OSPerProcessSetHandleOptions(struct PVRSRV_HANDLE_BASE
					       *psHandleBase)
{
	return LinuxMMapPerProcessHandleOptions(psHandleBase);
}
