/**********************************************************************
 Copyright (c) Imagination Technologies Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ******************************************************************************/



#ifdef LINUX
#include <asm/uaccess.h>
#endif

#include "img_types.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hotkey.h"


IMG_UINT32 DBGDIOCDrivCreateStream(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_CREATESTREAM psIn;
	IMG_VOID * *ppvOut;
	#ifdef LINUX
	static IMG_CHAR name[32];
	#endif

	psIn = (PDBG_IN_CREATESTREAM) pvInBuffer;
	ppvOut = (IMG_VOID * *) pvOutBuffer;

	#ifdef LINUX

	if(copy_from_user(name, psIn->pszName, 32) != 0)
	{
		return IMG_FALSE;
	}

	*ppvOut = ExtDBGDrivCreateStream(name, psIn->ui32CapMode, psIn->ui32OutMode, 0, psIn->ui32Pages);

	#else
	*ppvOut = ExtDBGDrivCreateStream(psIn->pszName, psIn->ui32CapMode, psIn->ui32OutMode, DEBUG_FLAGS_NO_BUF_EXPANDSION, psIn->ui32Pages);
	#endif


	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivDestroyStream(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *		pStream;
	PDBG_STREAM	psStream;

	pStream = (IMG_UINT32 *) pvInBuffer;
	psStream = (PDBG_STREAM) *pStream;

	PVR_UNREFERENCED_PARAMETER(	pvOutBuffer);

	ExtDBGDrivDestroyStream(psStream);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivGetStream(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_FINDSTREAM psParams;
	IMG_UINT32 *	pui32Stream;

	psParams		= (PDBG_IN_FINDSTREAM)pvInBuffer;
	pui32Stream	= (IMG_UINT32 *)pvOutBuffer;

	*pui32Stream = (IMG_UINT32)ExtDBGDrivFindStream(psParams->pszName, psParams->bResetStream);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivWriteString(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_WRITESTRING psParams;
	IMG_UINT32 *				pui32OutLen;

	psParams = (PDBG_IN_WRITESTRING) pvInBuffer;
	pui32OutLen = (IMG_UINT32 *) pvOutBuffer;

	*pui32OutLen = ExtDBGDrivWriteString((PDBG_STREAM) psParams->pvStream,psParams->pszString,psParams->ui32Level);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivWriteStringCM(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_WRITESTRING psParams;
	IMG_UINT32 *				pui32OutLen;

	psParams = (PDBG_IN_WRITESTRING) pvInBuffer;
	pui32OutLen = (IMG_UINT32 *) pvOutBuffer;

	*pui32OutLen = ExtDBGDrivWriteStringCM((PDBG_STREAM) psParams->pvStream,psParams->pszString,psParams->ui32Level);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivReadString(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *				pui32OutLen;
	PDBG_IN_READSTRING	psParams;

	psParams = (PDBG_IN_READSTRING) pvInBuffer;
	pui32OutLen = (IMG_UINT32 *) pvOutBuffer;

	*pui32OutLen = ExtDBGDrivReadString(psParams->pvStream,psParams->pszString,psParams->ui32StringLen);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivWrite(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *				pui32BytesCopied;
	PDBG_IN_WRITE		psInParams;

	psInParams = (PDBG_IN_WRITE) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	*pui32BytesCopied = ExtDBGDrivWrite((PDBG_STREAM) psInParams->pvStream,psInParams->pui8InBuffer,psInParams->ui32TransferSize,psInParams->ui32Level);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivWrite2(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *				pui32BytesCopied;
	PDBG_IN_WRITE		psInParams;

	psInParams = (PDBG_IN_WRITE) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	*pui32BytesCopied = ExtDBGDrivWrite2((PDBG_STREAM) psInParams->pvStream,psInParams->pui8InBuffer,psInParams->ui32TransferSize,psInParams->ui32Level);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivWriteCM(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *				pui32BytesCopied;
	PDBG_IN_WRITE		psInParams;

	psInParams = (PDBG_IN_WRITE) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	*pui32BytesCopied = ExtDBGDrivWriteCM((PDBG_STREAM) psInParams->pvStream,psInParams->pui8InBuffer,psInParams->ui32TransferSize,psInParams->ui32Level);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivRead(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *				pui32BytesCopied;
	PDBG_IN_READ		psInParams;

	psInParams = (PDBG_IN_READ) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	*pui32BytesCopied = ExtDBGDrivRead((PDBG_STREAM) psInParams->pvStream,psInParams->bReadInitBuffer, psInParams->ui32OutBufferSize,psInParams->pui8OutBuffer);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivSetCaptureMode(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETDEBUGMODE 	psParams;

	psParams = (PDBG_IN_SETDEBUGMODE) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetCaptureMode((PDBG_STREAM) psParams->pvStream,
						  psParams->ui32Mode,
						  psParams->ui32Start,
						  psParams->ui32End,
						  psParams->ui32SampleRate);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivSetOutMode(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETDEBUGOUTMODE psParams;

	psParams = (PDBG_IN_SETDEBUGOUTMODE) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetOutputMode((PDBG_STREAM) psParams->pvStream,psParams->ui32Mode);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivSetDebugLevel(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETDEBUGLEVEL psParams;

	psParams = (PDBG_IN_SETDEBUGLEVEL) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetDebugLevel((PDBG_STREAM) psParams->pvStream,psParams->ui32Level);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivSetFrame(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETFRAME	psParams;

	psParams = (PDBG_IN_SETFRAME) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetFrame((PDBG_STREAM) psParams->pvStream,psParams->ui32Frame);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivGetFrame(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *		pStream;
	PDBG_STREAM	psStream;
	IMG_UINT32 *		pui32Current;

	pStream = (IMG_UINT32 *) pvInBuffer;
	psStream = (PDBG_STREAM) *pStream;
	pui32Current = (IMG_UINT32 *) pvOutBuffer;

	*pui32Current = ExtDBGDrivGetFrame(psStream);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivIsCaptureFrame(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_ISCAPTUREFRAME psParams;
	IMG_UINT32 *		pui32Current;

	psParams = (PDBG_IN_ISCAPTUREFRAME) pvInBuffer;
	pui32Current = (IMG_UINT32 *) pvOutBuffer;

	*pui32Current = ExtDBGDrivIsCaptureFrame((PDBG_STREAM) psParams->pvStream, psParams->bCheckPreviousFrame);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivOverrideMode(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_OVERRIDEMODE	psParams;

	psParams = (PDBG_IN_OVERRIDEMODE) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(	pvOutBuffer);

	ExtDBGDrivOverrideMode((PDBG_STREAM) psParams->pvStream,psParams->ui32Mode);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivDefaultMode(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *		pStream;
	PDBG_STREAM	psStream;

	pStream = (IMG_UINT32 *) pvInBuffer;
	psStream = (PDBG_STREAM) *pStream;

	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivDefaultMode(psStream);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivSetMarker(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_SETMARKER	psParams;

	psParams = (PDBG_IN_SETMARKER) pvInBuffer;
	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivSetMarker((PDBG_STREAM) psParams->pvStream, psParams->ui32Marker);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivGetMarker(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *		pStream;
	PDBG_STREAM	psStream;
	IMG_UINT32 *		pui32Current;

	pStream = (IMG_UINT32 *) pvInBuffer;
	psStream = (PDBG_STREAM) *pStream;
	pui32Current = (IMG_UINT32 *) pvOutBuffer;

	*pui32Current = ExtDBGDrivGetMarker(psStream);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivGetServiceTable(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *	pui32Out;

	PVR_UNREFERENCED_PARAMETER(pvInBuffer);
	pui32Out = (IMG_UINT32 *) pvOutBuffer;

	*pui32Out = DBGDrivGetServiceTable();

    return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivWriteLF(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	PDBG_IN_WRITE_LF	psInParams;
	IMG_UINT32 *				pui32BytesCopied;

	psInParams = (PDBG_IN_WRITE_LF) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	*pui32BytesCopied = ExtDBGDrivWriteLF(psInParams->pvStream,
										psInParams->pui8InBuffer,
										psInParams->ui32BufferSize,
										psInParams->ui32Level,
										psInParams->ui32Flags);

	return IMG_TRUE;
}

IMG_UINT32 DBGDIOCDrivReadLF(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	IMG_UINT32 *				pui32BytesCopied;
	PDBG_IN_READ		psInParams;

	psInParams = (PDBG_IN_READ) pvInBuffer;
	pui32BytesCopied = (IMG_UINT32 *) pvOutBuffer;

	*pui32BytesCopied = ExtDBGDrivReadLF((PDBG_STREAM) psInParams->pvStream,psInParams->ui32OutBufferSize,psInParams->pui8OutBuffer);

	return(IMG_TRUE);
}

IMG_UINT32 DBGDIOCDrivWaitForEvent(IMG_VOID * pvInBuffer, IMG_VOID * pvOutBuffer)
{
	DBG_EVENT eEvent = (DBG_EVENT)(*(IMG_UINT32 *)pvInBuffer);

	PVR_UNREFERENCED_PARAMETER(pvOutBuffer);

	ExtDBGDrivWaitForEvent(eEvent);

	return(IMG_TRUE);
}
