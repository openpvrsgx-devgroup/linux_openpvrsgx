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

#ifndef _PDUMP_KM_H_
#define _PDUMP_KM_H_

#if defined(LINUX)
#define COMMON_PDUMP_OS_SUPPORT
#endif

#if defined(COMMON_PDUMP_OS_SUPPORT)
#if defined(SUPPORT_SGX) || defined(SUPPORT_MSVDX)
#define SGX_SUPPORT_COMMON_PDUMP
#if defined(SGX_SUPPORT_COMMON_PDUMP)
#include <pdump_osfunc.h>
#endif

#endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#define PDUMP_FLAGS_NEVER			0x08000000UL
#define PDUMP_FLAGS_TOOUT2MEM		0x10000000UL
#define PDUMP_FLAGS_LASTFRAME		0x20000000UL
#define PDUMP_FLAGS_RESETLFBUFFER	0x40000000UL
#define PDUMP_FLAGS_CONTINUOUS		0x80000000UL

#define PDUMP_PD_UNIQUETAG			(IMG_HANDLE)0
#define PDUMP_PT_UNIQUETAG			(IMG_HANDLE)0

#define PDUMP_STREAM_PARAM2			0
#define PDUMP_STREAM_SCRIPT2		1
#define PDUMP_STREAM_DRIVERINFO		2
#define PDUMP_NUM_STREAMS			3


#ifndef PDUMP
#define MAKEUNIQUETAG(hMemInfo)	(0)
#endif

#ifdef PDUMP

#define MAKEUNIQUETAG(hMemInfo)	(((BM_BUF *)(((PVRSRV_KERNEL_MEM_INFO *)hMemInfo)->sMemBlk.hBuffer))->pMapping)

	IMG_IMPORT PVRSRV_ERROR PDumpMemPolKM(PVRSRV_KERNEL_MEM_INFO *psMemInfo,
										  IMG_UINT32			ui32Offset,
										  IMG_UINT32			ui32Value,
										  IMG_UINT32			ui32Mask,
										  PDUMP_POLL_OPERATOR	eOperator,
										  IMG_UINT32			ui32Flags,
										  IMG_HANDLE			hUniqueTag);

	IMG_IMPORT PVRSRV_ERROR PDumpMemUM(PVRSRV_PER_PROCESS_DATA *psProcData,
									   IMG_PVOID			pvAltLinAddr,
									   IMG_PVOID			pvLinAddr,
									   PVRSRV_KERNEL_MEM_INFO	*psMemInfo,
									   IMG_UINT32			ui32Offset,
									   IMG_UINT32			ui32Bytes,
									   IMG_UINT32			ui32Flags,
									   IMG_HANDLE			hUniqueTag);

	IMG_IMPORT PVRSRV_ERROR PDumpMemKM(IMG_PVOID			pvAltLinAddr,
									   PVRSRV_KERNEL_MEM_INFO	*psMemInfo,
									   IMG_UINT32			ui32Offset,
									   IMG_UINT32			ui32Bytes,
									   IMG_UINT32			ui32Flags,
									   IMG_HANDLE			hUniqueTag);
	PVRSRV_ERROR PDumpMemPagesKM(PVRSRV_DEVICE_TYPE	eDeviceType,
								 IMG_DEV_PHYADDR		*pPages,
								 IMG_UINT32			ui32NumPages,
								 IMG_DEV_VIRTADDR	sDevAddr,
								 IMG_UINT32			ui32Start,
								 IMG_UINT32			ui32Length,
								 IMG_UINT32			ui32Flags,
								 IMG_HANDLE			hUniqueTag);

	PVRSRV_ERROR PDumpMem2KM(PVRSRV_DEVICE_TYPE	eDeviceType,
							 IMG_CPU_VIRTADDR	pvLinAddr,
							 IMG_UINT32			ui32Bytes,
							 IMG_UINT32			ui32Flags,
							 IMG_BOOL			bInitialisePages,
							 IMG_HANDLE			hUniqueTag1,
							 IMG_HANDLE			hUniqueTag2);
	IMG_VOID PDumpInitCommon(IMG_VOID);
	IMG_VOID PDumpDeInitCommon(IMG_VOID);
	IMG_VOID PDumpInit(IMG_VOID);
	IMG_VOID PDumpDeInit(IMG_VOID);
	PVRSRV_ERROR PDumpStartInitPhaseKM(IMG_VOID);
	PVRSRV_ERROR PDumpStopInitPhaseKM(IMG_VOID);
	IMG_IMPORT PVRSRV_ERROR PDumpSetFrameKM(IMG_UINT32 ui32Frame);
	IMG_IMPORT PVRSRV_ERROR PDumpCommentKM(IMG_CHAR *pszComment, IMG_UINT32 ui32Flags);
	IMG_IMPORT PVRSRV_ERROR PDumpDriverInfoKM(IMG_CHAR *pszString, IMG_UINT32 ui32Flags);

	PVRSRV_ERROR PDumpRegWithFlagsKM(IMG_UINT32 ui32RegAddr,
									 IMG_UINT32 ui32RegValue,
									 IMG_UINT32 ui32Flags);
	PVRSRV_ERROR PDumpRegPolWithFlagsKM(IMG_UINT32 ui32RegAddr,
										IMG_UINT32 ui32RegValue,
										IMG_UINT32 ui32Mask,
										IMG_UINT32 ui32Flags);
	PVRSRV_ERROR PDumpRegPolKM(IMG_UINT32 ui32RegAddr,
							   IMG_UINT32 ui32RegValue,
							   IMG_UINT32 ui32Mask);

	IMG_IMPORT PVRSRV_ERROR PDumpBitmapKM(IMG_CHAR *pszFileName,
										  IMG_UINT32 ui32FileOffset,
										  IMG_UINT32 ui32Width,
										  IMG_UINT32 ui32Height,
										  IMG_UINT32 ui32StrideInBytes,
										  IMG_DEV_VIRTADDR sDevBaseAddr,
										  IMG_UINT32 ui32Size,
										  PDUMP_PIXEL_FORMAT ePixelFormat,
										  PDUMP_MEM_FORMAT eMemFormat,
										  IMG_UINT32 ui32PDumpFlags);
	IMG_IMPORT PVRSRV_ERROR PDumpReadRegKM(IMG_CHAR *pszFileName,
										   IMG_UINT32 ui32FileOffset,
										   IMG_UINT32 ui32Address,
										   IMG_UINT32 ui32Size,
										   IMG_UINT32 ui32PDumpFlags);

	IMG_BOOL PDumpIsSuspended(IMG_VOID);

#if defined(SGX_SUPPORT_COMMON_PDUMP) || !defined(SUPPORT_VGX)

	PVRSRV_ERROR PDumpRegKM(IMG_UINT32		dwReg,
							IMG_UINT32		dwData);
	PVRSRV_ERROR PDumpComment(IMG_CHAR* pszFormat, ...);
	PVRSRV_ERROR PDumpCommentWithFlags(IMG_UINT32	ui32Flags,
									   IMG_CHAR*	pszFormat,
									   ...);

	PVRSRV_ERROR PDumpPDReg(IMG_UINT32	ui32Reg,
							IMG_UINT32	ui32dwData,
							IMG_HANDLE	hUniqueTag);
	PVRSRV_ERROR PDumpPDRegWithFlags(IMG_UINT32		ui32Reg,
									 IMG_UINT32		ui32Data,
									 IMG_UINT32		ui32Flags,
									 IMG_HANDLE		hUniqueTag);
#else
	IMG_VOID PDumpRegKM(IMG_UINT32		dwReg,
							IMG_UINT32		dwData);
	IMG_VOID PDumpComment(IMG_CHAR* pszFormat, ...);
	IMG_VOID PDumpCommentWithFlags(IMG_UINT32	ui32Flags,
									   IMG_CHAR*	pszFormat,
									   ...);


	IMG_VOID PDumpPDReg(IMG_UINT32	ui32Reg,
							IMG_UINT32	ui32dwData,
							IMG_HANDLE	hUniqueTag);
	IMG_VOID PDumpPDRegWithFlags(IMG_UINT32		ui32Reg,
									 IMG_UINT32		ui32Data,
									 IMG_UINT32		ui32Flags,
									 IMG_HANDLE		hUniqueTag);
#endif

	IMG_VOID PDumpMsvdxRegRead(const IMG_CHAR* const	pRegRegion,
							   const IMG_UINT32		dwRegOffset);

	IMG_VOID PDumpMsvdxRegWrite(const IMG_CHAR* const	pRegRegion,
								const IMG_UINT32		dwRegOffset,
								const IMG_UINT32		dwData);

	PVRSRV_ERROR PDumpMsvdxRegPol(const IMG_CHAR* const	pRegRegion,
								  const IMG_UINT32		ui32Offset,
								  const IMG_UINT32		ui32CheckFuncIdExt,
								  const IMG_UINT32		ui32RequValue,
								  const IMG_UINT32		ui32Enable,
								  const IMG_UINT32		ui32PollCount,
								  const IMG_UINT32		ui32TimeOut);

	PVRSRV_ERROR  PDumpMsvdxWriteRef(const IMG_CHAR* const	pRegRegion,
									 const IMG_UINT32		ui32VLROffset,
									 const IMG_UINT32		ui32Physical );

	IMG_BOOL PDumpIsLastCaptureFrameKM(IMG_VOID);
	IMG_IMPORT IMG_BOOL PDumpIsCaptureFrameKM(IMG_VOID);

	IMG_VOID PDumpMallocPagesPhys(PVRSRV_DEVICE_TYPE	eDeviceType,
								  IMG_UINT32			ui32DevVAddr,
								  IMG_PUINT32			pui32PhysPages,
								  IMG_UINT32			ui32NumPages,
								  IMG_HANDLE			hUniqueTag);
	PVRSRV_ERROR PDumpSetMMUContext(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_CHAR *pszMemSpace,
									IMG_UINT32 *pui32MMUContextID,
									IMG_UINT32 ui32MMUType,
									IMG_HANDLE hUniqueTag1,
									IMG_VOID *pvPDCPUAddr);
	PVRSRV_ERROR PDumpClearMMUContext(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_CHAR *pszMemSpace,
									IMG_UINT32 ui32MMUContextID,
									IMG_UINT32 ui32MMUType);

	PVRSRV_ERROR PDumpPDDevPAddrKM(PVRSRV_KERNEL_MEM_INFO *psMemInfo,
								   IMG_UINT32 ui32Offset,
								   IMG_DEV_PHYADDR sPDDevPAddr,
								   IMG_HANDLE hUniqueTag1,
								   IMG_HANDLE hUniqueTag2);

	IMG_BOOL PDumpTestNextFrame(IMG_UINT32 ui32CurrentFrame);


#if defined (COMMON_PDUMP_OS_SUPPORT) && !defined(SUPPORT_VGX)

	PVRSRV_ERROR PDumpTASignatureRegisters(IMG_UINT32	ui32DumpFrameNum,
								   IMG_UINT32	ui32TAKickCount,
								   IMG_BOOL		bLastFrame,
								   IMG_UINT32 *pui32Registers,
								   IMG_UINT32 ui32NumRegisters);

	PVRSRV_ERROR PDump3DSignatureRegisters(IMG_UINT32 ui32DumpFrameNum,
															IMG_BOOL bLastFrame,
															IMG_UINT32 *pui32Registers,
															IMG_UINT32 ui32NumRegisters);

	PVRSRV_ERROR PDumpCounterRegisters(IMG_UINT32 ui32DumpFrameNum,
					IMG_BOOL		bLastFrame,
					IMG_UINT32 *pui32Registers,
					IMG_UINT32 ui32NumRegisters);

	PVRSRV_ERROR PDumpRegRead(const IMG_UINT32 dwRegOffset, IMG_UINT32	ui32Flags);

	PVRSRV_ERROR PDumpCycleCountRegRead(const IMG_UINT32 dwRegOffset, IMG_BOOL bLastFrame);

	PVRSRV_ERROR PDumpIDLWithFlags(IMG_UINT32 ui32Clocks, IMG_UINT32 ui32Flags);
	PVRSRV_ERROR PDumpIDL(IMG_UINT32 ui32Clocks);

	PVRSRV_ERROR PDumpMallocPages(PVRSRV_DEVICE_TYPE	eDeviceType,
							  IMG_UINT32			ui32DevVAddr,
							  IMG_CPU_VIRTADDR		pvLinAddr,
							  IMG_HANDLE			hOSMemHandle,
							  IMG_UINT32			ui32NumBytes,
							  IMG_UINT32			ui32PageSize,
							  IMG_HANDLE			hUniqueTag);
	PVRSRV_ERROR PDumpMallocPageTable(PVRSRV_DEVICE_TYPE	eDeviceType,
								  IMG_CPU_VIRTADDR		pvLinAddr,
								  IMG_UINT32			ui32NumBytes,
								  IMG_HANDLE			hUniqueTag);
	PVRSRV_ERROR PDumpFreePages(struct _BM_HEAP_	*psBMHeap,
							IMG_DEV_VIRTADDR	sDevVAddr,
							IMG_UINT32			ui32NumBytes,
							IMG_UINT32			ui32PageSize,
							IMG_HANDLE      	hUniqueTag,
							IMG_BOOL			bInterleaved);
	PVRSRV_ERROR PDumpFreePageTable(PVRSRV_DEVICE_TYPE	eDeviceType,
								IMG_CPU_VIRTADDR	pvLinAddr,
								IMG_UINT32			ui32NumBytes,
								IMG_HANDLE			hUniqueTag);

	IMG_IMPORT PVRSRV_ERROR PDumpHWPerfCBKM(IMG_CHAR			*pszFileName,
										IMG_UINT32			ui32FileOffset,
										IMG_DEV_VIRTADDR	sDevBaseAddr,
										IMG_UINT32 			ui32Size,
										IMG_UINT32 			ui32PDumpFlags);

	PVRSRV_ERROR PDumpCBP(PPVRSRV_KERNEL_MEM_INFO	psROffMemInfo,
				  IMG_UINT32				ui32ROffOffset,
				  IMG_UINT32				ui32WPosVal,
				  IMG_UINT32				ui32PacketSize,
				  IMG_UINT32				ui32BufferSize,
				  IMG_UINT32				ui32Flags,
				  IMG_HANDLE				hUniqueTag);

#else
	IMG_VOID PDumpTASignatureRegisters(IMG_UINT32	ui32DumpFrameNum,
			   IMG_UINT32	ui32TAKickCount,
			   IMG_BOOL		bLastFrame,
			   IMG_UINT32 *pui32Registers,
			   IMG_UINT32 ui32NumRegisters);
	IMG_VOID PDump3DSignatureRegisters(IMG_UINT32 ui32DumpFrameNum,
			IMG_BOOL bLastFrame,
			IMG_UINT32 *pui32Registers,
			IMG_UINT32 ui32NumRegisters);
	IMG_VOID PDumpCounterRegisters(IMG_UINT32 ui32DumpFrameNum,
			IMG_BOOL		bLastFrame,
			IMG_UINT32 *pui32Registers,
			IMG_UINT32 ui32NumRegisters);

	IMG_VOID PDumpRegRead(const IMG_UINT32 dwRegOffset, IMG_UINT32	ui32Flags);
	IMG_VOID PDumpCycleCountRegRead(const IMG_UINT32 dwRegOffset, IMG_BOOL bLastFrame);

	IMG_VOID PDumpIDLWithFlags(IMG_UINT32 ui32Clocks, IMG_UINT32 ui32Flags);
	IMG_VOID PDumpIDL(IMG_UINT32 ui32Clocks);


	IMG_VOID PDumpMallocPages(PVRSRV_DEVICE_TYPE	eDeviceType,
							  IMG_UINT32			ui32DevVAddr,
							  IMG_CPU_VIRTADDR		pvLinAddr,
							  IMG_HANDLE			hOSMemHandle,
							  IMG_UINT32			ui32NumBytes,
							  IMG_UINT32			ui32PageSize,
							  IMG_HANDLE			hUniqueTag);
	IMG_VOID PDumpMallocPageTable(PVRSRV_DEVICE_TYPE	eDeviceType,
								  IMG_CPU_VIRTADDR		pvLinAddr,
								  IMG_UINT32			ui32NumBytes,
								  IMG_HANDLE			hUniqueTag);
	IMG_VOID PDumpFreePages(struct _BM_HEAP_	*psBMHeap,
							IMG_DEV_VIRTADDR	sDevVAddr,
							IMG_UINT32			ui32NumBytes,
							IMG_UINT32			ui32PageSize,
							IMG_HANDLE      	hUniqueTag,
							IMG_BOOL			bInterleaved);
	IMG_VOID PDumpFreePageTable(PVRSRV_DEVICE_TYPE	eDeviceType,
								IMG_CPU_VIRTADDR	pvLinAddr,
								IMG_UINT32			ui32NumBytes,
								IMG_HANDLE			hUniqueTag);

	IMG_IMPORT IMG_VOID PDumpHWPerfCBKM(IMG_CHAR			*pszFileName,
										IMG_UINT32			ui32FileOffset,
										IMG_DEV_VIRTADDR	sDevBaseAddr,
										IMG_UINT32 			ui32Size,
										IMG_UINT32 			ui32PDumpFlags);

	IMG_VOID PDumpCBP(PPVRSRV_KERNEL_MEM_INFO	psROffMemInfo,
				  IMG_UINT32				ui32ROffOffset,
				  IMG_UINT32				ui32WPosVal,
				  IMG_UINT32				ui32PacketSize,
				  IMG_UINT32				ui32BufferSize,
				  IMG_UINT32				ui32Flags,
				  IMG_HANDLE				hUniqueTag);

#endif

	IMG_VOID PDumpVGXMemToFile(IMG_CHAR *pszFileName,
							   IMG_UINT32 ui32FileOffset,
							   PVRSRV_KERNEL_MEM_INFO *psMemInfo,
							   IMG_UINT32 uiAddr,
							   IMG_UINT32 ui32Size,
							   IMG_UINT32 ui32PDumpFlags,
							   IMG_HANDLE hUniqueTag);

	IMG_VOID PDumpSuspendKM(IMG_VOID);
	IMG_VOID PDumpResumeKM(IMG_VOID);

	#define PDUMPMEMPOL				PDumpMemPolKM
	#define PDUMPMEM				PDumpMemKM
	#define PDUMPMEM2				PDumpMem2KM
	#define PDUMPMEMUM				PDumpMemUM
	#define PDUMPINIT				PDumpInitCommon
	#define PDUMPDEINIT				PDumpDeInitCommon
	#define PDUMPISLASTFRAME		PDumpIsLastCaptureFrameKM
	#define PDUMPTESTFRAME			PDumpIsCaptureFrameKM
	#define PDUMPTESTNEXTFRAME		PDumpTestNextFrame
	#define PDUMPREGWITHFLAGS		PDumpRegWithFlagsKM
	#define PDUMPREG				PDumpRegKM
	#define PDUMPCOMMENT			PDumpComment
	#define PDUMPCOMMENTWITHFLAGS	PDumpCommentWithFlags
	#define PDUMPREGPOL				PDumpRegPolKM
	#define PDUMPREGPOLWITHFLAGS	PDumpRegPolWithFlagsKM
	#define PDUMPMALLOCPAGES		PDumpMallocPages
	#define PDUMPMALLOCPAGETABLE	PDumpMallocPageTable
	#define PDUMPSETMMUCONTEXT		PDumpSetMMUContext
	#define PDUMPCLEARMMUCONTEXT	PDumpClearMMUContext
	#define PDUMPFREEPAGES			PDumpFreePages
	#define PDUMPFREEPAGETABLE		PDumpFreePageTable
	#define PDUMPPDREG				PDumpPDReg
	#define PDUMPPDREGWITHFLAGS		PDumpPDRegWithFlags
	#define PDUMPCBP				PDumpCBP
	#define PDUMPMALLOCPAGESPHYS	PDumpMallocPagesPhys
	#define PDUMPENDINITPHASE		PDumpStopInitPhaseKM
	#define PDUMPMSVDXREGWRITE		PDumpMsvdxRegWrite
	#define PDUMPMSVDXREGREAD		PDumpMsvdxRegRead
	#define PDUMPMSVDXPOL			PDumpMsvdxRegPol
	#define PDUMPMSVDXWRITEREF		PDumpMsvdxWriteRef
	#define PDUMPBITMAPKM			PDumpBitmapKM
	#define PDUMPDRIVERINFO			PDumpDriverInfoKM
	#define PDUMPIDLWITHFLAGS		PDumpIDLWithFlags
	#define PDUMPIDL				PDumpIDL
	#define PDUMPSUSPEND			PDumpSuspendKM
	#define PDUMPRESUME				PDumpResumeKM

#else
		#if ((defined(LINUX) || defined(GCC_IA32)) || defined(GCC_ARM))
			#define PDUMPMEMPOL(args...)
			#define PDUMPMEM(args...)
			#define PDUMPMEM2(args...)
			#define PDUMPMEMUM(args...)
			#define PDUMPINIT(args...)
			#define PDUMPDEINIT(args...)
			#define PDUMPISLASTFRAME(args...)
			#define PDUMPTESTFRAME(args...)
			#define PDUMPTESTNEXTFRAME(args...)
			#define PDUMPREGWITHFLAGS(args...)
			#define PDUMPREG(args...)
			#define PDUMPCOMMENT(args...)
			#define PDUMPREGPOL(args...)
			#define PDUMPREGPOLWITHFLAGS(args...)
			#define PDUMPMALLOCPAGES(args...)
			#define PDUMPMALLOCPAGETABLE(args...)
			#define PDUMPSETMMUCONTEXT(args...)
			#define PDUMPCLEARMMUCONTEXT(args...)
			#define PDUMPFREEPAGES(args...)
			#define PDUMPFREEPAGETABLE(args...)
			#define PDUMPPDREG(args...)
			#define PDUMPPDREGWITHFLAGS(args...)
			#define PDUMPSYNC(args...)
			#define PDUMPCOPYTOMEM(args...)
			#define PDUMPWRITE(args...)
			#define PDUMPCBP(args...)
			#define PDUMPCOMMENTWITHFLAGS(args...)
			#define PDUMPMALLOCPAGESPHYS(args...)
			#define PDUMPENDINITPHASE(args...)
			#define PDUMPMSVDXREG(args...)
			#define PDUMPMSVDXREGWRITE(args...)
			#define PDUMPMSVDXREGREAD(args...)
			#define PDUMPMSVDXPOLEQ(args...)
			#define PDUMPMSVDXPOL(args...)
			#define PDUMPBITMAPKM(args...)
			#define PDUMPDRIVERINFO(args...)
			#define PDUMPIDLWITHFLAGS(args...)
			#define PDUMPIDL(args...)
			#define PDUMPSUSPEND(args...)
			#define PDUMPRESUME(args...)
			#define PDUMPMSVDXWRITEREF(args...)
		#else
			#error Compiler not specified
		#endif
#endif

#if defined (__cplusplus)
}
#endif

#endif

