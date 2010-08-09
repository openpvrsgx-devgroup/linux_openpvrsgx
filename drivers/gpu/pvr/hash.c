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

#include "pvr_debug.h"
#include "img_defs.h"
#include "services.h"
#include "servicesint.h"
#include "hash.h"
#include "osfunc.h"

#define PRIVATE_MAX(a, b) ((a) > (b) ? (a) : (b))

#define	KEY_TO_INDEX(pHash, key, uSize) \
	((pHash)->pfnHashFunc((pHash)->uKeySize, key, uSize) % uSize)

#define	KEY_COMPARE(pHash, pKey1, pKey2) \
	((pHash)->pfnKeyComp((pHash)->uKeySize, pKey1, pKey2))

struct BUCKET {
	struct BUCKET *pNext;
	u32 v;
	u32 k[];
};
struct BUCKET;

struct HASH_TABLE {
	struct BUCKET **ppBucketTable;
	u32 uSize;
	u32 uCount;
	u32 uMinimumSize;
	u32 uKeySize;
	u32 (*pfnHashFunc)(size_t uKeySize, void *pkey, u32 uHashTabLen);
	IMG_BOOL (*pfnKeyComp)(size_t uKeySize, void *pKey1, void *pkey2);
};

u32 HASH_Func_Default(size_t uKeySize, void *pKey, u32 uHashTabLen)
{
	u32 *p = (u32 *) pKey;
	u32 uKeyLen = uKeySize / sizeof(u32);
	u32 ui;
	u32 uHashKey = 0;

	PVR_UNREFERENCED_PARAMETER(uHashTabLen);

	PVR_ASSERT((uKeySize % sizeof(u32)) == 0);

	for (ui = 0; ui < uKeyLen; ui++) {
		u32 uHashPart = (u32) *p++;

		uHashPart += (uHashPart << 12);
		uHashPart ^= (uHashPart >> 22);
		uHashPart += (uHashPart << 4);
		uHashPart ^= (uHashPart >> 9);
		uHashPart += (uHashPart << 10);
		uHashPart ^= (uHashPart >> 2);
		uHashPart += (uHashPart << 7);
		uHashPart ^= (uHashPart >> 12);

		uHashKey += uHashPart;
	}

	return uHashKey;
}

IMG_BOOL HASH_Key_Comp_Default(size_t uKeySize, void *pKey1, void *pKey2)
{
	u32 *p1 = (u32 *) pKey1;
	u32 *p2 = (u32 *) pKey2;
	u32 uKeyLen = uKeySize / sizeof(u32);
	u32 ui;

	PVR_ASSERT((uKeySize % sizeof(u32)) == 0);

	for (ui = 0; ui < uKeyLen; ui++)
		if (*p1++ != *p2++)
			return IMG_FALSE;

	return IMG_TRUE;
}

static enum PVRSRV_ERROR _ChainInsert(struct HASH_TABLE *pHash,
				      struct BUCKET *pBucket,
			 struct BUCKET **ppBucketTable, u32 uSize)
{
	u32 uIndex;

	PVR_ASSERT(pBucket != NULL);
	PVR_ASSERT(ppBucketTable != NULL);
	PVR_ASSERT(uSize != 0);

	if ((pBucket == NULL) || (ppBucketTable == NULL) || (uSize == 0)) {
		PVR_DPF(PVR_DBG_ERROR, "_ChainInsert: invalid parameter");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	uIndex = KEY_TO_INDEX(pHash, pBucket->k, uSize);
	pBucket->pNext = ppBucketTable[uIndex];
	ppBucketTable[uIndex] = pBucket;

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR _Rehash(struct HASH_TABLE *pHash,
				 struct BUCKET **ppOldTable, u32 uOldSize,
				 struct BUCKET **ppNewTable, u32 uNewSize)
{
	u32 uIndex;
	for (uIndex = 0; uIndex < uOldSize; uIndex++) {
		struct BUCKET *pBucket;
		pBucket = ppOldTable[uIndex];
		while (pBucket != NULL) {
			struct BUCKET *pNextBucket = pBucket->pNext;
			if (_ChainInsert(pHash, pBucket, ppNewTable, uNewSize)
			    != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR,
					"_Rehash: call to _ChainInsert failed");
				return PVRSRV_ERROR_GENERIC;
			}
			pBucket = pNextBucket;
		}
	}
	return PVRSRV_OK;
}

static IMG_BOOL _Resize(struct HASH_TABLE *pHash, u32 uNewSize)
{
	if (uNewSize != pHash->uSize) {
		struct BUCKET **ppNewTable;
		u32 uIndex;

		PVR_DPF(PVR_DBG_MESSAGE,
			 "HASH_Resize: oldsize=0x%x  newsize=0x%x  count=0x%x",
			 pHash->uSize, uNewSize, pHash->uCount);

		OSAllocMem(PVRSRV_PAGEABLE_SELECT,
			   sizeof(struct BUCKET *) * uNewSize,
			   (void **) &ppNewTable, NULL);
		if (ppNewTable == NULL)
			return IMG_FALSE;

		for (uIndex = 0; uIndex < uNewSize; uIndex++)
			ppNewTable[uIndex] = NULL;

		if (_Rehash(pHash, pHash->ppBucketTable, pHash->uSize,
			    ppNewTable, uNewSize) != PVRSRV_OK)
			return IMG_FALSE;

		OSFreeMem(PVRSRV_PAGEABLE_SELECT,
			  sizeof(struct BUCKET *) * pHash->uSize,
			  pHash->ppBucketTable, NULL);
		pHash->ppBucketTable = ppNewTable;
		pHash->uSize = uNewSize;
	}
	return IMG_TRUE;
}

struct HASH_TABLE *HASH_Create_Extended(u32 uInitialLen, size_t uKeySize,
				 u32 (*pfnHashFunc)(size_t uKeySize, void *pkey,
						    u32 uHashTabLen),
				 IMG_BOOL (*pfnKeyComp)(size_t uKeySize,
							void *pKey1,
							void *pkey2))
{
	struct HASH_TABLE *pHash;
	u32 uIndex;

	PVR_DPF(PVR_DBG_MESSAGE, "HASH_Create_Extended: InitialSize=0x%x",
		 uInitialLen);

	if (OSAllocMem(PVRSRV_PAGEABLE_SELECT,
		       sizeof(struct HASH_TABLE),
		       (void **) &pHash, NULL) != PVRSRV_OK)
		return NULL;

	pHash->uCount = 0;
	pHash->uSize = uInitialLen;
	pHash->uMinimumSize = uInitialLen;
	pHash->uKeySize = uKeySize;
	pHash->pfnHashFunc = pfnHashFunc;
	pHash->pfnKeyComp = pfnKeyComp;

	if (OSAllocMem(PVRSRV_PAGEABLE_SELECT,
		   sizeof(struct BUCKET *) * pHash->uSize,
		   (void **) &pHash->ppBucketTable, NULL) != PVRSRV_OK) {
		OSFreeMem(PVRSRV_PAGEABLE_SELECT, sizeof(struct HASH_TABLE),
			  pHash, NULL);
		return NULL;
	}

	for (uIndex = 0; uIndex < pHash->uSize; uIndex++)
		pHash->ppBucketTable[uIndex] = NULL;
	return pHash;
}

struct HASH_TABLE *HASH_Create(u32 uInitialLen)
{
	return HASH_Create_Extended(uInitialLen, sizeof(u32),
				    &HASH_Func_Default, &HASH_Key_Comp_Default);
}

void HASH_Delete(struct HASH_TABLE *pHash)
{
	if (pHash != NULL) {
		PVR_DPF(PVR_DBG_MESSAGE, "HASH_Delete");

		PVR_ASSERT(pHash->uCount == 0);
		OSFreeMem(PVRSRV_PAGEABLE_SELECT,
			  sizeof(struct BUCKET *) * pHash->uSize,
			  pHash->ppBucketTable, NULL);
		OSFreeMem(PVRSRV_PAGEABLE_SELECT, sizeof(struct HASH_TABLE),
			  pHash, NULL);
	}
}

IMG_BOOL HASH_Insert_Extended(struct HASH_TABLE *pHash, void *pKey, u32 v)
{
	struct BUCKET *pBucket;

	PVR_DPF(PVR_DBG_MESSAGE,
		 "HASH_Insert_Extended: Hash=%08X, pKey=%08X, v=0x%x", pHash,
		 pKey, v);

	PVR_ASSERT(pHash != NULL);

	if (pHash == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "HASH_Insert_Extended: invalid parameter");
		return IMG_FALSE;
	}

	if (OSAllocMem(PVRSRV_PAGEABLE_SELECT,
		       sizeof(struct BUCKET) + pHash->uKeySize,
		       (void **) &pBucket, NULL) != PVRSRV_OK)
		return IMG_FALSE;

	pBucket->v = v;
	OSMemCopy(pBucket->k, pKey, pHash->uKeySize);
	if (_ChainInsert(pHash, pBucket, pHash->ppBucketTable, pHash->uSize) !=
	    PVRSRV_OK) {
		OSFreeMem(PVRSRV_PAGEABLE_SELECT,
			  sizeof(struct BUCKET) + pHash->uKeySize,
			  pBucket, NULL);
		return IMG_FALSE;
	}

	pHash->uCount++;

	if (pHash->uCount << 1 > pHash->uSize)
		_Resize(pHash, pHash->uSize << 1);

	return IMG_TRUE;
}

IMG_BOOL HASH_Insert(struct HASH_TABLE *pHash, u32 k, u32 v)
{
	PVR_DPF(PVR_DBG_MESSAGE,
		 "HASH_Insert: Hash=%08X, k=0x%x, v=0x%x", pHash, k, v);

	return HASH_Insert_Extended(pHash, &k, v);
}

u32 HASH_Remove_Extended(struct HASH_TABLE *pHash, void *pKey)
{
	struct BUCKET **ppBucket;
	u32 uIndex;

	PVR_DPF(PVR_DBG_MESSAGE, "HASH_Remove: Hash=%08X, pKey=%08X", pHash,
		 pKey);

	PVR_ASSERT(pHash != NULL);

	if (pHash == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "FreeResourceByPtr: invalid parameter");
		return 0;
	}

	uIndex = KEY_TO_INDEX(pHash, pKey, pHash->uSize);

	for (ppBucket = &(pHash->ppBucketTable[uIndex]); *ppBucket != NULL;
	     ppBucket = &((*ppBucket)->pNext))
		if (KEY_COMPARE(pHash, (*ppBucket)->k, pKey)) {
			struct BUCKET *pBucket = *ppBucket;
			u32 v = pBucket->v;
			(*ppBucket) = pBucket->pNext;

			OSFreeMem(PVRSRV_PAGEABLE_SELECT,
				  sizeof(struct BUCKET) + pHash->uKeySize,
				  pBucket, NULL);

			pHash->uCount--;

			if (pHash->uSize > (pHash->uCount << 2) &&
			    pHash->uSize > pHash->uMinimumSize)

				_Resize(pHash,
					PRIVATE_MAX(pHash->uSize >> 1,
						    pHash->uMinimumSize));

			PVR_DPF(PVR_DBG_MESSAGE,
			"HASH_Remove_Extended: Hash=%08X, pKey=%08X = 0x%x",
				 pHash, pKey, v);
			return v;
		}
	PVR_DPF(PVR_DBG_MESSAGE,
		 "HASH_Remove_Extended: Hash=%08X, pKey=%08X = 0x0 !!!!", pHash,
		 pKey);
	return 0;
}

u32 HASH_Remove(struct HASH_TABLE *pHash, u32 k)
{
	PVR_DPF(PVR_DBG_MESSAGE, "HASH_Remove: Hash=%08X, k=0x%x", pHash, k);

	return HASH_Remove_Extended(pHash, &k);
}

u32 HASH_Retrieve_Extended(struct HASH_TABLE *pHash, void *pKey)
{
	struct BUCKET **ppBucket;
	u32 uIndex;

	PVR_DPF(PVR_DBG_MESSAGE, "HASH_Retrieve: Hash=%08X, pKey=%08X", pHash,
		 pKey);

	PVR_ASSERT(pHash != NULL);

	if (pHash == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "HASH_Retrieve_Extended: invalid parameter");
		return 0;
	}

	uIndex = KEY_TO_INDEX(pHash, pKey, pHash->uSize);

	for (ppBucket = &(pHash->ppBucketTable[uIndex]); *ppBucket != NULL;
	     ppBucket = &((*ppBucket)->pNext))
		if (KEY_COMPARE(pHash, (*ppBucket)->k, pKey)) {
			struct BUCKET *pBucket = *ppBucket;
			u32 v = pBucket->v;

			PVR_DPF(PVR_DBG_MESSAGE,
				 "HASH_Retrieve: Hash=%08X, pKey=%08X = 0x%x",
				 pHash, pKey, v);
			return v;
		}
	PVR_DPF(PVR_DBG_MESSAGE,
		 "HASH_Retrieve: Hash=%08X, pKey=%08X = 0x0 !!!!", pHash, pKey);
	return 0;
}

u32 HASH_Retrieve(struct HASH_TABLE *pHash, u32 k)
{
	PVR_DPF(PVR_DBG_MESSAGE, "HASH_Retrieve: Hash=%08X, k=0x%x", pHash, k);
	return HASH_Retrieve_Extended(pHash, &k);
}
