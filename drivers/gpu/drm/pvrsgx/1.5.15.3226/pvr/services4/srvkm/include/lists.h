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

#ifndef __LISTS_UTILS__
#define __LISTS_UTILS__

#include <stdarg.h>
#include "img_types.h"

#define DECLARE_LIST_FOR_EACH(TYPE) \
IMG_VOID List_##TYPE##_ForEach(TYPE *psHead, IMG_VOID(*pfnCallBack)(TYPE* psNode))

#define IMPLEMENT_LIST_FOR_EACH(TYPE) \
IMG_VOID List_##TYPE##_ForEach(TYPE *psHead, IMG_VOID(*pfnCallBack)(TYPE* psNode))\
{\
	while(psHead)\
	{\
		pfnCallBack(psHead);\
		psHead = psHead->psNext;\
	}\
}


#define DECLARE_LIST_FOR_EACH_VA(TYPE) \
IMG_VOID List_##TYPE##_ForEach_va(TYPE *psHead, IMG_VOID(*pfnCallBack)(TYPE* psNode, va_list va), ...)

#define IMPLEMENT_LIST_FOR_EACH_VA(TYPE) \
IMG_VOID List_##TYPE##_ForEach_va(TYPE *psHead, IMG_VOID(*pfnCallBack)(TYPE* psNode, va_list va), ...) \
{\
	va_list ap;\
	while(psHead)\
	{\
		va_start(ap, pfnCallBack);\
		pfnCallBack(psHead, ap);\
		psHead = psHead->psNext;\
		va_end(ap);\
	}\
}


#define DECLARE_LIST_ANY(TYPE) \
IMG_VOID* List_##TYPE##_Any(TYPE *psHead, IMG_VOID* (*pfnCallBack)(TYPE* psNode))

#define IMPLEMENT_LIST_ANY(TYPE) \
IMG_VOID* List_##TYPE##_Any(TYPE *psHead, IMG_VOID* (*pfnCallBack)(TYPE* psNode))\
{ \
	IMG_VOID *pResult;\
	TYPE *psNextNode;\
	pResult = IMG_NULL;\
	psNextNode = psHead;\
	while(psHead && !pResult)\
	{\
		psNextNode = psNextNode->psNext;\
		pResult = pfnCallBack(psHead);\
		psHead = psNextNode;\
	}\
	return pResult;\
}


#define DECLARE_LIST_ANY_VA(TYPE) \
IMG_VOID* List_##TYPE##_Any_va(TYPE *psHead, IMG_VOID*(*pfnCallBack)(TYPE* psNode, va_list va), ...)

#define IMPLEMENT_LIST_ANY_VA(TYPE) \
IMG_VOID* List_##TYPE##_Any_va(TYPE *psHead, IMG_VOID*(*pfnCallBack)(TYPE* psNode, va_list va), ...)\
{\
	va_list ap;\
	TYPE *psNextNode;\
	IMG_VOID* pResult = IMG_NULL;\
	while(psHead && !pResult)\
	{\
		psNextNode = psHead->psNext;\
		va_start(ap, pfnCallBack);\
		pResult = pfnCallBack(psHead, ap);\
		va_end(ap);\
		psHead = psNextNode;\
	}\
	return pResult;\
}

#define DECLARE_LIST_ANY_2(TYPE, RTYPE, CONTINUE) \
RTYPE List_##TYPE##_##RTYPE##_Any(TYPE *psHead, RTYPE (*pfnCallBack)(TYPE* psNode))

#define IMPLEMENT_LIST_ANY_2(TYPE, RTYPE, CONTINUE) \
RTYPE List_##TYPE##_##RTYPE##_Any(TYPE *psHead, RTYPE (*pfnCallBack)(TYPE* psNode))\
{ \
	RTYPE result;\
	TYPE *psNextNode;\
	result = CONTINUE;\
	psNextNode = psHead;\
	while(psHead && result == CONTINUE)\
	{\
		psNextNode = psNextNode->psNext;\
		result = pfnCallBack(psHead);\
		psHead = psNextNode;\
	}\
	return result;\
}


#define DECLARE_LIST_ANY_VA_2(TYPE, RTYPE, CONTINUE) \
RTYPE List_##TYPE##_##RTYPE##_Any_va(TYPE *psHead, RTYPE(*pfnCallBack)(TYPE* psNode, va_list va), ...)

#define IMPLEMENT_LIST_ANY_VA_2(TYPE, RTYPE, CONTINUE) \
RTYPE List_##TYPE##_##RTYPE##_Any_va(TYPE *psHead, RTYPE(*pfnCallBack)(TYPE* psNode, va_list va), ...)\
{\
	va_list ap;\
	TYPE *psNextNode;\
	RTYPE result = CONTINUE;\
	while(psHead && result == CONTINUE)\
	{\
		psNextNode = psHead->psNext;\
		va_start(ap, pfnCallBack);\
		result = pfnCallBack(psHead, ap);\
		va_end(ap);\
		psHead = psNextNode;\
	}\
	return result;\
}


#define DECLARE_LIST_REMOVE(TYPE) \
IMG_VOID List_##TYPE##_Remove(TYPE *psNode)

#define IMPLEMENT_LIST_REMOVE(TYPE) \
IMG_VOID List_##TYPE##_Remove(TYPE *psNode)\
{\
	(*psNode->ppsThis)=psNode->psNext;\
	if(psNode->psNext)\
	{\
		psNode->psNext->ppsThis = psNode->ppsThis;\
	}\
}

#define DECLARE_LIST_INSERT(TYPE) \
IMG_VOID List_##TYPE##_Insert(TYPE **ppsHead, TYPE *psNewNode)

#define IMPLEMENT_LIST_INSERT(TYPE) \
IMG_VOID List_##TYPE##_Insert(TYPE **ppsHead, TYPE *psNewNode)\
{\
	psNewNode->ppsThis = ppsHead;\
	psNewNode->psNext = *ppsHead;\
	*ppsHead = psNewNode;\
	if(psNewNode->psNext)\
	{\
		psNewNode->psNext->ppsThis = &(psNewNode->psNext);\
	}\
}


#define IS_LAST_ELEMENT(x) ((x)->psNext == IMG_NULL)

#endif
