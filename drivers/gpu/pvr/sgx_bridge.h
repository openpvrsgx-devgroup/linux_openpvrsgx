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

#if !defined(__SGX_BRIDGE_H__)
#define __SGX_BRIDGE_H__

#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "pvr_bridge.h"

#define PVRSRV_BRIDGE_SGX_CMD_BASE (PVRSRV_BRIDGE_LAST_NON_DEVICE_CMD+1)
#define PVRSRV_BRIDGE_SGX_GETCLIENTINFO					\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+0)
#define PVRSRV_BRIDGE_SGX_RELEASECLIENTINFO				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+1)
#define PVRSRV_BRIDGE_SGX_GETINTERNALDEVINFO				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+2)
#define PVRSRV_BRIDGE_SGX_DOKICK					\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+3)
#define PVRSRV_BRIDGE_SGX_GETPHYSPAGEADDR				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+4)
#define PVRSRV_BRIDGE_SGX_READREGISTRYDWORD				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+5)
#define PVRSRV_BRIDGE_SGX_SCHEDULECOMMAND				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+6)

#define PVRSRV_BRIDGE_SGX_2DQUERYBLTSCOMPLETE				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+9)

#define PVRSRV_BRIDGE_SGX_GETMMUPDADDR					\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+10)

#define PVRSRV_BRIDGE_SGX_SUBMITTRANSFER				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+13)
#define PVRSRV_BRIDGE_SGX_GETMISCINFO					\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+14)
#define PVRSRV_BRIDGE_SGXINFO_FOR_SRVINIT				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+15)
#define PVRSRV_BRIDGE_SGX_DEVINITPART2					\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+16)

#define PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+17)
#define PVRSRV_BRIDGE_SGX_UNREFSHAREDPBDESC				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+18)
#define PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC				\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+19)
#define PVRSRV_BRIDGE_SGX_REGISTER_HW_RENDER_CONTEXT			\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+20)
#define PVRSRV_BRIDGE_SGX_FLUSH_HW_RENDER_TARGET			\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+21)
#define PVRSRV_BRIDGE_SGX_UNREGISTER_HW_RENDER_CONTEXT			\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+22)
#define PVRSRV_BRIDGE_SGX_REGISTER_HW_TRANSFER_CONTEXT			\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+26)
#define PVRSRV_BRIDGE_SGX_UNREGISTER_HW_TRANSFER_CONTEXT		\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+27)

#define PVRSRV_BRIDGE_SGX_SCHEDULE_PROCESS_QUEUES			\
				PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+28)

#define PVRSRV_BRIDGE_SGX_READ_DIFF_COUNTERS				\
		PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+29)
#define PVRSRV_BRIDGE_SGX_READ_HWPERF_CB				\
		PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+30)

#if defined(PDUMP)
#define PVRSRV_BRIDGE_SGX_PDUMP_BUFFER_ARRAY				\
		PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+31)
#define PVRSRV_BRIDGE_SGX_PDUMP_3D_SIGNATURE_REGISTERS			\
		PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+32)
#define PVRSRV_BRIDGE_SGX_PDUMP_COUNTER_REGISTERS			\
		PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+33)
#define PVRSRV_BRIDGE_SGX_PDUMP_TA_SIGNATURE_REGISTERS			\
		PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+34)
#define PVRSRV_BRIDGE_SGX_PDUMP_HWPERFCB				\
		PVRSRV_IOWR(PVRSRV_BRIDGE_SGX_CMD_BASE+35)
#endif

#define PVRSRV_BRIDGE_LAST_SGX_CMD (PVRSRV_BRIDGE_SGX_CMD_BASE+35)

struct PVRSRV_BRIDGE_IN_GETPHYSPAGEADDR {
	u32 ui32BridgeFlags;
	void *hDevMemHeap;
	struct IMG_DEV_VIRTADDR sDevVAddr;
};

struct PVRSRV_BRIDGE_OUT_GETPHYSPAGEADDR {
	enum PVRSRV_ERROR eError;
	struct IMG_DEV_PHYADDR DevPAddr;
	struct IMG_CPU_PHYADDR CpuPAddr;
};

struct PVRSRV_BRIDGE_IN_SGX_GETMMU_PDADDR {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	void *hDevMemContext;
};

struct PVRSRV_BRIDGE_OUT_SGX_GETMMU_PDADDR {
	struct IMG_DEV_PHYADDR sPDDevPAddr;
	enum PVRSRV_ERROR eError;
};

struct PVRSRV_BRIDGE_IN_GETCLIENTINFO {
	u32 ui32BridgeFlags;
	void *hDevCookie;
};

struct PVRSRV_BRIDGE_OUT_GETINTERNALDEVINFO {
	struct SGX_INTERNAL_DEVINFO sSGXInternalDevInfo;
	enum PVRSRV_ERROR eError;
};

struct PVRSRV_BRIDGE_IN_GETINTERNALDEVINFO {
	u32 ui32BridgeFlags;
	void *hDevCookie;
};

struct PVRSRV_BRIDGE_OUT_GETCLIENTINFO {
	struct SGX_CLIENT_INFO sClientInfo;
	enum PVRSRV_ERROR eError;
};

struct PVRSRV_BRIDGE_IN_RELEASECLIENTINFO {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	struct SGX_CLIENT_INFO sClientInfo;
};

struct PVRSRV_BRIDGE_IN_ISPBREAKPOLL {
	u32 ui32BridgeFlags;
	void *hDevCookie;
};

struct PVRSRV_BRIDGE_IN_DOKICK {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	struct SGX_CCB_KICK sCCBKick;
};

struct PVRSRV_BRIDGE_IN_SGX_SCHEDULE_PROCESS_QUEUES {
	u32 ui32BridgeFlags;
	void *hDevCookie;
};

struct PVRSRV_BRIDGE_IN_SUBMITTRANSFER {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	struct PVRSRV_TRANSFER_SGX_KICK sKick;
};


struct PVRSRV_BRIDGE_IN_READREGDWORD {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	char *pszKey;
	char *pszValue;
};

struct PVRSRV_BRIDGE_OUT_READREGDWORD {
	enum PVRSRV_ERROR eError;
	u32 ui32Data;
};

struct PVRSRV_BRIDGE_IN_SCHEDULECOMMAND {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	enum SGXMKIF_COMMAND_TYPE eCommandType;
	struct SGXMKIF_COMMAND *psCommandData;

};

struct PVRSRV_BRIDGE_IN_SGXGETMISCINFO {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	struct SGX_MISC_INFO __user *psMiscInfo;
};

struct PVRSRV_BRIDGE_IN_SGXINFO_FOR_SRVINIT {
	u32 ui32BridgeFlags;
	void *hDevCookie;
};

struct PVRSRV_BRIDGE_OUT_SGXINFO_FOR_SRVINIT {
	enum PVRSRV_ERROR eError;
	struct SGX_BRIDGE_INFO_FOR_SRVINIT sInitInfo;
};

struct PVRSRV_BRIDGE_IN_SGXDEVINITPART2 {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	struct SGX_BRIDGE_INIT_INFO sInitInfo;
};

enum pvr_sync_wait_seq_type {
	_PVR_SYNC_WAIT_BLOCK,
	_PVR_SYNC_WAIT_NONBLOCK,
	_PVR_SYNC_WAIT_EVENT,
};

struct PVRSRV_BRIDGE_IN_2DQUERYBLTSCOMPLETE {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	void *hKernSyncInfo;
	u64 user_data;
	enum pvr_sync_wait_seq_type type;
};

#define PVRSRV_BRIDGE_SGX_SHAREDPBDESC_MAX_SUBMEMINFOS 10

struct PVRSRV_BRIDGE_IN_SGXFINDSHAREDPBDESC {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	IMG_BOOL bLockOnFailure;
	u32 ui32TotalPBSize;
};

struct PVRSRV_BRIDGE_OUT_SGXFINDSHAREDPBDESC {
	void *hKernelMemInfo;
	void *hSharedPBDesc;
	void *hSharedPBDescKernelMemInfoHandle;
	void *hHWPBDescKernelMemInfoHandle;
	void *hBlockKernelMemInfoHandle;
	void *ahSharedPBDescSubKernelMemInfoHandles
	    [PVRSRV_BRIDGE_SGX_SHAREDPBDESC_MAX_SUBMEMINFOS];
	u32 ui32SharedPBDescSubKernelMemInfoHandlesCount;
	enum PVRSRV_ERROR eError;
};

struct PVRSRV_BRIDGE_IN_SGXUNREFSHAREDPBDESC {
	u32 ui32BridgeFlags;
	void *hSharedPBDesc;
};

struct PVRSRV_BRIDGE_OUT_SGXUNREFSHAREDPBDESC {
	enum PVRSRV_ERROR eError;
};

struct PVRSRV_BRIDGE_IN_SGXADDSHAREDPBDESC {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	void *hSharedPBDescKernelMemInfo;
	void *hHWPBDescKernelMemInfo;
	void *hBlockKernelMemInfo;
	u32 ui32TotalPBSize;
	void * __user *phKernelMemInfoHandles;
	u32 ui32KernelMemInfoHandlesCount;
};

struct PVRSRV_BRIDGE_OUT_SGXADDSHAREDPBDESC {
	enum PVRSRV_ERROR eError;
	void *hSharedPBDesc;
};

#ifdef	PDUMP
struct PVRSRV_BRIDGE_IN_PDUMP_BUFFER_ARRAY {
	u32 ui32BridgeFlags;
	struct SGX_KICKTA_DUMP_BUFFER __user *psBufferArray;
	u32 ui32BufferArrayLength;
	IMG_BOOL bDumpPolls;
};

struct PVRSRV_BRIDGE_IN_PDUMP_3D_SIGNATURE_REGISTERS {
	u32 ui32BridgeFlags;
	u32 ui32DumpFrameNum;
	IMG_BOOL bLastFrame;
	u32 *pui32Registers;
	u32 ui32NumRegisters;
};

struct PVRSRV_BRIDGE_IN_PDUMP_COUNTER_REGISTERS {
	u32 ui32BridgeFlags;
	u32 ui32DumpFrameNum;
	IMG_BOOL bLastFrame;
	u32 *pui32Registers;
	u32 ui32NumRegisters;
};

struct PVRSRV_BRIDGE_IN_PDUMP_TA_SIGNATURE_REGISTERS {
	u32 ui32BridgeFlags;
	u32 ui32DumpFrameNum;
	u32 ui32TAKickCount;
	IMG_BOOL bLastFrame;
	u32 *pui32Registers;
	u32 ui32NumRegisters;
};

struct PVRSRV_BRIDGE_IN_PDUMP_HWPERFCB {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	char szFileName[PVRSRV_PDUMP_MAX_FILENAME_SIZE];
	u32 ui32FileOffset;
	u32 ui32PDumpFlags;

};

#endif

struct PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_RENDER_CONTEXT {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	struct IMG_DEV_VIRTADDR sHWRenderContextDevVAddr;
};

struct PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_RENDER_CONTEXT {
	enum PVRSRV_ERROR eError;
	void *hHWRenderContext;
};

struct PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_RENDER_CONTEXT {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	void *hHWRenderContext;
};

struct PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_TRANSFER_CONTEXT {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	struct IMG_DEV_VIRTADDR sHWTransferContextDevVAddr;
};

struct PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_TRANSFER_CONTEXT {
	enum PVRSRV_ERROR eError;
	void *hHWTransferContext;
};

struct PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_TRANSFER_CONTEXT {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	void *hHWTransferContext;
};

struct PVRSRV_BRIDGE_IN_SGX_FLUSH_HW_RENDER_TARGET {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	struct IMG_DEV_VIRTADDR sHWRTDataSetDevVAddr;
};

struct PVRSRV_BRIDGE_IN_SGX_READ_DIFF_COUNTERS {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	u32 ui32Reg;
	IMG_BOOL bNew;
	u32 ui32New;
	u32 ui32NewReset;
	u32 ui32CountersReg;
};

struct PVRSRV_BRIDGE_OUT_SGX_READ_DIFF_COUNTERS {
	enum PVRSRV_ERROR eError;
	u32 ui32Old;
	u32 ui32Time;
	IMG_BOOL bActive;
	struct PVRSRV_SGXDEV_DIFF_INFO sDiffs;
};

struct PVRSRV_BRIDGE_IN_SGX_READ_HWPERF_CB {
	u32 ui32BridgeFlags;
	void *hDevCookie;
	u32 ui32ArraySize;
	struct PVRSRV_SGX_HWPERF_CB_ENTRY __user *psHWPerfCBData;
};

struct PVRSRV_BRIDGE_OUT_SGX_READ_HWPERF_CB {
	enum PVRSRV_ERROR eError;
	u32 ui32DataCount;
	u32 ui32ClockSpeed;
	u32 ui32HostTimeStamp;
};

#endif
