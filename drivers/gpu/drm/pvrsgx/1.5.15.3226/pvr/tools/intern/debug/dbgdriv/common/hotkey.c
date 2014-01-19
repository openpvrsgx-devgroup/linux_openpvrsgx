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


#if !defined(LINUX)
#include <ntddk.h>
#include <windef.h>
#endif

#include "img_types.h"
#include "pvr_debug.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hotkey.h"
#include "hostfunc.h"





IMG_UINT32	g_ui32HotKeyFrame = 0xFFFFFFFF;
IMG_BOOL	g_bHotKeyPressed = IMG_FALSE;
IMG_BOOL	g_bHotKeyRegistered = IMG_FALSE;

PRIVATEHOTKEYDATA    g_PrivateHotKeyData;


IMG_VOID ReadInHotKeys(IMG_VOID)
{
	g_PrivateHotKeyData.ui32ScanCode = 0x58;
	g_PrivateHotKeyData.ui32ShiftState = 0x0;



#if 0
	if (_RegOpenKey(HKEY_LOCAL_MACHINE,pszRegPath,&hKey) == ERROR_SUCCESS)
	{


		QueryReg(hKey,"ui32ScanCode",&g_PrivateHotKeyData.ui32ScanCode);
		QueryReg(hKey,"ui32ShiftState",&g_PrivateHotKeyData.ui32ShiftState);
	}
#else
	HostReadRegistryDWORDFromString("DEBUG\\Streams", "ui32ScanCode"  , &g_PrivateHotKeyData.ui32ScanCode);
	HostReadRegistryDWORDFromString("DEBUG\\Streams", "ui32ShiftState", &g_PrivateHotKeyData.ui32ShiftState);
#endif
}

IMG_VOID RegisterKeyPressed(IMG_UINT32 dwui32ScanCode, PHOTKEYINFO pInfo)
{
	PDBG_STREAM	psStream;

	PVR_UNREFERENCED_PARAMETER(pInfo);

	if (dwui32ScanCode == g_PrivateHotKeyData.ui32ScanCode)
	{
		PVR_DPF((PVR_DBG_MESSAGE,"PDUMP Hotkey pressed !\n"));

		psStream = (PDBG_STREAM) g_PrivateHotKeyData.sHotKeyInfo.pvStream;

		if (!g_bHotKeyPressed)
		{


			g_ui32HotKeyFrame = psStream->ui32Current + 2;



			g_bHotKeyPressed = IMG_TRUE;
		}
	}
}

IMG_VOID ActivateHotKeys(PDBG_STREAM psStream)
{


	ReadInHotKeys();



	if (!g_PrivateHotKeyData.sHotKeyInfo.hHotKey)
	{
		if (g_PrivateHotKeyData.ui32ScanCode != 0)
		{
			PVR_DPF((PVR_DBG_MESSAGE,"Activate HotKey for PDUMP.\n"));



			g_PrivateHotKeyData.sHotKeyInfo.pvStream = psStream;

			DefineHotKey(g_PrivateHotKeyData.ui32ScanCode, g_PrivateHotKeyData.ui32ShiftState, &g_PrivateHotKeyData.sHotKeyInfo);
		}
		else
		{
			g_PrivateHotKeyData.sHotKeyInfo.hHotKey = 0;
		}
	}
}

IMG_VOID DeactivateHotKeys(IMG_VOID)
{
	if (g_PrivateHotKeyData.sHotKeyInfo.hHotKey != 0)
	{
		PVR_DPF((PVR_DBG_MESSAGE,"Deactivate HotKey.\n"));

		RemoveHotKey(g_PrivateHotKeyData.sHotKeyInfo.hHotKey);
		g_PrivateHotKeyData.sHotKeyInfo.hHotKey = 0;
	}
}


