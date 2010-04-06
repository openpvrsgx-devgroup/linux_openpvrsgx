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

#include <linux/uaccess.h>

#include "img_types.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hotkey.h"

u32 DBGDIOCDrivCreateStream(void *pvInBuffer,
				   void *pvOutBuffer)
{
	struct DBG_IN_CREATESTREAM *psIn;
	void **ppvOut;
	static char name[32];

	psIn = (struct DBG_IN_CREATESTREAM *)pvInBuffer;
	ppvOut = (void **)pvOutBuffer;


	if (copy_from_user(name, psIn->pszName, 32) != 0)
		return IMG_FALSE;
	*ppvOut =
	    ExtDBGDrivCreateStream(name, psIn->ui32CapMode, psIn->ui32OutMode,
				   0, psIn->ui32Pages);


	return IMG_TRUE;
}

u32 DBGDIOCDrivDestroyStream(void *pvInBuffer,
				    void *pvOutBuffer)
{
	u32 *pStream;
	struct DBG_STREAM *psStream;

	pStream = (u32 *) pvInBuffer;
	psStream = (struct DBG_STREAM *)*pStream;

	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivDestroyStream(psStream);

	return IMG_TRUE;
}

u32 DBGDIOCDrivGetStream(void *pvInBuffer, void *pvOutBuffer)
{
	struct DBG_IN_FINDSTREAM *psParams;
	u32 *pui32Stream;

	psParams = (struct DBG_IN_FINDSTREAM *)pvInBuffer;
	pui32Stream = (u32 *) pvOutBuffer;

	*pui32Stream =
	    (u32) ExtDBGDrivFindStream(psParams->pszName,
					      psParams->bResetStream);

	return IMG_TRUE;
}

u32 DBGDIOCDrivWriteString(void *pvInBuffer, void *pvOutBuffer)
{
	struct DBG_IN_WRITESTRING *psParams;
	u32 *pui32OutLen;

	psParams = (struct DBG_IN_WRITESTRING *)pvInBuffer;
	pui32OutLen = (u32 *) pvOutBuffer;

	*pui32OutLen =
	    ExtDBGDrivWriteString((struct DBG_STREAM *)psParams->pvStream,
				  psParams->pszString, psParams->ui32Level);

	return IMG_TRUE;
}

u32 DBGDIOCDrivWriteStringCM(void *pvInBuffer,
				    void *pvOutBuffer)
{
	struct DBG_IN_WRITESTRING *psParams;
	u32 *pui32OutLen;

	psParams = (struct DBG_IN_WRITESTRING *)pvInBuffer;
	pui32OutLen = (u32 *) pvOutBuffer;

	*pui32OutLen =
	    ExtDBGDrivWriteStringCM((struct DBG_STREAM *)psParams->pvStream,
				    psParams->pszString, psParams->ui32Level);

	return IMG_TRUE;
}

u32 DBGDIOCDrivReadString(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pui32OutLen;
	struct DBG_IN_READSTRING *psParams;

	psParams = (struct DBG_IN_READSTRING *)pvInBuffer;
	pui32OutLen = (u32 *) pvOutBuffer;

	*pui32OutLen =
	    ExtDBGDrivReadString(psParams->pvStream, psParams->pszString,
				 psParams->ui32StringLen);

	return IMG_TRUE;
}

u32 DBGDIOCDrivWrite(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pui32BytesCopied;
	struct DBG_IN_WRITE *psInParams;

	psInParams = (struct DBG_IN_WRITE *)pvInBuffer;
	pui32BytesCopied = (u32 *) pvOutBuffer;

	*pui32BytesCopied =
	    ExtDBGDrivWrite((struct DBG_STREAM *)psInParams->pvStream,
			    psInParams->pui8InBuffer,
			    psInParams->ui32TransferSize,
			    psInParams->ui32Level);

	return IMG_TRUE;
}

u32 DBGDIOCDrivWrite2(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pui32BytesCopied;
	struct DBG_IN_WRITE *psInParams;

	psInParams = (struct DBG_IN_WRITE *)pvInBuffer;
	pui32BytesCopied = (u32 *) pvOutBuffer;

	*pui32BytesCopied =
	    ExtDBGDrivWrite2((struct DBG_STREAM *)psInParams->pvStream,
			     psInParams->pui8InBuffer,
			     psInParams->ui32TransferSize,
			     psInParams->ui32Level);

	return IMG_TRUE;
}

u32 DBGDIOCDrivWriteCM(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pui32BytesCopied;
	struct DBG_IN_WRITE *psInParams;

	psInParams = (struct DBG_IN_WRITE *)pvInBuffer;
	pui32BytesCopied = (u32 *) pvOutBuffer;

	*pui32BytesCopied =
	    ExtDBGDrivWriteCM((struct DBG_STREAM *)psInParams->pvStream,
			      psInParams->pui8InBuffer,
			      psInParams->ui32TransferSize,
			      psInParams->ui32Level);

	return IMG_TRUE;
}

u32 DBGDIOCDrivRead(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pui32BytesCopied;
	struct DBG_IN_READ *psInParams;

	psInParams = (struct DBG_IN_READ *)pvInBuffer;
	pui32BytesCopied = (u32 *) pvOutBuffer;

	*pui32BytesCopied =
	    ExtDBGDrivRead((struct DBG_STREAM *)psInParams->pvStream,
			   psInParams->bReadInitBuffer,
			   psInParams->ui32OutBufferSize,
			   psInParams->pui8OutBuffer);

	return IMG_TRUE;
}

u32 DBGDIOCDrivSetCaptureMode(void *pvInBuffer,
				     void *pvOutBuffer)
{
	struct DBG_IN_SETDEBUGMODE *psParams;

	psParams = (struct DBG_IN_SETDEBUGMODE *)pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetCaptureMode((struct DBG_STREAM *)psParams->pvStream,
				 psParams->ui32Mode,
				 psParams->ui32Start,
				 psParams->ui32End, psParams->ui32SampleRate);

	return IMG_TRUE;
}

u32 DBGDIOCDrivSetOutMode(void *pvInBuffer, void *pvOutBuffer)
{
	struct DBG_IN_SETDEBUGOUTMODE *psParams;

	psParams = (struct DBG_IN_SETDEBUGOUTMODE *)pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetOutputMode((struct DBG_STREAM *)psParams->pvStream,
				psParams->ui32Mode);

	return IMG_TRUE;
}

u32 DBGDIOCDrivSetDebugLevel(void *pvInBuffer,
				    void *pvOutBuffer)
{
	struct DBG_IN_SETDEBUGLEVEL *psParams;

	psParams = (struct DBG_IN_SETDEBUGLEVEL *)pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetDebugLevel((struct DBG_STREAM *)psParams->pvStream,
				psParams->ui32Level);

	return IMG_TRUE;
}

u32 DBGDIOCDrivSetFrame(void *pvInBuffer, void *pvOutBuffer)
{
	struct DBG_IN_SETFRAME *psParams;

	psParams = (struct DBG_IN_SETFRAME *)pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetFrame((struct DBG_STREAM *)psParams->pvStream,
			   psParams->ui32Frame);

	return IMG_TRUE;
}

u32 DBGDIOCDrivGetFrame(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pStream;
	struct DBG_STREAM *psStream;
	u32 *pui32Current;

	pStream = (u32 *) pvInBuffer;
	psStream = (struct DBG_STREAM *)*pStream;
	pui32Current = (u32 *) pvOutBuffer;

	*pui32Current = ExtDBGDrivGetFrame(psStream);

	return IMG_TRUE;
}

u32 DBGDIOCDrivIsCaptureFrame(void *pvInBuffer,
				     void *pvOutBuffer)
{
	struct DBG_IN_ISCAPTUREFRAME *psParams;
	u32 *pui32Current;

	psParams = (struct DBG_IN_ISCAPTUREFRAME *)pvInBuffer;
	pui32Current = (u32 *) pvOutBuffer;

	*pui32Current =
	    ExtDBGDrivIsCaptureFrame((struct DBG_STREAM *)psParams->pvStream,
				     psParams->bCheckPreviousFrame);

	return IMG_TRUE;
}

u32 DBGDIOCDrivOverrideMode(void *pvInBuffer,
				   void *pvOutBuffer)
{
	struct DBG_IN_OVERRIDEMODE *psParams;

	psParams = (struct DBG_IN_OVERRIDEMODE *)pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivOverrideMode((struct DBG_STREAM *)psParams->pvStream,
			       psParams->ui32Mode);

	return IMG_TRUE;
}

u32 DBGDIOCDrivDefaultMode(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pStream;
	struct DBG_STREAM *psStream;

	pStream = (u32 *) pvInBuffer;
	psStream = (struct DBG_STREAM *)*pStream;

	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivDefaultMode(psStream);

	return IMG_TRUE;
}

u32 DBGDIOCDrivSetMarker(void *pvInBuffer, void *pvOutBuffer)
{
	struct DBG_IN_SETMARKER *psParams;

	psParams = (struct DBG_IN_SETMARKER *)pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetMarker((struct DBG_STREAM *)psParams->pvStream,
			    psParams->ui32Marker);

	return IMG_TRUE;
}

u32 DBGDIOCDrivGetMarker(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pStream;
	struct DBG_STREAM *psStream;
	u32 *pui32Current;

	pStream = (u32 *) pvInBuffer;
	psStream = (struct DBG_STREAM *)*pStream;
	pui32Current = (u32 *) pvOutBuffer;

	*pui32Current = ExtDBGDrivGetMarker(psStream);

	return IMG_TRUE;
}

u32 DBGDIOCDrivGetServiceTable(void *pvInBuffer,
				      void *pvOutBuffer)
{
	u32 *pui32Out;

	PVR_UNREFERENCED_PARAMETER(pvInBuffer);
	pui32Out = (u32 *) pvOutBuffer;

	*pui32Out = DBGDrivGetServiceTable();

	return IMG_TRUE;
}

u32 DBGDIOCDrivWriteLF(void *pvInBuffer, void *pvOutBuffer)
{
	struct DBG_IN_WRITE_LF *psInParams;
	u32 *pui32BytesCopied;

	psInParams = (struct DBG_IN_WRITE_LF *)pvInBuffer;
	pui32BytesCopied = (u32 *) pvOutBuffer;

	*pui32BytesCopied = ExtDBGDrivWriteLF(psInParams->pvStream,
					      psInParams->pui8InBuffer,
					      psInParams->ui32BufferSize,
					      psInParams->ui32Level,
					      psInParams->ui32Flags);

	return IMG_TRUE;
}

u32 DBGDIOCDrivReadLF(void *pvInBuffer, void *pvOutBuffer)
{
	u32 *pui32BytesCopied;
	struct DBG_IN_READ *psInParams;

	psInParams = (struct DBG_IN_READ *)pvInBuffer;
	pui32BytesCopied = (u32 *) pvOutBuffer;

	*pui32BytesCopied =
	    ExtDBGDrivReadLF((struct DBG_STREAM *)psInParams->pvStream,
			     psInParams->ui32OutBufferSize,
			     psInParams->pui8OutBuffer);

	return IMG_TRUE;
}

u32 DBGDIOCDrivWaitForEvent(void *pvInBuffer, void *pvOutBuffer)
{
	enum DBG_EVENT eEvent = (enum DBG_EVENT)(*(u32 *) pvInBuffer);

	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivWaitForEvent(eEvent);

	return IMG_TRUE;
}
