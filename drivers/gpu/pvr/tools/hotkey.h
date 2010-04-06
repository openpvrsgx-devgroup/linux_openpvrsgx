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

#ifndef _HOTKEY_
#define _HOTKEY_

struct HOTKEYINFO {
	u8 ui8ScanCode;
	u8 ui8Type;
	u8 ui8Flag;
	u8 ui8Filler1;
	u32 ui32ShiftState;
	u32 ui32HotKeyProc;
	void *pvStream;
	u32 hHotKey;
};

struct PRIVATEHOTKEYDATA {
	u32 ui32ScanCode;
	u32 ui32ShiftState;
	struct HOTKEYINFO sHotKeyInfo;
};

extern u32 g_ui32HotKeyFrame;
extern IMG_BOOL g_bHotKeyPressed;
extern IMG_BOOL g_bHotKeyRegistered;

void ReadInHotKeys(void);
void ActivateHotKeys(struct DBG_STREAM *psStream);
void DeactivateHotKeys(void);

void RemoveHotKey(u32 hHotKey);
void DefineHotKey(u32 ui32ScanCode, u32 ui32ShiftState,
		      struct HOTKEYINFO *psInfo);
void RegisterKeyPressed(u32 ui32ScanCode, struct HOTKEYINFO *psInfo);

#endif
