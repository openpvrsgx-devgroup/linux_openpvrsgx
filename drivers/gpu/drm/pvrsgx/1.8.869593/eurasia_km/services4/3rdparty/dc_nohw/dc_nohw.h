/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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

#ifndef __DC_NOHW_H__
#define __DC_NOHW_H__


#if defined(__cplusplus)
extern "C" {
#endif

#if defined(USE_BASE_VIDEO_FRAMEBUFFER)
#if defined (ENABLE_DISPLAY_MODE_TRACKING)
#error Cannot have both USE_BASE_VIDEO_FRAMEBUFFER and ENABLE_DISPLAY_MODE_TRACKING defined
#endif
#endif

#if !defined(DC_NOHW_BUFFER_WIDTH) && !defined(DC_NOHW_BUFFER_HEIGHT)
#define DC_NOHW_BUFFER_WIDTH		320
#define DC_NOHW_BUFFER_HEIGHT		240
#endif

#define DC_NOHW_BUFFER_BIT_DEPTH	32
#define	DC_NOHW_BUFFER_PIXEL_FORMAT	PVRSRV_PIXEL_FORMAT_ARGB8888

#define	DC_NOHW_DEPTH_BITS_PER_BYTE	8

#define	dc_nohw_byte_depth_from_bit_depth(bit_depth) (((IMG_UINT32)(bit_depth) + DC_NOHW_DEPTH_BITS_PER_BYTE - 1)/DC_NOHW_DEPTH_BITS_PER_BYTE)
#define	dc_nohw_bit_depth_from_byte_depth(byte_depth) ((IMG_UINT32)(byte_depth) * DC_NOHW_DEPTH_BITS_PER_BYTE)
#define dc_nohw_roundup_bit_depth(bd) dc_nohw_bit_depth_from_byte_depth(dc_nohw_byte_depth_from_bit_depth(bd))

#define	dc_nohw_byte_stride(width, bit_depth) ((IMG_UINT32)(width) * dc_nohw_byte_depth_from_bit_depth(bit_depth))

#if defined(DC_NOHW_GET_BUFFER_DIMENSIONS)
IMG_BOOL GetBufferDimensions(IMG_UINT32 *pui32Width, IMG_UINT32 *pui32Height, PVRSRV_PIXEL_FORMAT *pePixelFormat, IMG_UINT32 *pui32Stride);
#else
#define DC_NOHW_BUFFER_BYTE_STRIDE	dc_nohw_byte_stride(DC_NOHW_BUFFER_WIDTH, DC_NOHW_BUFFER_BIT_DEPTH)
#endif

extern IMG_BOOL IMG_IMPORT PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE *psJTable);

#define DC_NOHW_MAXFORMATS	(1)
#define DC_NOHW_MAXDIMS		(1)
#define DC_NOHW_MAX_BACKBUFFERS (3)


typedef void *       DC_HANDLE;

typedef struct DC_NOHW_BUFFER_TAG
{
	DC_HANDLE					hSwapChain;
	DC_HANDLE					hMemChunk;

	
	
#if defined(DC_NOHW_DISCONTIG_BUFFERS)
	IMG_SYS_PHYADDR				*psSysAddr;
#else
	IMG_SYS_PHYADDR				sSysAddr;
#endif
	IMG_DEV_VIRTADDR			sDevVAddr;
	IMG_CPU_VIRTADDR			sCPUVAddr;
	PVRSRV_SYNC_DATA*			psSyncData;

	struct DC_NOHW_BUFFER_TAG	*psNext;
} DC_NOHW_BUFFER;


typedef struct DC_NOHW_SWAPCHAIN_TAG
{
	unsigned long ulBufferCount;
	DC_NOHW_BUFFER *psBuffer;
} DC_NOHW_SWAPCHAIN;


typedef struct DC_NOHW_DEVINFO_TAG
{
	unsigned int			 uiDeviceID;

	
	DC_NOHW_BUFFER           sSystemBuffer;

	
	unsigned long            ulNumFormats;

	
	unsigned long            ulNumDims;

	
	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;

	
	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;

	


	DC_HANDLE                hPVRServices;

	
	DC_NOHW_BUFFER           asBackBuffers[DC_NOHW_MAX_BACKBUFFERS];

	
	unsigned long            ulRefCount;

	DC_NOHW_SWAPCHAIN       *psSwapChain;

	
	
	DISPLAY_INFO             sDisplayInfo;

	
	DISPLAY_FORMAT           sSysFormat;
	DISPLAY_DIMS             sSysDims;
	IMG_UINT32               ui32BufferSize;

	
	DISPLAY_FORMAT           asDisplayFormatList[DC_NOHW_MAXFORMATS];

	
	DISPLAY_DIMS             asDisplayDimList[DC_NOHW_MAXDIMS];

	
	DISPLAY_FORMAT           sBackBufferFormat[DC_NOHW_MAXFORMATS];

}  DC_NOHW_DEVINFO;


typedef enum _DC_ERROR_
{
	DC_OK								=  0,
	DC_ERROR_GENERIC					=  1,
	DC_ERROR_OUT_OF_MEMORY				=  2,
	DC_ERROR_TOO_FEW_BUFFERS			=  3,
	DC_ERROR_INVALID_PARAMS				=  4,
	DC_ERROR_INIT_FAILURE				=  5,
	DC_ERROR_CANT_REGISTER_CALLBACK 	=  6,
	DC_ERROR_INVALID_DEVICE				=  7,
	DC_ERROR_DEVICE_REGISTER_FAILED 	=  8
} DC_ERROR;


#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

DC_ERROR Init(void);
DC_ERROR Deinit(void);

#if defined(USE_BASE_VIDEO_FRAMEBUFFER) || defined (ENABLE_DISPLAY_MODE_TRACKING)
DC_ERROR OpenMiniport(void);
DC_ERROR CloseMiniport(void);
#endif  

#if defined(USE_BASE_VIDEO_FRAMEBUFFER)
PVRSRV_ERROR SetupDevInfo (DC_NOHW_DEVINFO *psDevInfo);
PVRSRV_ERROR FreeBackBuffers (DC_NOHW_DEVINFO *psDevInfo);
#endif

#if defined (ENABLE_DISPLAY_MODE_TRACKING)
DC_ERROR Shadow_Desktop_Resolution(DC_NOHW_DEVINFO	*psDevInfo);
#endif 

#if !defined(DC_NOHW_DISCONTIG_BUFFERS) && !defined(USE_BASE_VIDEO_FRAMEBUFFER)
IMG_SYS_PHYADDR CpuPAddrToSysPAddr(IMG_CPU_PHYADDR cpu_paddr);
IMG_CPU_PHYADDR SysPAddrToCpuPAddr(IMG_SYS_PHYADDR sys_paddr);
#endif

DC_ERROR OpenPVRServices  (DC_HANDLE *phPVRServices);
DC_ERROR ClosePVRServices (DC_HANDLE hPVRServices);

#if defined(DC_NOHW_DISCONTIG_BUFFERS)
DC_ERROR AllocDiscontigMemory(unsigned long ulSize,
                              DC_HANDLE * phMemChunk,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **pPhysAddr);

void FreeDiscontigMemory(unsigned long ulSize,
                         DC_HANDLE hMemChunk,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr);
#else



DC_ERROR AllocContigMemory(unsigned long ulSize,
                           DC_HANDLE * phMemHandle,
                           IMG_CPU_VIRTADDR *pLinAddr,
                           IMG_CPU_PHYADDR *pPhysAddr);

void FreeContigMemory(unsigned long ulSize,
                      DC_HANDLE hMemChunk,
                      IMG_CPU_VIRTADDR LinAddr,
                      IMG_CPU_PHYADDR PhysAddr);


#endif

void *AllocKernelMem(unsigned long ulSize);
void FreeKernelMem  (void *pvMem);

DC_ERROR GetLibFuncAddr (DC_HANDLE hExtDrv, char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);

#if defined(__cplusplus)
}
#endif

#endif 

