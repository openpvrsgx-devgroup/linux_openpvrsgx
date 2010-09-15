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

#ifndef __SGXAPI_KM_H__
#define __SGXAPI_KM_H__


#include "sgxdefs.h"

#if defined(__KERNEL__)
#include <linux/unistd.h>
#else
#include <unistd.h>
#endif

#define SGX_UNDEFINED_HEAP_ID				(~0LU)
#define SGX_GENERAL_HEAP_ID				0
#define SGX_TADATA_HEAP_ID				1
#define SGX_KERNEL_CODE_HEAP_ID				2
#define SGX_KERNEL_DATA_HEAP_ID				3
#define SGX_PIXELSHADER_HEAP_ID				4
#define SGX_VERTEXSHADER_HEAP_ID			5
#define SGX_PDSPIXEL_CODEDATA_HEAP_ID			6
#define SGX_PDSVERTEX_CODEDATA_HEAP_ID			7
#define SGX_SYNCINFO_HEAP_ID				8
#define SGX_3DPARAMETERS_HEAP_ID			9
#define SGX_MAX_HEAP_ID					10

#define SGX_MAX_TA_STATUS_VALS				32
#define SGX_MAX_3D_STATUS_VALS_OLD			2
#define SGX_MAX_3D_STATUS_VALS				4

#define SGX_MAX_SRC_SYNCS				4

#define	PVRSRV_SGX_HWPERF_NUM_COUNTERS			9

#define PVRSRV_SGX_HWPERF_INVALID			0x1

#define PVRSRV_SGX_HWPERF_TRANSFER			0x2
#define PVRSRV_SGX_HWPERF_TA				0x3
#define PVRSRV_SGX_HWPERF_3D				0x4
#define PVRSRV_SGX_HWPERF_2D				0x5

#define PVRSRV_SGX_HWPERF_MK_EVENT			0x101
#define PVRSRV_SGX_HWPERF_MK_TA				0x102
#define PVRSRV_SGX_HWPERF_MK_3D				0x103
#define PVRSRV_SGX_HWPERF_MK_2D				0x104

#define PVRSRV_SGX_HWPERF_TYPE_STARTEND_BIT		28
#define PVRSRV_SGX_HWPERF_TYPE_OP_MASK					\
	((1 << PVRSRV_SGX_HWPERF_TYPE_STARTEND_BIT) - 1)
#define PVRSRV_SGX_HWPERF_TYPE_OP_START					\
	(0 << PVRSRV_SGX_HWPERF_TYPE_STARTEND_BIT)
#define PVRSRV_SGX_HWPERF_TYPE_OP_END					\
	(1 << PVRSRV_SGX_HWPERF_TYPE_STARTEND_BIT)

#define PVRSRV_SGX_HWPERF_TYPE_TRANSFER_START				\
	(PVRSRV_SGX_HWPERF_TRANSFER | PVRSRV_SGX_HWPERF_TYPE_OP_START)
#define PVRSRV_SGX_HWPERF_TYPE_TRANSFER_END				\
	(PVRSRV_SGX_HWPERF_TRANSFER | PVRSRV_SGX_HWPERF_TYPE_OP_END)
#define PVRSRV_SGX_HWPERF_TYPE_TA_START					\
	(PVRSRV_SGX_HWPERF_TA | PVRSRV_SGX_HWPERF_TYPE_OP_START)
#define PVRSRV_SGX_HWPERF_TYPE_TA_END					\
	(PVRSRV_SGX_HWPERF_TA | PVRSRV_SGX_HWPERF_TYPE_OP_END)
#define PVRSRV_SGX_HWPERF_TYPE_3D_START					\
	(PVRSRV_SGX_HWPERF_3D | PVRSRV_SGX_HWPERF_TYPE_OP_START)
#define PVRSRV_SGX_HWPERF_TYPE_3D_END					\
	(PVRSRV_SGX_HWPERF_3D | PVRSRV_SGX_HWPERF_TYPE_OP_END)
#define PVRSRV_SGX_HWPERF_TYPE_2D_START					\
	(PVRSRV_SGX_HWPERF_2D | PVRSRV_SGX_HWPERF_TYPE_OP_START)
#define PVRSRV_SGX_HWPERF_TYPE_2D_END					\
	(PVRSRV_SGX_HWPERF_2D | PVRSRV_SGX_HWPERF_TYPE_OP_END)

#define PVRSRV_SGX_HWPERF_TYPE_MK_EVENT_START				\
	(PVRSRV_SGX_HWPERF_MK_EVENT | PVRSRV_SGX_HWPERF_TYPE_OP_START)
#define PVRSRV_SGX_HWPERF_TYPE_MK_EVENT_END				\
	(PVRSRV_SGX_HWPERF_MK_EVENT | PVRSRV_SGX_HWPERF_TYPE_OP_END)
#define PVRSRV_SGX_HWPERF_TYPE_MK_TA_START				\
	(PVRSRV_SGX_HWPERF_MK_TA | PVRSRV_SGX_HWPERF_TYPE_OP_START)
#define PVRSRV_SGX_HWPERF_TYPE_MK_TA_END				\
	(PVRSRV_SGX_HWPERF_MK_TA | PVRSRV_SGX_HWPERF_TYPE_OP_END)
#define PVRSRV_SGX_HWPERF_TYPE_MK_3D_START				\
	(PVRSRV_SGX_HWPERF_MK_3D | PVRSRV_SGX_HWPERF_TYPE_OP_START)
#define PVRSRV_SGX_HWPERF_TYPE_MK_3D_END				\
	(PVRSRV_SGX_HWPERF_MK_3D | PVRSRV_SGX_HWPERF_TYPE_OP_END)
#define PVRSRV_SGX_HWPERF_TYPE_MK_2D_START				\
	(PVRSRV_SGX_HWPERF_MK_2D | PVRSRV_SGX_HWPERF_TYPE_OP_START)
#define PVRSRV_SGX_HWPERF_TYPE_MK_2D_END				\
	(PVRSRV_SGX_HWPERF_MK_2D | PVRSRV_SGX_HWPERF_TYPE_OP_END)

#define PVRSRV_SGX_HWPERF_OFF					0x0
#define PVRSRV_SGX_HWPERF_GRAPHICS_ON				(1UL << 0)
#define PVRSRV_SGX_HWPERF_MK_EXECUTION_ON			(1UL << 1)

struct PVRSRV_SGX_HWPERF_CB_ENTRY {
	u32 ui32FrameNo;
	u32 ui32Type;
	u32 ui32Ordinal;
	u32 ui32Clocksx16;
	u32 ui32Counters[PVRSRV_SGX_HWPERF_NUM_COUNTERS];
};

struct PVRSRV_SGX_HWPERF_CBDATA {
	u32 ui32FrameNo;
	u32 ui32Type;
	u32 ui32StartTimeWraps;
	u32 ui32StartTime;
	u32 ui32EndTimeWraps;
	u32 ui32EndTime;
	u32 ui32ClockSpeed;
	u32 ui32TimeMax;
};

struct SGX_MISC_INFO_HWPERF_RETRIEVE_CB {
	struct PVRSRV_SGX_HWPERF_CBDATA *psHWPerfData;
	u32 ui32ArraySize;
	u32 ui32DataCount;
	u32 ui32Time;
};

struct CTL_STATUS {
	struct IMG_DEV_VIRTADDR sStatusDevAddr;
	u32 ui32StatusValue;
};

enum SGX_MISC_INFO_REQUEST {
	SGX_MISC_INFO_REQUEST_CLOCKSPEED = 0,
	SGX_MISC_INFO_REQUEST_SGXREV,
	SGX_MISC_INFO_REQUEST_DRIVER_SGXREV,
	SGX_MISC_INFO_REQUEST_SET_HWPERF_STATUS,
	SGX_MISC_INFO_REQUEST_HWPERF_CB_ON,
	SGX_MISC_INFO_REQUEST_HWPERF_CB_OFF,
	SGX_MISC_INFO_REQUEST_HWPERF_RETRIEVE_CB,
	SGX_MISC_INFO_REQUEST_FORCE_I16 = 0x7fff
};

struct PVRSRV_SGX_MISCINFO_FEATURES {
	u32 ui32CoreRev;
	u32 ui32CoreID;
	u32 ui32DDKVersion;
	u32 ui32DDKBuild;
	u32 ui32CoreIdSW;
	u32 ui32CoreRevSW;
	u32 ui32BuildOptions;
};

struct SGX_MISC_INFO {
	enum SGX_MISC_INFO_REQUEST eRequest;

	union {
		u32 reserved;
		struct PVRSRV_SGX_MISCINFO_FEATURES sSGXFeatures;
		u32 ui32SGXClockSpeed;
		u32 ui32NewHWPerfStatus;
		struct SGX_MISC_INFO_HWPERF_RETRIEVE_CB sRetrieveCB;
	} uData;
};

#define SGX_KICKTA_DUMPBITMAP_MAX_NAME_LENGTH		256

struct SGX_KICKTA_DUMPBITMAP {
	struct IMG_DEV_VIRTADDR sDevBaseAddr;
	u32 ui32Flags;
	u32 ui32Width;
	u32 ui32Height;
	u32 ui32Stride;
	u32 ui32PDUMPFormat;
	u32 ui32BytesPP;
	char pszName[SGX_KICKTA_DUMPBITMAP_MAX_NAME_LENGTH];
};

#define PVRSRV_SGX_PDUMP_CONTEXT_MAX_BITMAP_ARRAY_SIZE	16

struct PVRSRV_SGX_PDUMP_CONTEXT {
	u32 ui32CacheControl;
};

struct SGX_KICKTA_DUMP_ROFF {
	void *hKernelMemInfo;
	u32 uiAllocIndex;
	u32 ui32Offset;
	u32 ui32Value;
	char *pszName;
};

struct SGX_KICKTA_DUMP_BUFFER {
	u32 ui32SpaceUsed;
	u32 ui32Start;
	u32 ui32End;
	u32 ui32BufferSize;
	u32 ui32BackEndLength;
	u32 uiAllocIndex;
	void *hKernelMemInfo;
	void *pvLinAddr;
	char *pszName;
};

#ifdef PDUMP
struct SGX_KICKTA_PDUMP {

	struct SGX_KICKTA_DUMPBITMAP *psPDumpBitmapArray;
	u32 ui32PDumpBitmapSize;

	struct SGX_KICKTA_DUMP_BUFFER *psBufferArray;
	u32 ui32BufferArraySize;

	struct SGX_KICKTA_DUMP_ROFF *psROffArray;
	u32 ui32ROffArraySize;
};
#endif

#define SGX_MAX_TRANSFER_STATUS_VALS	2
#define SGX_MAX_TRANSFER_SYNC_OPS	5

#endif
