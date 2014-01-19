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

#ifndef _HASH_H_
#define _HASH_H_

#include "img_types.h"
#include "osfunc.h"

#if defined (__cplusplus)
extern "C" {
#endif

typedef IMG_UINT32 HASH_FUNC(IMG_SIZE_T uKeySize, IMG_VOID *pKey, IMG_UINT32 uHashTabLen);
typedef IMG_BOOL HASH_KEY_COMP(IMG_SIZE_T uKeySize, IMG_VOID *pKey1, IMG_VOID *pKey2);

typedef struct _HASH_TABLE_ HASH_TABLE;

IMG_UINT32 HASH_Func_Default (IMG_SIZE_T uKeySize, IMG_VOID *pKey, IMG_UINT32 uHashTabLen);

IMG_BOOL HASH_Key_Comp_Default (IMG_SIZE_T uKeySize, IMG_VOID *pKey1, IMG_VOID *pKey2);

HASH_TABLE * HASH_Create_Extended (IMG_UINT32 uInitialLen, IMG_SIZE_T uKeySize, HASH_FUNC *pfnHashFunc, HASH_KEY_COMP *pfnKeyComp);

HASH_TABLE * HASH_Create (IMG_UINT32 uInitialLen);

IMG_VOID HASH_Delete (HASH_TABLE *pHash);

IMG_BOOL HASH_Insert_Extended (HASH_TABLE *pHash, IMG_VOID *pKey, IMG_UINTPTR_T v);

IMG_BOOL HASH_Insert (HASH_TABLE *pHash, IMG_UINTPTR_T k, IMG_UINTPTR_T v);

IMG_UINTPTR_T HASH_Remove_Extended(HASH_TABLE *pHash, IMG_VOID *pKey);

IMG_UINTPTR_T HASH_Remove (HASH_TABLE *pHash, IMG_UINTPTR_T k);

IMG_UINTPTR_T HASH_Retrieve_Extended (HASH_TABLE *pHash, IMG_VOID *pKey);

IMG_UINTPTR_T HASH_Retrieve (HASH_TABLE *pHash, IMG_UINTPTR_T k);

#ifdef HASH_TRACE
IMG_VOID HASH_Dump (HASH_TABLE *pHash);
#endif

#if defined (__cplusplus)
}
#endif

#endif

