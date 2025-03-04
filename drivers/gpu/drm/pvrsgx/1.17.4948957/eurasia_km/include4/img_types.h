/*************************************************************************/ /*!
@Title          Global types for use by IMG APIs
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Defines type aliases for use by IMG APIs.
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
#ifndef __IMG_TYPES_H__
#define __IMG_TYPES_H__

#if   defined(LINUX) && defined(__KERNEL__)
	#include "kernel_types.h"
#elif defined(LINUX) || defined(__QNXNTO__) || defined(__METAG)
	#include <stddef.h>			/* NULL */
	#include <stdint.h>			/* intX_t/uintX_t, format specifiers */
	#include <limits.h>			/* INT_MIN, etc */
#else
	#error C99 support not set up for this build
#endif

/* define all address space bit depths: */
/* CPU virtual address space defaults to 32bits */
#if !defined(IMG_ADDRSPACE_CPUVADDR_BITS)
#define IMG_ADDRSPACE_CPUVADDR_BITS		32
#endif

/* Physical address space defaults to 32bits */
#if !defined(IMG_ADDRSPACE_PHYSADDR_BITS)
#define IMG_ADDRSPACE_PHYSADDR_BITS		32
#endif

typedef unsigned int	IMG_UINT,	*IMG_PUINT;
typedef signed int	IMG_INT,	*IMG_PINT;

typedef uint8_t		IMG_UINT8,	*IMG_PUINT8;
typedef uint8_t		IMG_BYTE,	*IMG_PBYTE;
typedef int8_t		IMG_INT8,	*IMG_PINT8;
typedef char		IMG_CHAR,	*IMG_PCHAR;

typedef uint16_t	IMG_UINT16,	*IMG_PUINT16;
typedef int16_t		IMG_INT16,	*IMG_PINT16;
typedef uint32_t	IMG_UINT32,	*IMG_PUINT32;
typedef int32_t		IMG_INT32,	*IMG_PINT32;

#define IMG_UINT32_MAX  UINT32_MAX

typedef uint64_t	IMG_UINT64, *IMG_PUINT64;
typedef int64_t		IMG_INT64,  *IMG_PINT64;

#if !(defined(LINUX) && defined (__KERNEL__))
/* Linux kernel mode does not use floating point */
typedef float		IMG_FLOAT,	*IMG_PFLOAT;
typedef double		IMG_DOUBLE, *IMG_PDOUBLE;
#endif

typedef	enum tag_img_bool
{
	IMG_FALSE		= 0,
	IMG_TRUE		= 1,
	IMG_FORCE_ALIGN = 0x7FFFFFFF
} IMG_BOOL, *IMG_PBOOL;

typedef void            IMG_VOID, *IMG_PVOID;

typedef IMG_INT32       IMG_RESULT;

#if defined(_WIN64)
	typedef unsigned __int64	IMG_UINTPTR_T;
	typedef signed   __int64    IMG_INTPTR_T;
	typedef signed __int64		IMG_PTRDIFF_T;
	typedef IMG_UINT64			IMG_SIZE_T;
#else
#if defined (__x86_64__) || !defined(USE_64BIT_COMPAT) || defined (__KERNEL__)
	typedef size_t		IMG_SIZE_T;
#else
	typedef IMG_UINT64		IMG_SIZE_T;
#endif
	typedef uintptr_t   	IMG_UINTPTR_T;
#endif

#if defined(LINUX) && defined (__KERNEL__)
	typedef IMG_PVOID       IMG_HANDLE;
#elif defined(LINUX)
	#if defined(USE_64BIT_COMPAT)
		typedef IMG_UINT64	IMG_HANDLE;
	#else
		typedef IMG_UINT32	IMG_HANDLE;
	#endif
#else
typedef void           *IMG_HANDLE;
#endif

typedef void**          IMG_HVOID,	* IMG_PHVOID;

#define IMG_NULL        0 

/* services/stream ID */
typedef IMG_UINTPTR_T	IMG_SID;

typedef IMG_UINTPTR_T	IMG_EVENTSID;

/*
 * Address types.
 * All types used to refer to a block of memory are wrapped in structures
 * to enforce some degree of type safety, i.e. a IMG_DEV_VIRTADDR cannot
 * be assigned to a variable of type IMG_DEV_PHYADDR because they are not the
 * same thing.
 *
 * There is an assumption that the system contains at most one non-cpu mmu,
 * and a memory block is only mapped by the MMU once.
 *
 * Different devices could have offset views of the physical address space.
 * 
 */


/*
 *
 * +------------+    +------------+      +------------+        +------------+
 * |    CPU     |    |    DEV     |      |    DEV     |        |    DEV     |
 * +------------+    +------------+      +------------+        +------------+
 *       |                 |                   |                     |
 *       | PVOID           |IMG_DEV_VIRTADDR   |IMG_DEV_VIRTADDR     |
 *       |                 \-------------------/                     |
 *       |                          |                                |
 * +------------+             +------------+                         |     
 * |    MMU     |             |    MMU     |                         |
 * +------------+             +------------+                         | 
 *       |                          |                                | 
 *       |                          |                                |
 *       |                          |                                |
 *   +--------+                +---------+                      +--------+
 *   | Offset |                | (Offset)|                      | Offset |
 *   +--------+                +---------+                      +--------+    
 *       |                          |                IMG_DEV_PHYADDR | 
 *       |                          |                                |
 *       |                          | IMG_DEV_PHYADDR                |
 * +---------------------------------------------------------------------+ 
 * |                         System Address bus                          |
 * +---------------------------------------------------------------------+
 *
 */

typedef IMG_PVOID IMG_CPU_VIRTADDR;

/* device virtual address */
typedef struct _IMG_DEV_VIRTADDR
{
	/* device virtual addresses are 32bit for now */
	IMG_UINT32  uiAddr;
#define IMG_CAST_TO_DEVVADDR_UINT(var)		(IMG_UINT32)(var)
	
} IMG_DEV_VIRTADDR;

typedef IMG_UINT32 IMG_DEVMEM_SIZE_T;

/* cpu physical address */
typedef struct _IMG_CPU_PHYADDR
{
	/* variable sized type (32,64) */
#if IMG_ADDRSPACE_PHYSADDR_BITS == 32
	/* variable sized type (32,64) */
	IMG_UINT32 uiAddr;
#else
	IMG_UINT64 uiAddr;
#endif
} IMG_CPU_PHYADDR;

/* device physical address */
typedef struct _IMG_DEV_PHYADDR
{
#if IMG_ADDRSPACE_PHYSADDR_BITS == 32
	/* variable sized type (32,64) */
	IMG_UINT32 uiAddr;
#else
	IMG_UINT64 uiAddr;
#endif
} IMG_DEV_PHYADDR;

/* system physical address */
typedef struct _IMG_SYS_PHYADDR
{
	/* variable sized type (32,64) */
#if IMG_ADDRSPACE_PHYSADDR_BITS == 32
	/* variable sized type (32,64) */
	IMG_UINT32 uiAddr;
#else
	IMG_UINT64 uiAddr;
#endif
} IMG_SYS_PHYADDR;

#include "img_defs.h"

#endif	/* __IMG_TYPES_H__ */
/******************************************************************************
 End of file (img_types.h)
******************************************************************************/
