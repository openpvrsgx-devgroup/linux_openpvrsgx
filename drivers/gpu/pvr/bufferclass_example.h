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

#ifndef __BC_EXAMPLE_H__
#define __BC_EXAMPLE_H__

#include "img_defs.h"
#include "servicesext.h"
#include "kernelbuffer.h"

extern IMG_BOOL PVRGetBufferClassJTable(
				struct PVRSRV_BC_BUFFER2SRV_KMJTABLE *psJTable);

#define BC_EXAMPLE_NUM_BUFFERS		3

#define YUV420				1
#ifdef YUV420

#define BC_EXAMPLE_WIDTH		320
#define BC_EXAMPLE_HEIGHT		160
#define BC_EXAMPLE_STRIDE		320
#define BC_EXAMPLE_PIXELFORMAT		PVRSRV_PIXEL_FORMAT_NV12

#else

#define BC_EXAMPLE_WIDTH		320
#define BC_EXAMPLE_HEIGHT		160
#define BC_EXAMPLE_STRIDE		(320 * 2)
#define BC_EXAMPLE_PIXELFORMAT		PVRSRV_PIXEL_FORMAT_RGB565

#endif

#define BC_EXAMPLE_DEVICEID		 0

struct BC_EXAMPLE_BUFFER {
	u32 ui32Size;
	void *hMemHandle;
	struct IMG_SYS_PHYADDR sSysAddr;
	struct IMG_SYS_PHYADDR sPageAlignSysAddr;
	void __iomem *sCPUVAddr;
	struct PVRSRV_SYNC_DATA *psSyncData;
	struct BC_EXAMPLE_BUFFER *psNext;
};

struct BC_EXAMPLE_DEVINFO {
	u32 ui32DeviceID;
	struct BC_EXAMPLE_BUFFER *psSystemBuffer;
	struct BUFFER_INFO sBufferInfo;
	u32 ui32NumBuffers;
	struct PVRSRV_BC_BUFFER2SRV_KMJTABLE sPVRJTable;
	struct PVRSRV_BC_SRV2BUFFER_KMJTABLE sBCJTable;
	void *hPVRServices;
	u32 ui32RefCount;
};

enum PVRSRV_ERROR BC_Example_Init(void);
enum PVRSRV_ERROR BC_Example_Deinit(void);

enum PVRSRV_ERROR BCOpenPVRServices(void **phPVRServices);
enum PVRSRV_ERROR BCClosePVRServices(void *hPVRServices);

void *BCAllocKernelMem(u32 ui32Size);
void BCFreeKernelMem(void *pvMem);

enum PVRSRV_ERROR BCAllocContigMemory(u32 ui32Size, void **phMemHandle,
				 void __iomem **pLinAddr,
				 struct IMG_CPU_PHYADDR *pPhysAddr);
void BCFreeContigMemory(u32 ui32Size, void *hMemHandle, void __iomem *LinAddr,
			struct IMG_CPU_PHYADDR PhysAddr);

struct IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(struct IMG_CPU_PHYADDR cpu_paddr);
struct IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(struct IMG_SYS_PHYADDR sys_paddr);

void *MapPhysAddr(struct IMG_SYS_PHYADDR sSysAddr, u32 ui32Size);
void UnMapPhysAddr(void *pvAddr, u32 ui32Size);

enum PVRSRV_ERROR BCGetLibFuncAddr(void *hExtDrv, char *szFunctionName,
	 IMG_BOOL (**ppfnFuncTable)(struct PVRSRV_BC_BUFFER2SRV_KMJTABLE *));
struct BC_EXAMPLE_DEVINFO *GetAnchorPtr(void);

#endif
