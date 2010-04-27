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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>

#include <linux/platform_device.h>

#include "img_defs.h"
#include "services.h"
#include "kerneldisplay.h"
#include "kernelbuffer.h"
#include "syscommon.h"
#include "pvrmmap.h"
#include "mutils.h"
#include "mm.h"
#include "mmap.h"
#include "pvr_debug.h"
#include "srvkm.h"
#include "perproc.h"
#include "handle.h"
#include "pvr_bridge_km.h"
#include "sgx_bridge_km.h"
#include "proc.h"
#include "pvrmodule.h"
#include "private_data.h"
#include "pvr_events.h"

#define DRVNAME		"pvrsrvkm"

#ifdef CONFIG_PVR_DEBUG_EXTRA
static int debug = DBGPRIV_WARNING;
#include <linux/moduleparam.h>
module_param(debug, int, 0);
#endif

static int pvr_open(struct inode unref__ * inode, struct file *filp)
{
	struct PVRSRV_FILE_PRIVATE_DATA *priv;
	void *block_alloc;
	int ret = -ENOMEM;
	enum PVRSRV_ERROR err;
	u32 pid;

	pvr_lock();

	pid = OSGetCurrentProcessIDKM();

	if (PVRSRVProcessConnect(pid) != PVRSRV_OK)
		goto err_unlock;

	err = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			    sizeof(*priv),
			    (void **)&priv, &block_alloc);

	if (err != PVRSRV_OK)
		goto err_unlock;

	priv->ui32OpenPID = pid;
	priv->hBlockAlloc = block_alloc;
	filp->private_data = priv;

	INIT_LIST_HEAD(&priv->event_list);
	init_waitqueue_head(&priv->event_wait);
	priv->event_space = 4096; /* set aside 4k for event buffer */

	ret = 0;
err_unlock:
	pvr_unlock();

	return ret;
}

static int pvr_release(struct inode unref__ * inode, struct file *filp)
{
	struct PVRSRV_FILE_PRIVATE_DATA *priv;

	pvr_lock();

	priv = filp->private_data;

	pvr_release_events(priv);

	PVRSRVProcessDisconnect(priv->ui32OpenPID);

	OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		  sizeof(*priv),
		  priv, priv->hBlockAlloc);

	pvr_unlock();

	return 0;
}

static const struct file_operations pvr_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= PVRSRV_BridgeDispatchKM,
	.open		= pvr_open,
	.release	= pvr_release,
	.mmap		= PVRMMap,
	.poll           = pvr_poll,
	.read           = pvr_read,
};

static void pvr_shutdown(struct platform_device *pdev)
{
	PVR_TRACE("pvr_shutdown(pdev=%p)", pdev);

	(void)PVRSRVSetPowerStateKM(PVRSRV_POWER_STATE_D3);
}

static int pvr_suspend(struct platform_device *pdev, pm_message_t state)
{
	PVR_TRACE("pvr_suspend(pdev=%p)", pdev);

	if (PVRSRVSetPowerStateKM(PVRSRV_POWER_STATE_D3) != PVRSRV_OK)
		return -EINVAL;
	return 0;
}

static int pvr_resume(struct platform_device *pdev)
{
	PVR_TRACE("pvr_resume(pdev=%p)", pdev);

	if (PVRSRVSetPowerStateKM(PVRSRV_POWER_STATE_D0) != PVRSRV_OK)
		return -EINVAL;
	return 0;
}

static struct miscdevice pvr_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DRVNAME,
	.fops = &pvr_fops,
};

static int __devinit pvr_probe(struct platform_device *pdev)
{
	struct SYS_DATA *sysdata;
	int ret;

	PVR_TRACE("pvr_probe(pdev=%p)", pdev);

	if (SysAcquireData(&sysdata) != PVRSRV_OK &&
	    SysInitialise(pdev) != PVRSRV_OK) {
		ret = -ENODEV;
		goto err_exit;
	}

	ret = misc_register(&pvr_miscdevice);
	if (ret < 0)
		goto err_exit;

	return 0;

err_exit:
	dev_err(&pdev->dev, "probe failed (%d)\n", ret);

	return ret;
}

static int __devexit pvr_remove(struct platform_device *pdev)
{
	struct SYS_DATA *sysdata;
	int ret;

	PVR_TRACE("pvr_remove(pdev=%p)", pdev);

	ret = misc_deregister(&pvr_miscdevice);
	if (ret < 0) {
		dev_err(&pdev->dev, "remove failed (%d)\n", ret);
		return ret;
	}

	if (SysAcquireData(&sysdata) == PVRSRV_OK)
		SysDeinitialise(sysdata);

	return 0;
}


static struct platform_driver pvr_driver = {
	.driver = {
		   .name = DRVNAME,
	},
	.probe		= pvr_probe,
	.remove		= __devexit_p(pvr_remove),
	.suspend	= pvr_suspend,
	.resume		= pvr_resume,
	.shutdown	= pvr_shutdown,
};

static int __init pvr_init(void)
{
	int error;

	pvr_dbg_init();

	PVR_TRACE("pvr_init");

	pvr_init_lock();

#ifdef CONFIG_PVR_DEBUG_EXTRA
	PVRDebugSetLevel(debug);
#endif

	error = CreateProcEntries();
	if (error < 0)
		goto err1;

	error = -ENOMEM;
	if (LinuxMMInit() != PVRSRV_OK)
		goto err2;

	if (LinuxBridgeInit() != PVRSRV_OK)
		goto err3;

	PVRMMapInit();

	error = platform_driver_register(&pvr_driver);
	if (error < 0)
		goto err4;

	pvr_init_events();

	return 0;

err4:
	PVRMMapCleanup();
	LinuxBridgeDeInit();
err3:
	LinuxMMCleanup();
err2:
	RemoveProcEntries();
err1:
	pr_err("%s: failed (%d)\n", __func__, error);

	return error;
}

static void __exit pvr_cleanup(void)
{
	struct SYS_DATA *sysdata;

	PVR_TRACE("pvr_cleanup");

	SysAcquireData(&sysdata);

	platform_driver_unregister(&pvr_driver);

	PVRMMapCleanup();
	LinuxMMCleanup();
	LinuxBridgeDeInit();
	RemoveProcEntries();

	PVR_TRACE("pvr_cleanup: unloading");

	pvr_dbg_cleanup();
}

module_init(pvr_init);
module_exit(pvr_cleanup);

MODULE_SUPPORTED_DEVICE(DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
