/*************************************************************************/ /*!
@Title          bufferclass_example kernel driver structures and prototypes
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
#ifndef __BC_EXAMPLE_H__
#define __BC_EXAMPLE_H__

/* IMG services headers */
#include "img_defs.h"
#include "servicesext.h"
#include "kernelbuffer.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define BC_EXAMPLE_NUM_BUFFERS  3

#define NV12 1
//#define YV12 1
//#define YUV422 1

#ifdef NV12

#define BC_EXAMPLE_WIDTH        (320)
#define BC_EXAMPLE_HEIGHT       (160)
#define BC_EXAMPLE_STRIDE       (320)
#define BC_EXAMPLE_PIXELFORMAT	(PVRSRV_PIXEL_FORMAT_NV12)

#else
#ifdef YV12

#define BC_EXAMPLE_WIDTH        (320)
#define BC_EXAMPLE_HEIGHT       (160)
#define BC_EXAMPLE_STRIDE       (320)
#define BC_EXAMPLE_PIXELFORMAT	(PVRSRV_PIXEL_FORMAT_YV12)

#else
#ifdef YUV422

#define BC_EXAMPLE_WIDTH        (320)
#define BC_EXAMPLE_HEIGHT       (160)
#define BC_EXAMPLE_STRIDE       (320*2)
#define BC_EXAMPLE_PIXELFORMAT	(PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY)

#else

#define BC_EXAMPLE_WIDTH        (320)
#define BC_EXAMPLE_HEIGHT       (160)
#define BC_EXAMPLE_STRIDE       (320*2)
#define BC_EXAMPLE_PIXELFORMAT  (PVRSRV_PIXEL_FORMAT_RGB565)

#endif
#endif
#endif

#define BC_EXAMPLE_DEVICEID      0

typedef void *       BCE_HANDLE;

typedef enum tag_bce_bool
{
	BCE_FALSE = 0,
	BCE_TRUE  = 1,
} BCE_BOOL, *BCE_PBOOL;

/* BC_NOHW buffer structure */
typedef struct BC_EXAMPLE_BUFFER_TAG
{
	unsigned long           ulSize;
	BCE_HANDLE              hMemHandle;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */
#if defined(BC_DISCONTIG_BUFFERS)
	IMG_SYS_PHYADDR				*psSysAddr;
#else
	IMG_SYS_PHYADDR				sSysAddr;
	IMG_SYS_PHYADDR         sPageAlignSysAddr;
#endif
	IMG_CPU_VIRTADDR        sCPUVAddr;
	PVRSRV_SYNC_DATA        *psSyncData;

	struct BC_EXAMPLE_BUFFER_TAG *psNext;
} BC_EXAMPLE_BUFFER;


/* kernel device information structure */
typedef struct BC_EXAMPLE_DEVINFO_TAG
{
	unsigned long           ulDeviceID;

	BC_EXAMPLE_BUFFER       *psSystemBuffer;

	/* number of supported buffers */
	unsigned long           ulNumBuffers;

	/* jump table into PVR services */
	PVRSRV_BC_BUFFER2SRV_KMJTABLE sPVRJTable;

	/* jump table into BC */
	PVRSRV_BC_SRV2BUFFER_KMJTABLE sBCJTable;

	/*
		handle for connection to kernel services
		- OS specific - may not be required
	*/
	BCE_HANDLE              hPVRServices;

	/* ref count */
	unsigned long           ulRefCount;

	/* IMG structures used, to minimise API function code */
	/* replace with own structures where necessary */
	BUFFER_INFO             sBufferInfo;

}  BC_EXAMPLE_DEVINFO;


/*!
 *****************************************************************************
 * Error values
 *****************************************************************************/
typedef enum _BCE_ERROR_
{
	BCE_OK                             =  0,
	BCE_ERROR_GENERIC                  =  1,
	BCE_ERROR_OUT_OF_MEMORY            =  2,
	BCE_ERROR_TOO_FEW_BUFFERS          =  3,
	BCE_ERROR_INVALID_PARAMS           =  4,
	BCE_ERROR_INIT_FAILURE             =  5,
	BCE_ERROR_CANT_REGISTER_CALLBACK   =  6,
	BCE_ERROR_INVALID_DEVICE           =  7,
	BCE_ERROR_DEVICE_REGISTER_FAILED   =  8,
	BCE_ERROR_NO_PRIMARY               =  9
} BCE_ERROR;


#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

#ifndef NULL
#define NULL 0
#endif

BCE_ERROR BC_Example_Register(void);
BCE_ERROR BC_Example_Unregister(void);
BCE_ERROR BC_Example_Buffers_Create(void);
BCE_ERROR BC_Example_Buffers_Destroy(void);
BCE_ERROR BC_Example_Init(void);
BCE_ERROR BC_Example_Deinit(void);

/* OS Specific APIs */
BCE_ERROR BCOpenPVRServices(BCE_HANDLE *phPVRServices);
BCE_ERROR BCClosePVRServices(BCE_HANDLE hPVRServices);

void *BCAllocKernelMem(unsigned long ulSize);
void BCFreeKernelMem(void *pvMem);
#if defined(BC_DISCONTIG_BUFFERS)
BCE_ERROR BCAllocDiscontigMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **ppPhysAddr);

void BCFreeDiscontigMemory(unsigned long ulSize,
                         BCE_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr);

#else

BCE_ERROR BCAllocContigMemory(unsigned long    ulSize,
                              BCE_HANDLE       *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_CPU_PHYADDR  *pPhysAddr);

void BCFreeContigMemory(unsigned long ulSize,
                        BCE_HANDLE hMemHandle,
                        IMG_CPU_VIRTADDR LinAddr,
                        IMG_CPU_PHYADDR PhysAddr);
#endif

IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(IMG_CPU_PHYADDR cpu_paddr);
IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(IMG_SYS_PHYADDR sys_paddr);

void *MapPhysAddr(IMG_SYS_PHYADDR sSysAddr, unsigned long ulSize);
void UnMapPhysAddr(void *pvAddr, unsigned long ulSize);

BCE_ERROR BCGetLibFuncAddr (BCE_HANDLE hExtDrv, char *szFunctionName, PFN_BC_GET_PVRJTABLE *ppfnFuncTable);
BC_EXAMPLE_DEVINFO * GetAnchorPtr(void);

#if defined(__cplusplus)
}
#endif

#endif /* __BC_EXAMPLE_H__ */

/******************************************************************************
 End of file (bufferclass_example.h)
******************************************************************************/

