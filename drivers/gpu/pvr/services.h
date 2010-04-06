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

#ifndef __SERVICES_H__
#define __SERVICES_H__


#include "img_defs.h"
#include "servicesext.h"
#include "pdumpdefs.h"

struct SYS_DATA;

#define PVRSRV_4K_PAGE_SIZE				4096UL

#define PVRSRV_MAX_CMD_SIZE				1024

#define PVRSRV_MAX_DEVICES				16

#define EVENTOBJNAME_MAXLENGTH				50

#define PVRSRV_MEM_READ					(1UL<<0)
#define PVRSRV_MEM_WRITE				(1UL<<1)
#define PVRSRV_MEM_CACHE_CONSISTENT			(1UL<<2)
#define PVRSRV_MEM_NO_SYNCOBJ				(1UL<<3)
#define PVRSRV_MEM_INTERLEAVED				(1UL<<4)
#define PVRSRV_MEM_DUMMY				(1UL<<5)
#define PVRSRV_MEM_EDM_PROTECT				(1UL<<6)
#define PVRSRV_MEM_ZERO					(1UL<<7)
#define PVRSRV_MEM_USER_SUPPLIED_DEVVADDR		(1UL<<8)
#define PVRSRV_MEM_RAM_BACKED_ALLOCATION		(1UL<<9)
#define PVRSRV_MEM_NO_RESMAN				(1UL<<10)
#define PVRSRV_MEM_EXPORTED				(1UL<<11)

#define PVRSRV_HAP_CACHED				(1UL<<12)
#define PVRSRV_HAP_UNCACHED				(1UL<<13)
#define PVRSRV_HAP_WRITECOMBINE				(1UL<<14)
#define PVRSRV_HAP_CACHETYPE_MASK	(PVRSRV_HAP_CACHED		  | \
					 PVRSRV_HAP_UNCACHED		  | \
					 PVRSRV_HAP_WRITECOMBINE)
#define PVRSRV_HAP_KERNEL_ONLY				(1UL<<15)
#define PVRSRV_HAP_SINGLE_PROCESS			(1UL<<16)
#define PVRSRV_HAP_MULTI_PROCESS			(1UL<<17)
#define PVRSRV_HAP_FROM_EXISTING_PROCESS		(1UL<<18)
#define PVRSRV_HAP_NO_CPU_VIRTUAL			(1UL<<19)
#define PVRSRV_HAP_MAPTYPE_MASK		(PVRSRV_HAP_KERNEL_ONLY		  | \
					 PVRSRV_HAP_SINGLE_PROCESS	  | \
					 PVRSRV_HAP_MULTI_PROCESS	  | \
					 PVRSRV_HAP_FROM_EXISTING_PROCESS | \
					 PVRSRV_HAP_NO_CPU_VIRTUAL)
#define PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT		24

#define PVRSRV_MAP_NOUSERVIRTUAL			(1UL << 27)

#define PVRSRV_NO_CONTEXT_LOSS				0
#define PVRSRV_SEVERE_LOSS_OF_CONTEXT			1
#define PVRSRV_PRE_STATE_CHANGE_MASK			0x80

#define PVRSRV_DEFAULT_DEV_COOKIE			1

#define PVRSRV_MISC_INFO_TIMER_PRESENT			(1UL << 0)
#define PVRSRV_MISC_INFO_CLOCKGATE_PRESENT		(1UL << 1)
#define PVRSRV_MISC_INFO_MEMSTATS_PRESENT		(1UL << 2)
#define PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT	(1UL << 3)
#define PVRSRV_MISC_INFO_DDKVERSION_PRESENT		(1UL << 4)

#define PVRSRV_PDUMP_MAX_FILENAME_SIZE			20
#define PVRSRV_PDUMP_MAX_COMMENT_SIZE			200

#define PVRSRV_CHANGEDEVMEM_ATTRIBS_CACHECOHERENT	0x00000001

#define PVRSRV_MAPEXTMEMORY_FLAGS_ALTERNATEVA		0x00000001
#define PVRSRV_MAPEXTMEMORY_FLAGS_PHYSCONTIG		0x00000002

#define PVRSRV_MODIFYSYNCOPS_FLAGS_WOP_INC		0x00000001
#define PVRSRV_MODIFYSYNCOPS_FLAGS_ROP_INC		0x00000002
#define PVRSRV_MODIFYSYNCOPS_FLAGS_WOC_INC		0x00000004
#define PVRSRV_MODIFYSYNCOPS_FLAGS_ROC_INC		0x00000008

enum PVRSRV_DEVICE_TYPE {
	PVRSRV_DEVICE_TYPE_UNKNOWN = 0,
	PVRSRV_DEVICE_TYPE_MBX1 = 1,
	PVRSRV_DEVICE_TYPE_MBX1_LITE = 2,

	PVRSRV_DEVICE_TYPE_M24VA = 3,
	PVRSRV_DEVICE_TYPE_MVDA2 = 4,
	PVRSRV_DEVICE_TYPE_MVED1 = 5,
	PVRSRV_DEVICE_TYPE_MSVDX = 6,

	PVRSRV_DEVICE_TYPE_SGX = 7,

	PVRSRV_DEVICE_TYPE_VGX = 8,

	PVRSRV_DEVICE_TYPE_EXT = 9,

	PVRSRV_DEVICE_TYPE_LAST = 9,

	PVRSRV_DEVICE_TYPE_FORCE_I32 = 0x7fffffff
};

#define HEAP_ID(_dev_ , _dev_heap_idx_)					\
	(((_dev_) << 24) | ((_dev_heap_idx_) & ((1 << 24) - 1)))

#define HEAP_IDX(_heap_id_)						\
	((_heap_id_) & ((1 << 24) - 1))

#define HEAP_DEV(_heap_id_)						\
	((_heap_id_) >> 24)

#define PVRSRV_UNDEFINED_HEAP_ID			(~0LU)

enum IMG_MODULE_ID {
	IMG_EGL =		0x00000001,
	IMG_OPENGLES1 =		0x00000002,
	IMG_OPENGLES2 =		0x00000003,
	IMG_D3DM =		0x00000004,
	IMG_SRV_UM =		0x00000005,
	IMG_OPENVG =		0x00000006,
	IMG_SRVCLIENT =		0x00000007,
	IMG_VISTAKMD =		0x00000008,
	IMG_VISTA3DNODE =	0x00000009,
	IMG_VISTAMVIDEONODE =	0x0000000A,
	IMG_VISTAVPBNODE =	0x0000000B,
	IMG_OPENGL =		0x0000000C,
	IMG_D3D =		0x0000000D
};

struct PVRSRV_CONNECTION {
	void *hServices;
	u32 ui32ProcessID;
};

struct PVRSRV_DEV_DATA {
	struct PVRSRV_CONNECTION sConnection;
	void *hDevCookie;
};

struct PVRSRV_HWREG {
	u32 ui32RegAddr;
	u32 ui32RegVal;
};

struct PVRSRV_MEMBLK {
	struct IMG_DEV_VIRTADDR sDevVirtAddr;
	void *hOSMemHandle;
	void *hOSWrapMem;
	void *hBuffer;
	void *hResItem;
	struct IMG_SYS_PHYADDR *psIntSysPAddr;
};

struct PVRSRV_KERNEL_MEM_INFO;

struct PVRSRV_CLIENT_MEM_INFO {
	void *pvLinAddr;
	void *pvLinAddrKM;
	struct IMG_DEV_VIRTADDR sDevVAddr;
	struct IMG_CPU_PHYADDR sCpuPAddr;
	u32 ui32Flags;
	u32 ui32ClientFlags;
	u32 ui32AllocSize;
	struct PVRSRV_CLIENT_SYNC_INFO *psClientSyncInfo;
	void *hMappingInfo;
	void *hKernelMemInfo;
	void *hResItem;
	struct PVRSRV_CLIENT_MEM_INFO *psNext;
};

#define PVRSRV_MAX_CLIENT_HEAPS (32)
struct PVRSRV_HEAP_INFO {
	u32 ui32HeapID;
	void *hDevMemHeap;
	struct IMG_DEV_VIRTADDR sDevVAddrBase;
	u32 ui32HeapByteSize;
	u32 ui32Attribs;
};

struct PVRSRV_DEVICE_IDENTIFIER {
	enum PVRSRV_DEVICE_TYPE eDeviceType;
	enum PVRSRV_DEVICE_CLASS eDeviceClass;
	u32 ui32DeviceIndex;

};

struct PVRSRV_EVENTOBJECT {
	char szName[EVENTOBJNAME_MAXLENGTH];
	void *hOSEventKM;
};

struct PVRSRV_MISC_INFO {
	u32 ui32StateRequest;
	u32 ui32StatePresent;

	void *pvSOCTimerRegisterKM;
	void *pvSOCTimerRegisterUM;
	void *hSOCTimerRegisterOSMemHandle;
	void *hSOCTimerRegisterMappingInfo;

	void *pvSOCClockGateRegs;
	u32 ui32SOCClockGateRegsSize;

	char *pszMemoryStr;
	u32 ui32MemoryStrLen;

	struct PVRSRV_EVENTOBJECT sGlobalEventObject;
	void *hOSGlobalEvent;

	u32 aui32DDKVersion[4];
};

enum PVRSRV_ERROR AllocateDeviceID(struct SYS_DATA *psSysData, u32 *pui32DevID);
enum PVRSRV_ERROR FreeDeviceID(struct SYS_DATA *psSysData, u32 ui32DevID);

#endif
