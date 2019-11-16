/*************************************************************************/ /*!
@Title          Bufferclass example private functions.
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

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the IMG POWERVR
 Services driver with 3rd Party display hardware.  It is NOT a specification for
 a display controller driver, rather a specification to extend the API for a
 pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG POWERVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.
 
 Functions of the API include
 - query primary surface attributes (width, height, stride, pixel format, CPU
     physical and virtual address)
 - swap/flip chain creation and subsequent query of surface attributes
 - asynchronous display surface flipping, taking account of asynchronous read
 (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map the
 display memory to any IMG POWERVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver may
 be extended to support the 3rd Party Display interface to POWERVR Services
 - IMG is not providing a display driver implementation.
 **************************************************************************/

#include "bufferclass_example.h"
#include "bufferclass_example_private.h"

#define MIN(a,b) ((a)<(b)?(a):(b))

static void FillNV12Image(void *pvDest, int width, int height, int bytestride)
{
	static int iPhase = 0;
	int             i, j;
	unsigned char   u,v,y;
	unsigned char  *pui8y = (unsigned char *)pvDest;
	unsigned short *pui16uv;
	unsigned int    count = 0;

	for(j=0;j<height;j++)
	{
		pui8y = (unsigned char *)pvDest + j * bytestride;
		count = 0;

		for(i=0;i<width;i++)
		{
			y = (((i+iPhase)>>6)%(2)==0)? 0x7f:0x00;

			pui8y[count++] = y;
		}
	}

	for(j=0;j<height;j+=2)
	{
		pui16uv = (unsigned short *)((unsigned char *)pvDest + height * bytestride + (j / 2) * bytestride);
		count = 0;

		for(i=0;i<width;i+=2)
		{
			u = (j<(height/2))? ((i<(width/2))? 0xFF:0x33) : ((i<(width/2))? 0x33:0xAA);
			v = (j<(height/2))? ((i<(width/2))? 0xAC:0x0) : ((i<(width/2))? 0x03:0xEE);

 			/* Byte order is VU */
			pui16uv[count++] = (v << 8) | u;

		}
	}

	iPhase++;
}

static void FillYV12Image(void *pvDest, int width, int height, int bytestride)
{
	static int iPhase = 0;
	int             i, j;
	unsigned char   u,v,y;
	unsigned char  *pui8y = (unsigned char *)pvDest;
	unsigned char *pui8u, *pui8v;
	unsigned int    count = 0;
	int uvplanestride = bytestride / 2;
	int uvplaneheight = height / 2;

	for(j=0;j<height;j++)
	{
		pui8y = (unsigned char *)pvDest + j * bytestride;
		count = 0;

		for(i=0;i<width;i++)
		{
			y = (((i+iPhase)>>6)%(2)==0)? 0x7f:0x00;

			pui8y[count++] = y;
		}
	}

	for(j=0;j<height;j+=2)
	{
		pui8v = (unsigned char *)pvDest + (height * bytestride) + ((j / 2) * uvplanestride);
		count = 0;

		for(i=0;i<width;i+=2)
		{
			v = (j<(height/2))? ((i<(width/2))? 0xAC:0x0) : ((i<(width/2))? 0x03:0xEE);

			pui8v[count++] = v;
		}
	}

	for(j=0;j<height;j+=2)
	{
		pui8u = (unsigned char *)pvDest + (height * bytestride) + (uvplaneheight * uvplanestride) + (j / 2) * uvplanestride;
		count = 0;

		for(i=0;i<width;i+=2)
		{
			u = (j<(height/2))? ((i<(width/2))? 0xFF:0x33) : ((i<(width/2))? 0x33:0xAA);

			pui8u[count++] = u;

		}
	}

	iPhase++;
}

static void FillYUV422Image(void *pvDest, int width, int height, int bytestride, PVRSRV_PIXEL_FORMAT pixelformat)
{
	static int iPhase = 0;
	int           x, y;
	unsigned char u,v,y0,y1;
	unsigned long *pui32yuv = (unsigned long *)pvDest;
	unsigned int  count = 0;

	for(y=0;y<height;y++)
	{
		pui32yuv = (unsigned long *)((unsigned char *)pvDest + y * bytestride);
		count = 0;

		for(x=0;x<width; x+=2)
		{
			u = (y<(height/2))? ((x<(width/2))? 0xFF:0x33) : ((x<(width/2))? 0x33:0xAA);
			v = (y<(height/2))? ((x<(width/2))? 0xAA:0x0) : ((x<(width/2))? 0x03:0xEE);

			y0 = y1 = (((x+iPhase)>>6)%(2)==0)? 0x7f:0x00;

			switch(pixelformat)
			{
				case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_VYUY:
					pui32yuv[count++] = (y1 << 24) | (u << 16) | (y0 << 8) | v;
					break;
				case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY:
					pui32yuv[count++] = (y1 << 24) | (v << 16) | (y0 << 8) | u;
					break;
				case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV:
					pui32yuv[count++] = (v << 24) | (y1 << 16) | (u << 8) | y0;
					break;
				case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YVYU:
					pui32yuv[count++] = (u << 24) | (y1 << 16) | (v << 8) | y0;
					break;
				
				default:
					break;

			}

		}
	}

	iPhase++;
}

static void FillRGB565Image(void *pvDest, int width, int height, int bytestride)
{
	int i, Count;
	unsigned long *pui32Addr  = (unsigned long *)pvDest;
	unsigned short *pui16Addr = (unsigned short *)pvDest;
	unsigned long   Colour32;
	unsigned short  Colour16;
	static unsigned char Colour8 = 0;//debug colour
	
	Colour16 = (Colour8>>3) | ((Colour8>>2)<<5) | ((Colour8>>3)<<11);
	Colour32 = Colour16 | Colour16 << 16;
			
	Count = (height * bytestride)>>2;

	for(i=0; i<Count; i++)
	{
		pui32Addr[i] = Colour32;
	}

	Count =  height;

	pui16Addr = (unsigned short *)((unsigned char *)pvDest + (2 * Colour8));

	for(i=0; i<Count; i++)
	{
		*pui16Addr = 0xF800U;

		pui16Addr = (unsigned short *)((unsigned char *)pui16Addr + bytestride);
	}
	Count = bytestride >> 2;
	
	pui32Addr = (unsigned long *)((unsigned char *)pvDest + (bytestride * (MIN(height - 1, 0xFF) - Colour8)));

	for(i=0; i<Count; i++)
	{
		pui32Addr[i] = 0x001F001FUL;
	}

	/* advance the colour */
	Colour8 = (Colour8 + 1) % MIN(height - 1, 0xFFU);
}


/*!
******************************************************************************

 @Function	FillBuffer
 
 @Description 
 
 Fills pixels into a buffer specified by index
 
 @Input	ui32BufferIndex - buffer index
 
 @Return	0 - success, -1 - failure

******************************************************************************/
int FillBuffer(unsigned int uiBufferIndex)
{
	BC_EXAMPLE_DEVINFO  *psDevInfo = GetAnchorPtr();
	BC_EXAMPLE_BUFFER   *psBuffer;
	BUFFER_INFO         *psBufferInfo;
	PVRSRV_SYNC_DATA    *psSyncData;

	/* check DevInfo has been setup */
	if(psDevInfo == NULL)
	{
		return -1;/* failure */
	}

	psBuffer = &psDevInfo->psSystemBuffer[uiBufferIndex];
	psBufferInfo = &psDevInfo->sBufferInfo;

	/* This may be NULL, as it is only registered once texture streaming starts. */
	psSyncData = psBuffer->psSyncData;

	if(psSyncData)
	{
		/* ensure all reads have flushed on the buffer */
		if(psSyncData->ui32ReadOpsPending != psSyncData->ui32ReadOpsComplete)
		{
			return -1;/* failure */
		}

		/* take a write-lock on the new buffer to capture to */
		psSyncData->ui32WriteOpsPending++;
	}

	switch(psBufferInfo->pixelformat)
	{
		case PVRSRV_PIXEL_FORMAT_RGB565:
		default:
		{
			FillRGB565Image(psBuffer->sCPUVAddr, 
							psBufferInfo->ui32Width, 
							psBufferInfo->ui32Height, 
							psBufferInfo->ui32ByteStride);
			break;
		}
		case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_VYUY:
		case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY:
		case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV:
		case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YVYU:
		{
			FillYUV422Image(psBuffer->sCPUVAddr, 
							psBufferInfo->ui32Width, 
							psBufferInfo->ui32Height, 
							psBufferInfo->ui32ByteStride, 
							psBufferInfo->pixelformat);
			break;
		}
		case PVRSRV_PIXEL_FORMAT_NV12:
		{
			FillNV12Image(psBuffer->sCPUVAddr, 
							psBufferInfo->ui32Width, 
							psBufferInfo->ui32Height, 
							psBufferInfo->ui32ByteStride);
			break;
		}
		case PVRSRV_PIXEL_FORMAT_YV12:
		{
			FillYV12Image(psBuffer->sCPUVAddr, 
							psBufferInfo->ui32Width, 
							psBufferInfo->ui32Height, 
							psBufferInfo->ui32ByteStride);
			break;
		}
	}

	/* unlock the buffer, signalling the writes are complete */
	if(psSyncData)
	{
		psSyncData->ui32WriteOpsComplete++;

		if (NULL != psDevInfo->sPVRJTable.pfnPVRSRVScheduleDevices)
		{
			(*psDevInfo->sPVRJTable.pfnPVRSRVScheduleDevices)();
		}
	}

	return 0;
}


/*!
******************************************************************************

 @Function	GetBufferCount
 
 @Description 
 
 returns buffer count
 
 @Output	pulBufferCount - buffer count
 
 @Return	0 - success, -1 - failure

******************************************************************************/
int GetBufferCount(unsigned int *puiBufferCount)
{
	BC_EXAMPLE_DEVINFO *psDevInfo = GetAnchorPtr();

	/* check DevInfo has been setup */
	if(psDevInfo == IMG_NULL)
	{
		return -1;/* failure */
	}

	/* return buffer count */
	*puiBufferCount = (unsigned int)psDevInfo->sBufferInfo.ui32BufferCount;

	return 0;
}



/******************************************************************************

 @Function  ReconfigureBuffer

 @Description

 returns whether reconfiguration succeeds or not 

 @Output   uiSucceed : 1 - succeeded, 0 - failed 

 @Return   0 - success, -1 - failure

******************************************************************************/
int ReconfigureBuffer(unsigned int *uiSucceed)
{
	BCE_ERROR eError;
 
	/* Destroy the shared buffers of the current buffer class device */
	eError = BC_Example_Buffers_Destroy();

	if (eError != BCE_OK)
	{
	    *uiSucceed = 0;
		return -1;
	}

	/* No need to un-register and then re-register the device with services module srvkm */


	/* Recreate shared buffers with reconfigured parameters */
	eError = BC_Example_Buffers_Create();

	if (eError != BCE_OK)
	{
	    *uiSucceed = 0;
		return -1;
	}

	/* return uiSucceed as succeeded 1 */
	*uiSucceed = 1;
	return 0;
}
