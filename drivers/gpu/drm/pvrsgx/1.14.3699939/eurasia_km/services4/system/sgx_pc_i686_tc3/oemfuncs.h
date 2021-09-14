/*************************************************************************/ /*!
@Title          SGX kernel/client driver interface structures and prototypes
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Exported services API details
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#if !defined(__OEMFUNCS_H__)
#define __OEMFUNCS_H__

#if defined (__cplusplus)
extern "C" {
#endif

/* function identifiers: */
#define OEM_EXCHANGE_POWER_STATE	(1<<0)
#define OEM_DEVICE_MEMORY_POWER		(1<<1)
#define OEM_DISPLAY_POWER			(1<<2)
#define OEM_GET_EXT_FUNCS			(1<<3)

typedef struct OEM_ACCESS_INFO_TAG
{
	IMG_UINT32		ui32Size;
	IMG_UINT32  	ui32FBPhysBaseAddress;
	IMG_UINT32		ui32FBMemAvailable;		/* size of usable FB memory */
	IMG_UINT32  	ui32SysPhysBaseAddress;
	IMG_UINT32		ui32SysSize;
	IMG_UINT32		ui32DevIRQ;
} OEM_ACCESS_INFO, *POEM_ACCESS_INFO; 
 
/* function in/out data structures: */
typedef IMG_UINT32   (*PFN_SRV_BRIDGEDISPATCH)( IMG_UINT32  Ioctl,
												IMG_BYTE   *pInBuf,
												IMG_UINT32  InBufLen, 
											    IMG_BYTE   *pOutBuf,
												IMG_UINT32  OutBufLen,
												IMG_UINT32 *pdwBytesTransferred);


typedef PVRSRV_ERROR (*PFN_SRV_READREGSTRING)(PPVRSRV_REGISTRY_INFO psRegInfo);

/*
	Function table for kernel 3rd party driver to kernel services
*/
typedef struct PVRSRV_DC_OEM_JTABLE_TAG
{
	PFN_SRV_BRIDGEDISPATCH			pfnOEMBridgeDispatch;
	PFN_SRV_READREGSTRING			pfnOEMReadRegistryString;
	PFN_SRV_READREGSTRING			pfnOEMWriteRegistryString;

} PVRSRV_DC_OEM_JTABLE;
#if defined(__cplusplus)
}
#endif

#endif	/* __OEMFUNCS_H__ */

/*****************************************************************************
 End of file (oemfuncs.h)
*****************************************************************************/


