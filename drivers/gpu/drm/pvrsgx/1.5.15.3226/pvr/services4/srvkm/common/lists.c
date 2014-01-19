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

#include "lists.h"
#include "services_headers.h"

IMPLEMENT_LIST_ANY_VA(BM_HEAP)
IMPLEMENT_LIST_ANY_2(BM_HEAP, PVRSRV_ERROR, PVRSRV_OK)
IMPLEMENT_LIST_ANY_VA_2(BM_HEAP, PVRSRV_ERROR, PVRSRV_OK)
IMPLEMENT_LIST_FOR_EACH_VA(BM_HEAP)
IMPLEMENT_LIST_REMOVE(BM_HEAP)
IMPLEMENT_LIST_INSERT(BM_HEAP)

IMPLEMENT_LIST_ANY_VA(BM_CONTEXT)
IMPLEMENT_LIST_ANY_VA_2(BM_CONTEXT, IMG_HANDLE, IMG_NULL)
IMPLEMENT_LIST_ANY_VA_2(BM_CONTEXT, PVRSRV_ERROR, PVRSRV_OK)
IMPLEMENT_LIST_FOR_EACH(BM_CONTEXT)
IMPLEMENT_LIST_REMOVE(BM_CONTEXT)
IMPLEMENT_LIST_INSERT(BM_CONTEXT)

IMPLEMENT_LIST_ANY_2(PVRSRV_DEVICE_NODE, PVRSRV_ERROR, PVRSRV_OK)
IMPLEMENT_LIST_ANY_VA(PVRSRV_DEVICE_NODE)
IMPLEMENT_LIST_ANY_VA_2(PVRSRV_DEVICE_NODE, PVRSRV_ERROR, PVRSRV_OK)
IMPLEMENT_LIST_FOR_EACH(PVRSRV_DEVICE_NODE)
IMPLEMENT_LIST_FOR_EACH_VA(PVRSRV_DEVICE_NODE)
IMPLEMENT_LIST_INSERT(PVRSRV_DEVICE_NODE)
IMPLEMENT_LIST_REMOVE(PVRSRV_DEVICE_NODE)

IMPLEMENT_LIST_ANY_VA(PVRSRV_POWER_DEV)
IMPLEMENT_LIST_ANY_VA_2(PVRSRV_POWER_DEV, PVRSRV_ERROR, PVRSRV_OK)
IMPLEMENT_LIST_INSERT(PVRSRV_POWER_DEV)
IMPLEMENT_LIST_REMOVE(PVRSRV_POWER_DEV)


IMG_VOID* MatchDeviceKM_AnyVaCb(PVRSRV_DEVICE_NODE* psDeviceNode, va_list va)
{
	IMG_UINT32 ui32DevIndex;
	IMG_BOOL bIgnoreClass;
	PVRSRV_DEVICE_CLASS eDevClass;

	ui32DevIndex = va_arg(va, IMG_UINT32);
	bIgnoreClass = va_arg(va, IMG_BOOL);
	if (!bIgnoreClass)
	{
		eDevClass = va_arg(va, PVRSRV_DEVICE_CLASS);
	}
	else
	{


		eDevClass = PVRSRV_DEVICE_CLASS_FORCE_I32;
	}

	if ((bIgnoreClass || psDeviceNode->sDevId.eDeviceClass == eDevClass) &&
		psDeviceNode->sDevId.ui32DeviceIndex == ui32DevIndex)
	{
		return psDeviceNode;
	}
	return IMG_NULL;
}

IMG_VOID* MatchPowerDeviceIndex_AnyVaCb(PVRSRV_POWER_DEV *psPowerDev, va_list va)
{
	IMG_UINT32 ui32DeviceIndex;

	ui32DeviceIndex = va_arg(va, IMG_UINT32);

	if (psPowerDev->ui32DeviceIndex == ui32DeviceIndex)
	{
		return psPowerDev;
	}
	else
	{
		return IMG_NULL;
	}
}
