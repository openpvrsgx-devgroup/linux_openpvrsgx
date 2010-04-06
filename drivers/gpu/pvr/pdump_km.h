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

#ifndef _PDUMP_KM_H_
#define _PDUMP_KM_H_


#define PDUMP_FLAGS_NEVER		0x08000000
#define PDUMP_FLAGS_TOOUT2MEM		0x10000000
#define PDUMP_FLAGS_LASTFRAME		0x20000000
#define PDUMP_FLAGS_RESETLFBUFFER	0x40000000
#define PDUMP_FLAGS_CONTINUOUS		0x80000000

#define PDUMP_PD_UNIQUETAG		((void *)0)
#define PDUMP_PT_UNIQUETAG		((void *)0)

#ifndef PDUMP
#define MAKEUNIQUETAG(hMemInfo)	(0)
#endif

#ifdef PDUMP

#define MAKEUNIQUETAG(hMemInfo)						\
	(((struct BM_BUF *)(((struct PVRSRV_KERNEL_MEM_INFO *)		\
			     hMemInfo)->sMemBlk.hBuffer))->pMapping)

#define PDUMP_REG_FUNC_NAME PDumpReg

enum PVRSRV_ERROR PDumpMemPolKM(struct PVRSRV_KERNEL_MEM_INFO
					   *psMemInfo, u32 ui32Offset,
					   u32 ui32Value, u32 ui32Mask,
					   enum PDUMP_POLL_OPERATOR eOperator,
					   IMG_BOOL bLastFrame,
					   IMG_BOOL bOverwrite,
					   void *hUniqueTag);

enum PVRSRV_ERROR PDumpMemUM(struct PVRSRV_PER_PROCESS_DATA
					*psProcData, void *pvAltLinAddr,
					void *pvLinAddr,
					struct PVRSRV_KERNEL_MEM_INFO
					*psMemInfo, u32 ui32Offset,
					u32 ui32Bytes, u32 ui32Flags,
					void *hUniqueTag);

enum PVRSRV_ERROR PDumpMemKM(void *pvAltLinAddr,
		struct PVRSRV_KERNEL_MEM_INFO *psMemInfo, u32 ui32Offset,
		u32 ui32Bytes, u32 ui32Flags, void *hUniqueTag);

enum PVRSRV_ERROR PDumpMemPagesKM(enum PVRSRV_DEVICE_TYPE eDeviceType,
		struct IMG_DEV_PHYADDR *pPages, u32 ui32NumPages,
		struct IMG_DEV_VIRTADDR sDevAddr, u32 ui32Start,
		u32 ui32Length, u32 ui32Flags, void *hUniqueTag);

enum PVRSRV_ERROR PDumpMem2KM(enum PVRSRV_DEVICE_TYPE eDeviceType,
			      void *pvLinAddr,
			      u32 ui32Bytes,
			      u32 ui32Flags,
			      IMG_BOOL bInitialisePages,
			      void *hUniqueTag1, void *hUniqueTag2);
void PDumpInitCommon(void);
void PDumpDeInitCommon(void);
void PDumpInit(void);
void PDumpDeInit(void);
enum PVRSRV_ERROR PDumpStartInitPhaseKM(void);
enum PVRSRV_ERROR PDumpStopInitPhaseKM(void);
enum PVRSRV_ERROR PDumpSetFrameKM(u32 ui32Frame);
enum PVRSRV_ERROR PDumpCommentKM(char *pszComment, u32 ui32Flags);
enum PVRSRV_ERROR PDumpDriverInfoKM(char *pszString, u32 ui32Flags);
enum PVRSRV_ERROR PDumpRegWithFlagsKM(u32 ui32RegAddr, u32 ui32RegValue,
		u32 ui32Flags);

enum PVRSRV_ERROR PDumpBitmapKM(char *pszFileName, u32 ui32FileOffset,
		u32 ui32Width, u32 ui32Height, u32 ui32StrideInBytes,
		struct IMG_DEV_VIRTADDR sDevBaseAddr, u32 ui32Size,
		enum PDUMP_PIXEL_FORMAT ePixelFormat,
		enum PDUMP_MEM_FORMAT eMemFormat, u32 ui32PDumpFlags);
void PDumpHWPerfCBKM(char *pszFileName, u32 ui32FileOffset,
		struct IMG_DEV_VIRTADDR sDevBaseAddr,
		u32 ui32Size, u32 ui32PDumpFlags);
enum PVRSRV_ERROR PDumpReadRegKM(char *pszFileName, u32 ui32FileOffset,
		u32 ui32Address, u32 ui32Size, u32 ui32PDumpFlags);
void PDUMP_REG_FUNC_NAME(u32 dwReg, u32 dwData);

void PDumpMsvdxRegRead(const char *const pRegRegion, const u32 dwRegOffset);

void PDumpMsvdxRegWrite(const char *const pRegRegion, const u32 dwRegOffset,
		const u32 dwData);

enum PVRSRV_ERROR PDumpMsvdxRegPol(const char *const pRegRegion,
		const u32 ui32Offset, const u32 ui32CheckFuncIdExt,
		const u32 ui32RequValue, const u32 ui32Enable,
		const u32 ui32PollCount, const u32 ui32TimeOut);

enum PVRSRV_ERROR PDumpMsvdxWriteRef(const char *const pRegRegion,
		const u32 ui32VLROffset, const u32 ui32Physical);

void PDumpComment(char *pszFormat, ...);

void PDumpCommentWithFlags(u32 ui32Flags, char *pszFormat, ...);
enum PVRSRV_ERROR PDumpRegPolKM(u32 ui32RegAddr, u32 ui32RegValue,
				u32 ui32Mask);
enum PVRSRV_ERROR PDumpRegPolWithFlagsKM(u32 ui32RegAddr, u32 ui32RegValue,
				u32 ui32Mask, u32 ui32Flags);

IMG_BOOL PDumpIsLastCaptureFrameKM(void);
IMG_BOOL PDumpIsCaptureFrameKM(void);

void PDumpMallocPages(enum PVRSRV_DEVICE_TYPE eDeviceType,
		      u32 ui32DevVAddr, void *pvLinAddr, void *hOSMemHandle,
		      u32 ui32NumBytes, u32 ui32PageSize, void *hUniqueTag);
void PDumpMallocPagesPhys(enum PVRSRV_DEVICE_TYPE eDeviceType,
		u32 ui32DevVAddr, u32 *pui32PhysPages, u32 ui32NumPages,
		void *hUniqueTag);
void PDumpMallocPageTable(enum PVRSRV_DEVICE_TYPE eDeviceType,
		void *pvLinAddr, u32 ui32NumBytes, void *hUniqueTag);
enum PVRSRV_ERROR PDumpSetMMUContext(enum PVRSRV_DEVICE_TYPE eDeviceType,
				     char *pszMemSpace, u32 *pui32MMUContextID,
				     u32 ui32MMUType, void *hUniqueTag1,
				     void *pvPDCPUAddr);
enum PVRSRV_ERROR PDumpClearMMUContext(enum PVRSRV_DEVICE_TYPE eDeviceType,
				       char *pszMemSpace,
				       u32 ui32MMUContextID, u32 ui32MMUType);
void PDumpFreePages(struct BM_HEAP *psBMHeap,
		struct IMG_DEV_VIRTADDR sDevVAddr, u32 ui32NumBytes,
		 u32 ui32PageSize, void *hUniqueTag, IMG_BOOL bInterleaved);
void PDumpFreePageTable(enum PVRSRV_DEVICE_TYPE eDeviceType,
		void *pvLinAddr, u32 ui32NumBytes, void *hUniqueTag);
void PDumpPDReg(u32 ui32Reg, u32 ui32dwData, void *hUniqueTag);
void PDumpPDRegWithFlags(u32 ui32Reg, u32 ui32Data, u32 ui32Flags,
		void *hUniqueTag);

enum PVRSRV_ERROR PDumpPDDevPAddrKM(struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
		u32 ui32Offset, struct IMG_DEV_PHYADDR sPDDevPAddr,
		void *hUniqueTag1, void *hUniqueTag2);

IMG_BOOL PDumpTestNextFrame(u32 ui32CurrentFrame);

void PDumpTASignatureRegisters(u32 ui32DumpFrameNum,
		u32 ui32TAKickCount, IMG_BOOL bLastFrame,
		u32 *pui32Registers, u32 ui32NumRegisters);

void PDump3DSignatureRegisters(u32 ui32DumpFrameNum, IMG_BOOL bLastFrame,
		u32 *pui32Registers, u32 ui32NumRegisters);

void PDumpRegRead(const u32 dwRegOffset, u32 ui32Flags);

void PDumpCycleCountRegRead(const u32 dwRegOffset, IMG_BOOL bLastFrame);

void PDumpCounterRegisters(u32 ui32DumpFrameNum, IMG_BOOL bLastFrame,
		u32 *pui32Registers, u32 ui32NumRegisters);

void PDumpCBP(struct PVRSRV_KERNEL_MEM_INFO *psROffMemInfo,
	      u32 ui32ROffOffset,
	      u32 ui32WPosVal,
	      u32 ui32PacketSize,
	      u32 ui32BufferSize, u32 ui32Flags, void *hUniqueTag);

void PDumpIDLWithFlags(u32 ui32Clocks, u32 ui32Flags);
void PDumpIDL(u32 ui32Clocks);

void PDumpSuspendKM(void);
void PDumpResumeKM(void);

#define PDUMPMEMPOL				PDumpMemPolKM
#define PDUMPMEM				PDumpMemKM
#define PDUMPMEM2				PDumpMem2KM
#define PDUMPMEMUM				PDumpMemUM
#define PDUMPINIT				PDumpInitCommon
#define PDUMPDEINIT				PDumpDeInitCommon
#define PDUMPISLASTFRAME			PDumpIsLastCaptureFrameKM
#define PDUMPTESTFRAME				PDumpIsCaptureFrameKM
#define PDUMPTESTNEXTFRAME			PDumpTestNextFrame
#define PDUMPREGWITHFLAGS			PDumpRegWithFlagsKM
#define PDUMPREG				PDUMP_REG_FUNC_NAME
#define PDUMPCOMMENT				PDumpComment
#define PDUMPCOMMENTWITHFLAGS			PDumpCommentWithFlags
#define PDUMPREGPOL				PDumpRegPolKM
#define PDUMPREGPOLWITHFLAGS			PDumpRegPolWithFlagsKM
#define PDUMPMALLOCPAGES			PDumpMallocPages
#define PDUMPMALLOCPAGETABLE			PDumpMallocPageTable
#define PDUMPSETMMUCONTEXT			PDumpSetMMUContext
#define PDUMPCLEARMMUCONTEXT			PDumpClearMMUContext
#define PDUMPFREEPAGES				PDumpFreePages
#define PDUMPFREEPAGETABLE			PDumpFreePageTable
#define PDUMPPDREG				PDumpPDReg
#define PDUMPPDREGWITHFLAGS			PDumpPDRegWithFlags
#define PDUMPCBP				PDumpCBP
#define PDUMPMALLOCPAGESPHYS			PDumpMallocPagesPhys
#define PDUMPENDINITPHASE			PDumpStopInitPhaseKM
#define PDUMPMSVDXREGWRITE			PDumpMsvdxRegWrite
#define PDUMPMSVDXREGREAD			PDumpMsvdxRegRead
#define PDUMPMSVDXPOL				PDumpMsvdxRegPol
#define PDUMPMSVDXWRITEREF			PDumpMsvdxWriteRef
#define PDUMPBITMAPKM				PDumpBitmapKM
#define PDUMPDRIVERINFO				PDumpDriverInfoKM
#define PDUMPIDLWITHFLAGS			PDumpIDLWithFlags
#define PDUMPIDL				PDumpIDL
#define PDUMPSUSPEND				PDumpSuspendKM
#define PDUMPRESUME				PDumpResumeKM

#else
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
#endif

#endif
