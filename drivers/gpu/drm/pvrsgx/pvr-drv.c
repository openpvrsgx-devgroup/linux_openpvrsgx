// SPDX-License-Identifier: (GPL-2.0 OR MIT)
/*
 * Linux DRM Driver for PoverVR SGX
 *
 * Copyright (C) 2019 Tony Lindgren <tony@atomide.com>
 *
 * Some parts of code based on earlier Imagination driver
 * Copyright (C) Imagination Technologies Ltd.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>

#include <drm/drmP.h>
#include <drm/drm.h>
#include <drm/drm_modeset_helper.h>

#include "pvr-drv.h"

/* Currently used Imagination SDK includes */
#include "services.h"
#include "mmap.h"
#include "linkage.h"
#include "pvr_bridge.h"
#include "pvrversion.h"

#define PVR_DRM_NAME		PVRSRV_MODNAME
#define PVR_DRM_DESC		"Imagination Technologies PVR DRM"
#define PVR_DRM_DATE		"20110701"

#define PVR_QUIRK_OMAP4		BIT(0)

struct pvr_capabilities {
	u32 quirks;
	unsigned long smp:1;
};

struct pvr {
	struct device *dev;
	struct drm_device *ddev;
	const struct pvr_capabilities *cap;
	u32 quirks;
};

struct platform_device *gpsPVRLDMDev;
static struct drm_device *gpsPVRDRMDev;

static int pvr_drm_load(struct drm_device *dev, unsigned long flags)
{
	dev_dbg(dev->dev, "%s\n", __func__);
	gpsPVRDRMDev = dev;
	gpsPVRLDMDev = to_platform_device(dev->dev);

	return PVRCore_Init();
}

static int pvr_drm_unload(struct drm_device *dev)
{
	dev_dbg(dev->dev, "%s\n", __func__);
	PVRCore_Cleanup();

	return 0;
}

static int pvr_open(struct drm_device *dev, struct drm_file *file)
{
	dev_dbg(dev->dev, "%s\n", __func__);

	return PVRSRVOpen(dev, file);
}

static int pvr_ioctl_command(struct drm_device *dev, void *arg, struct drm_file *filp)
{
	dev_dbg(dev->dev, "%s: dev: %px arg: %px filp: %px\n", __func__, dev, arg, filp);

	return PVRSRV_BridgeDispatchKM(dev, arg, filp);
}

static int pvr_ioctl_drm_is_master(struct drm_device *dev, void *arg, struct drm_file *filp)
{
	dev_dbg(dev->dev, "%s: dev: %px arg: %px filp: %px\n", __func__, dev, arg, filp);

	return 0;
}

static int pvr_ioctl_unpriv(struct drm_device *dev, void *arg, struct drm_file *filp)
{
	dev_dbg(dev->dev, "%s: dev: %px arg: %px filp: %px\n", __func__, dev, arg, filp);

	return 0;
}

static int pvr_ioctl_dbgdrv(struct drm_device *dev, void *arg, struct drm_file *filp)
{
	dev_dbg(dev->dev, "%s: dev: %px arg: %px filp: %px\n", __func__, dev, arg, filp);

#ifdef PDUMP
	return dbgdrv_ioctl(dev, arg, filp);
#endif

	return 0;
}

static struct drm_ioctl_desc pvr_ioctls[] = {
	DRM_IOCTL_DEF_DRV(PVR_SRVKM, pvr_ioctl_command, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(PVR_IS_MASTER, pvr_ioctl_drm_is_master,
			   DRM_RENDER_ALLOW | DRM_MASTER),
	DRM_IOCTL_DEF_DRV(PVR_UNPRIV, pvr_ioctl_unpriv, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(PVR_DBGDRV, pvr_ioctl_dbgdrv, DRM_RENDER_ALLOW),
#ifdef PVR_DISPLAY_CONTROLLER_DRM_IOCTL
	DRM_IOCTL_DEF_DRV(PVR_DISP, drm_invalid_op, DRM_MASTER)
#endif
};

/* REVISIT: This is used by dmabuf.c */
struct device *PVRLDMGetDevice(void)
{
	return gpsPVRDRMDev->dev;
}

static int pvr_drm_release(struct inode *inode, struct file *filp)
{
	struct drm_file *file_priv = filp->private_data;
	int error;

	error = drm_release(inode, filp);
	if (error)
		return error;

	PVRSRVRelease(file_priv->driver_priv);

	return 0;
}

static const struct file_operations pvr_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = pvr_drm_release,
	.unlocked_ioctl = drm_ioctl,
	.compat_ioctl = drm_compat_ioctl,
	.mmap = PVRMMap,
	.poll = drm_poll,
	.read = drm_read,
	.llseek = noop_llseek,
};

static struct drm_driver pvr_drm_driver = {
	.driver_features = DRIVER_RENDER,
	.dev_priv_size = 0,
	.open = pvr_open,
	.ioctls = pvr_ioctls,
	.num_ioctls = ARRAY_SIZE(pvr_ioctls),
	.fops = &pvr_fops,
	.name = "pvr",
	.desc = PVR_DRM_DESC,
	.date = PVR_DRM_DATE,
	.major = PVRVERSION_MAJ,
	.minor = PVRVERSION_MIN,
	.patchlevel = PVRVERSION_BUILD,
};

static int __maybe_unused pvr_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);

	return 0;
}

static int __maybe_unused pvr_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pvr_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct pvr *ddata = dev_get_drvdata(dev);
	struct drm_device *drm_dev = ddata->ddev;
	int error;

	error = drm_mode_config_helper_suspend(drm_dev);
	if (error)
		dev_warn(dev, "%s: error: %i\n", __func__, error);

	return PVRSRVDriverSuspend(pdev, PMSG_SUSPEND);
}

static int pvr_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct pvr *ddata = dev_get_drvdata(dev);
	struct drm_device *drm_dev = ddata->ddev;
	int error;

	error = drm_mode_config_helper_resume(drm_dev);
	if (error)
		dev_warn(dev, "%s: error: %i\n", __func__, error);

	return PVRSRVDriverResume(pdev);
}
#endif

static const struct dev_pm_ops pvr_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(pvr_suspend, pvr_resume)
	SET_RUNTIME_PM_OPS(pvr_runtime_suspend,
			    pvr_runtime_resume,
			    NULL)
};

static const struct pvr_capabilities __maybe_unused pvr_omap3 = {
};

static const struct pvr_capabilities __maybe_unused pvr_omap4 = {
	.quirks = PVR_QUIRK_OMAP4,
};

static const struct pvr_capabilities __maybe_unused pvr_omap4470 = {
	.smp = true,
};

static const struct pvr_capabilities __maybe_unused pvr_omap5 = {
	.smp = true,
};

static const struct of_device_id pvr_ids[] = {
	OMAP3_SGX530_121("ti,omap3-sgx530-121", &pvr_omap3)
	OMAP3630_SGX530_125("ti,omap3-sgx530-125", &pvr_omap3)
	AM3517_SGX530_125("ti,am3517-sgx530-125", &pvr_omap3)
	AM335X_SGX530_125("ti,am3352-sgx530-125", &pvr_omap3)
	AM4_SGX530_125("ti,am4-sgx530-125", &pvr_omap3)
	OMAP4_SGX540_120("ti,omap4-sgx540-120", &pvr_omap4)
	OMAP4470_SGX544_112("ti,omap4-sgx544-112", &pvr_omap4470)
	OMAP5_SGX544_116("ti,omap5-sgx544-116", &pvr_omap5)
	DRA7_SGX544_116("ti,dra7-sgx544-116", &pvr_omap5)
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, pvr_ids);

static int pvr_init_match(struct pvr *ddata)
{
	const struct pvr_capabilities *cap;

	cap = of_device_get_match_data(ddata->dev);
	if (!cap)
		return 0;

	ddata->cap = cap;

	ddata->quirks = cap->quirks;
	dev_info(ddata->dev, "Enabling quirks %08x\n", ddata->quirks);

	return 0;
}

static int pvr_probe(struct platform_device *pdev)
{
	struct drm_device *ddev;
	struct pvr *ddata;
	int error;

	ddata = devm_kzalloc(&pdev->dev, sizeof(*ddata), GFP_KERNEL);
	if (!ddata)
		return -ENOMEM;

	ddata->dev = &pdev->dev;
	platform_set_drvdata(pdev, ddata);

	error = pvr_init_match(ddata);
	if (error)
		return error;

	pm_runtime_enable(ddata->dev);
	error = pm_runtime_get_sync(ddata->dev);
	if (error < 0) {
		pm_runtime_put_noidle(ddata->dev);

		return error;
	}

	ddev = drm_dev_alloc(&pvr_drm_driver, ddata->dev);
	if (IS_ERR(ddev)) {
		error = PTR_ERR(ddev);
		goto out_err_idle;
	}

	error = pvr_drm_load(ddev, 0);
	if (error)
		goto out_err_free;

	error = drm_dev_register(ddev, 0);
	if (error < 0)
		goto out_err_unload;

	ddata->ddev = ddev;
	ddev->dev_private = ddata;

	gpsPVRLDMDev = pdev;

	return 0;

out_err_unload:
	pvr_drm_unload(gpsPVRDRMDev);

out_err_free:
	drm_put_dev(gpsPVRDRMDev);

out_err_idle:
	pm_runtime_put_sync(ddata->dev);
	pm_runtime_disable(ddata->dev);

	return error;
}

static int pvr_remove(struct platform_device *pdev)
{
	drm_put_dev(gpsPVRDRMDev);
	pvr_drm_unload(gpsPVRDRMDev);
	gpsPVRDRMDev = NULL;

	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static struct platform_driver pvr_driver = {
	.driver = {
		.name = PVR_DRM_NAME,
		.of_match_table = pvr_ids,
		.pm = &pvr_pm_ops,
	},
	.probe = pvr_probe,
	.remove = pvr_remove,
	.shutdown = PVRSRVDriverShutdown,
};

static int __init pvr_init(void)
{
	/* Must come before attempting to print anything via Services */
	PVRDPFInit();

	return platform_driver_register(&pvr_driver);
}

static void __exit pvr_exit(void)
{
	platform_driver_unregister(&pvr_driver);
}

module_init(pvr_init);
module_exit(pvr_exit);
