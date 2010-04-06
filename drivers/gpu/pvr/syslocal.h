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

#if !defined(__SYSLOCAL_H__)
#define __SYSLOCAL_H__

#include <linux/version.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <asm/atomic.h>

#include <linux/semaphore.h>
#include <linux/resource.h>

char *SysCreateVersionString(struct IMG_CPU_PHYADDR sRegRegion);

enum PVRSRV_ERROR InitSystemClocks(struct SYS_DATA *psSysData);
void CleanupSystemClocks(struct SYS_DATA *psSysData);
void DisableSystemClocks(struct SYS_DATA *psSysData);
enum PVRSRV_ERROR EnableSystemClocks(struct SYS_DATA *psSysData);

void DisableSGXClocks(struct SYS_DATA *psSysData);
enum PVRSRV_ERROR EnableSGXClocks(struct SYS_DATA *psSysData);

#define SYS_SPECIFIC_DATA_ENABLE_SYSCLOCKS	0x00000001
#define SYS_SPECIFIC_DATA_ENABLE_LISR		0x00000002
#define SYS_SPECIFIC_DATA_ENABLE_MISR		0x00000004
#define SYS_SPECIFIC_DATA_ENABLE_ENVDATA	0x00000008
#define SYS_SPECIFIC_DATA_ENABLE_LOCDEV		0x00000010
#define SYS_SPECIFIC_DATA_ENABLE_REGDEV		0x00000020
#define SYS_SPECIFIC_DATA_ENABLE_PDUMPINIT	0x00000040
#define SYS_SPECIFIC_DATA_ENABLE_INITDEV	0x00000080
#define SYS_SPECIFIC_DATA_ENABLE_LOCATEDEV	0x00000100

#define	SYS_SPECIFIC_DATA_PM_UNINSTALL_LISR	0x00000200
#define	SYS_SPECIFIC_DATA_PM_DISABLE_SYSCLOCKS	0x00000400
#define	SYS_SPECIFIC_DATA_ENABLE_PERF		0x00000800

#define	SYS_SPECIFIC_DATA_SET(psSysSpecData, flag) \
		((void)((psSysSpecData)->ui32SysSpecificData |= (flag)))

#define	SYS_SPECIFIC_DATA_CLEAR(psSysSpecData, flag) \
		((void)((psSysSpecData)->ui32SysSpecificData &= ~(flag)))

#define	SYS_SPECIFIC_DATA_TEST(psSysSpecData, flag) \
		(((psSysSpecData)->ui32SysSpecificData & (flag)) != 0)

struct SYS_SPECIFIC_DATA {
	u32 ui32SysSpecificData;
	struct PVRSRV_DEVICE_NODE *psSGXDevNode;
	IMG_BOOL bSGXInitComplete;
	u32 ui32SrcClockDiv;
	IMG_BOOL bConstraintNotificationsEnabled;
	atomic_t sSGXClocksEnabled;
	spinlock_t sPowerLock;
	atomic_t sPowerLockCPU;
	spinlock_t sNotifyLock;
	atomic_t sNotifyLockCPU;
	IMG_BOOL bCallVDD2PostFunc;

	struct clk *psCORE_CK;
	struct clk *psSGX_FCK;
	struct clk *psSGX_ICK;
	struct clk *psMPU_CK;
#if defined(DEBUG) || defined(TIMING)
	struct clk *psGPT11_FCK;
	struct clk *psGPT11_ICK;
	void __iomem *gpt_base;
#endif
	struct constraint_handle *pVdd2Handle;
};

extern struct SYS_SPECIFIC_DATA *gpsSysSpecificData;

#endif
