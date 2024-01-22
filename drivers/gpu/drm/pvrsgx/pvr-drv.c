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
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>

#include <drm/drm.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
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
	u16 ocp_offset;
	u16 sgx_version;
	u16 sgx_revision;
};

struct pvr {
	struct device *dev;
	void __iomem *sgx_base;
	void __iomem *ocp_base;
	struct drm_device *ddev;
	const struct pvr_capabilities *cap;
	u32 quirks;
};

struct platform_device *gpsPVRLDMDev;
static struct drm_device *gpsPVRDRMDev;

u32 pvr_sgx_readl(struct pvr *ddata, unsigned short offset)
{
	return readl_relaxed(ddata->sgx_base + offset);
}

void pvr_sgx_writel(struct pvr *ddata, u32 val, unsigned short offset)
{
	writel_relaxed(val, ddata->sgx_base + offset);
}

u32 pvr_ocp_readl(struct pvr *ddata, unsigned short offset)
{
	return readl_relaxed(ddata->ocp_base + offset);
}

void pvr_ocp_writel(struct pvr *ddata, u32 val, unsigned short offset)
{
	writel_relaxed(val, ddata->ocp_base + offset);
}

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

static void pvr_postclose(struct drm_device *dev, struct drm_file *file)
{
	dev_dbg(dev->dev, "%s\n", __func__);

	PVRSRVRelease(file->driver_priv);

	file->driver_priv = NULL;
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
	int ret = 0;

	dev_dbg(dev->dev, "%s: dev: %px arg: %px filp: %px\n", __func__, dev, arg, filp);

	if (arg == NULL)
	{
		ret = -EFAULT;
	}
	else
	{
		struct pvr_unpriv *unpriv = (struct pvr_unpriv *)arg;

		switch (unpriv->cmd)
		{
			case PVR_UNPRIV_INIT_SUCCESFUL:
				unpriv->res = PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL) ? 1 : 0;
				break;

			default:
				ret = -EFAULT;
		}

	}

	return ret;
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

static const struct file_operations pvr_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.compat_ioctl = drm_compat_ioctl,
	.mmap = PVRMMap,
	.poll = drm_poll,
	.read = drm_read,
	.llseek = noop_llseek,
};

static struct drm_driver pvr_drm_driver = {
	.driver_features = DRIVER_RENDER,
	.open = pvr_open,
	.postclose = pvr_postclose,
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
	struct pvr *ddata = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	/* Nothing to do if no OCP */
	if (!ddata->ocp_base)
		return 0;

	pvr_ocp_writel(ddata, BIT(0), SGX_OCP_IRQENABLE_CLR_2);

	return 0;
}

static int __maybe_unused pvr_runtime_resume(struct device *dev)
{
	struct pvr *ddata = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	/* Nothing to do if no OCP */
	if (!ddata->ocp_base)
		return 0;

	pvr_ocp_writel(ddata, BIT(31), SGX_OCP_DEBUG_CONFIG);
	pvr_ocp_writel(ddata, BIT(0), SGX_OCP_IRQSTATUS_2);
	pvr_ocp_writel(ddata, BIT(0), SGX_OCP_IRQENABLE_SET_2);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int pvr_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return PVRSRVDriverSuspend(pdev, PMSG_SUSPEND);
}

static int pvr_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return PVRSRVDriverResume(pdev);
}
#endif

static const struct dev_pm_ops pvr_pm_ops = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(pvr_suspend, pvr_resume)
	SET_RUNTIME_PM_OPS(pvr_runtime_suspend,
			    pvr_runtime_resume,
			    NULL)
};

/* revisit: should we name for different capabilities? pvr_default, pvr_omap4, pvr_smp? */

static const struct pvr_capabilities __maybe_unused pvr_omap34xx = {
	/* Has OCP registers but not accessible */
	.sgx_version = 530,
	.sgx_revision = 121,
};

static const struct pvr_capabilities __maybe_unused pvr_omap36xx = {
	.ocp_offset = 0xfe00,
	.sgx_version = 530,
	.sgx_revision = 125,
};

static const struct pvr_capabilities __maybe_unused pvr_sirf = {
	.ocp_offset = 0xfe00,	// CHECKME
	.sgx_version = 531,
	.sgx_revision = 125,	// CHECKME
};

static const struct pvr_capabilities __maybe_unused pvr_omap4430 = {
	.ocp_offset = 0xfe00,
	.quirks = PVR_QUIRK_OMAP4,
	.sgx_version = 540,
	.sgx_revision = 120,
};

static const struct pvr_capabilities __maybe_unused pvr_omap4470 = {
	.ocp_offset = 0xfe00,
	.smp = true,
	.sgx_version = 544,
	.sgx_revision = 112,
};

static const struct pvr_capabilities __maybe_unused pvr_omap5 = {
	.ocp_offset = 0xfe00,
	.smp = true,
	.sgx_version = 544,
	.sgx_revision = 116,
};

static const struct pvr_capabilities __maybe_unused pvr_jz4780 = {
	.smp = true,
	.sgx_version = 540,
	.sgx_revision = 130,
};

static const struct pvr_capabilities __maybe_unused pvr_s5pv210 = {
	.smp = true,
	.sgx_version = 540,
	.sgx_revision = 120,
};

static const struct pvr_capabilities __maybe_unused pvr_sun8i_a31 = {
	.sgx_version = 544,
	.sgx_revision = 115,
};

static const struct pvr_capabilities __maybe_unused pvr_sun8i_a83t = {
	.smp = true,
	.sgx_version = 544,
	.sgx_revision = 115,
};

/*
 * The #ifdef are currently needed to prevent the multiple instances of
 * the driver from trying to probe.
 */

static const struct of_device_id pvr_ids[] = {
#ifdef ti_omap3_sgx530_121
	{ .compatible = "ti,omap3430-gpu", .data =  &pvr_omap34xx, },
#endif

#ifdef ti_omap3630_sgx530_125
	{ .compatible = "ti,omap3630-gpu", .data =  &pvr_omap36xx, },
#endif

#ifdef ti_am3517_sgx530_125
	{ .compatible = "ti,am3517-gpu", .data =  &pvr_omap36xx, },
#endif

#ifdef ti_am3352_sgx530_125
	{ .compatible = "ti,am3352-gpu", .data =  &pvr_omap36xx, },
#endif

#ifdef ti_am4_sgx530_125
	{ .compatible = "ti,am4-gpu", .data =  &pvr_omap36xx, },
	{ .compatible = "ti,am6548-gpu", .data =  &pvr_omap5, },
#endif

#if 0
	{ .compatible = "csr,atlas7-sgx531", .data =  &pvr_sirf, },
	{ .compatible = "csr,prima2-sgx531", .data =  &pvr_sirf, },
#endif

#ifdef ti_omap4_sgx540_120
	{ .compatible = "ti,omap4430-gpu", .data =  &pvr_omap4430, },
#endif

#ifdef ti_omap4470_sgx544_112
	{ .compatible = "ti,omap4470-gpu", .data =  &pvr_omap4470, },
#endif

#ifdef ti_omap5_sgx544_116
	{ .compatible = "ti,omap5432-gpu", .data =  &pvr_omap5, },
#endif

#ifdef ti_dra7_sgx544_116
	{ .compatible = "ti,am5728-gpu", .data =  &pvr_omap5, },
#endif

#ifdef ingenic_jz4780_sgx540_130
	{ .compatible = "ingenic,jz4780-gpu", .data =  &pvr_jz4780, },
#endif

#ifdef samsung_s5pv210_sgx540_120
	{ .compatible = "samsung,s5pv210-gpu", .data =  &pvr_s5pv210, },
#endif

#if 0
	{ .compatible = "allwinner,sun6i-a31-gpu", .data =  &pvr_sun8i_a31, },
	{ .compatible = "allwinner,sun8i-a31s-gpu", .data =  &pvr_sun8i_a31, },
#endif

#ifdef allwinner_sun8i_a83t_sgx544_115
	{ .compatible = "allwinner,sun8i-a83t-gpu", .data =  &pvr_sun8i_a83t, },
#endif

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

	ddata->sgx_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(ddata->sgx_base))
		return PTR_ERR(ddata->sgx_base);

	if (ddata->cap->ocp_offset)
		ddata->ocp_base = ddata->sgx_base + ddata->cap->ocp_offset;

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
