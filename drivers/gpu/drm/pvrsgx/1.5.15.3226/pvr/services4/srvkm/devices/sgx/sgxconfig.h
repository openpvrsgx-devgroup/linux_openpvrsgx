/*************************************************************************/ /*!
@Title          device configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#ifndef __SGXCONFIG_H__
#define __SGXCONFIG_H__

#include "sgxdefs.h"

#define DEV_DEVICE_TYPE			PVRSRV_DEVICE_TYPE_SGX
#define DEV_DEVICE_CLASS		PVRSRV_DEVICE_CLASS_3D

#define DEV_MAJOR_VERSION		1
#define DEV_MINOR_VERSION		0

#if SGX_FEATURE_ADDRESS_SPACE_SIZE == 32
#if 0 /* ORIGIAL_SGX_MAP */
	#if defined(SGX_FEATURE_2D_HARDWARE)
	#define SGX_2D_HEAP_BASE					 0x00100000
	#define SGX_2D_HEAP_SIZE					(0x08000000-0x00100000-0x00001000)
	#else
		#if defined(FIX_HW_BRN_26915)
		#define SGX_CGBUFFER_HEAP_BASE					 0x00100000
		#define SGX_CGBUFFER_HEAP_SIZE					(0x08000000-0x00100000-0x00001000)
		#endif
	#endif

	#if defined(SUPPORT_SGX_GENERAL_MAPPING_HEAP)
	#define SGX_GENERAL_MAPPING_HEAP_BASE		 0x08000000
	#define SGX_GENERAL_MAPPING_HEAP_SIZE		(0x08000000-0x00001000)
	#endif

	#define SGX_GENERAL_HEAP_BASE				 0x10000000
	#define SGX_GENERAL_HEAP_SIZE				(0xC8000000-0x00001000)
#else /* EMGD Mapping */
	/* Leave 0 - 0x10000000 (256MB) untouched */
	#define SGX_2D_HEAP_BASE					 0x10000000
	#define SGX_2D_HEAP_SIZE					(0x08000000-0x00001000)

#if defined(SUPPORT_SGX_VIDEO_HEAP)

	#if defined(SUPPORT_SGX_GENERAL_MAPPING_HEAP)
	#define SGX_GENERAL_MAPPING_HEAP_BASE		 0x18000000
	#define SGX_GENERAL_MAPPING_HEAP_SIZE		(0x30000000-0x00001000)
	#endif

	#define SGX_VIDEO_HEAP_BASE					 0x48000000
	#define SGX_VIDEO_HEAP_SIZE					(0x18000000-0x00001000)

	#define SGX_GENERAL_HEAP_BASE				 0x60000000
	#define SGX_GENERAL_HEAP_SIZE				(0x78000000-0x00001000)

#else

	#if defined(SUPPORT_SGX_GENERAL_MAPPING_HEAP)
	#define SGX_GENERAL_MAPPING_HEAP_BASE		 0x18000000
	#define SGX_GENERAL_MAPPING_HEAP_SIZE		(0x08000000-0x00001000)
	#endif

	#define SGX_GENERAL_HEAP_BASE				 0x20000000
	#define SGX_GENERAL_HEAP_SIZE				(0xB8000000-0x00001000)

#endif

#endif
	#define SGX_3DPARAMETERS_HEAP_BASE			 0xD8000000
	#define SGX_3DPARAMETERS_HEAP_SIZE			(0x10000000-0x00001000)

	#define SGX_TADATA_HEAP_BASE				 0xE8000000
	#define SGX_TADATA_HEAP_SIZE				(0x0D000000-0x00001000)

	#define SGX_SYNCINFO_HEAP_BASE				 0xF5000000
	#define SGX_SYNCINFO_HEAP_SIZE				(0x01000000-0x00001000)

	#define SGX_PDSPIXEL_CODEDATA_HEAP_BASE		 0xF6000000
	#define SGX_PDSPIXEL_CODEDATA_HEAP_SIZE		(0x02000000-0x00001000)

	#define SGX_KERNEL_CODE_HEAP_BASE			 0xF8000000
	#define SGX_KERNEL_CODE_HEAP_SIZE			(0x00080000-0x00001000)

	#define SGX_PDSVERTEX_CODEDATA_HEAP_BASE	 0xF8400000
	#define SGX_PDSVERTEX_CODEDATA_HEAP_SIZE	(0x01C00000-0x00001000)

	#define SGX_KERNEL_DATA_HEAP_BASE		 	 0xFA000000
	#define SGX_KERNEL_DATA_HEAP_SIZE			(0x05000000-0x00001000)

	#define SGX_PIXELSHADER_HEAP_BASE			 0xFF000000
	#define SGX_PIXELSHADER_HEAP_SIZE			(0x00500000-0x00001000)

	#define SGX_VERTEXSHADER_HEAP_BASE			 0xFF800000
	#define SGX_VERTEXSHADER_HEAP_SIZE			(0x00200000-0x00001000)


	#define SGX_CORE_IDENTIFIED
#endif

#if SGX_FEATURE_ADDRESS_SPACE_SIZE == 28
	#if defined(SUPPORT_SGX_GENERAL_MAPPING_HEAP)
	#define SGX_GENERAL_MAPPING_HEAP_BASE		 0x00001000
	#define SGX_GENERAL_MAPPING_HEAP_SIZE		(0x01800000-0x00001000-0x00001000)
	#endif

	#define SGX_GENERAL_HEAP_BASE				 0x01800000
	#define SGX_GENERAL_HEAP_SIZE				(0x07000000-0x00001000)

	#define SGX_3DPARAMETERS_HEAP_BASE			 0x08800000
	#define SGX_3DPARAMETERS_HEAP_SIZE			(0x04000000-0x00001000)

	#define SGX_TADATA_HEAP_BASE				 0x0C800000
	#define SGX_TADATA_HEAP_SIZE				(0x01000000-0x00001000)

	#define SGX_SYNCINFO_HEAP_BASE				 0x0D800000
	#define SGX_SYNCINFO_HEAP_SIZE				(0x00400000-0x00001000)

	#define SGX_PDSPIXEL_CODEDATA_HEAP_BASE		 0x0DC00000
	#define SGX_PDSPIXEL_CODEDATA_HEAP_SIZE		(0x00800000-0x00001000)

	#define SGX_KERNEL_CODE_HEAP_BASE			 0x0E400000
	#define SGX_KERNEL_CODE_HEAP_SIZE			(0x00080000-0x00001000)

	#define SGX_PDSVERTEX_CODEDATA_HEAP_BASE	 0x0E800000
	#define SGX_PDSVERTEX_CODEDATA_HEAP_SIZE	(0x00800000-0x00001000)

	#define SGX_KERNEL_DATA_HEAP_BASE			 0x0F000000
	#define SGX_KERNEL_DATA_HEAP_SIZE			(0x00400000-0x00001000)

	#define SGX_PIXELSHADER_HEAP_BASE			 0x0F400000
	#define SGX_PIXELSHADER_HEAP_SIZE			(0x00500000-0x00001000)

	#define SGX_VERTEXSHADER_HEAP_BASE			 0x0FC00000
	#define SGX_VERTEXSHADER_HEAP_SIZE			(0x00200000-0x00001000)

	/* signal we've identified the core by the build */
	#define SGX_CORE_IDENTIFIED

#endif /* SGX_FEATURE_ADDRESS_SPACE_SIZE == 28 */

#if !defined(SGX_CORE_IDENTIFIED)
	#error "sgxconfig.h: ERROR: unspecified SGX Core version"
#endif

#endif /* __SGXCONFIG_H__ */

/*****************************************************************************
 End of file (sgxconfig.h)
*****************************************************************************/
