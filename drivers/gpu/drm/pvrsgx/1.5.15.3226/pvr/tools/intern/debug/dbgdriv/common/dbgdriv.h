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

#ifndef _DBGDRIV_
#define _DBGDRIV_

#define BUFFER_SIZE 64*PAGESIZE

#define DBGDRIV_VERSION 	0x100
#define MAX_PROCESSES 		2
#define BLOCK_USED			0x01
#define BLOCK_LOCKED		0x02
#define DBGDRIV_MONOBASE	0x000B0000


extern IMG_VOID *	g_pvAPIMutex;

IMG_VOID * IMG_CALLCONV DBGDrivCreateStream(IMG_CHAR *		pszName,
								   IMG_UINT32 	ui32CapMode,
								   IMG_UINT32 	ui32OutMode,
								   IMG_UINT32	ui32Flags,
								   IMG_UINT32 	ui32Pages);
IMG_VOID   IMG_CALLCONV DBGDrivDestroyStream(PDBG_STREAM psStream);
IMG_VOID * IMG_CALLCONV DBGDrivFindStream(IMG_CHAR * pszName, IMG_BOOL bResetStream);
IMG_UINT32 IMG_CALLCONV DBGDrivWriteString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV DBGDrivReadString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Limit);
IMG_UINT32 IMG_CALLCONV DBGDrivWrite(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV DBGDrivWrite2(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV DBGDrivRead(PDBG_STREAM psStream, IMG_BOOL bReadInitBuffer, IMG_UINT32 ui32OutBufferSize,IMG_UINT8 *pui8OutBuf);
IMG_VOID   IMG_CALLCONV DBGDrivSetCaptureMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode,IMG_UINT32 ui32Start,IMG_UINT32 ui32Stop,IMG_UINT32 ui32SampleRate);
IMG_VOID   IMG_CALLCONV DBGDrivSetOutputMode(PDBG_STREAM psStream,IMG_UINT32 ui32OutMode);
IMG_VOID   IMG_CALLCONV DBGDrivSetDebugLevel(PDBG_STREAM psStream,IMG_UINT32 ui32DebugLevel);
IMG_VOID   IMG_CALLCONV DBGDrivSetFrame(PDBG_STREAM psStream,IMG_UINT32 ui32Frame);
IMG_UINT32 IMG_CALLCONV DBGDrivGetFrame(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV DBGDrivOverrideMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode);
IMG_VOID   IMG_CALLCONV DBGDrivDefaultMode(PDBG_STREAM psStream);
IMG_UINT32 IMG_CALLCONV DBGDrivGetServiceTable(IMG_VOID);
IMG_UINT32 IMG_CALLCONV DBGDrivWriteStringCM(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV DBGDrivWriteCM(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_VOID   IMG_CALLCONV DBGDrivSetMarker(PDBG_STREAM psStream, IMG_UINT32 ui32Marker);
IMG_UINT32 IMG_CALLCONV DBGDrivGetMarker(PDBG_STREAM psStream);
IMG_BOOL   IMG_CALLCONV DBGDrivIsLastCaptureFrame(PDBG_STREAM psStream);
IMG_BOOL   IMG_CALLCONV DBGDrivIsCaptureFrame(PDBG_STREAM psStream, IMG_BOOL bCheckPreviousFrame);
IMG_UINT32 IMG_CALLCONV DBGDrivWriteLF(PDBG_STREAM psStream, IMG_UINT8 *pui8InBuf, IMG_UINT32 ui32InBuffSize, IMG_UINT32 ui32Level, IMG_UINT32 ui32Flags);
IMG_UINT32 IMG_CALLCONV DBGDrivReadLF(PDBG_STREAM psStream, IMG_UINT32 ui32OutBuffSize, IMG_UINT8 *pui8OutBuf);
IMG_VOID   IMG_CALLCONV DBGDrivStartInitPhase(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV DBGDrivStopInitPhase(PDBG_STREAM psStream);
IMG_UINT32 IMG_CALLCONV DBGDrivGetStreamOffset(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV DBGDrivSetStreamOffset(PDBG_STREAM psStream, IMG_UINT32 ui32StreamOffset);
IMG_VOID   IMG_CALLCONV DBGDrivWaitForEvent(DBG_EVENT eEvent);

IMG_VOID DestroyAllStreams(IMG_VOID);

IMG_UINT32 AtoI(IMG_CHAR *szIn);

IMG_VOID HostMemSet(IMG_VOID *pvDest,IMG_UINT8 ui8Value,IMG_UINT32 ui32Size);
IMG_VOID HostMemCopy(IMG_VOID *pvDest,IMG_VOID *pvSrc,IMG_UINT32 ui32Size);
IMG_BOOL StreamValid(PDBG_STREAM psStream);
IMG_VOID Write(PDBG_STREAM psStream,IMG_UINT8 *pui8Data,IMG_UINT32 ui32InBuffSize);
IMG_VOID MonoOut(IMG_CHAR * pszString,IMG_BOOL bNewLine);


IMG_VOID * IMG_CALLCONV ExtDBGDrivCreateStream(IMG_CHAR *	pszName, IMG_UINT32 ui32CapMode, IMG_UINT32	ui32OutMode, IMG_UINT32 ui32Flags, IMG_UINT32 ui32Size);
IMG_VOID   IMG_CALLCONV ExtDBGDrivDestroyStream(PDBG_STREAM psStream);
IMG_VOID * IMG_CALLCONV ExtDBGDrivFindStream(IMG_CHAR * pszName, IMG_BOOL bResetStream);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivReadString(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Limit);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWrite(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivRead(PDBG_STREAM psStream, IMG_BOOL bReadInitBuffer, IMG_UINT32 ui32OutBuffSize,IMG_UINT8 *pui8OutBuf);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetCaptureMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode,IMG_UINT32 ui32Start,IMG_UINT32 ui32End,IMG_UINT32 ui32SampleRate);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetOutputMode(PDBG_STREAM psStream,IMG_UINT32 ui32OutMode);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetDebugLevel(PDBG_STREAM psStream,IMG_UINT32 ui32DebugLevel);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetFrame(PDBG_STREAM psStream,IMG_UINT32 ui32Frame);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetFrame(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV ExtDBGDrivOverrideMode(PDBG_STREAM psStream,IMG_UINT32 ui32Mode);
IMG_VOID   IMG_CALLCONV ExtDBGDrivDefaultMode(PDBG_STREAM psStream);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWrite2(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteStringCM(PDBG_STREAM psStream,IMG_CHAR * pszString,IMG_UINT32 ui32Level);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteCM(PDBG_STREAM psStream,IMG_UINT8 *pui8InBuf,IMG_UINT32 ui32InBuffSize,IMG_UINT32 ui32Level);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetMarker(PDBG_STREAM psStream, IMG_UINT32 ui32Marker);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetMarker(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV ExtDBGDrivStartInitPhase(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV ExtDBGDrivStopInitPhase(PDBG_STREAM psStream);
IMG_BOOL   IMG_CALLCONV ExtDBGDrivIsLastCaptureFrame(PDBG_STREAM psStream);
IMG_BOOL   IMG_CALLCONV ExtDBGDrivIsCaptureFrame(PDBG_STREAM psStream, IMG_BOOL bCheckPreviousFrame);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivWriteLF(PDBG_STREAM psStream, IMG_UINT8 *pui8InBuf, IMG_UINT32 ui32InBuffSize, IMG_UINT32 ui32Level, IMG_UINT32 ui32Flags);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivReadLF(PDBG_STREAM psStream, IMG_UINT32 ui32OutBuffSize, IMG_UINT8 *pui8OutBuf);
IMG_UINT32 IMG_CALLCONV ExtDBGDrivGetStreamOffset(PDBG_STREAM psStream);
IMG_VOID   IMG_CALLCONV ExtDBGDrivSetStreamOffset(PDBG_STREAM psStream, IMG_UINT32 ui32StreamOffset);
IMG_VOID   IMG_CALLCONV ExtDBGDrivWaitForEvent(DBG_EVENT eEvent);

#endif

