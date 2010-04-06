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

#ifndef _DBGDRVIF_
#define _DBGDRVIF_

#include "ioctldef.h"

#define DEBUG_CAPMODE_FRAMED			0x00000001
#define DEBUG_CAPMODE_CONTINUOUS		0x00000002
#define DEBUG_CAPMODE_HOTKEY			0x00000004

#define DEBUG_OUTMODE_STANDARDDBG		0x00000001
#define DEBUG_OUTMODE_MONO			0x00000002
#define DEBUG_OUTMODE_STREAMENABLE		0x00000004
#define DEBUG_OUTMODE_ASYNC			0x00000008
#define DEBUG_OUTMODE_SGXVGA			0x00000010

#define DEBUG_FLAGS_USE_NONPAGED_MEM		0x00000001
#define DEBUG_FLAGS_NO_BUF_EXPANDSION		0x00000002
#define DEBUG_FLAGS_ENABLESAMPLE		0x00000004

#define DEBUG_FLAGS_TEXTSTREAM			0x80000000

#define DEBUG_LEVEL_0				0x00000001
#define DEBUG_LEVEL_1				0x00000003
#define DEBUG_LEVEL_2				0x00000007
#define DEBUG_LEVEL_3				0x0000000F
#define DEBUG_LEVEL_4				0x0000001F
#define DEBUG_LEVEL_5				0x0000003F
#define DEBUG_LEVEL_6				0x0000007F
#define DEBUG_LEVEL_7				0x000000FF
#define DEBUG_LEVEL_8				0x000001FF
#define DEBUG_LEVEL_9				0x000003FF
#define DEBUG_LEVEL_10				0x000007FF
#define DEBUG_LEVEL_11				0x00000FFF

#define DEBUG_LEVEL_SEL0			0x00000001
#define DEBUG_LEVEL_SEL1			0x00000002
#define DEBUG_LEVEL_SEL2			0x00000004
#define DEBUG_LEVEL_SEL3			0x00000008
#define DEBUG_LEVEL_SEL4			0x00000010
#define DEBUG_LEVEL_SEL5			0x00000020
#define DEBUG_LEVEL_SEL6			0x00000040
#define DEBUG_LEVEL_SEL7			0x00000080
#define DEBUG_LEVEL_SEL8			0x00000100
#define DEBUG_LEVEL_SEL9			0x00000200
#define DEBUG_LEVEL_SEL10			0x00000400
#define DEBUG_LEVEL_SEL11			0x00000800

#define DEBUG_SERVICE_IOCTL_BASE		0x800
#define DEBUG_SERVICE_CREATESTREAM					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x01,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_DESTROYSTREAM					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x02,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_GETSTREAM						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x03,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_WRITESTRING					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x04,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_READSTRING					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x05,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_WRITE						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x06,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_READ						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x07,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_SETDEBUGMODE					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x08,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_SETDEBUGOUTMODE					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x09,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_SETDEBUGLEVEL					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x0A,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_SETFRAME						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x0B,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_GETFRAME						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x0C,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_OVERRIDEMODE					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x0D,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_DEFAULTMODE					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x0E,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_GETSERVICETABLE					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x0F,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_WRITE2						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x10,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_WRITESTRINGCM					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x11,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_WRITECM						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x12,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_SETMARKER						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x13,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_GETMARKER						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x14,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_ISCAPTUREFRAME					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x15,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_WRITELF						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x16,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_READLF						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x17,	\
					METHOD_BUFFERED, FILE_ANY_ACCESS)
#define DEBUG_SERVICE_WAITFOREVENT					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, DEBUG_SERVICE_IOCTL_BASE + 0x18,	\
		 METHOD_BUFFERED, FILE_ANY_ACCESS)

enum DBG_EVENT {
	DBG_EVENT_STREAM_DATA = 1
};

struct DBG_IN_CREATESTREAM {
	u32 ui32Pages;
	u32 ui32CapMode;
	u32 ui32OutMode;
	char *pszName;
};

struct DBG_IN_FINDSTREAM {
	IMG_BOOL bResetStream;
	char *pszName;
};

struct DBG_IN_WRITESTRING {
	void *pvStream;
	u32 ui32Level;
	char *pszString;
};

struct DBG_IN_READSTRING {
	void *pvStream;
	u32 ui32StringLen;
	char *pszString;
};

struct DBG_IN_SETDEBUGMODE {
	void *pvStream;
	u32 ui32Mode;
	u32 ui32Start;
	u32 ui32End;
	u32 ui32SampleRate;
};

struct DBG_IN_SETDEBUGOUTMODE {
	void *pvStream;
	u32 ui32Mode;
};

struct DBG_IN_SETDEBUGLEVEL {
	void *pvStream;
	u32 ui32Level;
};

struct DBG_IN_SETFRAME {
	void *pvStream;
	u32 ui32Frame;
};

struct DBG_IN_WRITE {
	void *pvStream;
	u32 ui32Level;
	u32 ui32TransferSize;
	u8 *pui8InBuffer;
};

struct DBG_IN_READ {
	void *pvStream;
	IMG_BOOL bReadInitBuffer;
	u32 ui32OutBufferSize;
	u8 *pui8OutBuffer;
};

struct DBG_IN_OVERRIDEMODE {
	void *pvStream;
	u32 ui32Mode;
};

struct DBG_IN_ISCAPTUREFRAME {
	void *pvStream;
	IMG_BOOL bCheckPreviousFrame;
};

struct DBG_IN_SETMARKER {
	void *pvStream;
	u32 ui32Marker;
};

struct DBG_IN_WRITE_LF {
	u32 ui32Flags;
	void *pvStream;
	u32 ui32Level;
	u32 ui32BufferSize;
	u8 *pui8InBuffer;
};

#define WRITELF_FLAGS_RESETBUF		0x00000001

struct DBG_STREAM {
	struct DBG_STREAM *psNext;
	struct DBG_STREAM *psInitStream;
	IMG_BOOL bInitPhaseComplete;
	u32 ui32Flags;
	u32 ui32Base;
	u32 ui32Size;
	u32 ui32RPtr;
	u32 ui32WPtr;
	u32 ui32DataWritten;
	u32 ui32CapMode;
	u32 ui32OutMode;
	u32 ui32DebugLevel;
	u32 ui32DefaultMode;
	u32 ui32Start;
	u32 ui32End;
	u32 ui32Current;
	u32 ui32Access;
	u32 ui32SampleRate;
	u32 ui32Reserved;
	u32 ui32Timeout;
	u32 ui32Marker;
	char szName[30];
};

struct DBGKM_SERVICE_TABLE {
	u32 ui32Size;
	void *(*pfnCreateStream)(char *pszName, u32 ui32CapMode,
			u32 ui32OutMode, u32 ui32Flags, u32 ui32Pages);
	void  (*pfnDestroyStream)(struct DBG_STREAM *psStream);
	void *(*pfnFindStream)(char *pszName, IMG_BOOL bResetInitBuffer);
	u32   (*pfnWriteString)(struct DBG_STREAM *psStream, char *pszString,
			u32 ui32Level);
	u32   (*pfnReadString)(struct DBG_STREAM *psStream, char *pszString,
			u32 ui32Limit);
	u32   (*pfnWriteBIN)(struct DBG_STREAM *psStream, u8 *pui8InBuf,
			u32 ui32InBuffSize, u32 ui32Level);
	u32   (*pfnReadBIN)(struct DBG_STREAM *psStream,
			IMG_BOOL bReadInitBuffer, u32 ui32OutBufferSize,
			u8 *pui8OutBuf);
	void  (*pfnSetCaptureMode)(struct DBG_STREAM *psStream,
			u32 ui32CapMode, u32 ui32Start, u32 ui32Stop,
			u32 ui32SampleRate);
	void  (*pfnSetOutputMode)(struct DBG_STREAM *psStream,
			u32 ui32OutMode);
	void  (*pfnSetDebugLevel)(struct DBG_STREAM *psStream,
			u32 ui32DebugLevel);
	void  (*pfnSetFrame)(struct DBG_STREAM *psStream,
			u32 ui32Frame);
	u32   (*pfnGetFrame)(struct DBG_STREAM *psStream);
	void  (*pfnOverrideMode)(struct DBG_STREAM *psStream,
			u32 ui32Mode);
	void  (*pfnDefaultMode)(struct DBG_STREAM *psStream);
	u32   (*pfnDBGDrivWrite2)(struct DBG_STREAM *psStream,
			u8 *pui8InBuf, u32 ui32InBuffSize, u32 ui32Level);
	u32   (*pfnWriteStringCM)(struct DBG_STREAM *psStream, char *pszString,
			u32 ui32Level);
	u32   (*pfnWriteBINCM)(struct DBG_STREAM *psStream, u8 *pui8InBuf,
			u32 ui32InBuffSize, u32 ui32Level);
	void  (*pfnSetMarker)(struct DBG_STREAM *psStream, u32 ui32Marker);
	u32   (*pfnGetMarker)(struct DBG_STREAM *psStream);
	void (*pfnStartInitPhase) (struct DBG_STREAM *psStream);
	void (*pfnStopInitPhase) (struct DBG_STREAM *psStream);
	u32   (*pfnIsCaptureFrame)(struct DBG_STREAM *psStream,
			IMG_BOOL bCheckPreviousFrame);
	u32   (*pfnWriteLF)(struct DBG_STREAM *psStream, u8 *pui8InBuf,
			u32 ui32InBuffSize, u32 ui32Level, u32 ui32Flags);
	u32   (*pfnReadLF)(struct DBG_STREAM *psStream, u32 ui32OutBuffSize,
			u8 *pui8OutBuf);
	u32   (*pfnGetStreamOffset)(struct DBG_STREAM *psStream);
	void  (*pfnSetStreamOffset)(struct DBG_STREAM *psStream,
			u32 ui32StreamOffset);
	u32   (*pfnIsLastCaptureFrame)(struct DBG_STREAM *psStream);
	void (*pfnWaitForEvent) (enum DBG_EVENT eEvent);
};

extern struct DBGKM_SERVICE_TABLE g_sDBGKMServices;

void DBGDrvGetServiceTable(void **fn_table);

#endif
