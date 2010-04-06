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

#include <linux/string.h>

#include "img_types.h"
#include "pvr_debug.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hotkey.h"
#include "hostfunc.h"

#define LAST_FRAME_BUF_SIZE	1024

struct DBG_LASTFRAME_BUFFER {
	struct DBG_STREAM *psStream;
	u8 ui8Buffer[LAST_FRAME_BUF_SIZE];
	u32 ui32BufLen;
	struct DBG_LASTFRAME_BUFFER *psNext;
};

static struct DBG_STREAM *g_psStreamList;
static struct DBG_LASTFRAME_BUFFER *g_psLFBufferList;

static u32 g_ui32LOff;
static u32 g_ui32Line;
static u32 g_ui32MonoLines = 25;

static IMG_BOOL g_bHotkeyMiddump = IMG_FALSE;
static u32 g_ui32HotkeyMiddumpStart = 0xffffffff;
static u32 g_ui32HotkeyMiddumpEnd = 0xffffffff;

void *g_pvAPIMutex;

IMG_BOOL gbDumpThisFrame = IMG_FALSE;

static u32 SpaceInStream(struct DBG_STREAM *psStream);
static IMG_BOOL ExpandStreamBuffer(struct DBG_STREAM *psStream,
				   u32 ui32NewSize);
struct DBG_LASTFRAME_BUFFER *FindLFBuf(struct DBG_STREAM *psStream);

struct DBGKM_SERVICE_TABLE g_sDBGKMServices = {
	sizeof(struct DBGKM_SERVICE_TABLE),
	ExtDBGDrivCreateStream,
	ExtDBGDrivDestroyStream,
	ExtDBGDrivFindStream,
	ExtDBGDrivWriteString,
	ExtDBGDrivReadString,
	ExtDBGDrivWrite,
	ExtDBGDrivRead,
	ExtDBGDrivSetCaptureMode,
	ExtDBGDrivSetOutputMode,
	ExtDBGDrivSetDebugLevel,
	ExtDBGDrivSetFrame,
	ExtDBGDrivGetFrame,
	ExtDBGDrivOverrideMode,
	ExtDBGDrivDefaultMode,
	ExtDBGDrivWrite2,
	ExtDBGDrivWriteStringCM,
	ExtDBGDrivWriteCM,
	ExtDBGDrivSetMarker,
	ExtDBGDrivGetMarker,
	ExtDBGDrivStartInitPhase,
	ExtDBGDrivStopInitPhase,
	ExtDBGDrivIsCaptureFrame,
	ExtDBGDrivWriteLF,
	ExtDBGDrivReadLF,
	ExtDBGDrivGetStreamOffset,
	ExtDBGDrivSetStreamOffset,
	ExtDBGDrivIsLastCaptureFrame,
	ExtDBGDrivWaitForEvent
};

void *ExtDBGDrivCreateStream(char *pszName,
					      u32 ui32CapMode,
					      u32 ui32OutMode,
					      u32 ui32Flags,
					      u32 ui32Size)
{
	void *pvRet;

	HostAquireMutex(g_pvAPIMutex);

	pvRet =
	    DBGDrivCreateStream(pszName, ui32CapMode, ui32OutMode, ui32Flags,
				ui32Size);

	HostReleaseMutex(g_pvAPIMutex);

	return pvRet;
}

void ExtDBGDrivDestroyStream(struct DBG_STREAM *psStream)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivDestroyStream(psStream);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void *ExtDBGDrivFindStream(char *pszName,
					    IMG_BOOL bResetStream)
{
	void *pvRet;

	HostAquireMutex(g_pvAPIMutex);

	pvRet = DBGDrivFindStream(pszName, bResetStream);

	HostReleaseMutex(g_pvAPIMutex);

	return pvRet;
}

u32 ExtDBGDrivWriteString(struct DBG_STREAM *psStream,
					      char *pszString,
					      u32 ui32Level)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivWriteString(psStream, pszString, ui32Level);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

u32 ExtDBGDrivReadString(struct DBG_STREAM *psStream,
					     char *pszString,
					     u32 ui32Limit)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivReadString(psStream, pszString, ui32Limit);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

u32 ExtDBGDrivWrite(struct DBG_STREAM *psStream,
					u8 *pui8InBuf,
					u32 ui32InBuffSize,
					u32 ui32Level)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivWrite(psStream, pui8InBuf, ui32InBuffSize, ui32Level);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

u32 ExtDBGDrivRead(struct DBG_STREAM *psStream,
				       IMG_BOOL bReadInitBuffer,
				       u32 ui32OutBuffSize,
				       u8 *pui8OutBuf)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret =
	    DBGDrivRead(psStream, bReadInitBuffer, ui32OutBuffSize, pui8OutBuf);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

void ExtDBGDrivSetCaptureMode(struct DBG_STREAM *psStream,
					   u32 ui32Mode,
					   u32 ui32Start,
					   u32 ui32End,
					   u32 ui32SampleRate)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetCaptureMode(psStream, ui32Mode, ui32Start, ui32End,
			      ui32SampleRate);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void ExtDBGDrivSetOutputMode(struct DBG_STREAM *psStream,
					  u32 ui32OutMode)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetOutputMode(psStream, ui32OutMode);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void ExtDBGDrivSetDebugLevel(struct DBG_STREAM *psStream,
					  u32 ui32DebugLevel)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetDebugLevel(psStream, ui32DebugLevel);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void ExtDBGDrivSetFrame(struct DBG_STREAM *psStream, u32 ui32Frame)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetFrame(psStream, ui32Frame);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

u32 ExtDBGDrivGetFrame(struct DBG_STREAM *psStream)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivGetFrame(psStream);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

u32 ExtDBGDrivIsLastCaptureFrame(struct DBG_STREAM *psStream)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivIsLastCaptureFrame(psStream);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

u32 ExtDBGDrivIsCaptureFrame(struct DBG_STREAM *psStream,
						 IMG_BOOL bCheckPreviousFrame)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivIsCaptureFrame(psStream, bCheckPreviousFrame);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

void ExtDBGDrivOverrideMode(struct DBG_STREAM *psStream,
					 u32 ui32Mode)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivOverrideMode(psStream, ui32Mode);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void ExtDBGDrivDefaultMode(struct DBG_STREAM *psStream)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivDefaultMode(psStream);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

u32 ExtDBGDrivWrite2(struct DBG_STREAM *psStream,
					 u8 *pui8InBuf,
					 u32 ui32InBuffSize,
					 u32 ui32Level)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivWrite2(psStream, pui8InBuf, ui32InBuffSize, ui32Level);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

u32 ExtDBGDrivWriteStringCM(struct DBG_STREAM *psStream,
						char *pszString,
						u32 ui32Level)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivWriteStringCM(psStream, pszString, ui32Level);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

u32 ExtDBGDrivWriteCM(struct DBG_STREAM *psStream,
					  u8 *pui8InBuf,
					  u32 ui32InBuffSize,
					  u32 ui32Level)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret =
	    DBGDrivWriteCM(psStream, pui8InBuf, ui32InBuffSize, ui32Level);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

void ExtDBGDrivSetMarker(struct DBG_STREAM *psStream,
				      u32 ui32Marker)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetMarker(psStream, ui32Marker);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

u32 ExtDBGDrivGetMarker(struct DBG_STREAM *psStream)
{
	u32 ui32Marker;

	HostAquireMutex(g_pvAPIMutex);

	ui32Marker = DBGDrivGetMarker(psStream);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Marker;
}

u32 ExtDBGDrivWriteLF(struct DBG_STREAM *psStream,
					  u8 *pui8InBuf,
					  u32 ui32InBuffSize,
					  u32 ui32Level,
					  u32 ui32Flags)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret =
	    DBGDrivWriteLF(psStream, pui8InBuf, ui32InBuffSize, ui32Level,
			   ui32Flags);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

u32 ExtDBGDrivReadLF(struct DBG_STREAM *psStream,
					 u32 ui32OutBuffSize,
					 u8 *pui8OutBuf)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivReadLF(psStream, ui32OutBuffSize, pui8OutBuf);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

void ExtDBGDrivStartInitPhase(struct DBG_STREAM *psStream)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivStartInitPhase(psStream);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

void ExtDBGDrivStopInitPhase(struct DBG_STREAM *psStream)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivStopInitPhase(psStream);

	HostReleaseMutex(g_pvAPIMutex);

	return;
}

u32 ExtDBGDrivGetStreamOffset(struct DBG_STREAM *psStream)
{
	u32 ui32Ret;

	HostAquireMutex(g_pvAPIMutex);

	ui32Ret = DBGDrivGetStreamOffset(psStream);

	HostReleaseMutex(g_pvAPIMutex);

	return ui32Ret;
}

void ExtDBGDrivSetStreamOffset(struct DBG_STREAM *psStream,
						u32 ui32StreamOffset)
{

	HostAquireMutex(g_pvAPIMutex);

	DBGDrivSetStreamOffset(psStream, ui32StreamOffset);

	HostReleaseMutex(g_pvAPIMutex);
}

void ExtDBGDrivWaitForEvent(enum DBG_EVENT eEvent)
{
#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	DBGDrivWaitForEvent(eEvent);
#else
	PVR_UNREFERENCED_PARAMETER(eEvent);
#endif
}

u32 AtoI(char *szIn)
{
	u32 ui32Len = 0;
	u32 ui32Value = 0;
	u32 ui32Digit = 1;
	u32 ui32Base = 10;
	int iPos;
	char bc;

	while (szIn[ui32Len] > 0)
		ui32Len++;

	if (ui32Len == 0)
		return 0;

	iPos = 0;
	while (szIn[iPos] == '0')
		iPos++;
	if (szIn[iPos] == '\0')
		return 0;
	if (szIn[iPos] == 'x' || szIn[iPos] == 'X') {
		ui32Base = 16;
		szIn[iPos] = '0';
	}

	for (iPos = ui32Len - 1; iPos >= 0; iPos--) {
		bc = szIn[iPos];

		if ((bc >= 'a') && (bc <= 'f') && ui32Base == 16)
			bc -= 'a' - 0xa;
		else if ((bc >= 'A') && (bc <= 'F') && ui32Base == 16)
			bc -= 'A' - 0xa;
		else if ((bc >= '0') && (bc <= '9'))
			bc -= '0';
		else
			return 0;

		ui32Value += bc * ui32Digit;

		ui32Digit = ui32Digit * ui32Base;
	}
	return ui32Value;
}

IMG_BOOL StreamValid(struct DBG_STREAM *psStream)
{
	struct DBG_STREAM *psThis;

	psThis = g_psStreamList;

	while (psThis)
		if (psStream && (psThis == psStream))
			return IMG_TRUE;
		else
			psThis = psThis->psNext;

	return IMG_FALSE;
}

void Write(struct DBG_STREAM *psStream, u8 *pui8Data,
	   u32 ui32InBuffSize)
{

	if ((psStream->ui32WPtr + ui32InBuffSize) > psStream->ui32Size) {
		u32 ui32B1 = psStream->ui32Size - psStream->ui32WPtr;
		u32 ui32B2 = ui32InBuffSize - ui32B1;

		HostMemCopy((void *) (psStream->ui32Base +
					  psStream->ui32WPtr),
			    (void *) pui8Data, ui32B1);

		HostMemCopy((void *) psStream->ui32Base,
			    (void *) ((u32) pui8Data + ui32B1),
			    ui32B2);

		psStream->ui32WPtr = ui32B2;
	} else {
		HostMemCopy((void *) (psStream->ui32Base +
					  psStream->ui32WPtr),
			    (void *) pui8Data, ui32InBuffSize);

		psStream->ui32WPtr += ui32InBuffSize;

		if (psStream->ui32WPtr == psStream->ui32Size)
			psStream->ui32WPtr = 0;
	}
	psStream->ui32DataWritten += ui32InBuffSize;
}

void MonoOut(char *pszString, IMG_BOOL bNewLine)
{
	u32 i;
	char *pScreen;

	pScreen = (char *)DBGDRIV_MONOBASE;

	pScreen += g_ui32Line * 160;

	i = 0;
	do {
		pScreen[g_ui32LOff + (i * 2)] = pszString[i];
		pScreen[g_ui32LOff + (i * 2) + 1] = 127;
		i++;
	} while ((pszString[i] != 0) && (i < 4096));

	g_ui32LOff += i * 2;

	if (bNewLine) {
		g_ui32LOff = 0;
		g_ui32Line++;
	}

	if (g_ui32Line == g_ui32MonoLines) {
		g_ui32Line = g_ui32MonoLines - 1;

		HostMemCopy((void *) DBGDRIV_MONOBASE,
			    (void *) (DBGDRIV_MONOBASE + 160),
			    160 * (g_ui32MonoLines - 1));

		HostMemSet((void *) (DBGDRIV_MONOBASE +
					 (160 * (g_ui32MonoLines - 1))), 0,
			   160);
	}
}

void AppendName(char *pszOut, char *pszBase, char *pszName)
{
	u32 i;
	u32 ui32Off;

	i = 0;

	while (pszBase[i] != 0) {
		pszOut[i] = pszBase[i];
		i++;
	}

	ui32Off = i;
	i = 0;

	while (pszName[i] != 0) {
		pszOut[ui32Off + i] = pszName[i];
		i++;
	}

	pszOut[ui32Off + i] = pszName[i];
}

void *DBGDrivCreateStream(char *pszName,
					   u32 ui32CapMode,
					   u32 ui32OutMode,
					   u32 ui32Flags,
					   u32 ui32Size)
{
	struct DBG_STREAM *psStream;
	struct DBG_STREAM *psInitStream;
	struct DBG_LASTFRAME_BUFFER *psLFBuffer;
	u32 ui32Off;
	void *pvBase;

	psStream = (struct DBG_STREAM *)DBGDrivFindStream(pszName, IMG_FALSE);

	if (psStream)
		return (void *)psStream;

	psStream = HostNonPageablePageAlloc(1);
	psInitStream = HostNonPageablePageAlloc(1);
	psLFBuffer = HostNonPageablePageAlloc(1);
	if ((!psStream) || (!psInitStream) || (!psLFBuffer)
	    ) {
		PVR_DPF(PVR_DBG_ERROR,
			 "DBGDriv: Couldn't create buffer !!!!!\n\r");
		return NULL;
	}

	if ((ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
		pvBase = HostNonPageablePageAlloc(ui32Size);
	else
		pvBase = HostPageablePageAlloc(ui32Size);

	if (!pvBase) {
		PVR_DPF(PVR_DBG_ERROR,
			 "DBGDriv: Couldn't create buffer !!!!!\n\r");
		HostNonPageablePageFree(psStream);
		return NULL;
	}

	psStream->psNext = 0;
	psStream->ui32Flags = ui32Flags;
	psStream->ui32Base = (u32) pvBase;
	psStream->ui32Size = ui32Size * 4096;
	psStream->ui32RPtr = 0;
	psStream->ui32WPtr = 0;
	psStream->ui32DataWritten = 0;
	psStream->ui32CapMode = ui32CapMode;
	psStream->ui32OutMode = ui32OutMode;
	psStream->ui32DebugLevel = DEBUG_LEVEL_0;
	psStream->ui32DefaultMode = ui32CapMode;
	psStream->ui32Start = 0;
	psStream->ui32End = 0;
	psStream->ui32Current = 0;
	psStream->ui32SampleRate = 1;
	psStream->ui32Access = 0;
	psStream->ui32Timeout = 0;
	psStream->ui32Marker = 0;
	psStream->bInitPhaseComplete = IMG_FALSE;

	if ((ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
		pvBase = HostNonPageablePageAlloc(ui32Size);
	else
		pvBase = HostPageablePageAlloc(ui32Size);

	if (!pvBase) {
		PVR_DPF(PVR_DBG_ERROR,
			 "DBGDriv: Couldn't create buffer !!!!!\n\r");

		if ((psStream->ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
			HostNonPageablePageFree((void *) psStream->
						ui32Base);
		else
			HostPageablePageFree((void *) psStream->ui32Base);
		HostNonPageablePageFree(psStream);
		return NULL;
	}

	psInitStream->psNext = 0;
	psInitStream->ui32Flags = ui32Flags;
	psInitStream->ui32Base = (u32) pvBase;
	psInitStream->ui32Size = ui32Size * 4096;
	psInitStream->ui32RPtr = 0;
	psInitStream->ui32WPtr = 0;
	psInitStream->ui32DataWritten = 0;
	psInitStream->ui32CapMode = ui32CapMode;
	psInitStream->ui32OutMode = ui32OutMode;
	psInitStream->ui32DebugLevel = DEBUG_LEVEL_0;
	psInitStream->ui32DefaultMode = ui32CapMode;
	psInitStream->ui32Start = 0;
	psInitStream->ui32End = 0;
	psInitStream->ui32Current = 0;
	psInitStream->ui32SampleRate = 1;
	psInitStream->ui32Access = 0;
	psInitStream->ui32Timeout = 0;
	psInitStream->ui32Marker = 0;
	psInitStream->bInitPhaseComplete = IMG_FALSE;

	psStream->psInitStream = psInitStream;

	psLFBuffer->psStream = psStream;
	psLFBuffer->ui32BufLen = 0;

	g_bHotkeyMiddump = IMG_FALSE;
	g_ui32HotkeyMiddumpStart = 0xffffffff;
	g_ui32HotkeyMiddumpEnd = 0xffffffff;

	ui32Off = 0;

	do {
		psStream->szName[ui32Off] = pszName[ui32Off];

		ui32Off++;
	} while ((pszName[ui32Off] != 0)
		 && (ui32Off < (4096 - sizeof(struct DBG_STREAM))));

	psStream->szName[ui32Off] = pszName[ui32Off];

	psStream->psNext = g_psStreamList;
	g_psStreamList = psStream;

	psLFBuffer->psNext = g_psLFBufferList;
	g_psLFBufferList = psLFBuffer;

	return (void *)psStream;
}

void DBGDrivDestroyStream(struct DBG_STREAM *psStream)
{
	struct DBG_STREAM *psStreamThis;
	struct DBG_STREAM *psStreamPrev;
	struct DBG_LASTFRAME_BUFFER *psLFBuffer;
	struct DBG_LASTFRAME_BUFFER *psLFThis;
	struct DBG_LASTFRAME_BUFFER *psLFPrev;

	PVR_DPF(PVR_DBG_MESSAGE, "DBGDriv: Destroying stream %s\r\n",
		 psStream->szName);

	if (!StreamValid(psStream))
		return;

	psLFBuffer = FindLFBuf(psStream);

	psStreamThis = g_psStreamList;
	psStreamPrev = 0;

	while (psStreamThis)
		if (psStreamThis == psStream) {
			if (psStreamPrev)
				psStreamPrev->psNext = psStreamThis->psNext;
			else
				g_psStreamList = psStreamThis->psNext;

			psStreamThis = 0;
		} else {
			psStreamPrev = psStreamThis;
			psStreamThis = psStreamThis->psNext;
		}

	psLFThis = g_psLFBufferList;
	psLFPrev = 0;

	while (psLFThis)
		if (psLFThis == psLFBuffer) {
			if (psLFPrev)
				psLFPrev->psNext = psLFThis->psNext;
			else
				g_psLFBufferList = psLFThis->psNext;

			psLFThis = 0;
		} else {
			psLFPrev = psLFThis;
			psLFThis = psLFThis->psNext;
		}

	if (psStream->ui32CapMode & DEBUG_CAPMODE_HOTKEY)
		DeactivateHotKeys();

	if ((psStream->ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0) {
		HostNonPageablePageFree((void *) psStream->ui32Base);
		HostNonPageablePageFree((void *) psStream->psInitStream->
					ui32Base);
	} else {
		HostPageablePageFree((void *) psStream->ui32Base);
		HostPageablePageFree((void *) psStream->psInitStream->
				     ui32Base);
	}

	HostNonPageablePageFree(psStream->psInitStream);
	HostNonPageablePageFree(psStream);
	HostNonPageablePageFree(psLFBuffer);

	if (g_psStreamList == 0)
		PVR_DPF(PVR_DBG_MESSAGE, "DBGDriv: Stream list now empty");

	return;
}

void *DBGDrivFindStream(char *pszName,
					 IMG_BOOL bResetStream)
{
	struct DBG_STREAM *psStream;
	struct DBG_STREAM *psThis;
	u32 ui32Off;
	IMG_BOOL bAreSame;

	psStream = 0;

	for (psThis = g_psStreamList; psThis != NULL;
	     psThis = psThis->psNext) {
		bAreSame = IMG_TRUE;
		ui32Off = 0;

		if (strlen(psThis->szName) == strlen(pszName)) {
			while ((psThis->szName[ui32Off] != 0)
			       && (pszName[ui32Off] != 0) && (ui32Off < 128)
			       && bAreSame) {
				if (psThis->szName[ui32Off] != pszName[ui32Off])
					bAreSame = IMG_FALSE;

				ui32Off++;
			}
		} else {
			bAreSame = IMG_FALSE;
		}

		if (bAreSame) {
			psStream = psThis;
			break;
		}
	}

	if (bResetStream && psStream) {
		static char szComment[] = "-- Init phase terminated\r\n";
		psStream->psInitStream->ui32RPtr = 0;
		psStream->ui32RPtr = 0;
		psStream->ui32WPtr = 0;
		psStream->ui32DataWritten =
		    psStream->psInitStream->ui32DataWritten;
		if (psStream->bInitPhaseComplete == IMG_FALSE) {
			if (psStream->ui32Flags & DEBUG_FLAGS_TEXTSTREAM)
				DBGDrivWrite2(psStream, (u8 *) szComment,
					      sizeof(szComment) - 1, 0x01);
			psStream->bInitPhaseComplete = IMG_TRUE;
		}
	}

	return (void *)psStream;
}

u32 DBGDrivWriteStringCM(struct DBG_STREAM *psStream,
					     char *pszString,
					     u32 ui32Level)
{

	if (!StreamValid(psStream))
		return 0xFFFFFFFF;

	if (psStream->ui32CapMode & DEBUG_CAPMODE_FRAMED) {
		if (!(psStream->ui32Flags & DEBUG_FLAGS_ENABLESAMPLE))
			return 0;
	} else {
		if (psStream->ui32CapMode == DEBUG_CAPMODE_HOTKEY)
			if ((psStream->ui32Current != g_ui32HotKeyFrame)
			    || (g_bHotKeyPressed == IMG_FALSE))
				return 0;
	}

	return DBGDrivWriteString(psStream, pszString, ui32Level);

}

u32 DBGDrivWriteString(struct DBG_STREAM *psStream,
					   char *pszString,
					   u32 ui32Level)
{
	u32 ui32Len;
	u32 ui32Space;
	u32 ui32WPtr;
	u8 *pui8Buffer;

	if (!StreamValid(psStream))
		return 0xFFFFFFFF;

	if (!(psStream->ui32DebugLevel & ui32Level))
		return 0xFFFFFFFF;

	if (!(psStream->ui32OutMode & DEBUG_OUTMODE_ASYNC)) {
		if (psStream->ui32OutMode & DEBUG_OUTMODE_STANDARDDBG)
			PVR_DPF(PVR_DBG_MESSAGE, "%s: %s\r\n",
				 psStream->szName, pszString);

		if (psStream->ui32OutMode & DEBUG_OUTMODE_MONO) {
			MonoOut(psStream->szName, IMG_FALSE);
			MonoOut(": ", IMG_FALSE);
			MonoOut(pszString, IMG_TRUE);
		}
	}

	if (!((psStream->ui32OutMode & DEBUG_OUTMODE_STREAMENABLE) ||
	      (psStream->ui32OutMode & DEBUG_OUTMODE_ASYNC)
	    )
	    )
		return 0xFFFFFFFF;

	ui32Space = SpaceInStream(psStream);

	if (ui32Space > 0)
		ui32Space--;

	ui32Len = 0;
	ui32WPtr = psStream->ui32WPtr;
	pui8Buffer = (u8 *) psStream->ui32Base;

	while ((pszString[ui32Len] != 0) && (ui32Len < ui32Space)) {
		pui8Buffer[ui32WPtr] = pszString[ui32Len];
		ui32Len++;
		ui32WPtr++;
		if (ui32WPtr == psStream->ui32Size)
			ui32WPtr = 0;
	}

	if (ui32Len < ui32Space) {

		pui8Buffer[ui32WPtr] = pszString[ui32Len];
		ui32Len++;
		ui32WPtr++;
		if (ui32WPtr == psStream->ui32Size)
			ui32WPtr = 0;

		psStream->ui32WPtr = ui32WPtr;
		psStream->ui32DataWritten += ui32Len;
	} else {
		ui32Len = 0;
	}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	if (ui32Len)
		HostSignalEvent(DBG_EVENT_STREAM_DATA);
#endif

	return ui32Len;
}

u32 DBGDrivReadString(struct DBG_STREAM *psStream,
					  char *pszString,
					  u32 ui32Limit)
{
	u32 ui32OutLen;
	u32 ui32Len;
	u32 ui32Offset;
	u8 *pui8Buff;

	if (!StreamValid(psStream))
		return 0;

	pui8Buff = (u8 *) psStream->ui32Base;
	ui32Offset = psStream->ui32RPtr;

	if (psStream->ui32RPtr == psStream->ui32WPtr)
		return 0;

	ui32Len = 0;
	while ((pui8Buff[ui32Offset] != 0)
	       && (ui32Offset != psStream->ui32WPtr)) {
		ui32Offset++;
		ui32Len++;

		if (ui32Offset == psStream->ui32Size)
			ui32Offset = 0;
	}

	ui32OutLen = ui32Len + 1;

	if (ui32Len > ui32Limit)
		return 0;

	ui32Offset = psStream->ui32RPtr;
	ui32Len = 0;

	while ((pui8Buff[ui32Offset] != 0) && (ui32Len < ui32Limit)) {
		pszString[ui32Len] = pui8Buff[ui32Offset];
		ui32Offset++;
		ui32Len++;

		if (ui32Offset == psStream->ui32Size)
			ui32Offset = 0;
	}

	pszString[ui32Len] = pui8Buff[ui32Offset];

	psStream->ui32RPtr = ui32Offset + 1;

	if (psStream->ui32RPtr == psStream->ui32Size)
		psStream->ui32RPtr = 0;

	return ui32OutLen;
}

u32 DBGDrivWrite(struct DBG_STREAM *psMainStream,
				     u8 *pui8InBuf,
				     u32 ui32InBuffSize,
				     u32 ui32Level)
{
	u32 ui32Space;
	struct DBG_STREAM *psStream;

	if (!StreamValid(psMainStream))
		return 0xFFFFFFFF;

	if (!(psMainStream->ui32DebugLevel & ui32Level))
		return 0xFFFFFFFF;

	if (psMainStream->ui32CapMode & DEBUG_CAPMODE_FRAMED) {
		if (!(psMainStream->ui32Flags & DEBUG_FLAGS_ENABLESAMPLE))
			return 0xFFFFFFFF;
	} else {
	if (psMainStream->ui32CapMode == DEBUG_CAPMODE_HOTKEY)
		if ((psMainStream->ui32Current != g_ui32HotKeyFrame)
		    || (g_bHotKeyPressed == IMG_FALSE))
			return 0xFFFFFFFF;

	}

	if (psMainStream->bInitPhaseComplete)
		psStream = psMainStream;
	else
		psStream = psMainStream->psInitStream;

	ui32Space = SpaceInStream(psStream);

	if (!(psStream->ui32OutMode & DEBUG_OUTMODE_STREAMENABLE))
		return 0;

	if (ui32Space < 8)
		return 0;

	if (ui32Space <= (ui32InBuffSize + 4))
		ui32InBuffSize = ui32Space - 8;

	Write(psStream, (u8 *) &ui32InBuffSize, 4);
	Write(psStream, pui8InBuf, ui32InBuffSize);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	if (ui32InBuffSize)
		HostSignalEvent(DBG_EVENT_STREAM_DATA);
#endif
	return ui32InBuffSize;
}

u32 DBGDrivWriteCM(struct DBG_STREAM *psStream,
				       u8 *pui8InBuf,
				       u32 ui32InBuffSize,
				       u32 ui32Level)
{

	if (!StreamValid(psStream))
		return 0xFFFFFFFF;

	if (psStream->ui32CapMode & DEBUG_CAPMODE_FRAMED) {
		if (!(psStream->ui32Flags & DEBUG_FLAGS_ENABLESAMPLE))
			return 0xFFFFFFFF;
	} else {
		if (psStream->ui32CapMode == DEBUG_CAPMODE_HOTKEY)
			if ((psStream->ui32Current != g_ui32HotKeyFrame)
			    || (g_bHotKeyPressed == IMG_FALSE))
				return 0xFFFFFFFF;
	}

	return DBGDrivWrite2(psStream, pui8InBuf, ui32InBuffSize, ui32Level);
}

u32 DBGDrivWrite2(struct DBG_STREAM *psMainStream,
				      u8 *pui8InBuf,
				      u32 ui32InBuffSize,
				      u32 ui32Level)
{
	u32 ui32Space;
	struct DBG_STREAM *psStream;

	if (!StreamValid(psMainStream))
		return 0xFFFFFFFF;

	if (!(psMainStream->ui32DebugLevel & ui32Level))
		return 0xFFFFFFFF;

	if (psMainStream->bInitPhaseComplete)
		psStream = psMainStream;
	else
		psStream = psMainStream->psInitStream;

	ui32Space = SpaceInStream(psStream);

	if (!(psStream->ui32OutMode & DEBUG_OUTMODE_STREAMENABLE))
		return 0;

	if (psStream->ui32Flags & DEBUG_FLAGS_NO_BUF_EXPANDSION) {

		if (ui32Space < 32)
			return 0;
	} else {
		if ((ui32Space < 32) || (ui32Space <= (ui32InBuffSize + 4))) {
			u32 ui32NewBufSize;

			ui32NewBufSize = 2 * psStream->ui32Size;

			if (ui32InBuffSize > psStream->ui32Size)
				ui32NewBufSize += ui32InBuffSize;

			if (!ExpandStreamBuffer(psStream, ui32NewBufSize))
				if (ui32Space < 32)
					return 0;

			ui32Space = SpaceInStream(psStream);
		}
	}

	if (ui32Space <= (ui32InBuffSize + 4))
		ui32InBuffSize = ui32Space - 4;

	Write(psStream, pui8InBuf, ui32InBuffSize);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	if (ui32InBuffSize)
		HostSignalEvent(DBG_EVENT_STREAM_DATA);
#endif
	return ui32InBuffSize;
}

u32 DBGDrivRead(struct DBG_STREAM *psMainStream,
				    IMG_BOOL bReadInitBuffer,
				    u32 ui32OutBuffSize,
				    u8 *pui8OutBuf)
{
	u32 ui32Data;
	struct DBG_STREAM *psStream;

	if (!StreamValid(psMainStream))
		return 0;

	if (bReadInitBuffer)
		psStream = psMainStream->psInitStream;
	else
		psStream = psMainStream;

	if (psStream->ui32RPtr == psStream->ui32WPtr)
		return 0;

	if (psStream->ui32RPtr <= psStream->ui32WPtr) {
		ui32Data = psStream->ui32WPtr - psStream->ui32RPtr;
	} else {
		ui32Data =
		    psStream->ui32WPtr + (psStream->ui32Size -
					  psStream->ui32RPtr);
	}

	if (ui32Data > ui32OutBuffSize)
		ui32Data = ui32OutBuffSize;

	if ((psStream->ui32RPtr + ui32Data) > psStream->ui32Size) {
		u32 ui32B1 = psStream->ui32Size - psStream->ui32RPtr;
		u32 ui32B2 = ui32Data - ui32B1;

		HostMemCopy((void *) pui8OutBuf,
			    (void *) (psStream->ui32Base +
					  psStream->ui32RPtr), ui32B1);

		HostMemCopy((void *) ((u32) pui8OutBuf + ui32B1),
			    (void *) psStream->ui32Base, ui32B2);

		psStream->ui32RPtr = ui32B2;
	} else {
		HostMemCopy((void *) pui8OutBuf,
			    (void *) (psStream->ui32Base +
					  psStream->ui32RPtr), ui32Data);

		psStream->ui32RPtr += ui32Data;

		if (psStream->ui32RPtr == psStream->ui32Size)
			psStream->ui32RPtr = 0;
	}

	return ui32Data;
}

void DBGDrivSetCaptureMode(struct DBG_STREAM *psStream,
					u32 ui32Mode,
					u32 ui32Start,
					u32 ui32End,
					u32 ui32SampleRate)
{

	if (!StreamValid(psStream))
		return;

	psStream->ui32CapMode = ui32Mode;
	psStream->ui32DefaultMode = ui32Mode;
	psStream->ui32Start = ui32Start;
	psStream->ui32End = ui32End;
	psStream->ui32SampleRate = ui32SampleRate;

	if (psStream->ui32CapMode & DEBUG_CAPMODE_HOTKEY)
		ActivateHotKeys(psStream);
}

void DBGDrivSetOutputMode(struct DBG_STREAM *psStream,
				       u32 ui32OutMode)
{

	if (!StreamValid(psStream))
		return;

	psStream->ui32OutMode = ui32OutMode;
}

void DBGDrivSetDebugLevel(struct DBG_STREAM *psStream,
				       u32 ui32DebugLevel)
{

	if (!StreamValid(psStream))
		return;

	psStream->ui32DebugLevel = ui32DebugLevel;
}

void DBGDrivSetFrame(struct DBG_STREAM *psStream, u32 ui32Frame)
{

	if (!StreamValid(psStream))
		return;

	psStream->ui32Current = ui32Frame;

	if ((ui32Frame >= psStream->ui32Start) &&
	    (ui32Frame <= psStream->ui32End) &&
	    (((ui32Frame - psStream->ui32Start) % psStream->ui32SampleRate) ==
	     0))
		psStream->ui32Flags |= DEBUG_FLAGS_ENABLESAMPLE;
	else
		psStream->ui32Flags &= ~DEBUG_FLAGS_ENABLESAMPLE;

	if (g_bHotkeyMiddump) {
		if ((ui32Frame >= g_ui32HotkeyMiddumpStart) &&
		    (ui32Frame <= g_ui32HotkeyMiddumpEnd) &&
		    (((ui32Frame -
		       g_ui32HotkeyMiddumpStart) % psStream->ui32SampleRate) ==
		     0)) {
			psStream->ui32Flags |= DEBUG_FLAGS_ENABLESAMPLE;
		} else {
			psStream->ui32Flags &= ~DEBUG_FLAGS_ENABLESAMPLE;
			if (psStream->ui32Current > g_ui32HotkeyMiddumpEnd)
				g_bHotkeyMiddump = IMG_FALSE;
		}
	}

	if (g_bHotKeyRegistered) {
		g_bHotKeyRegistered = IMG_FALSE;

		PVR_DPF(PVR_DBG_MESSAGE, "Hotkey pressed (%08x)!\n",
			 psStream);

		if (!g_bHotKeyPressed) {

			g_ui32HotKeyFrame = psStream->ui32Current + 2;

			g_bHotKeyPressed = IMG_TRUE;
		}

		if ((psStream->ui32CapMode & DEBUG_CAPMODE_FRAMED)
		    && (psStream->ui32CapMode & DEBUG_CAPMODE_HOTKEY)) {
			if (!g_bHotkeyMiddump) {

				g_ui32HotkeyMiddumpStart =
				    g_ui32HotKeyFrame + 1;
				g_ui32HotkeyMiddumpEnd = 0xffffffff;
				g_bHotkeyMiddump = IMG_TRUE;
				PVR_DPF(PVR_DBG_MESSAGE,
					 "Sampling every %d frame(s)\n",
					 psStream->ui32SampleRate);
			} else {

				g_ui32HotkeyMiddumpEnd = g_ui32HotKeyFrame;
				PVR_DPF(PVR_DBG_MESSAGE,
					 "Turning off sampling\n");
			}
		}

	}

	if (psStream->ui32Current > g_ui32HotKeyFrame)
		g_bHotKeyPressed = IMG_FALSE;
}

u32 DBGDrivGetFrame(struct DBG_STREAM *psStream)
{

	if (!StreamValid(psStream))
		return 0;

	return psStream->ui32Current;
}

u32 DBGDrivIsLastCaptureFrame(struct DBG_STREAM *psStream)
{
	u32 ui32NextFrame;

	if (!StreamValid(psStream))
		return IMG_FALSE;

	if (psStream->ui32CapMode & DEBUG_CAPMODE_FRAMED) {
		ui32NextFrame =
		    psStream->ui32Current + psStream->ui32SampleRate;
		if (ui32NextFrame > psStream->ui32End)
			return IMG_TRUE;
	}
	return IMG_FALSE;
}

u32 DBGDrivIsCaptureFrame(struct DBG_STREAM *psStream,
					      IMG_BOOL bCheckPreviousFrame)
{
	u32 ui32FrameShift = bCheckPreviousFrame ? 1 : 0;

	if (!StreamValid(psStream))
		return IMG_FALSE;

	if (psStream->ui32CapMode & DEBUG_CAPMODE_FRAMED) {

		if (g_bHotkeyMiddump) {
			if ((psStream->ui32Current >=
			     (g_ui32HotkeyMiddumpStart - ui32FrameShift))
			    && (psStream->ui32Current <=
				(g_ui32HotkeyMiddumpEnd - ui32FrameShift))
			    &&
			    ((((psStream->ui32Current + ui32FrameShift) -
			       g_ui32HotkeyMiddumpStart) %
			      psStream->ui32SampleRate) == 0))
				return IMG_TRUE;
		} else {
			if ((psStream->ui32Current >=
			     (psStream->ui32Start - ui32FrameShift))
			    && (psStream->ui32Current <=
				(psStream->ui32End - ui32FrameShift))
			    &&
			    ((((psStream->ui32Current + ui32FrameShift) -
			       psStream->ui32Start) %
			      psStream->ui32SampleRate) == 0))
				return IMG_TRUE;
		}
	} else if (psStream->ui32CapMode == DEBUG_CAPMODE_HOTKEY) {
		if ((psStream->ui32Current ==
		     (g_ui32HotKeyFrame - ui32FrameShift))
		    && (g_bHotKeyPressed))
			return IMG_TRUE;
	}


	return IMG_FALSE;
}

void DBGDrivOverrideMode(struct DBG_STREAM *psStream, u32 ui32Mode)
{

	if (!StreamValid(psStream))
		return;

	psStream->ui32CapMode = ui32Mode;
}

void DBGDrivDefaultMode(struct DBG_STREAM *psStream)
{

	if (!StreamValid(psStream))
		return;

	psStream->ui32CapMode = psStream->ui32DefaultMode;
}

void DBGDrivSetMarker(struct DBG_STREAM *psStream, u32 ui32Marker)
{

	if (!StreamValid(psStream))
		return;

	psStream->ui32Marker = ui32Marker;
}

u32 DBGDrivGetMarker(struct DBG_STREAM *psStream)
{

	if (!StreamValid(psStream))
		return 0;

	return psStream->ui32Marker;
}

u32 DBGDrivGetStreamOffset(struct DBG_STREAM *psMainStream)
{
	struct DBG_STREAM *psStream;

	if (!StreamValid(psMainStream))
		return 0;

	if (psMainStream->bInitPhaseComplete)
		psStream = psMainStream;
	else
		psStream = psMainStream->psInitStream;

	return psStream->ui32DataWritten;
}

void DBGDrivSetStreamOffset(struct DBG_STREAM *psMainStream,
					     u32 ui32StreamOffset)
{
	struct DBG_STREAM *psStream;

	if (!StreamValid(psMainStream))
		return;

	if (psMainStream->bInitPhaseComplete)
		psStream = psMainStream;
	else
		psStream = psMainStream->psInitStream;

	psStream->ui32DataWritten = ui32StreamOffset;
}

u32 DBGDrivGetServiceTable(void)
{
	return (u32)&g_sDBGKMServices;
}

u32 DBGDrivWriteLF(struct DBG_STREAM *psStream,
				       u8 *pui8InBuf,
				       u32 ui32InBuffSize,
				       u32 ui32Level,
				       u32 ui32Flags)
{
	struct DBG_LASTFRAME_BUFFER *psLFBuffer;

	if (!StreamValid(psStream))
		return 0xFFFFFFFF;

	if (!(psStream->ui32DebugLevel & ui32Level))
		return 0xFFFFFFFF;

	if (psStream->ui32CapMode & DEBUG_CAPMODE_FRAMED) {
		if (!(psStream->ui32Flags & DEBUG_FLAGS_ENABLESAMPLE))
			return 0xFFFFFFFF;
	} else if (psStream->ui32CapMode == DEBUG_CAPMODE_HOTKEY) {
		if ((psStream->ui32Current != g_ui32HotKeyFrame)
		    || (g_bHotKeyPressed == IMG_FALSE))
			return 0xFFFFFFFF;
	}

	psLFBuffer = FindLFBuf(psStream);

	if (ui32Flags & WRITELF_FLAGS_RESETBUF) {

		ui32InBuffSize =
		    (ui32InBuffSize >
		     LAST_FRAME_BUF_SIZE) ? LAST_FRAME_BUF_SIZE :
		    ui32InBuffSize;
		HostMemCopy((void *) psLFBuffer->ui8Buffer,
			    (void *) pui8InBuf, ui32InBuffSize);
		psLFBuffer->ui32BufLen = ui32InBuffSize;
	} else {

		ui32InBuffSize =
		    ((psLFBuffer->ui32BufLen + ui32InBuffSize) >
		     LAST_FRAME_BUF_SIZE) ? (LAST_FRAME_BUF_SIZE -
					     psLFBuffer->
					     ui32BufLen) : ui32InBuffSize;
		HostMemCopy((void *) (&psLFBuffer->
					  ui8Buffer[psLFBuffer->ui32BufLen]),
			    (void *) pui8InBuf, ui32InBuffSize);
		psLFBuffer->ui32BufLen += ui32InBuffSize;
	}

	return ui32InBuffSize;
}

u32 DBGDrivReadLF(struct DBG_STREAM *psStream,
				      u32 ui32OutBuffSize,
				      u8 *pui8OutBuf)
{
	struct DBG_LASTFRAME_BUFFER *psLFBuffer;
	u32 ui32Data;

	if (!StreamValid(psStream))
		return 0;

	psLFBuffer = FindLFBuf(psStream);

	ui32Data =
	    (ui32OutBuffSize <
	     psLFBuffer->ui32BufLen) ? ui32OutBuffSize : psLFBuffer->ui32BufLen;

	HostMemCopy((void *) pui8OutBuf, (void *) psLFBuffer->ui8Buffer,
		    ui32Data);

	return ui32Data;
}

void DBGDrivStartInitPhase(struct DBG_STREAM *psStream)
{
	psStream->bInitPhaseComplete = IMG_FALSE;
}

void DBGDrivStopInitPhase(struct DBG_STREAM *psStream)
{
	psStream->bInitPhaseComplete = IMG_TRUE;
}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
void DBGDrivWaitForEvent(enum DBG_EVENT eEvent)
{
	HostWaitForEvent(eEvent);
}
#endif

static IMG_BOOL ExpandStreamBuffer(struct DBG_STREAM *psStream, u32 ui32NewSize)
{
	void *pvNewBuf;
	u32 ui32NewSizeInPages;
	u32 ui32NewWOffset;
	u32 ui32SpaceInOldBuf;

	if (psStream->ui32Size >= ui32NewSize)
		return IMG_FALSE;

	ui32SpaceInOldBuf = SpaceInStream(psStream);

	ui32NewSizeInPages = ((ui32NewSize + 0xfff) & ~0xfff) / 4096;

	if ((psStream->ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
		pvNewBuf = HostNonPageablePageAlloc(ui32NewSizeInPages);
	else
		pvNewBuf = HostPageablePageAlloc(ui32NewSizeInPages);

	if (pvNewBuf == NULL)
		return IMG_FALSE;

	if (psStream->ui32RPtr <= psStream->ui32WPtr) {

		HostMemCopy((void *) pvNewBuf,
			    (void *) (psStream->ui32Base +
					  psStream->ui32RPtr),
			    psStream->ui32WPtr - psStream->ui32RPtr);
	} else {
		u32 ui32FirstCopySize;

		ui32FirstCopySize = psStream->ui32Size - psStream->ui32RPtr;

		HostMemCopy((void *) pvNewBuf,
			    (void *) (psStream->ui32Base +
					  psStream->ui32RPtr),
			    ui32FirstCopySize);

		HostMemCopy((void *) ((u32) pvNewBuf +
					  ui32FirstCopySize),
			    (void *) psStream->ui32Base,
			    psStream->ui32WPtr);
	}

	ui32NewWOffset = psStream->ui32Size - ui32SpaceInOldBuf;

	if ((psStream->ui32Flags & DEBUG_FLAGS_USE_NONPAGED_MEM) != 0)
		HostNonPageablePageFree((void *) psStream->ui32Base);
	else
		HostPageablePageFree((void *) psStream->ui32Base);

	psStream->ui32Base = (u32) pvNewBuf;
	psStream->ui32RPtr = 0;
	psStream->ui32WPtr = ui32NewWOffset;
	psStream->ui32Size = ui32NewSizeInPages * 4096;

	return IMG_TRUE;
}

static u32 SpaceInStream(struct DBG_STREAM *psStream)
{
	u32 ui32Space;

	if (psStream->ui32RPtr > psStream->ui32WPtr)
		ui32Space = psStream->ui32RPtr - psStream->ui32WPtr;
	else
		ui32Space =
		    psStream->ui32RPtr + (psStream->ui32Size -
					  psStream->ui32WPtr);

	return ui32Space;
}

void DestroyAllStreams(void)
{
	while (g_psStreamList != NULL)
		DBGDrivDestroyStream(g_psStreamList);
	return;
}

struct DBG_LASTFRAME_BUFFER *FindLFBuf(struct DBG_STREAM *psStream)
{
	struct DBG_LASTFRAME_BUFFER *psLFBuffer;

	psLFBuffer = g_psLFBufferList;

	while (psLFBuffer) {
		if (psLFBuffer->psStream == psStream)
			break;

		psLFBuffer = psLFBuffer->psNext;
	}

	return psLFBuffer;
}
