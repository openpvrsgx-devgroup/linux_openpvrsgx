// SPDX-License-Identifier: GPL-2.0+
// Copyright (C) 2020 Andreas Kemnade

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>

#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_dma_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_file.h>
#include <drm/drm_format_helper.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_gem_atomic_helper.h>
#include <drm/drm_gem_dma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>

#include <drm/drm_ioctl.h>
#include <drm/drm_panel.h>
#include <drm/drm_prime.h>
#include <drm/drm_probe_helper.h>
#include "mxc_epdc.h"

#define DRIVER_NAME "mxc_epdc"
#define DRIVER_DESC "IMX EPDC"
#define DRIVER_DATE "20201007"
#define DRIVER_MAJOR 1
#define DRIVER_MINOR 0
#define DRIVER_PATCHLEVEL 0

#define to_mxc_epdc(x) container_of(x, struct mxc_epdc, drm)

int mxc_epdc_framebuffer_dirty(struct drm_framebuffer *fb,
			      struct drm_file *file_priv, unsigned int flags,
			      unsigned int color, struct drm_clip_rect *clips,
			      unsigned int num_clips)
{
	return 0;
}


static const struct drm_framebuffer_funcs mxc_epdc_framebuffer_funcs = {
	.create_handle = drm_gem_fb_create_handle,
	.dirty = mxc_epdc_framebuffer_dirty,
	.destroy = drm_gem_fb_destroy,
};

struct drm_framebuffer *mxc_epdc_fb_create(struct drm_device *dev, struct drm_file *file,
						const struct drm_mode_fb_cmd2 *mode_cmd)
{
	return drm_gem_fb_create_with_funcs(dev, file, mode_cmd,
					    &mxc_epdc_framebuffer_funcs);
}


static const struct drm_mode_config_funcs mxc_epdc_mode_config_funcs = {
	.fb_create = mxc_epdc_fb_create,
	.atomic_check	   = drm_atomic_helper_check,
	.atomic_commit	  = drm_atomic_helper_commit,
};

static struct mxc_epdc *
drm_pipe_to_mxc_epdc(struct drm_simple_display_pipe *pipe)
{
	return container_of(pipe, struct mxc_epdc, pipe);
}

static struct mxc_epdc *
drm_connector_to_mxc_epdc(struct drm_connector *connector)
{
	return container_of(connector, struct mxc_epdc, connector);
}

static void mxc_epdc_setup_mode_config(struct drm_device *drm)
{
	drm_mode_config_init(drm);

	drm->mode_config.min_width = 0;
	drm->mode_config.min_height = 0;
	/*
	 * Maximum update buffer image width due to v2.0 and v2.1 errata
	 * ERR005313.
	 */
	drm->mode_config.max_width = 2047;
	drm->mode_config.max_height = 2048;
	drm->mode_config.funcs = &mxc_epdc_mode_config_funcs;
}


DEFINE_DRM_GEM_DMA_FOPS(fops);

static int mxc_epdc_get_modes(struct drm_connector *connector)
{
	struct mxc_epdc *priv = drm_connector_to_mxc_epdc(connector);

	if (priv->panel)
		return drm_panel_get_modes(priv->panel, connector);

	return 0;
}

static const struct
drm_connector_helper_funcs mxc_epdc_connector_helper_funcs = {
	.get_modes = mxc_epdc_get_modes,
};

static const struct drm_connector_funcs mxc_epdc_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy  = drm_connector_cleanup,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

int mxc_epdc_output(struct drm_device *drm)
{
	struct mxc_epdc *priv = to_mxc_epdc(drm);
	struct device_node *remote_node;
	int ret;

	remote_node = of_graph_get_remote_node(drm->dev->of_node, 0, 0);
	if (!remote_node) {
		dev_dbg(drm->dev, "failed to find video sink\n");
		return -ENODEV;
	}

	priv->panel = of_drm_find_panel(remote_node);
	if (IS_ERR(priv->panel))
		return PTR_ERR(priv->panel);

	priv->connector.dpms = DRM_MODE_DPMS_OFF;
	priv->connector.polled = 0;
	drm_connector_helper_add(&priv->connector,
				 &mxc_epdc_connector_helper_funcs);
	ret = drm_connector_init(drm, &priv->connector,
				 &mxc_epdc_connector_funcs,
				 DRM_MODE_CONNECTOR_Unknown);
	if (ret)
		return ret;
	ret = drm_panel_attach(priv->panel, &priv->connector);
	return ret;
}

static void mxc_epdc_pipe_enable(struct drm_simple_display_pipe *pipe,
				   struct drm_crtc_state *crtc_state,
				   struct drm_plane_state *plane_state)
{
	struct mxc_epdc *priv = drm_pipe_to_mxc_epdc(pipe);
	struct drm_display_mode *m = &pipe->crtc.state->adjusted_mode;

	dev_info(priv->drm.dev, "Mode: %d x %d\n", m->hdisplay, m->vdisplay);
}

static void mxc_epdc_pipe_disable(struct drm_simple_display_pipe *pipe)
{
	struct mxc_epdc *priv = drm_pipe_to_mxc_epdc(pipe);

	dev_dbg(priv->drm.dev, "pipe disable\n");
}

static void mxc_epdc_pipe_update(struct drm_simple_display_pipe *pipe,
				   struct drm_plane_state *plane_state)
{
	struct mxc_epdc *priv = drm_pipe_to_mxc_epdc(pipe);

	dev_dbg(priv->drm.dev, "pipe update\n");
}

static const struct drm_simple_display_pipe_funcs mxc_epdc_funcs = {
	.enable	 = mxc_epdc_pipe_enable,
	.disable = mxc_epdc_pipe_disable,
	.update	= mxc_epdc_pipe_update,
	.prepare_fb = drm_gem_simple_display_pipe_prepare_fb,
};


static const uint32_t mxc_epdc_formats[] = {
	DRM_FORMAT_XRGB8888,
};

static struct drm_driver mxc_epdc_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.fops = &fops,
	.dumb_create	    = drm_gem_dma_dumb_create,
	.prime_handle_to_fd     = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle     = drm_gem_prime_fd_to_handle,
	.gem_prime_import_sg_table = drm_gem_dma_prime_import_sg_table,
	.gem_prime_mmap	 = drm_gem_prime_mmap,

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
};


static int mxc_epdc_probe(struct platform_device *pdev)
{
	struct mxc_epdc *priv;
	int ret;

	priv = devm_drm_dev_alloc(&pdev->dev, &mxc_epdc_driver, struct mxc_epdc, drm);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	platform_set_drvdata(pdev, priv);

	mxc_epdc_setup_mode_config(&priv->drm);

	ret = mxc_epdc_output(&priv->drm);
	if (ret)
		return ret;

	drm_simple_display_pipe_init(&priv->drm, &priv->pipe, &mxc_epdc_funcs,
				     mxc_epdc_formats,
				     ARRAY_SIZE(mxc_epdc_formats),
				     NULL,
				     &priv->connector);

	drm_mode_config_reset(&priv->drm);

	ret = drm_dev_register(&priv->drm, 0);

	drm_fbdev_generic_setup(&priv->drm, 32);
	return 0;
}

static int mxc_epdc_remove(struct platform_device *pdev)
{
	struct mxc_epdc *priv = platform_get_drvdata(pdev);

	drm_dev_unregister(&priv->drm);
	drm_kms_helper_poll_fini(&priv->drm);
	drm_mode_config_cleanup(&priv->drm);
	drm_panel_detach(priv->panel);
	return 0;
}

static const struct of_device_id imx_epdc_dt_ids[] = {
	{ .compatible = "fsl,imx6sl-epdc", },
	{ .compatible = "fsl,imx6sll-epdc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_epdc_dt_ids);

static struct platform_driver pdev = {
	.driver = {
		.name   = "mxc_epdc",
		.of_match_table = of_match_ptr(imx_epdc_dt_ids),
	},
	.probe  = mxc_epdc_probe,
	.remove = mxc_epdc_remove,
};

module_platform_driver(pdev);
MODULE_DESCRIPTION("IMX EPDC driver");
MODULE_LICENSE("GPL");

