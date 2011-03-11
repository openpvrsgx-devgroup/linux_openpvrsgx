/*
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
 */

#include <asm/atomic.h>
#include <stdarg.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "pvrversion.h"
#include "sgxmmu.h"
#include "mm.h"

#include "pvr_pdump.h"
#include "pvr_pdumpfs.h"

/*
 * There is no sense in having SGX_MMU_PAGE_SIZE differ from PAGE_SIZE.
 * Especially the calculations in this file, while obviously an attempt to work
 * around possibly differing host and gpu page sizes, are impossible when
 * page size is not the same everywhere.
 */
#if PAGE_SIZE != SGX_MMU_PAGE_SIZE
#error Host page size differs from GPU page size!
#endif

static atomic_t gsPDumpSuspended = ATOMIC_INIT(0);

#define SZ_COMMENT_SIZE_MAX		PVRSRV_PDUMP_MAX_COMMENT_SIZE
#define SZ_SCRIPT_SIZE_MAX		(SZ_COMMENT_SIZE_MAX + 5)
#define SZ_FILENAME_SIZE_MAX		SZ_COMMENT_SIZE_MAX
static char *gpszComment;
static char *gpszScript;
static char *gpszFile;

void PDumpSuspendKM(void)
{
	atomic_inc(&gsPDumpSuspended);
}

void PDumpResumeKM(void)
{
	atomic_dec(&gsPDumpSuspended);
}

static inline IMG_BOOL PDumpSuspended(void)
{
	return atomic_read(&gsPDumpSuspended) != 0;
}

static void
pdump_print(u32 flags, char *format, ...)
{
	va_list ap;

	if (PDumpSuspended())
		return;

	if (!pdumpfs_flags_check(flags))
		return;

	va_start(ap, format);
	vsnprintf(gpszScript, SZ_SCRIPT_SIZE_MAX, format, ap);
	va_end(ap);

	pdumpfs_write_string(gpszScript);
}

static enum PVRSRV_ERROR
pdump_dump(u32 flags, void *buffer, u32 size, bool from_user)
{
	enum PVRSRV_ERROR eError;

	if (PDumpSuspended())
		return PVRSRV_OK;

	if (!pdumpfs_flags_check(flags))
		return PVRSRV_OK;

	pdump_print(flags, "BIN 0x%08X:", size);

	eError = pdumpfs_write_data(buffer, size, from_user);

	pdump_print(flags, "-- BIN END\r\n");

	return eError;
}

void PDumpCommentKM(char *pszComment, u32 ui32Flags)
{
	int len = strlen(pszComment);

	if ((len > 1) && (pszComment[len - 1] == '\n'))
		pszComment[len - 1] = 0;

	if ((len > 2) && (pszComment[len - 2] == '\r'))
		pszComment[len - 2] = 0;

	pdump_print(ui32Flags, "-- %s\r\n", pszComment);
}

void PDumpComment(char *pszFormat, ...)
{
	va_list ap;

	va_start(ap, pszFormat);
	vsnprintf(gpszComment, SZ_COMMENT_SIZE_MAX, pszFormat, ap);
	va_end(ap);

	PDumpCommentKM(gpszComment, PDUMP_FLAGS_CONTINUOUS);
}

void PDumpCommentWithFlags(u32 ui32Flags, char *pszFormat, ...)
{
	va_list ap;

	va_start(ap, pszFormat);
	vsnprintf(gpszComment, SZ_COMMENT_SIZE_MAX, pszFormat, ap);
	va_end(ap);

	PDumpCommentKM(gpszComment, ui32Flags);
}

void PDumpSetFrameKM(u32 ui32PID, u32 ui32Frame)
{
	if (PDumpSuspended())
		return;

	PDumpComment("Ending current Frame\r\n");
	pdumpfs_frame_set(ui32PID, ui32Frame);
	PDumpComment("PID %d: Starting Frame %d\r\n", ui32PID, ui32Frame);
}

IMG_BOOL PDumpIsCaptureFrameKM(void)
{
	if (PDumpSuspended())
		return IMG_FALSE;

	return pdumpfs_capture_enabled();
}

void PDumpInit(void)
{
	if (!gpszFile)
		if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			       SZ_FILENAME_SIZE_MAX,
			       (void **)&gpszFile,
			       NULL) != PVRSRV_OK)
			goto init_failed;

	if (!gpszComment)
		if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			       SZ_COMMENT_SIZE_MAX,
			       (void **)&gpszComment,
			       NULL) != PVRSRV_OK)
			goto init_failed;

	if (!gpszScript)
		if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			       SZ_SCRIPT_SIZE_MAX,
			       (void **)&gpszScript,
			       NULL) != PVRSRV_OK)
			goto init_failed;

	if (pdumpfs_init()) {
		pr_err("%s: pdumpfs_init failed.\n", __func__);
		goto init_failed;
	}

	PDumpComment("Driver Product Name: %s", VS_PRODUCT_NAME);
	PDumpComment("Driver Product Version: %s (%s)",
		     PVRVERSION_STRING, PVRVERSION_FILE);
	PDumpComment("Start of Init Phase");

	return;

 init_failed:

	if (gpszFile) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, SZ_FILENAME_SIZE_MAX,
			  (void *)gpszFile, NULL);
		gpszFile = NULL;
	}

	if (gpszScript) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, SZ_SCRIPT_SIZE_MAX,
			  (void *)gpszScript, NULL);
		gpszScript = NULL;
	}

	if (gpszComment) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, SZ_COMMENT_SIZE_MAX,
			  (void *)gpszComment, NULL);
		gpszComment = NULL;
	}
}

void PDumpDeInit(void)
{
	pdumpfs_cleanup();

	if (gpszFile) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, SZ_FILENAME_SIZE_MAX,
			  (void *)gpszFile, NULL);
		gpszFile = NULL;
	}

	if (gpszScript) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, SZ_SCRIPT_SIZE_MAX,
			  (void *)gpszScript, NULL);
		gpszScript = NULL;
	}

	if (gpszComment) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, SZ_COMMENT_SIZE_MAX,
			  (void *)gpszComment, NULL);
		gpszComment = NULL;
	}
}

void PDumpRegWithFlagsKM(u32 ui32Reg, u32 ui32Data, u32 ui32Flags)
{
	pdump_print(ui32Flags,
		    "WRW :SGXREG:0x%8.8X 0x%8.8X\r\n", ui32Reg, ui32Data);
}

void PDumpReg(u32 ui32Reg, u32 ui32Data)
{
	PDumpRegWithFlagsKM(ui32Reg, ui32Data, PDUMP_FLAGS_CONTINUOUS);
}

void PDumpRegPolWithFlagsKM(u32 ui32RegAddr, u32 ui32RegValue,
			    u32 ui32Mask, u32 ui32Flags)
{
#define POLL_DELAY		1000
#define POLL_COUNT_LONG		(2000000000 / POLL_DELAY)
#define POLL_COUNT_SHORT	(1000000 / POLL_DELAY)

	u32 ui32PollCount;

	if ((ui32RegAddr == EUR_CR_EVENT_STATUS) &&
	    ((ui32RegValue & ui32Mask) &
	     (EUR_CR_EVENT_STATUS_TA_FINISHED_MASK |
	      EUR_CR_EVENT_STATUS_PIXELBE_END_RENDER_MASK |
	      EUR_CR_EVENT_STATUS_DPM_3D_MEM_FREE_MASK)))
		ui32PollCount = POLL_COUNT_LONG;
	else
		ui32PollCount = POLL_COUNT_SHORT;

	pdump_print(ui32Flags,
		    "POL :SGXREG:0x%8.8X 0x%8.8X 0x%8.8X %d %u %d\r\n",
		    ui32RegAddr, ui32RegValue, ui32Mask, 0, ui32PollCount,
		    POLL_DELAY);
}

void PDumpRegPolKM(u32 ui32RegAddr, u32 ui32RegValue, u32 ui32Mask)
{
	PDumpRegPolWithFlagsKM(ui32RegAddr, ui32RegValue, ui32Mask,
			       PDUMP_FLAGS_CONTINUOUS);
}

void PDumpMallocPages(u32 ui32DevVAddr, void *hOSMemHandle,
		      u32 ui32NumBytes, void *hUniqueTag)
{
	struct IMG_CPU_PHYADDR sCpuPAddr;
	struct IMG_DEV_PHYADDR sDevPAddr;
	u32 ui32Offset;

	PVR_ASSERT(((u32) ui32DevVAddr & ~PAGE_MASK) == 0);
	PVR_ASSERT(hOSMemHandle);
	PVR_ASSERT(((u32) ui32NumBytes & ~PAGE_MASK) == 0);

	PDumpComment("MALLOC :SGXMEM:VA_%8.8X 0x%8.8X %u\r\n",
		     ui32DevVAddr, ui32NumBytes, PAGE_SIZE);

	for (ui32Offset = 0; ui32Offset < ui32NumBytes;
	     ui32Offset += PAGE_SIZE) {
		sCpuPAddr = OSMemHandleToCpuPAddr(hOSMemHandle, ui32Offset);
		sDevPAddr = SysCpuPAddrToDevPAddr(0, sCpuPAddr);

		pdump_print(PDUMP_FLAGS_CONTINUOUS,
			    "MALLOC :SGXMEM:PA_%8.8X%8.8X %u %u 0x%8.8X\r\n",
			    (u32)hUniqueTag, sDevPAddr.uiAddr, PAGE_SIZE,
			    PAGE_SIZE, sDevPAddr.uiAddr);
	}
}

void PDumpMallocPageTable(void *pvLinAddr, void *hUniqueTag)
{
	struct IMG_CPU_PHYADDR sCpuPAddr;
	struct IMG_DEV_PHYADDR sDevPAddr;

	PVR_ASSERT(((u32) pvLinAddr & ~PAGE_MASK) == 0);

	PDumpComment("MALLOC :SGXMEM:PAGE_TABLE 0x%8.8X %lu\r\n",
		     PAGE_SIZE, PAGE_SIZE);

	sCpuPAddr = OSMapLinToCPUPhys(pvLinAddr);
	sDevPAddr = SysCpuPAddrToDevPAddr(0, sCpuPAddr);

	pdump_print(PDUMP_FLAGS_CONTINUOUS, "MALLOC :SGXMEM:PA_%8.8X%8.8lX "
		    "0x%lX %lu 0x%8.8lX\r\n", (u32)hUniqueTag,
		    sDevPAddr.uiAddr, PAGE_SIZE,
		    PAGE_SIZE, sDevPAddr.uiAddr);
}

void PDumpFreePages(struct BM_HEAP *psBMHeap, struct IMG_DEV_VIRTADDR sDevVAddr,
		    u32 ui32NumBytes, void *hUniqueTag, IMG_BOOL bInterleaved)
{
	struct IMG_DEV_PHYADDR sDevPAddr;
	struct PVRSRV_DEVICE_NODE *psDeviceNode =
		psBMHeap->pBMContext->psDeviceNode;
	int i;

	PVR_ASSERT(((u32) sDevVAddr.uiAddr & ~PAGE_MASK) == 0);
	PVR_ASSERT(((u32) ui32NumBytes & ~PAGE_MASK) == 0);

	PDumpComment("FREE :SGXMEM:VA_%8.8X\r\n", sDevVAddr.uiAddr);

	for (i = 0; (i * PAGE_SIZE) < ui32NumBytes; i++) {
		if (!bInterleaved || (i % 2) == 0) {
			sDevPAddr =
			    psDeviceNode->pfnMMUGetPhysPageAddr(psBMHeap->
								pMMUHeap,
								sDevVAddr);
			pdump_print(PDUMP_FLAGS_CONTINUOUS,
				    "FREE :SGXMEM:PA_%8.8X%8.8X\r\n",
				    (u32)hUniqueTag, sDevPAddr.uiAddr);
		}

		sDevVAddr.uiAddr += PAGE_SIZE;
	}
}

void PDumpFreePageTable(void *pvLinAddr)
{
	struct IMG_CPU_PHYADDR sCpuPAddr;
	struct IMG_DEV_PHYADDR sDevPAddr;

	PVR_ASSERT(((u32) pvLinAddr & ~PAGE_MASK) == 0);

	PDumpComment("FREE :SGXMEM:PAGE_TABLE\r\n");

	sCpuPAddr = OSMapLinToCPUPhys(pvLinAddr);
	sDevPAddr = SysCpuPAddrToDevPAddr(0, sCpuPAddr);

	pdump_print(PDUMP_FLAGS_CONTINUOUS, "FREE :SGXMEM:PA_%8.8X%8.8lX\r\n",
		    PDUMP_PT_UNIQUETAG, sDevPAddr.uiAddr);
}

void PDumpPDRegWithFlags(u32 ui32Reg, u32 ui32Data, u32 ui32Flags)
{
	pdump_print(ui32Flags,
		    "WRW :SGXREG:0x%8.8X :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX\r\n",
		    ui32Reg, PDUMP_PD_UNIQUETAG,
		    ui32Data & PAGE_MASK,
		    ui32Data & ~PAGE_MASK);
}

void PDumpPDReg(u32 ui32Reg, u32 ui32Data)
{
	PDumpPDRegWithFlags(ui32Reg, ui32Data, PDUMP_FLAGS_CONTINUOUS);
}

void PDumpMemPolKM(struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
		   u32 ui32Offset, u32 ui32Value, u32 ui32Mask,
		   enum PDUMP_POLL_OPERATOR eOperator, void *hUniqueTag)
{
#define MEMPOLL_DELAY		(1000)
#define MEMPOLL_COUNT		(2000000000 / MEMPOLL_DELAY)

	struct IMG_DEV_PHYADDR sDevPAddr;
	struct IMG_DEV_VIRTADDR sDevVPageAddr;
	u32 ui32PageOffset;

	PVR_ASSERT((ui32Offset + sizeof(u32)) <=
		   psMemInfo->ui32AllocSize);

	sDevVPageAddr.uiAddr = psMemInfo->sDevVAddr.uiAddr + ui32Offset;
	ui32PageOffset = sDevVPageAddr.uiAddr & ~PAGE_MASK;
	BM_GetPhysPageAddr(psMemInfo, sDevVPageAddr, &sDevPAddr);

	pdump_print(0, "POL :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX 0x%8.8X "
		    "0x%8.8X %d %d %d\r\n", (u32)hUniqueTag,
		    sDevPAddr.uiAddr, ui32PageOffset,
		    ui32Value, ui32Mask, eOperator,
		    MEMPOLL_COUNT, MEMPOLL_DELAY);
}

static void
pdump_mem_print(u32 ui32Flags, struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
		u32 ui32Offset, u32 ui32Bytes, void *hUniqueTag)
{
	struct IMG_DEV_VIRTADDR sDevVPageAddr;
	struct IMG_DEV_PHYADDR sDevPAddr;
	u32 ui32PageOffset;

	PDumpCommentWithFlags(ui32Flags, "LDB :SGXMEM:VA_%8.8X:0x%8.8X "
			      "0x%8.8X\r\n",
			      psMemInfo->sDevVAddr.uiAddr, ui32Offset,
			      ui32Bytes);

	ui32PageOffset =
		(psMemInfo->sDevVAddr.uiAddr + ui32Offset) & ~PAGE_MASK;

	while (ui32Bytes) {
		u32 ui32BlockBytes =
			min(ui32Bytes, (u32)PAGE_SIZE - ui32PageOffset);

		sDevVPageAddr.uiAddr =
			psMemInfo->sDevVAddr.uiAddr + ui32Offset;
		BM_GetPhysPageAddr(psMemInfo, sDevVPageAddr, &sDevPAddr);

		pdump_print(ui32Flags, "LDB :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX"
			    " 0x%8.8X\r\n", (u32) hUniqueTag,
			    sDevPAddr.uiAddr, ui32PageOffset,
			    ui32BlockBytes);

		ui32PageOffset = 0;
		ui32Bytes -= ui32BlockBytes;
		ui32Offset += ui32BlockBytes;
	}
}

enum PVRSRV_ERROR
PDumpMemKM(void *pvAltLinAddr, struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
	   u32 ui32Offset, u32 ui32Bytes, u32 ui32Flags, void *hUniqueTag)
{
	enum PVRSRV_ERROR eError;

	PVR_ASSERT((ui32Offset + ui32Bytes) <= psMemInfo->ui32AllocSize);

	if (!ui32Bytes)
		return PVRSRV_ERROR_GENERIC;

	if (pvAltLinAddr)
		eError = pdump_dump(ui32Flags, pvAltLinAddr, ui32Bytes, false);
	else if (psMemInfo->pvLinAddrKM)
		eError = pdump_dump(ui32Flags,
				    psMemInfo->pvLinAddrKM + ui32Offset,
				    ui32Bytes, false);
	else
		return PVRSRV_ERROR_GENERIC;

	if (eError != PVRSRV_OK)
		return eError;

	pdump_mem_print(ui32Flags, psMemInfo, ui32Offset, ui32Bytes,
			hUniqueTag);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR
PDumpMemUM(void *pvAltLinAddrUM, void *pvLinAddrUM,
	   struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
	   u32 ui32Offset, u32 ui32Bytes, u32 ui32Flags, void *hUniqueTag)
{
	enum PVRSRV_ERROR eError;

	PVR_ASSERT((ui32Offset + ui32Bytes) <= psMemInfo->ui32AllocSize);

	if (!ui32Bytes)
		return PVRSRV_ERROR_GENERIC;

	if (pvAltLinAddrUM)
		eError = pdump_dump(ui32Flags, pvAltLinAddrUM, ui32Bytes, true);
	else if (psMemInfo->pvLinAddrKM)
		eError = pdump_dump(ui32Flags,
				    psMemInfo->pvLinAddrKM + ui32Offset,
				    ui32Bytes, false);
	else if (pvLinAddrUM)
		eError = pdump_dump(ui32Flags, pvLinAddrUM + ui32Offset,
				    ui32Bytes, true);
	else
		return PVRSRV_ERROR_GENERIC;

	if (eError != PVRSRV_OK)
		return eError;

	pdump_mem_print(ui32Flags, psMemInfo, ui32Offset, ui32Bytes,
			hUniqueTag);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR
PDumpPageTableKM(void *pvLinAddr, u32 ui32Bytes, IMG_BOOL bInitialisePages,
		 void *hUniqueTag1, void *hUniqueTag2)
{
	struct IMG_DEV_PHYADDR sDevPAddr;
	struct IMG_CPU_PHYADDR sCpuPAddr;
	enum PVRSRV_ERROR eError;

	if (!pvLinAddr)
		return PVRSRV_ERROR_GENERIC;

	if (bInitialisePages) {
		eError = pdump_dump(PDUMP_FLAGS_CONTINUOUS, pvLinAddr,
				    ui32Bytes, false);
		if (eError != PVRSRV_OK)
			return eError;
	}

	while (ui32Bytes) {
		u32 ui32BlockBytes =
			min(ui32Bytes,
			    (u32)(PAGE_SIZE - ((u32)pvLinAddr & ~PAGE_MASK)));

		sCpuPAddr = OSMapLinToCPUPhys(pvLinAddr);
		sDevPAddr = SysCpuPAddrToDevPAddr(0, sCpuPAddr);

		if (bInitialisePages) {
			pdump_print(PDUMP_FLAGS_CONTINUOUS, "LDB :SGXMEM:"
				    "PA_%8.8X%8.8lX:0x%8.8lX 0x%8.8X\r\n",
				    (u32) hUniqueTag1,
				    sDevPAddr.uiAddr & PAGE_MASK,
				    sDevPAddr.uiAddr & ~PAGE_MASK,
				    ui32BlockBytes);
		} else {
			u32 ui32Offset;

			for (ui32Offset = 0; ui32Offset < ui32BlockBytes;
			     ui32Offset += sizeof(u32)) {
				u32 ui32PTE =
					*((u32 *)(pvLinAddr + ui32Offset));

				if ((ui32PTE & ~PAGE_MASK) != 0) {
					pdump_print(PDUMP_FLAGS_CONTINUOUS,
"WRW :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX\r\n",
						 (u32)hUniqueTag1,
						 sDevPAddr.uiAddr & PAGE_MASK,
						 sDevPAddr.uiAddr & ~PAGE_MASK,
						 (u32)hUniqueTag2,
						 ui32PTE & PAGE_MASK,
						 ui32PTE & ~PAGE_MASK);
				} else {
					PVR_ASSERT(!
						   (ui32PTE &
						    SGX_MMU_PTE_VALID));
					pdump_print(PDUMP_FLAGS_CONTINUOUS,
		"WRW :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX 0x%8.8X%8.8X\r\n",
						 (u32) hUniqueTag1,
						 sDevPAddr.uiAddr & PAGE_MASK,
						 sDevPAddr.uiAddr & ~PAGE_MASK,
						 ui32PTE, (u32)hUniqueTag2);
				}

				sDevPAddr.uiAddr += sizeof(u32);
			}
		}

		ui32Bytes -= ui32BlockBytes;
		pvLinAddr += ui32BlockBytes;
	}

	return PVRSRV_OK;
}

void
PDumpPDDevPAddrKM(struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
		  u32 ui32Offset, struct IMG_DEV_PHYADDR sPDDevPAddr,
		  void *hUniqueTag1, void *hUniqueTag2)
{
	struct IMG_DEV_VIRTADDR sDevVPageAddr;
	struct IMG_DEV_PHYADDR sDevPAddr;
	u32 ui32PageOffset;

	sDevVPageAddr.uiAddr = psMemInfo->sDevVAddr.uiAddr + ui32Offset;
	ui32PageOffset = sDevVPageAddr.uiAddr & ~PAGE_MASK;
	BM_GetPhysPageAddr(psMemInfo, sDevVPageAddr, &sDevPAddr);

	if ((sPDDevPAddr.uiAddr & PAGE_MASK) != 0) {
		pdump_print(PDUMP_FLAGS_CONTINUOUS,
"WRW :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX\r\n",
			    (u32) hUniqueTag1,
			    sDevPAddr.uiAddr, ui32PageOffset,
			    (u32)hUniqueTag2,
			    sPDDevPAddr.uiAddr & PAGE_MASK,
			    sPDDevPAddr.uiAddr & ~PAGE_MASK);
	} else {
		PVR_ASSERT(!(sDevPAddr.uiAddr & SGX_MMU_PTE_VALID));
		pdump_print(PDUMP_FLAGS_CONTINUOUS,
			    "WRW :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX 0x%8.8X\r\n",
			    (u32)hUniqueTag1,
			    sDevPAddr.uiAddr, ui32PageOffset,
			    sPDDevPAddr.uiAddr);
	}
}

void PDumpBitmapKM(char *pszFileName, u32 ui32FileOffset,
		   u32 ui32Width, u32 ui32Height, u32 ui32StrideInBytes,
		   struct IMG_DEV_VIRTADDR sDevBaseAddr,
		   u32 ui32Size, enum PDUMP_PIXEL_FORMAT ePixelFormat,
		   enum PDUMP_MEM_FORMAT eMemFormat, u32 ui32PDumpFlags)
{
	PDumpCommentWithFlags(ui32PDumpFlags, "Dump bitmap of render\r\n");

	pdump_print(ui32PDumpFlags,
		    "SII %s %s.bin :SGXMEM:v:0x%08X 0x%08X "
		    "0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n",
		    pszFileName, pszFileName, sDevBaseAddr.uiAddr, ui32Size,
		    ui32FileOffset, ePixelFormat, ui32Width, ui32Height,
		    ui32StrideInBytes, eMemFormat);
}

static void PDumpReadRegKM(char *pszFileName, u32 ui32FileOffset,
			   u32 ui32Address)
{
	pdump_print(0, "SAB :SGXREG:0x%08X 0x%08X %s\r\n",
		    ui32Address, ui32FileOffset, pszFileName);
}

void PDump3DSignatureRegisters(u32 ui32DumpFrameNum,
			       u32 *pui32Registers, u32 ui32NumRegisters)
{
	u32 i;

	PDumpCommentWithFlags(0, "Dump 3D signature registers\r\n");
	snprintf(gpszFile, SZ_FILENAME_SIZE_MAX, "out%u_3d.sig",
		 ui32DumpFrameNum);

	for (i = 0; i < ui32NumRegisters; i++)
		PDumpReadRegKM(gpszFile, i * sizeof(u32), pui32Registers[i]);
}

void PDumpCounterRegisters(u32 ui32DumpFrameNum,
			   u32 *pui32Registers, u32 ui32NumRegisters)
{
	u32 i;

	PDumpCommentWithFlags(0, "Dump counter registers\r\n");
	snprintf(gpszFile, SZ_FILENAME_SIZE_MAX, "out%u.perf",
		 ui32DumpFrameNum);

	for (i = 0; i < ui32NumRegisters; i++)
		PDumpReadRegKM(gpszFile, i * sizeof(u32), pui32Registers[i]);
}

void PDumpTASignatureRegisters(u32 ui32DumpFrameNum, u32 ui32TAKickCount,
			       u32 *pui32Registers, u32 ui32NumRegisters)
{
	u32 i, ui32FileOffset;

	PDumpCommentWithFlags(0, "Dump TA signature registers\r\n");
	snprintf(gpszFile, SZ_FILENAME_SIZE_MAX, "out%u_ta.sig",
		 ui32DumpFrameNum);

	ui32FileOffset = ui32TAKickCount * ui32NumRegisters * sizeof(u32);

	for (i = 0; i < ui32NumRegisters; i++)
		PDumpReadRegKM(gpszFile, ui32FileOffset + i * sizeof(u32),
			       pui32Registers[i]);
}

void PDumpRegRead(const u32 ui32RegOffset, u32 ui32Flags)
{
	pdump_print(ui32Flags, "RDW :SGXREG:0x%X\r\n", ui32RegOffset);
}

void PDumpCycleCountRegRead(const u32 ui32RegOffset)
{
	PDumpRegRead(ui32RegOffset, 0);
}

void PDumpHWPerfCBKM(char *pszFileName, u32 ui32FileOffset,
		     struct IMG_DEV_VIRTADDR sDevBaseAddr, u32 ui32Size,
		     u32 ui32PDumpFlags)
{
	PDumpCommentWithFlags(ui32PDumpFlags,
			      "Dump Hardware Performance Circular Buffer\r\n");
	pdump_print(ui32PDumpFlags,
		    "SAB :SGXMEM:v:0x%08X 0x%08X 0x%08X %s.bin\r\n",
		    sDevBaseAddr.uiAddr, ui32Size, ui32FileOffset, pszFileName);
}

void PDumpCBP(struct PVRSRV_KERNEL_MEM_INFO *psROffMemInfo,
	      u32 ui32ROffOffset, u32 ui32WPosVal, u32 ui32PacketSize,
	      u32 ui32BufferSize, void *hUniqueTag)
{
	struct IMG_DEV_PHYADDR sDevPAddr;
	struct IMG_DEV_VIRTADDR sDevVPageAddr;
	u32 ui32PageOffset;

	PVR_ASSERT((ui32ROffOffset + sizeof(u32)) <=
		   psROffMemInfo->ui32AllocSize);

	sDevVPageAddr.uiAddr =
		psROffMemInfo->sDevVAddr.uiAddr + ui32ROffOffset;
	ui32PageOffset = sDevVPageAddr.uiAddr & ~PAGE_MASK;
	BM_GetPhysPageAddr(psROffMemInfo, sDevVPageAddr, &sDevPAddr);

	pdump_print(0, "CBP :SGXMEM:PA_%8.8X%8.8lX:0x%8.8lX 0x%8.8X"
		    " 0x%8.8X 0x%8.8X\r\n", (u32) hUniqueTag,
		    sDevPAddr.uiAddr, ui32PageOffset,
		    ui32WPosVal, ui32PacketSize, ui32BufferSize);
}

void PDumpIDLWithFlags(u32 ui32Clocks, u32 ui32Flags)
{
	pdump_print(ui32Flags, "IDL %u\r\n", ui32Clocks);
}
