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

#ifndef _HASH_H_
#define _HASH_H_

#include "img_types.h"
#include "osfunc.h"

struct HASH_TABLE;
u32 HASH_Func_Default(size_t uKeySize, void *pKey, u32 uHashTabLen);
IMG_BOOL HASH_Key_Comp_Default(size_t uKeySize, void *pKey1, void *pKey2);
struct HASH_TABLE *HASH_Create_Extended(u32 uInitialLen, size_t uKeySize,
				 u32 (*pfnHashFunc)(size_t uKeySize, void *pkey,
						    u32 uHashTabLen),
				 IMG_BOOL (*pfnKeyComp)(size_t uKeySize,
							void *pKey1,
							void *pkey2));
struct HASH_TABLE *HASH_Create(u32 uInitialLen);
void HASH_Delete(struct HASH_TABLE *pHash);
IMG_BOOL HASH_Insert_Extended(struct HASH_TABLE *pHash, void *pKey, u32 v);
IMG_BOOL HASH_Insert(struct HASH_TABLE *pHash, u32 k, u32 v);
u32 HASH_Remove_Extended(struct HASH_TABLE *pHash, void *pKey);
u32 HASH_Remove(struct HASH_TABLE *pHash, u32 k);
u32 HASH_Retrieve_Extended(struct HASH_TABLE *pHash, void *pKey);
u32 HASH_Retrieve(struct HASH_TABLE *pHash, u32 k);

#endif
