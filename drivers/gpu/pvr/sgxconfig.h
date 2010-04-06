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

#ifndef __SGXCONFIG_H__
#define __SGXCONFIG_H__

#define DEV_DEVICE_TYPE					PVRSRV_DEVICE_TYPE_SGX
#define DEV_DEVICE_CLASS				PVRSRV_DEVICE_CLASS_3D

#define DEV_MAJOR_VERSION				1
#define DEV_MINOR_VERSION				0

#define SGX_GENERAL_HEAP_BASE				0x01800000
#define SGX_GENERAL_HEAP_SIZE				(0x06C00000-0x00001000)

#define SGX_3DPARAMETERS_HEAP_BASE			0x08400000
#define SGX_3DPARAMETERS_HEAP_SIZE			(0x04000000-0x00001000)

#define SGX_TADATA_HEAP_BASE				0x0C400000
#define SGX_TADATA_HEAP_SIZE				(0x01000000-0x00001000)

#define SGX_SYNCINFO_HEAP_BASE				0x0D400000
#define SGX_SYNCINFO_HEAP_SIZE				(0x00400000-0x00001000)

#define SGX_PDSPIXEL_CODEDATA_HEAP_BASE			0x0D800000
#define SGX_PDSPIXEL_CODEDATA_HEAP_SIZE			(0x00800000-0x00001000)

#define SGX_PDSVERTEX_CODEDATA_HEAP_BASE		0x0E000000
#define SGX_PDSVERTEX_CODEDATA_HEAP_SIZE		(0x00800000-0x00001000)

#define SGX_RESERVED_CODE_HEAP_BASE			0x0E800000
#define SGX_RESERVED_CODE_HEAP_SIZE			(0x00080000-0x00001000)

#define SGX_KERNEL_CODE_HEAP_BASE			0x0EC00000
#define SGX_KERNEL_CODE_HEAP_SIZE			(0x00080000-0x00001000)

#define SGX_KERNEL_DATA_HEAP_BASE			0x0F000000
#define SGX_KERNEL_DATA_HEAP_SIZE			(0x00400000-0x00001000)

#define SGX_PIXELSHADER_HEAP_BASE			0x0F400000
#define SGX_PIXELSHADER_HEAP_SIZE			(0x00500000-0x00001000)

#define SGX_VERTEXSHADER_HEAP_BASE			0x0FC00000
#define SGX_VERTEXSHADER_HEAP_SIZE			(0x00200000-0x00001000)

#define SGX_CORE_IDENTIFIED

#if !defined(SGX_CORE_IDENTIFIED)
#error "sgxconfig.h: ERROR: unspecified SGX Core version"
#endif

#endif
