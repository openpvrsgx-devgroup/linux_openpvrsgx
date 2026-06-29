/**********************************************************************
 * Copyright (c) 2002-2010, Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/
#define MODULE_NAME hal.pvr3dd

#include "drm_emgd_private.h"

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "emgd_dc.h"


#if !defined(SUPPORT_DRI_DRM)
#error "SUPPORT_DRI_DRM must be set"
#endif


/* The following macros exist to help the PVR services code name/find and call
 * the Init and Cleanup functions below.  The DISPLAY_CONTROLLER macro must be
 * set in the build, so that the PVR service code can use the same magic to
 * discover the name of the Init and Cleanup functions for this driver.
 */
#if !defined(DISPLAY_CONTROLLER)
#define DISPLAY_CONTROLLER emgd_dc
#endif
#define MAKENAME_HELPER(x, y) x ## y
#define	MAKENAME2(x, y) MAKENAME_HELPER(x, y)
#define	MAKENAME(x) MAKENAME2(DISPLAY_CONTROLLER, x)


/* The following tells GCC to not warn about ununsed functions: */
#define unref__ __attribute__ ((unused))


/**
 * Function that initializes and starts this 3rd-party display driver.  This is
 * called by the PVR services when it starts.
 *
 * @param dev (IN) The drm_device associated with this driver.
 */
int MAKENAME(_Init)(struct drm_device unref__ *dev)
{
	EMGD_TRACE_ENTER;

	if (emgddc_init(dev) != EMGD_OK) {
		EMGD_ERROR_EXIT(DRVNAME " init failed (" DISPLAY_DEVICE_NAME ")\n");
		return -ENODEV;
	}

	EMGD_TRACE_EXIT;
	return 0;
}


/**
 * Function that shuts-down and de-initializes this 3rd-party display driver.
 * This is called by the PVR services when it ends.
 *
 * @param dev (IN) The drm_device associated with this driver.
 */
void MAKENAME(_Cleanup)(struct drm_device unref__ *dev)
{
	EMGD_TRACE_ENTER;

	if (emgddc_deinit() != EMGD_OK) {
		EMGD_ERROR_EXIT(DRVNAME " de-init failed (" DISPLAY_DEVICE_NAME ")\n");
	}

	EMGD_TRACE_EXIT;
}


/**
 * Set the display to the surface specified by buffer.
 *
 * @param swap_chain (IN) Pointer to the swap chain associated with buffer.
 * @param buffer (IN) Pointer to the buffer to display.
 */
void emgddc_flip(emgddc_swapchain_t *swap_chain, emgddc_buffer_t *buffer)
{
	drm_emgd_priv_t *priv = buffer->priv;
	igd_context_t *context = priv->context;
	igd_surface_t surf;
	int ret;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("Parameters: swap_chain=0x%p", swap_chain);
	EMGD_DEBUG("  buffer=0x%p, *buffer->offset=0x%08lx",
		buffer, buffer->offset);

	if (EMGD_FALSE == swap_chain->valid) {
		EMGD_DEBUG("Not flipping--swap chain invalidated by a mode change.");
		EMGD_TRACE_EXIT;
		return;
	}

	surf.offset = buffer->offset;
	surf.pitch = buffer->pitch;
	surf.width = buffer->width;
	surf.height = buffer->height;
	surf.pixel_format = buffer->pixel_format;
	surf.flags = IGD_SURFACE_RENDER | IGD_SURFACE_DISPLAY;

	/* Flip the primary surface.  Select a different EMGD display depending on
	 * the DC & devinfo:
	 */
	if (IGD_DC_EXTENDED(priv->dc) &&
		(swap_chain->devinfo->which_devinfo == 1))  {
		ret = context->dispatch.set_surface(priv->secondary,
			IGD_PRIORITY_NORMAL, IGD_BUFFER_DISPLAY, &surf, NULL, 0);
		if (ret != 0) {
			printk(KERN_ERR "%s: set_surface() returned %d", __FUNCTION__, ret);
		}
	} else {
		ret = context->dispatch.set_surface(priv->primary,
			IGD_PRIORITY_NORMAL, IGD_BUFFER_DISPLAY, &surf, NULL, 0);
		if (ret != 0) {
			printk(KERN_ERR "%s: set_surface() returned %d", __FUNCTION__, ret);
		}
	}

	/* Flip the secondary surface: */
	if (IGD_DC_CLONE(priv->dc)) {
		/* If in clone mode, flip the other pipe too: */
		ret = context->dispatch.set_surface(priv->secondary,
			IGD_PRIORITY_NORMAL, IGD_BUFFER_DISPLAY, &surf, NULL, 0);
		if (ret != 0) {
			printk(KERN_ERR "%s: set_surface() returned %d", __FUNCTION__, ret);
		}
	}


	EMGD_TRACE_EXIT;
} /* emgddc_flip() */
