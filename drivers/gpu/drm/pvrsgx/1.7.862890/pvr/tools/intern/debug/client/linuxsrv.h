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
 **************************************************************************/

#ifndef _LINUXSRV_H__
#define _LINUXSRV_H__

typedef struct tagIOCTL_PACKAGE
{
	IMG_UINT32 ui32Cmd;              // ioctl command
	IMG_UINT32 ui32Size;			   // needs to be correctly set
	IMG_VOID 	*pInBuffer;          // input data buffer
	IMG_UINT32  ui32InBufferSize;     // size of input data buffer
	IMG_VOID    *pOutBuffer;         // output data buffer
	IMG_UINT32  ui32OutBufferSize;    // size of output data buffer
} IOCTL_PACKAGE;

IMG_UINT32 DeviceIoControl(IMG_UINT32 hDevice,		
						IMG_UINT32 ui32ControlCode, 
						IMG_VOID *pInBuffer,		
						IMG_UINT32 ui32InBufferSize,
						IMG_VOID *pOutBuffer,		
						IMG_UINT32 ui32OutBufferSize,  
						IMG_UINT32 *pui32BytesReturned); 

#endif /* _LINUXSRV_H__*/
