#ifndef __MTK_DEBUG_H__
#define __MTK_DEBUG_H__

#include "mtk_version.h"

#include "img_types.h"

#include "servicesext.h"
#include "mutex.h"

#include "mtk_pp.h"

#if defined (__cplusplus)
extern "C" {
#endif

#ifdef MTK_DEBUG

IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugInit(IMG_VOID);
IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugDeinit(IMG_VOID);
IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugSetInfo(const IMG_CHAR* acDebugMsg, IMG_INT32 i32Size);
IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugGetInfo(IMG_CHAR* acDebugMsg, IMG_INT32 i32Size);
IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugEnable3DMemInfo(IMG_BOOL bEnable);
IMG_IMPORT IMG_BOOL IMG_CALLCONV MTKDebugIsEnable3DMemInfo(IMG_VOID);

#endif // MTK_DEBUG

#if defined (__cplusplus)
}
#endif

#endif	/* __MTK_DEBUG_H__ */

/******************************************************************************
 End of file (mtk_debug.h)
******************************************************************************/

