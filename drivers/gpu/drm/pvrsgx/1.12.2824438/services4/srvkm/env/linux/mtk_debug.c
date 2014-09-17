#include <linux/sched.h>
#include "img_types.h"
#include <linux/string.h>	
#include "mtk_debug.h"
#include "servicesext.h"
#include "mutex.h"
#include "services.h"
#include "osfunc.h"

#ifdef MTK_DEBUG

static IMG_UINT32 g_PID;
static PVRSRV_LINUX_MUTEX g_sDebugMutex;
static IMG_CHAR g_acMsgBuffer[MTK_DEBUG_MSG_LENGTH];
static IMG_BOOL g_bEnable3DMemInfo;

IMG_VOID MTKDebugInit(IMG_VOID)
{
	LinuxInitMutex(&g_sDebugMutex);
    g_bEnable3DMemInfo = IMG_FALSE;
    g_PID = 0;
	g_acMsgBuffer[0] = '\0';
	
#ifdef MTK_DEBUG_PROC_PRINT
	MTKPP_Init();
#endif
}

IMG_VOID MTKDebugDeinit(IMG_VOID)
{
#ifdef MTK_DEBUG_PROC_PRINT
	MTKPP_Deinit();
#endif
}

IMG_VOID MTKDebugSetInfo(
	const IMG_CHAR* pszInfo,
    IMG_INT32       i32Size)
{
    if (i32Size > MTK_DEBUG_MSG_LENGTH)
    {
        i32Size = MTK_DEBUG_MSG_LENGTH;
    }
	LinuxLockMutex(&g_sDebugMutex);
    if (pszInfo && i32Size > 0)
    {
    	g_PID = OSGetCurrentProcessIDKM();
        memcpy(g_acMsgBuffer, pszInfo, i32Size);
    	g_acMsgBuffer[i32Size - 1] = '\0';
    }
    else
    {
        g_PID = 0;
        g_acMsgBuffer[0] = '\0';
    }
	LinuxUnLockMutex(&g_sDebugMutex);
}

IMG_VOID MTKDebugGetInfo(
    IMG_CHAR* acDebugMsg,
    IMG_INT32 i32Size)
{
    if (i32Size > MTK_DEBUG_MSG_LENGTH)
    {
        i32Size = MTK_DEBUG_MSG_LENGTH;
    }
	LinuxLockMutex(&g_sDebugMutex);
	if ((g_PID == OSGetCurrentProcessIDKM()) || (g_acMsgBuffer[0] == '\0'))
	{
		memcpy(acDebugMsg, g_acMsgBuffer, i32Size);
        acDebugMsg[i32Size - 1] = '\0';
	}
	else
	{
		snprintf(acDebugMsg, MTK_DEBUG_MSG_LENGTH, "{None}");
	}
	LinuxUnLockMutex(&g_sDebugMutex);
}

IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugEnable3DMemInfo(IMG_BOOL bEnable)
{
	LinuxLockMutex(&g_sDebugMutex);
    g_bEnable3DMemInfo = bEnable;
	LinuxUnLockMutex(&g_sDebugMutex);
}

IMG_IMPORT IMG_BOOL IMG_CALLCONV MTKDebugIsEnable3DMemInfo(IMG_VOID)
{
    return g_bEnable3DMemInfo;
}

#endif

