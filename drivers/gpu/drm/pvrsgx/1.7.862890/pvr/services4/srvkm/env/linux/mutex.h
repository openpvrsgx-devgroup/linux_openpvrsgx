/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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

#ifndef __INCLUDED_LINUX_MUTEX_H_
#define __INCLUDED_LINUX_MUTEX_H_

#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))
#include <linux/mutex.h>
#else
#include <asm/semaphore.h>
#endif



#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))

typedef struct mutex PVRSRV_LINUX_MUTEX;

#else 


typedef struct {
    struct semaphore sSemaphore;
    
    atomic_t Count;
}PVRSRV_LINUX_MUTEX;

#endif


static inline IMG_VOID  LinuxInitMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex) {
    mutex_init(psPVRSRVMutex);
}

static inline IMG_VOID LinuxLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex) {
    mutex_lock(psPVRSRVMutex);
}

static inline PVRSRV_ERROR LinuxLockMutexInterruptible(PVRSRV_LINUX_MUTEX *psPVRSRVMutex) {
	if(mutex_lock_interruptible(psPVRSRVMutex) == -EINTR) {
		return PVRSRV_ERROR_MUTEX_INTERRUPTIBLE_ERROR; 
	} else {
		return PVRSRV_OK;
	}
}

static inline IMG_INT32 LinuxTryLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex) {
	return mutex_trylock(psPVRSRVMutex);
}

static inline IMG_VOID LinuxUnLockMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex) {
	mutex_unlock(psPVRSRVMutex);
}

static inline  IMG_BOOL LinuxIsLockedMutex(PVRSRV_LINUX_MUTEX *psPVRSRVMutex) {
	return (mutex_is_locked(psPVRSRVMutex)) ? IMG_TRUE : IMG_FALSE;
}

#endif 

