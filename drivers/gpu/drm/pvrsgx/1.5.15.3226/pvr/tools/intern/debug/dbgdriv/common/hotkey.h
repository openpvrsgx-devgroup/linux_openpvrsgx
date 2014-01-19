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

#ifndef _HOTKEY_
#define _HOTKEY_


typedef struct _hotkeyinfo
{
	IMG_UINT8 ui8ScanCode;
	IMG_UINT8 ui8Type;
	IMG_UINT8 ui8Flag;
	IMG_UINT8 ui8Filler1;
	IMG_UINT32 ui32ShiftState;
	IMG_UINT32 ui32HotKeyProc;
	IMG_VOID *pvStream;
	IMG_UINT32 hHotKey;
} HOTKEYINFO, *PHOTKEYINFO;

typedef struct _privatehotkeydata
{
	IMG_UINT32		ui32ScanCode;
	IMG_UINT32		ui32ShiftState;
	HOTKEYINFO	sHotKeyInfo;
} PRIVATEHOTKEYDATA, *PPRIVATEHOTKEYDATA;


IMG_VOID ReadInHotKeys (IMG_VOID);
IMG_VOID ActivateHotKeys(PDBG_STREAM psStream);
IMG_VOID DeactivateHotKeys(IMG_VOID);

IMG_VOID RemoveHotKey (IMG_UINT32 hHotKey);
IMG_VOID DefineHotKey (IMG_UINT32 ui32ScanCode, IMG_UINT32 ui32ShiftState, PHOTKEYINFO psInfo);
IMG_VOID RegisterKeyPressed (IMG_UINT32 ui32ScanCode, PHOTKEYINFO psInfo);

#endif

