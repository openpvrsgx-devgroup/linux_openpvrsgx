/**************************************************************************
 * Copyright (c) 2011, Intel Corporation.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 **************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/console.h>

#include <drm/drmP.h>
#include <drm/drm.h>
#include <drm/drm_crtc.h>

#include "psb_drv.h"
#include "psb_intel_reg.h"
#include "psb_intel_drv.h"
#include "psb_ttm_userobj_api.h"
#include "psb_fb.h"
#include "psb_sgx.h"
#include "psb_pvr_glue.h"


extern int MRSTLFBHandleChangeFB(struct drm_device* dev, struct psb_framebuffer *psbfb);

struct MRSTLFB_BUFFER_TAG;
uint32_t MRSTLFBGetSize(struct MRSTLFB_BUFFER_TAG *pBuffer);
void* MRSTLFBGetCPUVAddr(struct MRSTLFB_BUFFER_TAG *pBuffer);
uint32_t MRSTLFBGetDevVAddr(struct MRSTLFB_BUFFER_TAG *pBuffer);
extern int MRSTLFBAllocBuffer(struct drm_device *dev,
			IMG_UINT32 ui32Size, struct MRSTLFB_BUFFER_TAG **ppBuffer);
extern int MRSTLFBFreeBuffer(struct drm_device *dev,
			struct MRSTLFB_BUFFER_TAG **ppBuffer);

static void psb_user_framebuffer_destroy(struct drm_framebuffer *fb);
static int psb_user_framebuffer_create_handle(struct drm_framebuffer *fb,
					      struct drm_file *file_priv,
					      unsigned int *handle);

static const struct drm_framebuffer_funcs psb_fb_funcs = {
	.destroy = psb_user_framebuffer_destroy,
	.create_handle = psb_user_framebuffer_create_handle,
};

#define CMAP_TOHW(_val, _width) ((((_val) << (_width)) + 0x7FFF - (_val)) >> 16)

void *psbfb_vdc_reg(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv;
	dev_priv = (struct drm_psb_private *) dev->dev_private;
	return dev_priv->vdc_reg;
}
/*EXPORT_SYMBOL(psbfb_vdc_reg); */

static int psbfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			   unsigned blue, unsigned transp,
			   struct fb_info *info)
{
	struct psb_fbdev * fbdev = info->par;
	struct drm_framebuffer *fb = fbdev->psb_fb_helper.fb;
	uint32_t v;

	if (!fb)
		return -ENOMEM;

	if (regno > 255)
		return 1;

	red = CMAP_TOHW(red, info->var.red.length);
	blue = CMAP_TOHW(blue, info->var.blue.length);
	green = CMAP_TOHW(green, info->var.green.length);
	transp = CMAP_TOHW(transp, info->var.transp.length);

	v = (red << info->var.red.offset) |
	    (green << info->var.green.offset) |
	    (blue << info->var.blue.offset) |
	    (transp << info->var.transp.offset);

	if (regno < 16) {
		switch (fb->bits_per_pixel) {
		case 16:
			((uint32_t *) info->pseudo_palette)[regno] = v;
			break;
		case 24:
		case 32:
			((uint32_t *) info->pseudo_palette)[regno] = v;
			break;
		}
	}

	return 0;
}

static int psbfb_kms_off(struct drm_device *dev, int suspend)
{
	struct drm_framebuffer *fb = 0;
	struct psb_framebuffer * psbfb = to_psb_fb(fb);
	DRM_DEBUG_KMS("psbfb_kms_off_ioctl\n");

	mutex_lock(&dev->mode_config.mutex);
	list_for_each_entry(fb, &dev->mode_config.fb_list, head) {
		struct fb_info *info = psbfb->fbdev;

		if (suspend) {
			fb_set_suspend(info, 1);
			drm_fb_helper_blank(FB_BLANK_POWERDOWN, info);
		}
	}
	mutex_unlock(&dev->mode_config.mutex);
	return 0;
}

int psbfb_kms_off_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	int ret;

	console_lock();
	ret = psbfb_kms_off(dev, 0);
	console_unlock();

	return ret;
}

static int psbfb_kms_on(struct drm_device *dev, int resume)
{
	struct drm_framebuffer *fb = 0;
	struct psb_framebuffer * psbfb = to_psb_fb(fb);

	DRM_DEBUG_KMS("psbfb_kms_on_ioctl\n");

	mutex_lock(&dev->mode_config.mutex);
	list_for_each_entry(fb, &dev->mode_config.fb_list, head) {
		struct fb_info *info = psbfb->fbdev;

		if (resume) {
			fb_set_suspend(info, 0);
			drm_fb_helper_blank(FB_BLANK_UNBLANK, info);
		}
	}
	mutex_unlock(&dev->mode_config.mutex);

	return 0;
}

int psbfb_kms_on_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	int ret;

	console_lock();
	ret = psbfb_kms_on(dev, 0);
	console_unlock();
	drm_helper_disable_unused_functions(dev);
	return ret;
}

void psbfb_suspend(struct drm_device *dev)
{
	console_lock();
	psbfb_kms_off(dev, 1);
	console_unlock();
}

void psbfb_resume(struct drm_device *dev)
{
	console_lock();
	psbfb_kms_on(dev, 1);
	console_unlock();
	drm_helper_disable_unused_functions(dev);
}

static struct fb_ops psbfb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = drm_fb_helper_check_var,
	.fb_set_par = drm_fb_helper_set_par,
	.fb_blank = drm_fb_helper_blank,
	.fb_setcolreg = psbfb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};

static struct drm_framebuffer *psb_framebuffer_create
			(struct drm_device *dev, struct drm_mode_fb_cmd *r,
			 void *mm_private)
{
	struct psb_framebuffer *fb;
	int ret;

	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if (!fb)
		return NULL;

	ret = drm_framebuffer_init(dev, &fb->base, &psb_fb_funcs);

	if (ret)
		goto err;

	drm_helper_mode_fill_fb_struct(&fb->base, r);

	fb->pvrBO = mm_private;

	return &fb->base;

err:
	kfree(fb);
	return NULL;
}

static struct drm_framebuffer *psb_user_framebuffer_create
			(struct drm_device *dev, struct drm_file *filp,
			 struct drm_mode_fb_cmd *r)
{
	struct psb_framebuffer *psbfb;
	struct drm_framebuffer *fb;
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo = IMG_NULL;
	IMG_HANDLE hKernelMemInfo = (IMG_HANDLE)r->handle;
	struct drm_psb_private *dev_priv
		= (struct drm_psb_private *) dev->dev_private;
	struct psb_gtt *pg = dev_priv->pg;
	struct MRSTLFB_BUFFER_TAG *buffer;
	int ret;
	uint32_t offset;
	uint64_t size;

	ret = psb_get_meminfo_by_handle(hKernelMemInfo, &psKernelMemInfo);
	if (ret) {
		DRM_ERROR("Cannot get meminfo for handle 0x%x\n",
			  (IMG_UINT32)hKernelMemInfo);

		return NULL;
	}

	DRM_DEBUG_KMS("Got Kernel MemInfo for handle %x\n",
		  (IMG_UINT32)hKernelMemInfo);

	/* JB: TODO not drop, make smarter */
	size = psKernelMemInfo->uAllocSize;
	if (size < r->height * r->pitch)
		return NULL;

	/* JB: TODO not drop, refcount buffer */
	/* return psb_framebuffer_create(dev, r, bo); */

	fb = psb_framebuffer_create(dev, r, (void *)psKernelMemInfo);
	if (!fb) {
		DRM_ERROR("failed to allocate fb.\n");
		return NULL;
	}

	psbfb = to_psb_fb(fb);
	psbfb->size = size;
	psbfb->hKernelMemInfo = hKernelMemInfo;

	DRM_DEBUG_KMS("Mapping to gtt..., KernelMemInfo %p\n", psKernelMemInfo);

	buffer = (struct MRSTLFB_BUFFER_TAG *)dev_priv->fb_reloc;
	/*if not VRAM, map it into tt aperture*/
	if (psKernelMemInfo->pvLinAddrKM != pg->vram_addr &&
	    (!buffer || psKernelMemInfo->pvLinAddrKM != MRSTLFBGetCPUVAddr(buffer))) {
		ret = psb_gtt_map_meminfo(dev, hKernelMemInfo, &offset);
		if (ret) {
			DRM_ERROR("map meminfo for 0x%x failed\n",
				  (IMG_UINT32)hKernelMemInfo);
			return NULL;
		}
		psbfb->offset = (offset << PAGE_SHIFT);
	} else {
		if (buffer)
			psbfb->offset = MRSTLFBGetDevVAddr(buffer);
		else
			psbfb->offset = 0;
	}

	DRM_DEBUG_KMS("Mapping to gtt offset %x, KernelMemInfo %p for FB ID %d\n", psbfb->offset, psKernelMemInfo, fb->base.id);
	MRSTLFBHandleChangeFB(dev, psbfb);

	return fb;
}

static int psbfb_create(struct psb_fbdev * fbdev, struct drm_fb_helper_surface_size * sizes) 
{
	struct drm_device * dev = fbdev->psb_fb_helper.dev;
	struct drm_psb_private * dev_priv = (struct drm_psb_private *)dev->dev_private;
	struct psb_gtt *pg = dev_priv->pg;
	struct fb_info * info;
	struct drm_framebuffer *fb;
	struct psb_framebuffer * psbfb;
	struct drm_mode_fb_cmd mode_cmd;
	struct device * device = &dev->pdev->dev;
	struct MRSTLFB_BUFFER_TAG *buffer = NULL;
	int size, aligned_size;
	int ret, stride;

	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;

	DRM_DEBUG_KMS("psbfb_create called with w = %d h = %d\n", sizes->surface_width, sizes->surface_height);

	mode_cmd.bpp = 32;
        //HW requires pitch to be 64 byte aligned
        /*
	 * The framebuffer is used by PVR driver. And it expects that the 
         * stride is aligned to 32 in pixels. So we will first align the width to
	 * 32. As currently it uses the 32bpp in framebuffer, it can assure that
	 * it is aligned to 64 bytes.	
	 */
        stride = ALIGN(mode_cmd.width, 32);
        mode_cmd.pitch =  ALIGN(stride * ((mode_cmd.bpp + 1) / 8), 64);
        mode_cmd.depth = 24;

	size = mode_cmd.pitch * mode_cmd.height;
	aligned_size = ALIGN(size, PAGE_SIZE);

	mutex_lock(&dev->struct_mutex);

	if (aligned_size > pg->stolen_size) {
		/* 
		 * allocate new buffer if the request size is larger than
		 * the stolen memory size
		 */
		ret = MRSTLFBAllocBuffer(dev, aligned_size, &buffer);
		if (ret) {
			ret = -ENOMEM;
			goto out_err0;
		}
		dev_priv->fb_reloc = buffer;
	}

        fb = psb_framebuffer_create(dev, &mode_cmd, NULL);
        if (!fb) {
                ret = -ENOMEM;
                goto out_err1;
        }

        psbfb = to_psb_fb(fb);
        psbfb->size = size;
	if (buffer)
		psbfb->offset = MRSTLFBGetDevVAddr(buffer);

	info = framebuffer_alloc(sizeof(struct psb_fbdev), device);
	if(!info) {
		ret = -ENOMEM;
		goto out_err2;
	}

	info->par = fbdev;
	psbfb->fbdev = info;

	fbdev->psb_fb_helper.fb = fb;
	fbdev->psb_fb_helper.fbdev = info;
	fbdev->pfb = psbfb;

	strcpy(info->fix.id, "psbfb");

	info->flags = FBINFO_DEFAULT;
	info->fbops = &psbfb_ops;
	info->fix.smem_start = dev->mode_config.fb_base;
	info->fix.smem_len = size;

	if (buffer)
		info->screen_base = MRSTLFBGetCPUVAddr(buffer);
	else
		info->screen_base = (char *)pg->vram_addr;

	info->screen_size = size;

	/* kick off vesafb */
	if (pg->stolen_size) {       
		info->apertures = alloc_apertures(1);
		if (!info->apertures) {                
			ret = -ENOMEM;                 
			goto out_err2;        
		}
		info->apertures->ranges[0].base = dev->mode_config.fb_base;
		info->apertures->ranges[0].size = pg->stolen_size;                                                                   
	}
	  
	drm_fb_helper_fill_fix(info, fb->pitch, fb->depth);
	drm_fb_helper_fill_var(info, &fbdev->psb_fb_helper, sizes->fb_width, sizes->fb_height);

	info->fix.mmio_start = pci_resource_start(dev->pdev, 0);
	info->fix.mmio_len = pci_resource_len(dev->pdev, 0);

	info->pixmap.size = 64 * 1024;
	info->pixmap.buf_align = 8;
	info->pixmap.access_align = 32;
	info->pixmap.flags = FB_PIXMAP_SYSTEM;
	info->pixmap.scan_align = 1;

	DRM_DEBUG_KMS("fb depth is %d\n", fb->depth);
	DRM_DEBUG_KMS("   pitch is %d\n", fb->pitch);
	DRM_DEBUG_KMS("allocated %dx%d fb\n", psbfb->base.width, psbfb->base.height);	
	DRM_DEBUG_KMS("The GTT offset %x for FB ID %d\n", psbfb->offset, fb->base.id);	

	mutex_unlock(&dev->struct_mutex);

	return 0;
out_err2:
	fb->funcs->destroy(fb);
out_err1:
	MRSTLFBFreeBuffer(dev, &buffer);
out_err0:
	mutex_unlock(&dev->struct_mutex);
	return ret;
}

static void psbfb_gamma_set(struct drm_crtc *crtc, u16 red, u16 green, u16 blue, int regno)
{
	DRM_DEBUG_KMS("%s\n", __FUNCTION__);
}

static void psbfb_gamma_get(struct drm_crtc *crtc, u16 *red, u16 *green, u16 *blue, int regno)
{
	DRM_DEBUG_KMS("%s\n", __FUNCTION__);
}

static int psbfb_probe(struct drm_fb_helper *helper, struct drm_fb_helper_surface_size *sizes)
{
	struct psb_fbdev * psb_fbdev = (struct psb_fbdev *)helper;
	int new_fb = 0;
	int ret;

	DRM_DEBUG_KMS("%s\n", __FUNCTION__);

	if(!helper->fb) {
		ret = psbfb_create(psb_fbdev, sizes);
		if(ret) {
			return ret;
		}

		new_fb = 1;
	}

	return new_fb;
}

struct drm_fb_helper_funcs psb_fb_helper_funcs = {
	.gamma_set = psbfb_gamma_set,
	.gamma_get = psbfb_gamma_get,
	.fb_probe = psbfb_probe,
};

int psb_fbdev_destroy(struct drm_device * dev, struct psb_fbdev * fbdev)
{
	struct fb_info * info;
	struct psb_framebuffer * psbfb = fbdev->pfb;

	if(fbdev->psb_fb_helper.fbdev) {
		info = fbdev->psb_fb_helper.fbdev;
		unregister_framebuffer(info);
		iounmap(info->screen_base);
		framebuffer_release(info);
	}

	drm_fb_helper_fini(&fbdev->psb_fb_helper);

	drm_framebuffer_cleanup(&psbfb->base);
	
	return 0;
}

int psb_fbdev_init(struct drm_device * dev) 
{
	struct psb_fbdev * fbdev;
	struct drm_psb_private * dev_priv = 
		(struct drm_psb_private *)dev->dev_private;
	int num_crtc;
	
	fbdev = kzalloc(sizeof(struct psb_fbdev), GFP_KERNEL);
	if(!fbdev) {
		DRM_ERROR("no memory\n");
		return -ENOMEM;
	}

	dev_priv->fbdev = fbdev;
	fbdev->psb_fb_helper.funcs = &psb_fb_helper_funcs;

	if( IS_CDV(dev) ) {
		num_crtc = 2;
	}

	drm_fb_helper_init(dev, &fbdev->psb_fb_helper, num_crtc, INTELFB_CONN_LIMIT);

	drm_fb_helper_single_add_all_connectors(&fbdev->psb_fb_helper);
	drm_fb_helper_initial_config(&fbdev->psb_fb_helper, 32);
	return 0;
}

void psb_fbdev_fini(struct drm_device * dev)
{
	struct drm_psb_private * dev_priv = 
		(struct drm_psb_private *)dev->dev_private;

	if(!dev_priv->fbdev) {
		return;
	}

	psb_fbdev_destroy(dev, dev_priv->fbdev);
	MRSTLFBFreeBuffer(dev, (struct MRSTLFB_BUFFER_TAG **)(&dev_priv->fb_reloc));
	kfree(dev_priv->fbdev);
	dev_priv->fbdev = NULL;
}

static void psbfb_output_poll_changed(struct drm_device * dev)
{
	struct drm_psb_private * dev_priv = (struct drm_psb_private *)dev->dev_private;
	struct psb_fbdev * fbdev = (struct psb_fbdev *)dev_priv->fbdev;
	drm_fb_helper_hotplug_event(&fbdev->psb_fb_helper);
}

int psbfb_remove(struct drm_device *dev, struct drm_framebuffer *fb)
{
	struct fb_info *info;
	struct psb_framebuffer * psbfb = to_psb_fb(fb);


	info = psbfb->fbdev;
	psbfb->pvrBO = NULL;

	if (info) {
		framebuffer_release(info);
	}

	return 0;
}
/*EXPORT_SYMBOL(psbfb_remove); */

static int psb_user_framebuffer_create_handle(struct drm_framebuffer *fb,
					      struct drm_file *file_priv,
					      unsigned int *handle)
{
	/* JB: TODO currently we can't go from a bo to a handle with ttm */
	(void) file_priv;
	*handle = 0;
	return 0;
}

static void psb_user_framebuffer_destroy(struct drm_framebuffer *fb)
{
	struct drm_device *dev = fb->dev;
	struct psb_framebuffer *psbfb = to_psb_fb(fb);

	/*ummap gtt pages*/
	if (psbfb->hKernelMemInfo)
		psb_gtt_unmap_meminfo(dev, psbfb->hKernelMemInfo);
	if (psbfb->fbdev)
	{
		psbfb_remove(dev, fb);
	}
	DRM_DEBUG_KMS("Remove FB ID %d\n", fb->base.id);
	/* JB: TODO not drop, refcount buffer */
	drm_framebuffer_cleanup(fb);

	kfree(fb);
	/* update DC info by using the console fb when only one FB is left */
	if (dev->mode_config.num_fb == 1) {
		struct drm_psb_private *dev_priv =
	    		(struct drm_psb_private *) dev->dev_private;
		struct psb_fbdev * psPsbFBDev = (struct psb_fbdev *)dev_priv->fbdev;

        	fb = psPsbFBDev->psb_fb_helper.fb;
		psbfb = to_psb_fb(fb);
		MRSTLFBHandleChangeFB(dev, psbfb);
		DRM_DEBUG_KMS("Update the DC info by using the original console fb ID %d\n",
						fb->base.id);
	}
}

static const struct drm_mode_config_funcs psb_mode_funcs = {
	.fb_create = psb_user_framebuffer_create,
	.output_poll_changed = psbfb_output_poll_changed,
};

static void cdv_disable_vga(struct drm_device *dev)
{
	u8 sr1;
	u32 vga_reg;

	vga_reg = VGACNTRL;

	outb(1, VGA_SR_INDEX);
	sr1 = inb(VGA_SR_DATA);
	outb(sr1 | 1<<5, VGA_SR_DATA);
	udelay(300);

	REG_WRITE(vga_reg, VGA_DISP_DISABLE);
	REG_READ(vga_reg);
}

static void psb_setup_outputs(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct drm_connector *connector;

	PSB_DEBUG_ENTRY("\n");

	drm_mode_create_scaling_mode_property(dev);

	/* disable the VGA plane explicitly on CDV*/
	cdv_disable_vga(dev);

	/* Setting CRT for CDV connector */
	psb_intel_crt_init(dev, &dev_priv->mode_dev);

	/* Set up integrated LVDS */
	psb_intel_lvds_init(dev, &dev_priv->mode_dev);

	if (REG_READ(SDVOB) & SDVO_DETECTED) {
		//found = psb_intel_sdvo_init(dev, SDVOB);
		//if (!found)
		mdfld_hdmi_init(dev, &dev_priv->mode_dev, SDVOB);
		if (REG_READ(DP_B) & DP_DETECTED) {
			DRM_DEBUG_KMS("Probe DP on DP B port\n");
			psb_intel_dp_init(dev, &dev_priv->mode_dev, DP_B);
		}
	}
 
	if (REG_READ(SDVOC) & SDVO_DETECTED) {
		//found = psb_intel_sdvo_init(dev, SDVOC);
		//if (!found)
		mdfld_hdmi_init(dev, &dev_priv->mode_dev, SDVOC);
		if (REG_READ(DP_C) & DP_DETECTED) {
			DRM_DEBUG_KMS("Probe DP on DP B port\n");
			psb_intel_dp_init(dev, &dev_priv->mode_dev, DP_C);
		}
	}
		
	list_for_each_entry(connector, &dev->mode_config.connector_list,
			    head) {
		struct psb_intel_output *psb_intel_output =
		    to_psb_intel_output(connector);
		struct drm_encoder *encoder = &psb_intel_output->enc;
		int crtc_mask = 0, clone_mask = 0;

		/* valid crtcs */
		switch (psb_intel_output->type) {
		case INTEL_OUTPUT_ANALOG:
			crtc_mask = (1 << 0) | (1 << 1);
			clone_mask = (1 << INTEL_OUTPUT_ANALOG);
			break;
		case INTEL_OUTPUT_SDVO:
			crtc_mask = ((1 << 0) | (1 << 1));
			clone_mask = (1 << INTEL_OUTPUT_SDVO);
			break;
		case INTEL_OUTPUT_LVDS:
			crtc_mask = (1 << 1);
			clone_mask = (1 << INTEL_OUTPUT_LVDS);
			break;
		case INTEL_OUTPUT_HDMI:
			crtc_mask = (1 << 0) | (1 << 1);
			clone_mask = (1 << INTEL_OUTPUT_HDMI);
			break;
		case INTEL_OUTPUT_DISPLAYPORT:
			crtc_mask = (1 << 0) | (1 << 1);
			clone_mask = (1 << INTEL_OUTPUT_DISPLAYPORT);
			break;
		case INTEL_OUTPUT_EDP:
			crtc_mask = (1 << 1);
			clone_mask = (1 << INTEL_OUTPUT_EDP);
		}

		encoder->possible_crtcs = crtc_mask;
		encoder->possible_clones =
		    psb_intel_connector_clones(dev, clone_mask);

	}
	if (dev_priv->int_lvds_connector && dev_priv->int_edp_connector) {
		DRM_ERROR("VBIOS initialize the LVDS/eDP. Buggy VBIOS\n");
	}
}

static void *psb_bo_from_handle(struct drm_device *dev,
				struct drm_file *file_priv,
				unsigned int handle)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo = IMG_NULL;
	IMG_HANDLE hKernelMemInfo = (IMG_HANDLE)handle;
	int ret;

	ret = psb_get_meminfo_by_handle(hKernelMemInfo, &psKernelMemInfo);
	if (ret) {
		DRM_ERROR("Cannot get meminfo for handle 0x%x\n",
			  (IMG_UINT32)hKernelMemInfo);
		return NULL;
	}

	return (void *)psKernelMemInfo;
}

static size_t psb_bo_size(struct drm_device *dev, void *bof)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo	= (PVRSRV_KERNEL_MEM_INFO *)bof;
	return (size_t)psKernelMemInfo->uAllocSize;
}

static size_t psb_bo_offset(struct drm_device *dev, void *bof)
{
	struct psb_framebuffer *psbfb
		= (struct psb_framebuffer *)bof;

	return (size_t)psbfb->offset;
}

static int psb_bo_pin_for_scanout(struct drm_device *dev, void *bo)
{
	 return 0;
}

static int psb_bo_unpin_for_scanout(struct drm_device *dev, void *bo) 
{
	return 0;
}

/* Cedarview display clock gating */
void psb_intel_clock_gating (struct drm_device *dev)
{
	uint32_t reg_value;
	reg_value = REG_READ(DSPCLK_GATE_D);

	reg_value |= (DPUNIT_PIPEB_GATE_DISABLE |
			DPUNIT_PIPEA_GATE_DISABLE |
			DPCUNIT_CLOCK_GATE_DISABLE |
			DPLSUNIT_CLOCK_GATE_DISABLE |
			DPOUNIT_CLOCK_GATE_DISABLE |
		 	DPIOUNIT_CLOCK_GATE_DISABLE);	

	REG_WRITE(DSPCLK_GATE_D, reg_value);

	udelay(500);		
}
/* Cedarview display workarounds */
void psb_intel_display_wa (struct drm_device *dev)
{
	/* Disable bonus launch.
	 *
	 * Copy Ben's mail below.
	 * 	On Win 7 we are experiencing possible flicker issue related to processor
	 *	arbitration bug.
	 *
	 *	CPU and GPU competes for memory and display misses updates and flickers.
	 *	Worst with dual core, dual displays.
	 *
	 *	Fixes were done to Win 7 gfx driver to disable a feature called Bonus
	 *	Launch to work around the issue, by degrading performance.
	 * */

	CDV_MSG_WRITE32(3, 0x30, 0x08027108);
}

void psb_modeset_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct psb_intel_mode_device *mode_dev = &dev_priv->mode_dev;
	int i;

	PSB_DEBUG_ENTRY("\n");
	/* Init mm functions */
	mode_dev->bo_from_handle = psb_bo_from_handle;
	mode_dev->bo_size = psb_bo_size;
	mode_dev->bo_offset = psb_bo_offset;
	mode_dev->bo_pin_for_scanout = psb_bo_pin_for_scanout;
	mode_dev->bo_unpin_for_scanout = psb_bo_unpin_for_scanout;

	drm_mode_config_init(dev);

	psb_intel_clock_gating(dev);

	dev->mode_config.min_width = 0;
	dev->mode_config.min_height = 0;

	dev->mode_config.funcs = (void *) &psb_mode_funcs;

	/* set memory base */
	/* MRST and PSB should use BAR 2*/
	pci_read_config_dword(dev->pdev, PSB_BSM, (uint32_t *) &(dev->mode_config.fb_base));

	for (i = 0; i < dev_priv->num_pipe; i++)
		psb_intel_crtc_init(dev, i, mode_dev);

	dev->mode_config.max_width = 8192;
	dev->mode_config.max_height = 8192;

	psb_setup_outputs(dev);

	if (!dev_priv->ovl_buf) {
		struct MRSTLFB_BUFFER_TAG *ovl_buffer = NULL;
		if (MRSTLFBAllocBuffer(dev, 64 * 1024, &ovl_buffer)) {
			printk(KERN_ERR "Can't allocate GTT memory for overlay\n");
			dev_priv->ovl_offset = 0;
		} else {
			dev_priv->ovl_buf = ovl_buffer;
			memset(MRSTLFBGetCPUVAddr(ovl_buffer), 0, 64 * 1024);
			dev_priv->ovl_offset = MRSTLFBGetDevVAddr(ovl_buffer);
			printk(KERN_INFO "Overlay GTT address is %x\n", dev_priv->ovl_offset);
		}
	}

	psb_intel_display_wa(dev);

	/* setup fbs */
	/* drm_initial_config(dev); */
}

void psb_modeset_cleanup(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	mutex_lock(&dev->struct_mutex);

	drm_kms_helper_poll_fini(dev);
	psb_fbdev_fini(dev);

	if (dev_priv->ovl_buf) {
		struct MRSTLFB_BUFFER_TAG *ovl_buffer = dev_priv->ovl_buf;
		MRSTLFBFreeBuffer(dev, &ovl_buffer);
		dev_priv->ovl_buf = NULL;
	}

	drm_mode_config_cleanup(dev);

	mutex_unlock(&dev->struct_mutex);
}
