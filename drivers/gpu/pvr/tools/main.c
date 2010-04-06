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

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

#include "img_types.h"
#include "linuxsrv.h"
#include "ioctl.h"
#include "dbgdrvif.h"
#include "dbgdriv.h"
#include "hostfunc.h"
#include "pvr_debug.h"

#define DRVNAME "dbgdrv"

MODULE_AUTHOR("Imagination Technologies Ltd. <gpl-support@imgtec.com>");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE(DRVNAME);

static int AssignedMajorNumber;

static long dbgdrv_ioctl(struct file *, unsigned int, unsigned long);

static int dbgdrv_open(struct inode unref__ * pInode,
		       struct file unref__ * pFile)
{
	return 0;
}

static int dbgdrv_release(struct inode unref__ * pInode,
			  struct file unref__ * pFile)
{
	return 0;
}

static int dbgdrv_mmap(struct file *pFile, struct vm_area_struct *ps_vma)
{
	return 0;
}

static const struct file_operations dbgdrv_fops = {
owner:	THIS_MODULE,
unlocked_ioctl : dbgdrv_ioctl,
open :	dbgdrv_open,
release : dbgdrv_release,
mmap :	dbgdrv_mmap,
};

void DBGDrvGetServiceTable(void **fn_table)
{
	*fn_table = &g_sDBGKMServices;

}
EXPORT_SYMBOL(DBGDrvGetServiceTable);

void cleanup_module(void)
{
	if (AssignedMajorNumber > 0)
		unregister_chrdev(AssignedMajorNumber, DRVNAME);

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
	HostDestroyEventObjects();
#endif

	if (g_pvAPIMutex != NULL)
		HostDestroyMutex(g_pvAPIMutex);

	return;
}

int init_module(void)
{
	g_pvAPIMutex = HostCreateMutex();
	if (g_pvAPIMutex == NULL) {
		cleanup_module();
		return -ENOMEM;
	}
#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)

	(void)HostCreateEventObjects();
#endif

	AssignedMajorNumber =
	    register_chrdev(AssignedMajorNumber, DRVNAME, &dbgdrv_fops);

	if (AssignedMajorNumber <= 0) {
		PVR_DPF(PVR_DBG_ERROR, " unable to get major\n");
		cleanup_module();
		return -EBUSY;
	}

	return 0;
}

static long dbgdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct IOCTL_PACKAGE *pIP = (struct IOCTL_PACKAGE *)arg;

	char *buffer, *in, *out;

	if ((pIP->ui32InBufferSize > (PAGE_SIZE >> 1))
	    || (pIP->ui32OutBufferSize > (PAGE_SIZE >> 1))) {
		PVR_DPF(PVR_DBG_ERROR,
		"Sizes of the buffers are too large, cannot do ioctl\n");
		return -1;
	}

	buffer = (char *)HostPageablePageAlloc(1);
	if (!buffer) {
		PVR_DPF(PVR_DBG_ERROR,
			 "Failed to allocate buffer, cannot do ioctl\n");
		return -EFAULT;
	}

	in = buffer;
	out = buffer + (PAGE_SIZE >> 1);

	if (copy_from_user(in, pIP->pInBuffer, pIP->ui32InBufferSize) != 0)
		goto init_failed;

	cmd = ((pIP->ui32Cmd >> 2) & 0xFFF) - 0x801;

	if (pIP->ui32Cmd == DEBUG_SERVICE_READ) {
		char *ui8Tmp;
		u32 *pui32BytesCopied = (u32 *) out;
		struct DBG_IN_READ *psReadInParams = (struct DBG_IN_READ *)in;

		ui8Tmp = vmalloc(psReadInParams->ui32OutBufferSize);
		if (!ui8Tmp)
			goto init_failed;
		*pui32BytesCopied = ExtDBGDrivRead((struct DBG_STREAM *)
				   psReadInParams->pvStream,
				   psReadInParams->bReadInitBuffer,
				   psReadInParams->ui32OutBufferSize, ui8Tmp);
		if (copy_to_user(psReadInParams->pui8OutBuffer, ui8Tmp,
				 *pui32BytesCopied) != 0) {
			vfree(ui8Tmp);
			goto init_failed;
		}
		vfree(ui8Tmp);
	} else {
		(g_DBGDrivProc[cmd]) (in, out);
	}

	if (copy_to_user(pIP->pOutBuffer, out, pIP->ui32OutBufferSize) != 0)
		goto init_failed;

	HostPageablePageFree((void *) buffer);
	return 0;

init_failed:
	HostPageablePageFree((void *) buffer);
	return -EFAULT;
}

void RemoveHotKey(unsigned hHotKey)
{

}

void DefineHotKey(unsigned ScanCode, unsigned ShiftState, void *pInfo)
{

}
