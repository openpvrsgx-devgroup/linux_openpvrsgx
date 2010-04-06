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

#ifndef _DBGDRIV_
#define _DBGDRIV_

#define BUFFER_SIZE (64 * PAGESIZE)

#define DBGDRIV_VERSION 	0x100
#define MAX_PROCESSES 		2
#define BLOCK_USED			0x01
#define BLOCK_LOCKED		0x02
#define DBGDRIV_MONOBASE	0x000B0000

extern void *g_pvAPIMutex;

void *DBGDrivCreateStream(char *pszName,
					   u32 ui32CapMode,
					   u32 ui32OutMode,
					   u32 ui32Flags,
					   u32 ui32Pages);
void DBGDrivDestroyStream(struct DBG_STREAM *psStream);
void *DBGDrivFindStream(char *pszName,
					 IMG_BOOL bResetStream);
u32 DBGDrivWriteString(struct DBG_STREAM *psStream,
					   char *pszString,
					   u32 ui32Level);
u32 DBGDrivReadString(struct DBG_STREAM *psStream,
					  char *pszString,
					  u32 ui32Limit);
u32 DBGDrivWrite(struct DBG_STREAM *psStream,
				     u8 *pui8InBuf,
				     u32 ui32InBuffSize,
				     u32 ui32Level);
u32 DBGDrivWrite2(struct DBG_STREAM *psStream,
				      u8 *pui8InBuf,
				      u32 ui32InBuffSize,
				      u32 ui32Level);
u32 DBGDrivRead(struct DBG_STREAM *psStream,
				    IMG_BOOL bReadInitBuffer,
				    u32 ui32OutBufferSize,
				    u8 *pui8OutBuf);
void DBGDrivSetCaptureMode(struct DBG_STREAM *psStream,
					    u32 ui32Mode,
					    u32 ui32Start,
					    u32 ui32Stop,
					    u32 ui32SampleRate);
void DBGDrivSetOutputMode(struct DBG_STREAM *psStream,
					   u32 ui32OutMode);
void DBGDrivSetDebugLevel(struct DBG_STREAM *psStream,
					   u32 ui32DebugLevel);
void DBGDrivSetFrame(struct DBG_STREAM *psStream,
				      u32 ui32Frame);
u32 DBGDrivGetFrame(struct DBG_STREAM *psStream);
void DBGDrivOverrideMode(struct DBG_STREAM *psStream,
					  u32 ui32Mode);
void DBGDrivDefaultMode(struct DBG_STREAM *psStream);
u32 DBGDrivGetServiceTable(void);
u32 DBGDrivWriteStringCM(struct DBG_STREAM *psStream,
					     char *pszString,
					     u32 ui32Level);
u32 DBGDrivWriteCM(struct DBG_STREAM *psStream,
				       u8 *pui8InBuf,
				       u32 ui32InBuffSize,
				       u32 ui32Level);
void DBGDrivSetMarker(struct DBG_STREAM *psStream,
				       u32 ui32Marker);
u32 DBGDrivGetMarker(struct DBG_STREAM *psStream);
u32 DBGDrivIsLastCaptureFrame(struct DBG_STREAM *psStream);
u32 DBGDrivIsCaptureFrame(struct DBG_STREAM *psStream,
					      IMG_BOOL bCheckPreviousFrame);
u32 DBGDrivWriteLF(struct DBG_STREAM *psStream,
				       u8 *pui8InBuf,
				       u32 ui32InBuffSize,
				       u32 ui32Level,
				       u32 ui32Flags);
u32 DBGDrivReadLF(struct DBG_STREAM *psStream,
				      u32 ui32OutBuffSize,
				      u8 *pui8OutBuf);
void DBGDrivStartInitPhase(struct DBG_STREAM *psStream);
void DBGDrivStopInitPhase(struct DBG_STREAM *psStream);
u32 DBGDrivGetStreamOffset(struct DBG_STREAM *psStream);
void DBGDrivSetStreamOffset(struct DBG_STREAM *psStream, u32 ui32StreamOffset);
void DBGDrivWaitForEvent(enum DBG_EVENT eEvent);

void DestroyAllStreams(void);

u32 AtoI(char *szIn);

void HostMemSet(void *pvDest, u8 ui8Value, u32 ui32Size);
void HostMemCopy(void *pvDest, void *pvSrc, u32 ui32Size);
IMG_BOOL StreamValid(struct DBG_STREAM *psStream);
void Write(struct DBG_STREAM *psStream, u8 *pui8Data,
	       u32 ui32InBuffSize);
void MonoOut(char *pszString, IMG_BOOL bNewLine);

void *ExtDBGDrivCreateStream(char *pszName,
					      u32 ui32CapMode,
					      u32 ui32OutMode,
					      u32 ui32Flags,
					      u32 ui32Size);
void ExtDBGDrivDestroyStream(struct DBG_STREAM *psStream);
void *ExtDBGDrivFindStream(char *pszName,
					    IMG_BOOL bResetStream);
u32 ExtDBGDrivWriteString(struct DBG_STREAM *psStream,
					      char *pszString,
					      u32 ui32Level);
u32 ExtDBGDrivReadString(struct DBG_STREAM *psStream,
					     char *pszString,
					     u32 ui32Limit);
u32 ExtDBGDrivWrite(struct DBG_STREAM *psStream,
					u8 *pui8InBuf,
					u32 ui32InBuffSize,
					u32 ui32Level);
u32 ExtDBGDrivRead(struct DBG_STREAM *psStream,
				       IMG_BOOL bReadInitBuffer,
				       u32 ui32OutBuffSize,
				       u8 *pui8OutBuf);
void ExtDBGDrivSetCaptureMode(struct DBG_STREAM *psStream,
					       u32 ui32Mode,
					       u32 ui32Start,
					       u32 ui32End,
					       u32 ui32SampleRate);
void ExtDBGDrivSetOutputMode(struct DBG_STREAM *psStream,
					      u32 ui32OutMode);
void ExtDBGDrivSetDebugLevel(struct DBG_STREAM *psStream,
					      u32 ui32DebugLevel);
void ExtDBGDrivSetFrame(struct DBG_STREAM *psStream,
					 u32 ui32Frame);
u32 ExtDBGDrivGetFrame(struct DBG_STREAM *psStream);
void ExtDBGDrivOverrideMode(struct DBG_STREAM *psStream,
					     u32 ui32Mode);
void ExtDBGDrivDefaultMode(struct DBG_STREAM *psStream);
u32 ExtDBGDrivWrite2(struct DBG_STREAM *psStream, u8 *pui8InBuf,
		     u32 ui32InBuffSize, u32 ui32Level);
u32 ExtDBGDrivWriteStringCM(struct DBG_STREAM *psStream, char *pszString,
					  u32 ui32Level);
u32 ExtDBGDrivWriteCM(struct DBG_STREAM *psStream, u8 *pui8InBuf,
		      u32 ui32InBuffSize, u32 ui32Level);
void ExtDBGDrivSetMarker(struct DBG_STREAM *psStream, u32 ui32Marker);
u32 ExtDBGDrivGetMarker(struct DBG_STREAM *psStream);
void ExtDBGDrivStartInitPhase(struct DBG_STREAM *psStream);
void ExtDBGDrivStopInitPhase(struct DBG_STREAM *psStream);
u32 ExtDBGDrivIsLastCaptureFrame(struct DBG_STREAM *psStream);
u32 ExtDBGDrivIsCaptureFrame(struct DBG_STREAM *psStream,
						 IMG_BOOL bCheckPreviousFrame);
u32 ExtDBGDrivWriteLF(struct DBG_STREAM *psStream,
					  u8 *pui8InBuf,
					  u32 ui32InBuffSize,
					  u32 ui32Level,
					  u32 ui32Flags);
u32 ExtDBGDrivReadLF(struct DBG_STREAM *psStream,
					 u32 ui32OutBuffSize,
					 u8 *pui8OutBuf);
u32 ExtDBGDrivGetStreamOffset(struct DBG_STREAM *psStream);
void ExtDBGDrivSetStreamOffset(struct DBG_STREAM *psStream,
						u32 ui32StreamOffset);
void ExtDBGDrivWaitForEvent(enum DBG_EVENT eEvent);

#endif
