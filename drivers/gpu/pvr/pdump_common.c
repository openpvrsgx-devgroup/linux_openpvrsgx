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

#if defined(PDUMP)
#include "services_headers.h"
#include "pdump_km.h"

#if !defined(PDUMP_TEMP_BUFFER_SIZE)
#define PDUMP_TEMP_BUFFER_SIZE (64 * 1024L)
#endif

#define	MIN(x, y)		(((x) < (y)) ? (x) : (y))
#define	PTR_PLUS(t, p, x)	((t *)(((char *)(p)) + (x)))
#define	VPTR_PLUS(p, x)		PTR_PLUS(void, p, x)
#define	VPTR_INC(p, x)		(p = VPTR_PLUS(p, x))
#define MAX_PDUMP_MMU_CONTEXTS	10
static void *gpvTempBuffer;
static void *ghTempBufferBlockAlloc;
static u16 gui16MMUContextUsage;

static void *GetTempBuffer(void)
{
	if (gpvTempBuffer == NULL) {
		enum PVRSRV_ERROR eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						      PDUMP_TEMP_BUFFER_SIZE,
						      &gpvTempBuffer,
						      &ghTempBufferBlockAlloc);
		if (eError != PVRSRV_OK)
			PVR_DPF(PVR_DBG_ERROR,
				 "GetTempBuffer: OSAllocMem failed: %d",
				 eError);
	}

	return gpvTempBuffer;
}

static void FreeTempBuffer(void)
{
	if (gpvTempBuffer != NULL) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, PDUMP_TEMP_BUFFER_SIZE,
			     gpvTempBuffer, ghTempBufferBlockAlloc);
		gpvTempBuffer = NULL;
	}
}

void PDumpInitCommon(void)
{
	(void)GetTempBuffer();
	PDumpInit();
}

void PDumpDeInitCommon(void)
{
	FreeTempBuffer();
	PDumpDeInit();
}

enum PVRSRV_ERROR PDumpMemUM(struct PVRSRV_PER_PROCESS_DATA *psPerProc,
			     void *pvAltLinAddrUM, void *pvLinAddrUM,
			     struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
			     u32 ui32Offset, u32 ui32Bytes, u32 ui32Flags,
			     void *hUniqueTag)
{
	void *pvAddrUM;
	void *pvAddrKM;
	u32 ui32BytesDumped;
	u32 ui32CurrentOffset;

	if (psMemInfo->pvLinAddrKM != NULL && pvAltLinAddrUM == NULL)
		return PDumpMemKM(NULL, psMemInfo, ui32Offset, ui32Bytes,
				  ui32Flags, hUniqueTag);

	pvAddrUM = (pvAltLinAddrUM != NULL) ? pvAltLinAddrUM :
			((pvLinAddrUM != NULL) ? VPTR_PLUS(pvLinAddrUM,
							  ui32Offset) : NULL);

	pvAddrKM = GetTempBuffer();

	PVR_ASSERT(pvAddrUM != NULL && pvAddrKM != NULL);
	if (pvAddrUM == NULL || pvAddrKM == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "PDumpMemUM: Nothing to dump");
		return PVRSRV_ERROR_GENERIC;
	}

	if (ui32Bytes > PDUMP_TEMP_BUFFER_SIZE)
		PDumpCommentWithFlags(ui32Flags,
		"Dumping 0x%8.8lx bytes of memory, in blocks of 0x%8.8lx bytes",
				      ui32Bytes, (u32) PDUMP_TEMP_BUFFER_SIZE);

	ui32CurrentOffset = ui32Offset;
	for (ui32BytesDumped = 0; ui32BytesDumped < ui32Bytes;) {
		enum PVRSRV_ERROR eError;
		u32 ui32BytesToDump =
		    MIN(PDUMP_TEMP_BUFFER_SIZE, ui32Bytes - ui32BytesDumped);

		eError = OSCopyFromUser(psPerProc,
					pvAddrKM, pvAddrUM, ui32BytesToDump);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
			"PDumpMemUM: OSCopyFromUser failed (%d), eError");
			return PVRSRV_ERROR_GENERIC;
		}

		eError = PDumpMemKM(pvAddrKM, psMemInfo, ui32CurrentOffset,
				    ui32BytesToDump, ui32Flags, hUniqueTag);

		if (eError != PVRSRV_OK) {
			if (ui32BytesDumped != 0)
				PVR_DPF(PVR_DBG_ERROR,
					 "PDumpMemUM: PDumpMemKM failed (%d)",
					 eError);
			PVR_ASSERT(ui32BytesDumped == 0);
			return eError;
		}

		VPTR_INC(pvAddrUM, ui32BytesToDump);
		ui32CurrentOffset += ui32BytesToDump;
		ui32BytesDumped += ui32BytesToDump;
	}

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR _PdumpAllocMMUContext(u32 *pui32MMUContextID)
{
	u32 i;

	for (i = 0; i < MAX_PDUMP_MMU_CONTEXTS; i++)
		if ((gui16MMUContextUsage & (1UL << i)) == 0) {
			gui16MMUContextUsage |= 1UL << i;
			*pui32MMUContextID = i;
			return PVRSRV_OK;
		}

	PVR_DPF(PVR_DBG_ERROR,
		 "_PdumpAllocMMUContext: no free MMU context ids");

	return PVRSRV_ERROR_GENERIC;
}

static enum PVRSRV_ERROR _PdumpFreeMMUContext(u32 ui32MMUContextID)
{
	if (ui32MMUContextID < MAX_PDUMP_MMU_CONTEXTS) {

		gui16MMUContextUsage &= ~(1UL << ui32MMUContextID);
		return PVRSRV_OK;
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "_PdumpFreeMMUContext: MMU context ids invalid");

	return PVRSRV_ERROR_GENERIC;
}

enum PVRSRV_ERROR PDumpSetMMUContext(enum PVRSRV_DEVICE_TYPE eDeviceType,
				     char *pszMemSpace, u32 *pui32MMUContextID,
				     u32 ui32MMUType, void *hUniqueTag1,
				     void *pvPDCPUAddr)
{
	u8 *pui8LinAddr = (u8 *) pvPDCPUAddr;
	struct IMG_CPU_PHYADDR sCpuPAddr;
	struct IMG_DEV_PHYADDR sDevPAddr;
	u32 ui32MMUContextID;
	enum PVRSRV_ERROR eError;

	eError = _PdumpAllocMMUContext(&ui32MMUContextID);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PDumpSetMMUContext: _PdumpAllocMMUContext failed: %d",
			 eError);
		return eError;
	}

	sCpuPAddr = OSMapLinToCPUPhys(pui8LinAddr);
	sDevPAddr = SysCpuPAddrToDevPAddr(eDeviceType, sCpuPAddr);

	sDevPAddr.uiAddr &= ~PVRSRV_4K_PAGE_SIZE;

	PDumpComment("Set MMU Context\r\n");

	PDumpComment("MMU :%s:v%d %d :%s:PA_%8.8lX%8.8lX\r\n",
		     pszMemSpace, ui32MMUContextID, ui32MMUType, pszMemSpace,
		     hUniqueTag1, sDevPAddr.uiAddr);

	*pui32MMUContextID = ui32MMUContextID;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PDumpClearMMUContext(enum PVRSRV_DEVICE_TYPE eDeviceType,
				       char *pszMemSpace,
				       u32 ui32MMUContextID, u32 ui32MMUType)
{
	enum PVRSRV_ERROR eError;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	PDumpComment("Clear MMU Context\r\n");

	PDumpComment("MMU :%s:v%d %d\r\n",
		     pszMemSpace, ui32MMUContextID, ui32MMUType);

	eError = _PdumpFreeMMUContext(ui32MMUContextID);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"PDumpClearMMUContext: _PdumpFreeMMUContext failed: %d",
			 eError);
		return eError;
	}

	return PVRSRV_OK;
}

#endif
