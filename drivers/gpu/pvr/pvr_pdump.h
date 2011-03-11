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

#ifndef _PVR_PDUMP_H_
#define _PVR_PDUMP_H_


#define PDUMP_FLAGS_NEVER		0x08000000
#define PDUMP_FLAGS_TOOUT2MEM		0x10000000
#define PDUMP_FLAGS_LASTFRAME		0x20000000
#define PDUMP_FLAGS_RESETLFBUFFER	0x40000000
#define PDUMP_FLAGS_CONTINUOUS		0x80000000

#define PDUMP_PD_UNIQUETAG		((void *)0)
#define PDUMP_PT_UNIQUETAG		((void *)0)

#ifdef PDUMP

#define MAKEUNIQUETAG(hMemInfo)						\
	(((struct BM_BUF *)(((struct PVRSRV_KERNEL_MEM_INFO *)		\
			     hMemInfo)->sMemBlk.hBuffer))->pMapping)

void PDumpMemPolKM(struct PVRSRV_KERNEL_MEM_INFO
					   *psMemInfo, u32 ui32Offset,
					   u32 ui32Value, u32 ui32Mask,
					   enum PDUMP_POLL_OPERATOR eOperator,
					   void *hUniqueTag);

enum PVRSRV_ERROR PDumpMemUM(void *pvAltLinAddr, void *pvLinAddr,
			     struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
			     u32 ui32Offset, u32 ui32Bytes, u32 ui32Flags,
			     void *hUniqueTag);

enum PVRSRV_ERROR PDumpMemKM(void *pvAltLinAddr,
			     struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
			     u32 ui32Offset, u32 ui32Bytes,
			     u32 ui32Flags, void *hUniqueTag);

enum PVRSRV_ERROR PDumpPageTableKM(void *pvLinAddr, u32 ui32Bytes,
				   IMG_BOOL bInitialisePages,
				   void *hUniqueTag1, void *hUniqueTag2);
void PDumpInit(void);
void PDumpDeInit(void);
void PDumpSetFrameKM(u32 ui32PID, u32 ui32Frame);
void PDumpCommentKM(char *pszComment, u32 ui32Flags);
void PDumpRegWithFlagsKM(u32 ui32RegAddr, u32 ui32RegValue,
			 u32 ui32Flags);

void PDumpBitmapKM(char *pszFileName, u32 ui32FileOffset,
		   u32 ui32Width, u32 ui32Height, u32 ui32StrideInBytes,
		   struct IMG_DEV_VIRTADDR sDevBaseAddr, u32 ui32Size,
		   enum PDUMP_PIXEL_FORMAT ePixelFormat,
		   enum PDUMP_MEM_FORMAT eMemFormat, u32 ui32PDumpFlags);
void PDumpHWPerfCBKM(char *pszFileName, u32 ui32FileOffset,
		     struct IMG_DEV_VIRTADDR sDevBaseAddr,
		     u32 ui32Size, u32 ui32PDumpFlags);
void PDumpReg(u32 dwReg, u32 dwData);

void PDumpComment(char *pszFormat, ...);

void PDumpCommentWithFlags(u32 ui32Flags, char *pszFormat, ...);
void PDumpRegPolKM(u32 ui32RegAddr, u32 ui32RegValue,
		   u32 ui32Mask);
void PDumpRegPolWithFlagsKM(u32 ui32RegAddr, u32 ui32RegValue,
			    u32 ui32Mask, u32 ui32Flags);

IMG_BOOL PDumpIsCaptureFrameKM(void);

void PDumpMallocPages(u32 ui32DevVAddr, void *hOSMemHandle,
		      u32 ui32NumBytes, void *hUniqueTag);
void PDumpMallocPageTable(void *pvLinAddr, void *hUniqueTag);
void PDumpFreePages(struct BM_HEAP *psBMHeap,
		    struct IMG_DEV_VIRTADDR sDevVAddr, u32 ui32NumBytes,
		    void *hUniqueTag, IMG_BOOL bInterleaved);
void PDumpFreePageTable(void *pvLinAddr);
void PDumpPDReg(u32 ui32Reg, u32 ui32dwData);
void PDumpPDRegWithFlags(u32 ui32Reg, u32 ui32Data, u32 ui32Flags);

void PDumpPDDevPAddrKM(struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
		       u32 ui32Offset, struct IMG_DEV_PHYADDR sPDDevPAddr,
		       void *hUniqueTag1, void *hUniqueTag2);

void PDumpTASignatureRegisters(u32 ui32DumpFrameNum, u32 ui32TAKickCount,
			       u32 *pui32Registers, u32 ui32NumRegisters);

void PDump3DSignatureRegisters(u32 ui32DumpFrameNum,
			       u32 *pui32Registers, u32 ui32NumRegisters);

void PDumpRegRead(const u32 dwRegOffset, u32 ui32Flags);

void PDumpCycleCountRegRead(const u32 dwRegOffset);

void PDumpCounterRegisters(u32 ui32DumpFrameNum,
			   u32 *pui32Registers, u32 ui32NumRegisters);

void PDumpCBP(struct PVRSRV_KERNEL_MEM_INFO *psROffMemInfo,
	      u32 ui32ROffOffset,
	      u32 ui32WPosVal,
	      u32 ui32PacketSize,
	      u32 ui32BufferSize, void *hUniqueTag);

void PDumpIDLWithFlags(u32 ui32Clocks, u32 ui32Flags);

void PDumpSuspendKM(void);
void PDumpResumeKM(void);

#define PDUMPMEMPOL				PDumpMemPolKM
#define PDUMPMEM				PDumpMemKM
#define PDUMPPAGETABLE				PDumpPageTableKM
#define PDUMPMEMUM				PDumpMemUM
#define PDUMPINIT				PDumpInit
#define PDUMPDEINIT				PDumpDeInit
#define PDUMPREGWITHFLAGS			PDumpRegWithFlagsKM
#define PDUMPREG				PDumpReg
#define PDUMPCOMMENT				PDumpComment
#define PDUMPCOMMENTWITHFLAGS			PDumpCommentWithFlags
#define PDUMPREGPOL				PDumpRegPolKM
#define PDUMPREGPOLWITHFLAGS			PDumpRegPolWithFlagsKM
#define PDUMPMALLOCPAGES			PDumpMallocPages
#define PDUMPMALLOCPAGETABLE			PDumpMallocPageTable
#define PDUMPFREEPAGES				PDumpFreePages
#define PDUMPFREEPAGETABLE			PDumpFreePageTable
#define PDUMPPDREGWITHFLAGS			PDumpPDRegWithFlags
#define PDUMPCBP				PDumpCBP
#define PDUMPIDLWITHFLAGS			PDumpIDLWithFlags
#define PDUMPSUSPEND				PDumpSuspendKM
#define PDUMPRESUME				PDumpResumeKM

#else

#define MAKEUNIQUETAG(hMemInfo)	(0)

#define PDUMPMEMPOL(args...)
#define PDUMPMEM(args...)
#define PDUMPPAGETABLE(args...)
#define PDUMPMEMUM(args...)
#define PDUMPINIT(args...)
#define PDUMPDEINIT(args...)
#define PDUMPREGWITHFLAGS(args...)
#define PDUMPREG(args...)
#define PDUMPCOMMENT(args...)
#define PDUMPCOMMENTWITHFLAGS(args...)
#define PDUMPREGPOL(args...)
#define PDUMPREGPOLWITHFLAGS(args...)
#define PDUMPMALLOCPAGES(args...)
#define PDUMPMALLOCPAGETABLE(args...)
#define PDUMPFREEPAGES(args...)
#define PDUMPFREEPAGETABLE(args...)
#define PDUMPPDREGWITHFLAGS(args...)
#define PDUMPCBP(args...)
#define PDUMPIDLWITHFLAGS(args...)
#define PDUMPSUSPEND(args...)
#define PDUMPRESUME(args...)
#endif

#endif /* _PVR_PDUMP_H_ */
