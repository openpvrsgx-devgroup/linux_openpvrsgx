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

#include "bufferclass_example.h"
#include "bufferclass_example_private.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void FillYUV420Image(void __iomem *pvDest, int width, int height,
			    int bytestride)
{
	static int iPhase;
	int i, j;
	unsigned char u, v, y;
	unsigned char *pui8y = (unsigned char __force *)pvDest;
	unsigned short *pui16uv;
	unsigned int count = 0;

	for (j = 0; j < height; j++)
		for (i = 0; i < width; i++) {
			y = (((i + iPhase) >> 6) % (2) == 0) ? 0x7f : 0x00;

			pui8y[count++] = y;
		}

	pui16uv = (unsigned short *)
	    ((unsigned char __force *)pvDest + (width * height));
	count = 0;

	for (j = 0; j < height; j += 2)
		for (i = 0; i < width; i += 2) {
			u = (j < (height / 2)) ?
				((i < (width / 2)) ? 0xFF : 0x33) :
				((i < (width / 2)) ?  0x33 : 0xAA);
			v = (j < (height / 2)) ?
				((i < (width / 2)) ?  0xAC : 0x0) :
				((i < (width / 2)) ?  0x03 : 0xEE);

			pui16uv[count++] = (v << 8) | u;

		}

	iPhase++;
}

static void FillYUV422Image(void __iomem *pvDest, int width, int height,
			    int bytestride)
{
	static int iPhase;
	int x, y;
	unsigned char u, v, y0, y1;
	unsigned int *pui32yuv = (unsigned int __force *)pvDest;
	unsigned int count = 0;

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x += 2) {
			u = (y < (height / 2)) ?
			      ((x < (width / 2)) ? 0xFF : 0x33) :
			      ((x < (width / 2)) ? 0x33 : 0xAA);
			v = (y < (height / 2)) ?
			      ((x < (width / 2)) ? 0xAA : 0x0) :
			      ((x < (width / 2)) ? 0x03 : 0xEE);

			y0 = y1 =
			    (((x + iPhase) >> 6) % (2) == 0) ? 0x7f : 0x00;

			pui32yuv[count++] =
			    (y1 << 24) | (v << 16) | (y0 << 8) | u;

		}

	iPhase++;
}

static void FillRGB565Image(void __iomem *pvDest, int width, int height,
			    int bytestride)
{
	int i, Count;
	unsigned long *pui32Addr = (unsigned long __force *)pvDest;
	unsigned short *pui16Addr = (unsigned short __force *)pvDest;
	unsigned long Colour32;
	unsigned short Colour16;
	static unsigned char Colour8;

	Colour16 = (Colour8 >> 3) | ((Colour8 >> 2) << 5) |
		   ((Colour8 >> 3) << 11);
	Colour32 = Colour16 | Colour16 << 16;

	Count = (height * bytestride) >> 2;

	for (i = 0; i < Count; i++)
		pui32Addr[i] = Colour32;

	Count = height;

	pui16Addr = (unsigned short *)
			((unsigned char __force *)pvDest + (2 * Colour8));

	for (i = 0; i < Count; i++) {
		*pui16Addr = 0xF800;

		pui16Addr =
		    (unsigned short *)((unsigned char *)pui16Addr + bytestride);
	}
	Count = bytestride >> 2;

	pui32Addr = (unsigned long *)((unsigned char __force *)pvDest +
			      (bytestride * (MIN(height - 1, 0xFF) - Colour8)));

	for (i = 0; i < Count; i++)
		pui32Addr[i] = 0x001F001F;

	Colour8 = (Colour8 + 1) % MIN(height - 1, 0xFF);
}

int FillBuffer(unsigned int ui32BufferIndex)
{
	struct BC_EXAMPLE_DEVINFO *psDevInfo = GetAnchorPtr();
	struct BC_EXAMPLE_BUFFER *psBuffer;
	struct BUFFER_INFO *psBufferInfo;
	struct PVRSRV_SYNC_DATA *psSyncData;

	if (psDevInfo == NULL)
		return -1;

	psBuffer = &psDevInfo->psSystemBuffer[ui32BufferIndex];
	psBufferInfo = &psDevInfo->sBufferInfo;

	psSyncData = psBuffer->psSyncData;

	if (psSyncData) {
		if (psSyncData->ui32ReadOpsPending !=
		    psSyncData->ui32ReadOpsComplete)
			return -1;

		psSyncData->ui32WriteOpsPending++;
	}

	switch (psBufferInfo->pixelformat) {
	case PVRSRV_PIXEL_FORMAT_RGB565:
	default:
		FillRGB565Image(psBuffer->sCPUVAddr, BC_EXAMPLE_WIDTH,
				BC_EXAMPLE_HEIGHT, BC_EXAMPLE_STRIDE);
		break;
	case PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY:
		FillYUV422Image(psBuffer->sCPUVAddr, BC_EXAMPLE_WIDTH,
				BC_EXAMPLE_HEIGHT, BC_EXAMPLE_STRIDE);
		break;
	case PVRSRV_PIXEL_FORMAT_NV12:
		FillYUV420Image(psBuffer->sCPUVAddr, BC_EXAMPLE_WIDTH,
				BC_EXAMPLE_HEIGHT, BC_EXAMPLE_STRIDE);
		break;
	}

	if (psSyncData)
		psSyncData->ui32WriteOpsComplete++;

	return 0;
}

int GetBufferCount(unsigned int *pui32BufferCount)
{
	struct BC_EXAMPLE_DEVINFO *psDevInfo = GetAnchorPtr();

	if (psDevInfo == NULL)
		return -1;

	*pui32BufferCount = psDevInfo->sBufferInfo.ui32BufferCount;

	return 0;
}
