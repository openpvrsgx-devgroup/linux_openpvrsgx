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

#include <linux/spinlock.h>

#include "drm_emgd_private.h"

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "emgd_dc.h"
#include "emgd_drm.h"


#if !defined(SUPPORT_DRI_DRM)
#error "SUPPORT_DRI_DRM must be set"
#endif


/* Function to get the PVR services jump table */
extern IMG_BOOL PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE *jtable);

#ifdef SUPPORT_FB_EVENTS
static emgd_error_t enable_event_notification(emgddc_devinfo_t *devinfo);
static emgd_error_t disable_event_notification(emgddc_devinfo_t *devinfo);
#endif /* SUPPORT_FB_EVENTS */
static void flush_flip_queue(emgddc_swapchain_t *swap_chain);
static PVRSRV_ERROR do_mode_change(igd_context_t *context,
		emgddc_devinfo_t *devinfo,
		drm_emgd_priv_t *priv,
		DISPLAY_SURF_ATTRIBUTES *dst_surf_attrib);


/* Special value used to register with the PVR services command queue: */
#define EMGDDC_COMMAND_COUNT		1


/**
 * This is a pointer to the global emgddc_devinfo_t structure, used in various
 * parts of this file.
 */
static emgddc_devinfo_t *global_devinfo[] = {NULL, NULL};


/**
 * Pairs of equivalent pixel formats, in EMGD and PVR formats:
 */
emgddc_pixelformat_translator_t known_pfs[] = {
	/* 1 Byte-per-pixel [A]RGB Pixel Formats: */
	{IGD_PF_ARGB8_INDEXED, PVRSRV_PIXEL_FORMAT_PAL8},   /* IMG numbered */
	{IGD_PF_ARGB4_INDEXED, PVRSRV_PIXEL_FORMAT_PAL4},   /* IMG numbered */

	/* 2 Byte-per-pixel [A]RGB Pixel Formats: */
	{IGD_PF_ARGB16_4444, PVRSRV_PIXEL_FORMAT_ARGB4444}, /* IMG# & PVR2D-known */
	{IGD_PF_ARGB16_1555, PVRSRV_PIXEL_FORMAT_ARGB1555}, /* IMG# & PVR2D-known */
	{IGD_PF_RGB16_565, PVRSRV_PIXEL_FORMAT_RGB565},     /* IMG# & PVR2D-known */
	{IGD_PF_xRGB16_555, PVRSRV_PIXEL_FORMAT_RGB555},    /* IMG numbered */

	/* 3 Byte-per-pixel [A]RGB Pixel Formats: */
	{IGD_PF_RGB24, PVRSRV_PIXEL_FORMAT_RGB888},         /* IMG numbered */

	/* 4 Byte-per-pixel [A]RGB Pixel Formats: */
	{IGD_PF_xRGB32_8888, PVRSRV_PIXEL_FORMAT_XRGB8888}, /* IMG numbered */
	/* a.k.a. IGD_PF_ARGB32_8888 */
	{IGD_PF_ARGB32, PVRSRV_PIXEL_FORMAT_ARGB8888},      /* IMG# & PVR2D-known */
	{IGD_PF_xBGR32_8888, PVRSRV_PIXEL_FORMAT_XBGR8888}, /* IMG numbered */
	{IGD_PF_ABGR32_8888, PVRSRV_PIXEL_FORMAT_ABGR8888}, /* IMG numbered */

	/* YUV Packed Pixel Formats: */
	{IGD_PF_YUV422_PACKED_YUY2, PVRSRV_PIXEL_FORMAT_YUY2},
	{IGD_PF_YUV422_PACKED_YVYU, PVRSRV_PIXEL_FORMAT_YVYU},
	{IGD_PF_YUV422_PACKED_UYVY, PVRSRV_PIXEL_FORMAT_UYVY},
	{IGD_PF_YUV422_PACKED_VYUY, PVRSRV_PIXEL_FORMAT_VYUY},
	/* UNKNOWN to IMG
	{IGD_PF_YUV411_PACKED_Y41P, PVRSRV_PIXEL_FORMAT_UNKNOWN},
	*/

	/* YUV Planar Pixel Formats: */
	/* a.k.a. IGD_PF_YUV420_PLANAR_IYUV */
	{IGD_PF_YUV420_PLANAR_I420, PVRSRV_PIXEL_FORMAT_I420},/* IMG numbered */
	{IGD_PF_YUV420_PLANAR_YV12, PVRSRV_PIXEL_FORMAT_YV12},/* IMG numbered */
	/* UNKNOWN to IMG
	{IGD_PF_YUV410_PLANAR_YVU9, PVRSRV_PIXEL_FORMAT_UNKNOWN},
	*/
	{IGD_PF_YUV420_PLANAR_NV12, PVRSRV_PIXEL_FORMAT_NV12},
};
unsigned int num_known_pfs =
	sizeof(known_pfs) / sizeof(emgddc_pixelformat_translator_t);


/**
 * Translate EMGD-specific pixel formats into PVR-specific pixel formats.
 *
 * @param emgd_pf (IN) EMGD-specific pixel format.
 * @return Translated PVR-specific pixel format.
 */
static unsigned long pvr2emgd_pf(PVRSRV_PIXEL_FORMAT pvr_pf)
{
	int i;
	for (i = 0 ; i < num_known_pfs ; i++) {
		if (known_pfs[i].pvr_pf == pvr_pf) {
			return known_pfs[i].emgd_pf;
		}
	}

	/* If we get to here, we didn't find a known PVR pixel format: */
	return IGD_PF_UNKNOWN;
}


/**
 * Translate EMGD-specific pixel formats into PVR-specific pixel formats.
 *
 * @param emgd_pf (IN) EMGD-specific pixel format.
 * @return Translated PVR-specific pixel format.
 */
static PVRSRV_PIXEL_FORMAT emgd2pvr_pf(unsigned long emgd_pf)
{
	int i;
	for (i = 0 ; i < num_known_pfs ; i++) {
		if (known_pfs[i].emgd_pf == emgd_pf) {
			return known_pfs[i].pvr_pf;
		}
	}

	/* If we get to here, we didn't find a known PVR pixel format: */
	return IGD_PF_UNKNOWN;
}


/**
 * Determines if the user-space-provided pointer (to a devinfo) is valid.
 *
 * @param devinfo (IN) The user-space-provided pointer to a devinfo.
 */
static int is_valid_devinfo(emgddc_devinfo_t *devinfo)
{
	if ((devinfo == global_devinfo[0]) || (devinfo == global_devinfo[1])) {
		return 1;
	} else {
		return 0;
	}
} /* is_valid_devinfo() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnOpenDCDevice()
 * function.  This function is called when a client wants to use PVR services
 * with the specified device.
 *
 * @param device_id (IN) The device_id associated with this device (i.e.
 * obtained when emgddc_init() called
 * PVRSRV_DC_DISP2SRV_KMJTABLE.pfnPVRSRVRegisterDCDevice()).
 * @param device_h (OUT) The handle for this device (an opaque pointer to
 * devinfo).
 * @param system_buffer_sync_data (IN) Sync data for this device's system
 * buffer.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 device_id,
	IMG_HANDLE *device_h,
	PVRSRV_SYNC_DATA* system_buffer_sync_data)
{
	emgddc_devinfo_t *devinfo;

	EMGD_TRACE_ENTER;


	UNREFERENCED_PARAMETER(device_id);

	/* Look up the device (for DIH/Extended mode): */
	if (device_id == global_devinfo[0]->device_id) {
		devinfo = global_devinfo[0];
		EMGD_DEBUG("devinfo = global_devinfo[0] = 0x%p", devinfo);
	} else if (device_id == global_devinfo[1]->device_id) {
		devinfo = global_devinfo[1];
		EMGD_DEBUG("devinfo = global_devinfo[1] = 0x%p", devinfo);
	} else {
		printk(KERN_ERR "[EMGD] OpenDCDevice() called with unknown device ID "
			"%lu\n", device_id);
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	devinfo->system_buffer.sync_data = system_buffer_sync_data;
	*device_h = (IMG_HANDLE) devinfo;


	EMGD_TRACE_EXIT;

	return PVRSRV_OK;
} /* OpenDCDevice() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnCloseDCDevice()
 * function.  This function is called when a client is finished using PVR
 * services with the specified device.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE device_h)
{
	EMGD_TRACE_STUB;

	EMGD_DEBUG("device_h = 0x%p", device_h);
	if (!is_valid_devinfo((emgddc_devinfo_t *) device_h)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p)\n",
			__FUNCTION__, device_h);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
} /* CloseDCDevice() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnEnumDCFormats()
 * function.  This function is called when a client wants to determine the
 * pixel format currently being used with, and potentially, to determine which
 * pixel formats can be used with the specified device.  The first entry in the
 * array is the current pixel format.
 *
 * Note: this function must be called twice.  The first time, the formats
 * parameter is set to NULL, and the client is trying to determine the number
 * of pixel formats.  The second time, the formats parameter is non-NULL, and
 * points to enough memory for num_formats-worth of pixel formats.  Note: this
 * creates a small window of time between calls where a mode change could
 * occur; the probability is considered so small, as to not be a worry.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param num_formats (OUT) The number of pixel formats for this device.
 * @param format (IN/OUT) An array of the pixel formats for this device (ignore
 * if NULL).
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE device_h,
	IMG_UINT32 *num_formats,
	DISPLAY_FORMAT *format)
{
	emgddc_devinfo_t	*devinfo;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!device_h || !num_formats) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	if (!is_valid_devinfo(devinfo)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p)\n",
			__FUNCTION__, devinfo);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*num_formats = devinfo->num_formats;

	if (format) {
		unsigned long i;

		for (i = 0 ; i < devinfo->num_formats ; i++) {
			format[i] = devinfo->display_format_list[i];
		}
	}


	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* EnumDCFormats() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnEnumDCDims() function.
 * This function is called when a client wants to determine the current
 * dimensions (similar to an EMGD mode, but just the dimensions) of, and
 * potentially, to determine the possible dimensions that can be used with this
 * device.  The first entry in the array is the current dimension.
 *
 * Note: this function must be called twice.  The first time, the dims
 * parameter is set to NULL, and the client is trying to determine the number
 * of dimensions.  The second time, the dims parameter is non-NULL, and points
 * to enough memory for num_dims-worth of dimensions.  Note: this
 * creates a small window of time between calls where a mode change could
 * occur; the probability is considered so small, as to not be a worry.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param format (IN) A pointer to a pixel format (unused).
 * @param num_dims (OUT) The number of dimensions for this device.
 * @param dims (IN/OUT) An array of the dimensions for this device (ignore
 * if NULL).
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR EnumDCDims(IMG_HANDLE device_h,
	DISPLAY_FORMAT *format,
	IMG_UINT32 *num_dims,
	DISPLAY_DIMS *dims)
{
	emgddc_devinfo_t	*devinfo;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!device_h || !format || !num_dims) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	if (!is_valid_devinfo(devinfo)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p)\n",
			__FUNCTION__, devinfo);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*num_dims = devinfo->num_dims;

	if (dims) {
		unsigned long i;

		for (i = 0 ; i < devinfo->num_dims ; i++) {
			dims[i] = devinfo->display_dim_list[i];
		}
	}


	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* EnumDCDims() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnGetDCSystemBuffer()
 * function.  This function returns a handle to the system buffer
 * (a.k.a. "frame buffer" or "front buffer") of the specified device.  The
 * handle is an opaque pointer to the buffer.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param buffer_h (OUT) The handle for this buffer (an opaque pointer to
 * devinfo->system_buffer).
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE device_h, IMG_HANDLE *buffer_h)
{
	emgddc_devinfo_t	*devinfo;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!device_h || !buffer_h) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	if (!is_valid_devinfo(devinfo)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p)\n",
			__FUNCTION__, devinfo);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*buffer_h = (IMG_HANDLE) &devinfo->system_buffer;


	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* GetDCSystemBuffer() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnGetDCInfo() function.
 * This function returns a pointer to the DISPLAY_INFO structure associated
 * with this device, which contains the driver's name, and information about
 * swap chains that can be created (i.e. all static information).
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param dc_info (OUT) The DISPLAY_INFO structure associated with this device.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR GetDCInfo(IMG_HANDLE device_h, DISPLAY_INFO *dc_info)
{
	emgddc_devinfo_t	*devinfo;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!device_h || !dc_info) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	if (!is_valid_devinfo(devinfo)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p)\n",
			__FUNCTION__, devinfo);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*dc_info = devinfo->display_info;


	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* GetDCInfo() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnGetDCBufferAddr()
 * function.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param buffer_h (IN) The handle of a buffer (an opaque pointer to an
 * emgddc_buffer_t) to return information of.
 * @param phys_addr (OUT) A pointer to an array pointer of page addresses of
 * the buffer.  For many devices, this would be a pointer to the physical
 * address of a contiguous set of memory associated with the buffer, but since
 * EMGD doesn't use contiguous memory for a buffer, it returns the array of
 * addresses.
 * @param fb_size (OUT) The size (in bytes) of the buffer.
 * @param virt_addr (OUT) A pointer to the virtual address (in kernel space) of
 * the buffer.
 * @param os_map_info_h (OUT) Ununsed by PVR services.
 * @param is_contiguous (OUT) A pointer to a boolean that is set to IMG_FALSE,
 * because EMGD uses non-contiguous pages of memory for buffers.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE device_h,
	IMG_HANDLE buffer_h,
	IMG_SYS_PHYADDR **phys_addr,
	IMG_UINT32 *fb_size,
	IMG_VOID **virt_addr,
	IMG_HANDLE *os_map_info_h,
	IMG_BOOL *is_contiguous)
{
	emgddc_devinfo_t *devinfo;
	igd_context_t *context;
	emgddc_buffer_t *system_buffer;
	unsigned long page_count = 0;
	int ret;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!device_h) {
		printk(KERN_ERR "[EMGD] %s() Null device handle.\n", __FUNCTION__);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	if (!is_valid_devinfo(devinfo)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p)\n",
			__FUNCTION__, devinfo);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	context = devinfo->priv->context;
	if (!context || (context->dispatch.gmm_get_page_list == NULL)) {
		printk(KERN_ERR "[EMGD] %s() HAL not configured.\n", __FUNCTION__);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!buffer_h) {
		printk(KERN_ERR "[EMGD] %s() Null buffer handle.\n", __FUNCTION__);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	system_buffer = (emgddc_buffer_t *) buffer_h;

	if (!phys_addr) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	EMGD_DEBUG("  phys_addr = 0x%p", phys_addr);
	EMGD_DEBUG("  *phys_addr = 0x%p", (*phys_addr));

	if ((ret = context->dispatch.gmm_get_page_list(system_buffer->offset,
		(unsigned long **) phys_addr, &page_count)) != 0) {
		printk(KERN_ERR"Cannot get the page addresses for the buffer at offset "
			"0x%08lx\n", system_buffer->offset);
		EMGD_TRACE_EXIT;
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	EMGD_DEBUG("  phys_addr = 0x%p", phys_addr);
	EMGD_DEBUG("  *phys_addr = 0x%p", (*phys_addr));
	EMGD_DEBUG("  (*phys_addr)->uiAddr = 0x%lx", (*phys_addr)->uiAddr);

	if (!fb_size) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (fb_size) {
		*fb_size = (IMG_UINT32) system_buffer->size;
		EMGD_DEBUG("  fb_size = 0x%lx", *fb_size);
	}

	if (virt_addr) {
		*virt_addr = system_buffer->virt_addr;
		EMGD_DEBUG("  virt_addr = 0x%p", *virt_addr);
	}

	/* Note: this value is ignored by the PVR services code: */
	if (os_map_info_h) {
		*os_map_info_h = (IMG_HANDLE)system_buffer->offset;
		EMGD_DEBUG("  os_map_info_h = 0x%p", *os_map_info_h);
	}

	/*
	 * Other than cursor, memory allocations are not contiguous pages
	 */
	if (is_contiguous) {
		if(system_buffer->is_contiguous == IMG_TRUE)
		{	*is_contiguous = IMG_TRUE;
		
		}
		else{
			*is_contiguous = IMG_FALSE;
		
		}
	}

	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* GetDCBufferAddr() */


/**
 * Determines if the user-space-provided pointers (to a devinfo and swap chain)
 * are valid.
 *
 * @param devinfo (IN) The user-space-provided pointer to a devinfo.
 * @param swap_chain (IN) The swap chain to add to the list.
 * @param must_be_flipable (IN) Non-zero if swap_chain must be flip-able.
 * @return non-zero if valid, zero if not.
 */
static int is_valid_swap_chain(emgddc_devinfo_t *devinfo,
	emgddc_swapchain_t *swap_chain, int must_be_flipable)
{
	if (!is_valid_devinfo(devinfo)) {
		return 0;
	} else {
		/* Search both lists for this swap chain: */
		emgddc_swapchain_t *swap = devinfo->flipable_swapchains;
		while (swap) {
			if (swap == swap_chain) {
				return 1;
			}
			swap = swap->next;
		}
		if (!must_be_flipable) {
			swap = devinfo->pixmap_swapchains;
			while (swap) {
				if (swap == swap_chain) {
					return 1;
				}
				swap = swap->next;
			}
		}
	}
	/* We didn't find this swap chain pointer, so it's not valid: */
	return 0;
} /* is_valid_swap_chain() */


/**
 * Called by CreateDCSwapChain() to add a new swap chain to one of the lists of
 * swap chains.
 *
 * @param list (IN) A pointer to a list of swap chains.
 * @param swap_chain (IN) The swap chain to add to the list.
 */
static void add_swap_chain_to_list(emgddc_swapchain_t **list,
	emgddc_swapchain_t *swap_chain)
{
	/* It's simplest to add to the front of the list: */
	if (*list == NULL) {
		*list = swap_chain;
	} else {
		swap_chain->next = *list;
		*list = swap_chain;
	}
} /* add_swap_chain_to_list() */


/**
 * Called by DestroyDCSwapChain() to remove an about-to-be-deleted swap chain
 * from one of the lists of swap chains.
 *
 * @param list (IN) A pointer to a list of swap chains.
 * @param swap_chain (IN) The swap chain to remove from the list.
 */
static void remove_swap_chain_from_list(emgddc_swapchain_t **list,
	emgddc_swapchain_t *swap_chain)
{
	emgddc_swapchain_t *swap, *prev;

	swap = *list;
	prev = *list;
	while (swap) {
		if (swap == swap_chain) {
			/* Found match */
			if (swap == *list) {
				*list = swap->next;
			} else {
				prev->next = swap->next;
			}
			break;
		}
		prev = swap;
		swap = swap->next;
	}
} /* remove_swap_chain_from_list() */


/**
 * Called by either CreateDCSwapChain() or DestroyDCSwapChain() to free all GMM
 * and kernel space memory memory of the specified swap chain.
 *
 * @param swap_chain (IN) The swap chain to free.
 * @param context (IN) The EMGD context to use to call gmm_free().
 */
static void free_swap_chain(emgddc_swapchain_t *swap_chain,
	igd_context_t *context)
{
	emgddc_buffer_t *buffers;
	int i=0;

	/*
	 * Free and unmap the buffers.  Must ensure that the HAL is running before
	 * calling it, and ensure that we don't free/unmap the first buffer if is
	 * actually the frame buffer.
	 */
	if (swap_chain->devinfo->priv->hal_running) {
		buffers = swap_chain->buffers;
			
		for (i = 0 ; i < swap_chain->buffer_count ; i++) {
			if(!buffers[i].is_contiguous){	
				if (!buffers[i].is_fb) {	
					if (buffers[i].virt_addr) {
						context->dispatch.gmm_unmap(buffers[i].virt_addr);
					}
					if (buffers[i].offset) {
						context->dispatch.gmm_free(buffers[i].offset);
					}
				}				
			}
			else{
				context->dispatch.gmm_unmap_ci((unsigned long)buffers[i].virt_addr);
			}
				
		}
	}

	if (swap_chain->flip_queue) {
		OS_FREE(swap_chain->flip_queue);
	}
	OS_FREE(swap_chain->buffers);
	OS_FREE(swap_chain);

	EMGD_TRACE_EXIT;
} /* free_swap_chain() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnCreateDCSwapChain()
 * function.  This function was originally designed to create a flip-able swap
 * chain on the specified device, but has also been augmented to allow the X
 * driver to create pixmaps or other buffers in GTT memory.  A flip-able swap
 * chain consists of a front buffer (the "system buffer, a.k.a. the "frame
 * buffer") and one or more back buffers (therefore, the number of buffers
 * includes the frame buffer).
 *
 * Note: Only full-screen flipping is supported (the hardware is pointed at one
 * complete buffer at a time).  There is no support for the
 * SetDC{Src|Dst}Rect() functions to define a smaller region of the display for
 * the buffers/flipping.
 *
 * Note: The DDK documentation says that the mode should be changed if the
 * dimensions and pixel format do not match the current dimensions and pixel
 * format.  However, this can only be supported if the X server isn't running
 * (which is in charge of changing modes when it is running).  If the X server
 * is running, an error is returned unless the current dimensions and pixel
 * format are specified.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param flags (IN) Unused.
 * @param dst_surf_attrib (IN) While not described in the DDK documentation,
 * this presumably specifies the desired dimensions and pixel format of the
 * front buffer.
 * @param src_surf_attrib (IN) Specifies the desired dimensions and pixel
 * format of the back buffers.
 * @param buffer_count (IN) Number of buffers required in this swap chain.
 * @param sync_data (IN) While not described in the DDK documentation,
 * this is an array of sync data for each buffer.
 * @param oem_flags (IN) Unused.
 * @param swap_chain_h (OUT) The handle for this swap chain (an opaque pointer
 * to a emgddc_swapchain_t).
 * @param swap_chain_id (OUT) Unused.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE device_h,
	IMG_UINT32 flags,
	DISPLAY_SURF_ATTRIBUTES *dst_surf_attrib,
	DISPLAY_SURF_ATTRIBUTES *src_surf_attrib,
	IMG_UINT32 buffer_count,
	PVRSRV_SYNC_DATA **sync_data,
	IMG_UINT32 oem_flags,
	IMG_HANDLE *swap_chain_h,
	IMG_UINT32 *swap_chain_id)
{
	emgddc_devinfo_t	*devinfo;
	emgddc_swapchain_t *swap_chain;
	emgddc_buffer_t *buffers;
	IMG_UINT32 i;
	emgddc_flip_queue_item_t *flip_queue = NULL;
	unsigned long lock_flags;

	struct drm_device* drm_dev;
	drm_emgd_priv_t *priv;
	igd_context_t *context;
	igd_dispatch_t *dispatch;
	int flipable;

	IMG_UINT32 ci_offset=0;


	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p, buffer_count = %lu", device_h, buffer_count);
	EMGD_DEBUG("flags = 0x%08lx, oem_flags = 0x%08lx", flags, oem_flags);
	
	if(flags & PVR2D_CREATE_FLIPCHAIN_CI)
	{	
		ci_offset = src_surf_attrib->ui32Reseved;		
		
	}
		
	/*
	 * Check the parameters and dependencies:
	 */
	if (!device_h || !dst_surf_attrib || !src_surf_attrib || !sync_data ||
		!swap_chain_h) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*
	 * The oem_flags will determine what type of swapchain this is.  The
	 * following types are for a non-flip-able swap chain (e.g. for a pixmap):
	 *
	 *  PVR2D_CREATE_FLIPCHAIN_OEMDISPLAY - General purpose displayable
	 *  PVR2D_CREATE_FLIPCHAIN_OEMGENERAL - General purpose non-displayable
	 *  PVR2D_CREATE_FLIPCHAIN_OEMOVERLAY - Overlay
	 *
	 *  Currently, if none of these flags are set, assume this is going
	 *  to create a set of back buffers, or a "flip-able" swap chain:
	 *
	 *  PVR2D_CREATE_FLIPCHAIN_OEMFLIPCHAIN - Flip-able buffers
	 */
	/* Is this is an OEM call (I.E. allocating a buffer)? */
	if ((oem_flags & (PVR2D_CREATE_FLIPCHAIN_OEMDISPLAY |
				PVR2D_CREATE_FLIPCHAIN_OEMGENERAL |
				PVR2D_CREATE_FLIPCHAIN_OEMOVERLAY |
				PVR2D_CREATE_FLIPCHAIN_CI))) {
		flipable = 0;
		
	} else {
		/*
		 * If this is suppose to be an actual flip-able swap chain, then
		 * make sure there are at least 2 buffers.
		 */
		if (buffer_count < 2) {
			return PVRSRV_ERROR_TOO_FEW_BUFFERS;
		}
		flipable = 1;
		
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	if (!is_valid_devinfo(devinfo)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p)\n",
			__FUNCTION__, devinfo);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	drm_dev = devinfo->drm_device;
	priv = drm_dev->dev_private;
	context = priv->context;
	dispatch = &(context->dispatch);


	if (buffer_count > devinfo->display_info.ui32MaxSwapChainBuffers) {
		return PVRSRV_ERROR_TOOMANYBUFFERS;
	}

	/* Ensure the source & destination attributes match each other: */
	if ((dst_surf_attrib->pixelformat != src_surf_attrib->pixelformat) ||
		(dst_surf_attrib->sDims.ui32ByteStride !=
		src_surf_attrib->sDims.ui32ByteStride) ||
		(dst_surf_attrib->sDims.ui32Width != src_surf_attrib->sDims.ui32Width)||
		(dst_surf_attrib->sDims.ui32Height !=
		src_surf_attrib->sDims.ui32Height)) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (flipable) {
		PVRSRV_ERROR err =
			do_mode_change(context, devinfo, priv, dst_surf_attrib);
		if (err != PVRSRV_OK) {
			EMGD_DEBUG("Exiting early because of an error in do_mode_change()");
			EMGD_TRACE_EXIT;
			return err;
		}
	}

	/*
	 * Allocate data structures:
	 */
	swap_chain = (emgddc_swapchain_t *) OS_ALLOC(sizeof(emgddc_swapchain_t));
	if (!swap_chain) {
		EMGD_ERROR_EXIT("Can not allocate memory for a swap chain");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OS_MEMSET(swap_chain, 0, sizeof(emgddc_swapchain_t));

	buffers = (emgddc_buffer_t *) OS_ALLOC(sizeof(emgddc_buffer_t) *
		buffer_count);
	if (!buffers) {
		OS_FREE(swap_chain);
		EMGD_ERROR_EXIT("Can not allocate memory for swap chain buffers");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OS_MEMSET(buffers, 0, sizeof(emgddc_buffer_t) * buffer_count);

	if (flipable) {
		flip_queue = (emgddc_flip_queue_item_t *)
			OS_ALLOC(sizeof(emgddc_flip_queue_item_t) * buffer_count);
		if (!flip_queue) {
			OS_FREE(buffers);
			OS_FREE(swap_chain);
			EMGD_ERROR_EXIT("Can not allocate memory for flip queue");
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
		OS_MEMSET(flip_queue, 0,
			sizeof(emgddc_flip_queue_item_t) * buffer_count);
	}

	/*
	 * Initialize data structures:
	 */
	swap_chain->devinfo = devinfo;
	swap_chain->valid = EMGD_TRUE;
	swap_chain->buffer_count = (unsigned long) buffer_count;
	swap_chain->buffers = buffers;
	swap_chain->flags = flags;
	swap_chain->next = NULL;
	if (flipable) {
		swap_chain->flip_queue = flip_queue;
		swap_chain->insert_index = 0;
		swap_chain->remove_index = 0;
	}
	swap_chain->pvr_jtable = &devinfo->pvr_jtable;

	/* Link the buffers of the swap chain: */
	for (i = 0 ; i < buffer_count-1 ; i++) {
		buffers[i].next = &buffers[i+1];
	}
	buffers[i].next = &buffers[0];

	i = 0;
	if (flipable) {
		/* The first buffer is the frame buffer (a.k.a. the front buffer). */
		buffers[i].priv = priv;
		buffers[i].offset = devinfo->system_buffer.offset;
		buffers[i].pixel_format = devinfo->system_buffer.pixel_format;
		buffers[i].width = devinfo->system_buffer.width;
		buffers[i].height = devinfo->system_buffer.height;
		buffers[i].pitch = devinfo->system_buffer.pitch;
		buffers[i].size = devinfo->system_buffer.size;
		buffers[i].virt_addr = devinfo->system_buffer.virt_addr;
		buffers[i].sync_data = sync_data[0];
		buffers[i].is_fb = 1;
		swap_chain->flags |= PVR2D_CREATE_FLIPCHAIN_OEMFLIPCHAIN;
		i++;
	}

	/*
	 * Allocate memory for the buffers
	 */
	for (; i < buffer_count ; i++) {
		unsigned long offset;
		unsigned long virt_addr;
		unsigned int width = 0;
		unsigned int height = 0;
		unsigned int pitch = 0;
		unsigned long size = 0;
		unsigned long pf;
		unsigned long flags = IGD_SURFACE_RENDER;
		int ret;
		unsigned int map_method=1;	/*1: gtt map by va driver*/

		if (!(oem_flags & PVR2D_CREATE_FLIPCHAIN_OEMGENERAL)) {
			flags |= IGD_SURFACE_DISPLAY;
		}

		buffers[i].priv = priv;

		/*
		 * What should be used for the surface attributes, the source
		 * surface attributes or the destination surface attributes?
		 * Can we assume that source is the surface requested?
		 */
		pf = pvr2emgd_pf(dst_surf_attrib->pixelformat);
		width = dst_surf_attrib->sDims.ui32Width;
		height = dst_surf_attrib->sDims.ui32Height;
		pitch = dst_surf_attrib->sDims.ui32ByteStride;
		flags |= IGD_MIN_PITCH;
		
		if(oem_flags & PVR2D_CREATE_FLIPCHAIN_CI){
			if(oem_flags & PVR2D_CREATE_FLIPCHAIN_CI_V4L2_MAP)
				map_method = 0;		


			size=height*pitch;
			ret = dispatch->gmm_map_ci(&offset,
				ci_offset,	
				&virt_addr,		
				map_method,
				size);
			
						
			if (0 != ret) {
				free_swap_chain(swap_chain, context);
				EMGD_ERROR_EXIT("gmm_alloc_surface() failed (%d)", ret);
				return PVRSRV_ERROR_OUT_OF_MEMORY;
			}
			buffers[i].is_contiguous = IMG_TRUE;

		}
		else{
			
			ret = dispatch->gmm_alloc_surface(&offset,
				pf,
				&width, &height,
				&pitch, &size,
				IGD_GMM_ALLOC_TYPE_NORMAL, &flags);
			if (0 != ret) {
				free_swap_chain(swap_chain, context);
				EMGD_ERROR_EXIT("gmm_alloc_surface() failed (%d)", ret);
				return PVRSRV_ERROR_OUT_OF_MEMORY;
			}
			buffers[i].is_contiguous = IMG_FALSE;
		}

		dst_surf_attrib->sDims.ui32ByteStride = pitch;
		src_surf_attrib->sDims.ui32ByteStride = pitch;

		buffers[i].pixel_format = pf;
		buffers[i].width = width;
		buffers[i].height = height;
		buffers[i].pitch = pitch;
		buffers[i].size = (size + (PAGE_SIZE - 1)) & PAGE_MASK;
		
		buffers[i].offset = offset;		
		
		if(oem_flags & PVR2D_CREATE_FLIPCHAIN_CI){			
			
				buffers[i].virt_addr = (IMG_CPU_VIRTADDR)virt_addr;			
		}
		else
			buffers[i].virt_addr = dispatch->gmm_map(offset);
		buffers[i].sync_data = sync_data[i];
		buffers[i].is_fb = 0;
	} /* for */


	if (flipable) {
		/* Initialize what's needed for flip-able swap chains: */
		int must_enable;

		for (i = 0 ; i < buffer_count ; i++) {
			flip_queue[i].valid = EMGD_FALSE;
			flip_queue[i].flipped = EMGD_FALSE;
			flip_queue[i].cmd_completed = EMGD_FALSE;
		}

		spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);

		must_enable = (devinfo->flipable_swapchains == NULL) ? 1 : 0;

		/* Add this swap chain to the list of flip-able swap chains: */
		add_swap_chain_to_list(&devinfo->flipable_swapchains, swap_chain);

		/* Unlock here (before enabling interrupts), to prevent deadlock: */
		spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);

		if (!devinfo->flush_commands) {
			if (must_enable) {
				/* Enable interrupts for vblanks: */
				if (!devinfo->interrupt_h || dispatch->
					enable_vblank_callback(devinfo->interrupt_h)) {
					/* For some reason (rare), interrupts weren't enabled: */
					EMGD_ERROR_EXIT("Can not enable VBlank interrupts!  "
									"Therefore, cannot do buffer flipping!");
					/* Properly clean up: */
					remove_swap_chain_from_list(&devinfo->flipable_swapchains,
												swap_chain);
					free_swap_chain(swap_chain, context);
					return PVRSRV_ERROR_BAD_MAPPING;
				} else {
					devinfo->flipping_disabled = EMGD_FALSE;
				}
			}
		}

#ifdef SUPPORT_FB_EVENTS
		if (must_enable) {
			/* Enable fb events: */
			if (enable_event_notification(devinfo)!= EMGD_OK) {
				EMGD_ERROR_EXIT("Can not enable framebuffer event "
					"notification");
				/* Properly clean up: */
				if (devinfo->interrupt_h) {
					dispatch->disable_vblank_callback(
						devinfo->interrupt_h);
				}
				spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);
				remove_swap_chain_from_list(&devinfo->flipable_swapchains,
					swap_chain);
				spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);
				free_swap_chain(swap_chain, context);
				return PVRSRV_ERROR_BAD_MAPPING;
			}
		}
#endif /* SUPPORT_FB_EVENTS */
	} else {
		/* Add this swap chain to the list of pixmap swap chains: */
		add_swap_chain_to_list(&devinfo->pixmap_swapchains, swap_chain);
	}


	*swap_chain_id = ++devinfo->swap_chain_id_counter;
	*swap_chain_h = (IMG_HANDLE) swap_chain;
	EMGD_DEBUG("swap_chain_h = 0x%p, *swap_chain_id = %lu",
		swap_chain_h, *swap_chain_id);

	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* CreateDCSwapChain() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnDestroyDCSwapChain()
 * function.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param swap_chain_h (IN) The handle for this swap chain (an opaque pointer
 * to a emgddc_swapchain_t).
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h)
{
	emgddc_devinfo_t	*devinfo;
	drm_emgd_priv_t *priv;
	igd_context_t *context;
	igd_dispatch_t *dispatch;
	emgddc_swapchain_t *swap_chain;
	unsigned long lock_flags;
#ifdef SUPPORT_FB_EVENTS
	emgd_error_t error;
#endif /* SUPPORT_FB_EVENTS */

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!device_h || !swap_chain_h) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	priv = devinfo->priv;
	context = priv->context;
	dispatch = &(context->dispatch);
	swap_chain = (emgddc_swapchain_t *) swap_chain_h;
	if (!is_valid_swap_chain(devinfo, swap_chain, 0)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p) and/or "
			"swap chain handle (0x%p)\n",__FUNCTION__, devinfo, swap_chain);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Remove swap chain from the appropriate list: */
	if (swap_chain->flags & PVR2D_CREATE_FLIPCHAIN_OEMFLIPCHAIN) {
		/* De-initialize what's needed for flip-able swap chains: */
		int must_disable;
		spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);
		must_disable =
			((devinfo->flipable_swapchains == swap_chain) &&
			 (devinfo->flipable_swapchains->next == NULL)) ? 1 : 0;

		/* Remove this swap chain from the list of flip-able swap chains: */
		remove_swap_chain_from_list(&devinfo->flipable_swapchains, swap_chain);

		spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);

#ifdef SUPPORT_FB_EVENTS
		if (must_disable) {
			/* Disable fb events: */
			error = disable_event_notification(devinfo);
			if (error != EMGD_OK) {
				EMGD_ERROR("Could not disable framebuffer event notification");
			}
		}
#endif /* SUPPORT_FB_EVENTS */

		/* Disable interrupts for vblanks: */
		if (must_disable) {
			if (devinfo->interrupt_h) {
				dispatch->disable_vblank_callback(devinfo->interrupt_h);
			}
		}

		/* Flush any pending flips: */
		flush_flip_queue(swap_chain);

		/* Flip back to the system buffer: */
		emgddc_flip(swap_chain, &devinfo->system_buffer);
	} else {
		/* Remove this swap chain from the list of pixmap swap chains: */
		remove_swap_chain_from_list(&devinfo->pixmap_swapchains, swap_chain);
	}

	/* Free all GMM and kernel space memory for this swap chain: */
	free_swap_chain(swap_chain, context);

	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* DestroyDCSwapChain() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnSetDCDstRect()
 * function.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param swap_chain_h (IN) The handle for this swap chain (an opaque pointer
 * to a emgddc_swapchain_t).
 * @param rect (IN) Unused.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h,
	IMG_RECT *rect)
{
	UNREFERENCED_PARAMETER(device_h);
	UNREFERENCED_PARAMETER(swap_chain_h);
	UNREFERENCED_PARAMETER(rect);

	EMGD_TRACE_STUB;

	return PVRSRV_ERROR_NOT_SUPPORTED;
}


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnSetDCSrcRect()
 * function.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param swap_chain_h (IN) The handle for this swap chain (an opaque pointer
 * to a emgddc_swapchain_t).
 * @param rect (IN) Unused.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h,
	IMG_RECT *rect)
{
	UNREFERENCED_PARAMETER(device_h);
	UNREFERENCED_PARAMETER(swap_chain_h);
	UNREFERENCED_PARAMETER(rect);

	EMGD_TRACE_STUB;

	return PVRSRV_ERROR_NOT_SUPPORTED;
}


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnSetDCDstColourKey()
 * function.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param swap_chain_h (IN) The handle for this swap chain (an opaque pointer
 * to a emgddc_swapchain_t).
 * @param color (IN) Unused.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h,
	IMG_UINT32 color)
{
	UNREFERENCED_PARAMETER(device_h);
	UNREFERENCED_PARAMETER(swap_chain_h);
	UNREFERENCED_PARAMETER(color);

	EMGD_TRACE_STUB;

	return PVRSRV_ERROR_NOT_SUPPORTED;
}


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnSetDCSrcColourKey()
 * function.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param swap_chain_h (IN) The handle for this swap chain (an opaque pointer
 * to a emgddc_swapchain_t).
 * @param color (IN) Unused.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h,
	IMG_UINT32 color)
{
	UNREFERENCED_PARAMETER(device_h);
	UNREFERENCED_PARAMETER(swap_chain_h);
	UNREFERENCED_PARAMETER(color);

	EMGD_TRACE_STUB;

	return PVRSRV_ERROR_NOT_SUPPORTED;
}


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnGetDCBuffers()
 * function.
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param swap_chain_h (IN) The handle for this swap chain (an opaque pointer
 * to a emgddc_swapchain_t).
 * @param buffer_count (OUT) The number of buffers in the specified swap chain.
 * @param buffer_h (IN/OUT) An array of buffer handles (an opaque pointer to a
 * emgddc_buffer_t) in the specified swap chain (memory must be allocated by
 * the caller).
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h,
	IMG_UINT32 *buffer_count,
	IMG_HANDLE *buffer_h)
{
	emgddc_devinfo_t *devinfo;
	emgddc_swapchain_t *swap_chain;
	unsigned long i;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!device_h || !swap_chain_h || !buffer_count || !buffer_h) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	swap_chain = (emgddc_swapchain_t *) swap_chain_h;
	if (!is_valid_swap_chain(devinfo, swap_chain, 0)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p) and/or "
			"swap chain handle (0x%p)\n",__FUNCTION__, devinfo, swap_chain);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	*buffer_count = (IMG_UINT32) swap_chain->buffer_count;

	for (i = 0 ; i < swap_chain->buffer_count ; i++) {
		buffer_h[i] = (IMG_HANDLE) &swap_chain->buffers[i];
	}


	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* GetDCBuffers() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnSwapToDCBuffer()
 * function.  This function is supposed to cause a flip to the specified
 * buffer.  However, it is no longer called by PVR services!
 *
 * NOTE: As can be seen, this function was never completely implemented
 * (i.e. in the DDK used to create this version).  This is because the PVR code
 * doesn't call this function.  Instead, it calls emgddc_process_flip().
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param buffer_h (IN) The handle for this buffer (an opaque pointer to a
 * emgddc_buffer_t).
 * @param swap_interval (IN) Unused.
 * @param private_tag_h (IN) Unused.
 * @param clip_rect_count (IN) Largely unused.
 * @param clip_rect (IN) Unused.
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE device_h,
	IMG_HANDLE buffer_h,
	IMG_UINT32 swap_interval,
	IMG_HANDLE private_tag_h,
	IMG_UINT32 clip_rect_count,
	IMG_RECT *clip_rect)
{
	/* This function is never called by PVR services, and so it is stubbed: */
	UNREFERENCED_PARAMETER(device_h);
	UNREFERENCED_PARAMETER(buffer_h);
	UNREFERENCED_PARAMETER(swap_interval);
	UNREFERENCED_PARAMETER(private_tag_h);
	UNREFERENCED_PARAMETER(clip_rect_count);
	UNREFERENCED_PARAMETER(clip_rect);

	EMGD_TRACE_STUB;
	return PVRSRV_ERROR_NOT_SUPPORTED;

} /* SwapToDCBuffer() */


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnSwapToDCSystem()
 * function.  This function causes a flip to the "system buffer" (a.k.a. frame
 * buffer).
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param swap_chain_h (IN) The handle for this swap chain (an opaque pointer
 * to a emgddc_swapchain_t).
 * @return PVRSRV_OK or a PVRSRV_ERROR enum value.
 */
static PVRSRV_ERROR SwapToDCSystem(IMG_HANDLE device_h,
	IMG_HANDLE swap_chain_h)
{
	emgddc_devinfo_t   *devinfo;
	emgddc_swapchain_t *swap_chain;
	unsigned long      lock_flags;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!device_h || !swap_chain_h) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	devinfo = (emgddc_devinfo_t *) device_h;
	swap_chain = (emgddc_swapchain_t *) swap_chain_h;
	if (!is_valid_swap_chain(devinfo, swap_chain, 1)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p) and/or "
			"swap chain handle (0x%p)\n",__FUNCTION__, devinfo, swap_chain);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	/* This must be a flip-able swap chain, or we can't cause a flip for it: */
	if (!(swap_chain->flags & PVR2D_CREATE_FLIPCHAIN_OEMFLIPCHAIN)) {
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);

	flush_flip_queue(swap_chain);
	emgddc_flip(swap_chain, &devinfo->system_buffer);

	spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);


	EMGD_TRACE_EXIT;
	return PVRSRV_OK;

} /* SwapToDCSystem() */


/**
 * Drains the circular queue of buffers to flip for a specified swap chain.
 * The queue is processed in order.  For every item in the queue, processing is
 * as follows:
 *
 * - An item that has been "completed" (i.e. PVR services has been told that
 *   the flip occured), but hasn't reached the end of its swap interval, is
 *   immediately ended and cleared.
 *
 * - An item that has been flipped, but hasn't been "completed" with PVR
 *   services, is "completed," ended, and cleared.
 *
 * - An item that has been queued, but hasn't been flipped, is immediately
 *   flipped (i.e. given the Poulsbo hardware, this really means that the
 *   hardware is told to flip them at the start of the next vertical blanking
 *   period), "completed," ended, and cleared.
 *
 * Note: This function assumes that the calling function has already obtained
 * the spin lock.
 *
 * @param swap_chain (IN) The swap chain to flush the queue for.
 */
static void flush_flip_queue(emgddc_swapchain_t *swap_chain)
{
	emgddc_flip_queue_item_t *flip_item;
	unsigned long max_index;
	unsigned long i;

	EMGD_TRACE_ENTER;


	/* Get the first item to drain in the circular queue: */
	flip_item = &swap_chain->flip_queue[swap_chain->remove_index];
	max_index = swap_chain->buffer_count - 1;

	for (i = 0 ; i < swap_chain->buffer_count ; i++) {
		if (flip_item->valid == EMGD_FALSE) {
			continue;
		}

		EMGD_DEBUG("Flushing buffer offset=0x%lx", flip_item->buffer->offset);

		if ((swap_chain->devinfo->flipping_disabled == EMGD_FALSE) &&
			(flip_item->flipped == EMGD_FALSE) &&
			(swap_chain->valid == EMGD_TRUE)) {
			EMGD_DEBUG("Flipping to buffer offset=0x%lx",
				flip_item->buffer->offset);
			emgddc_flip(swap_chain, flip_item->buffer);
		}

		if (flip_item->cmd_completed == EMGD_FALSE) {
			PVRSRV_DC_DISP2SRV_KMJTABLE	*pvr_jtable = swap_chain->pvr_jtable;

			EMGD_DEBUG("Calling pfnPVRSRVCmdComplete() for buffer offset=0x%lx",
				flip_item->buffer->offset);
			pvr_jtable->pfnPVRSRVCmdComplete(flip_item->cmd_complete, IMG_TRUE);
		}

		/* We're done with this item in the queue.  Prepare for processing the
		 * next item:
		 */
		flip_item->flipped = EMGD_FALSE;
		flip_item->cmd_completed = EMGD_FALSE;
		flip_item->valid = EMGD_FALSE;

		/* Point to the next item in the circular queue: */
		swap_chain->remove_index++;
		if (swap_chain->remove_index > max_index) {
			swap_chain->remove_index = 0;
		}

		/* Get the next item in the circular queue: */
		flip_item = &swap_chain->flip_queue[swap_chain->remove_index];
	}

	/* Reset the circular queue to the start: */
	swap_chain->insert_index = 0;
	swap_chain->remove_index = 0;


	EMGD_TRACE_EXIT;

} /* flush_flip_queue() */


static void set_flush_state_internal_nolock(emgddc_devinfo_t* devinfo,
	emgd_bool flush_state)
{
	emgddc_swapchain_t *swap_chain = devinfo->flipable_swapchains;

	EMGD_TRACE_ENTER;


	if (flush_state) {
		if (devinfo->set_flush_state_ref_count == 0) {
			/* Don't actually disable interrupts.  Just set a flag so that
			 * buffer flips won't be queued, then flush the circular buffer of
			 * all pending flips, for all swap chains:
			 */
			devinfo->flush_commands = EMGD_TRUE;
			while (swap_chain != NULL) {
				flush_flip_queue(swap_chain);
				swap_chain = swap_chain->next;
			}
		}
		devinfo->set_flush_state_ref_count++;
	} else {
		if (devinfo->set_flush_state_ref_count != 0) {
			devinfo->set_flush_state_ref_count--;
			if (devinfo->set_flush_state_ref_count == 0) {
				/* Don't actually enable interrupts.  Just set a flag so that
				 * buffer flips will be queued:
				 */
				devinfo->flush_commands = EMGD_FALSE;
			}
		}
	}


	EMGD_TRACE_EXIT;
}


static void set_flush_state_external(emgddc_devinfo_t* devinfo,
	emgd_bool flush_state)
{
	unsigned long lock_flags;

	EMGD_TRACE_ENTER;


	spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);

	if (devinfo->flush_commands != flush_state) {
		devinfo->flush_commands = flush_state;
		set_flush_state_internal_nolock(devinfo, flush_state);
	}

	spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);


	EMGD_TRACE_EXIT;
}


/**
 * Implementation of the PVRSRV_DC_SRV2DISP_KMJTABLE.pfnSetDCState() function.
 * This function is supposed to set some state flags in the driver, affecting
 * interrupt-driven buffer flips.
 *
 * Note: The following function used to never be called, but now is called when
 * X11 is rotated 90 degrees (it is called as a result of an interrupt handler
 * dealing with an SGX hardware reset).  As such, the code that this calls
 * cannot enable/disable interrupts (which may cause an interrupt handler to be
 * registered/unregistered).
 *
 * Note: The code that this calls seems overly complicated.  If we never enable
 * SUPPORT_FB_EVENTS, it can be simplified (e.g. no use of
 * set_flush_state_ref_count).
 *
 * @param device_h (IN) The handle for this device (an opaque pointer to
 * devinfo).
 * @param state (IN) Either DC_STATE_FLUSH_COMMANDS or
 * DC_STATE_NO_FLUSH_COMMANDS.
 */
static IMG_VOID SetDCState(IMG_HANDLE device_h, IMG_UINT32 state)
{
	emgddc_devinfo_t *devinfo = (emgddc_devinfo_t *) device_h;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("device_h = 0x%p", device_h);


	if (!is_valid_devinfo((emgddc_devinfo_t *) device_h)) {
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p)\n",
			__FUNCTION__, device_h);
		return;
	}

	switch (state) {
	case DC_STATE_FLUSH_COMMANDS:
		set_flush_state_external(devinfo, EMGD_TRUE);
		break;
	case DC_STATE_NO_FLUSH_COMMANDS:
		set_flush_state_external(devinfo, EMGD_FALSE);
		break;
	default:
		break;
	}


	EMGD_TRACE_EXIT;
} /* SetDCState() */


#ifdef SUPPORT_FB_EVENTS
/* NOTE -- The following code is probably not correct.  It was inherited from
 * an early IMG/UMG DDK, and has never been used.  If we ever decide to use
 * this code, we should look at the latest DDK and UMG code, to glean what may
 * be really needed.  At a minimum, the following code needs to affect all swap
 * chains.
 */

static int emgddc_fb_events(struct notifier_block *notif,
	unsigned long event, void *data)
{
	emgddc_devinfo_t *devinfo;
	emgddc_swapchain_t *swap_chain;
	struct fb_event *fb_event = (struct fb_event *) data;
	emgd_bool blanked;
	unsigned long lock_flags;

	EMGD_TRACE_ENTER;


	if (event != FB_EVENT_BLANK) {
		return 0;
	}

	/* Look up the device (for DIH/Extended mode): */
	if (notif == &(global_devinfo[0]->lin_notif_block)) {
		devinfo = global_devinfo[0];
	} else if (notif == &(global_devinfo[1]->lin_notif_block)) {
		devinfo = global_devinfo[1];
	} else {
		printk(KERN_ERR "[EMGD] emgddc_fb_events() cannot find its device\n");
		return -ENODEV;
	}
	swap_chain = devinfo->flipable_swapchains;

	blanked = (*(IMG_INT *) fb_event->data != 0) ? EMGD_TRUE : EMGD_FALSE;

	if (blanked != swap_chain->blanked) {
		swap_chain->blanked = blanked;

		spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);

		if (blanked) {
			set_flush_state_internal_nolock(devinfo, EMGD_TRUE);
		} else {
			set_flush_state_internal_nolock(devinfo, EMGD_FALSE);
		}

		spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);
	}


	EMGD_TRACE_EXIT;

	return 0;
}


static emgd_error_t enable_event_notification(emgddc_devinfo_t *devinfo)
{
	int res;
	emgddc_swapchain_t *swap_chain = devinfo->flipable_swapchains;

	EMGD_TRACE_ENTER;


	memset(&devinfo->lin_notif_block, 0, sizeof(devinfo->lin_notif_block));

	devinfo->lin_notif_block.notifier_call = emgddc_fb_events;
	swap_chain->blanked = EMGD_FALSE;
	res = fb_register_client(&devinfo->lin_notif_block);
	if (res != 0) {
		EMGD_ERROR_EXIT("fb_register_client() failed (%d)", res);
		return EMGD_ERROR_GENERIC;
	}


	EMGD_TRACE_EXIT;

	return EMGD_OK;
}


static emgd_error_t disable_event_notification(emgddc_devinfo_t *devinfo)
{
	int res;


	EMGD_TRACE_ENTER;


	res = fb_unregister_client(&devinfo->lin_notif_block);
	if (res != 0) {
		EMGD_ERROR_EXIT("fb_unregister_client() failed (%d)", res);
		return EMGD_ERROR_GENERIC;
	}


	EMGD_TRACE_EXIT;

	return EMGD_OK;
}
#endif /* SUPPORT_FB_EVENTS */


/**
 * This function does per-vblank processing of the circular queue of buffers to
 * flip.  It is called for each flip-able swap chain, during a VBlank
 * interrupt.  The first item in the queue is processed, and if it is cleared
 * (see below), the next item is processed, etc.  Items are processed as
 * follows:
 *
 * - An item that hasn't been flipped, is flipped.  In this case, processing
 *   stops.  It is assumed that the swap interval for this item is at least
 *   one, and therefore, another vblank is needed for this flip.
 *
 * - An item that has been flipped, but hasn't been "completed" (i.e. PVR
 *   services has been told that the flip occured), is "completed" and has its
 *   swap interval decremented by 1.  If the swap interval is now 0, the item
 *   is cleared, allowing processing of the next item to start.
 *
 * - An item that has been "completed," but hasn't reached the end of its swap
 *   interval, has its swap interval decremented by 1.  If the swap interval is
 *   now 0, the item is cleared, allowing processing of the next item to start.
 *
 * @param swap_chain (IN) The swap chain to process the queue for.
 */
emgd_bool emgddc_process_flip_queue_for_vblank(emgddc_swapchain_t *swap_chain)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE	*pvr_jtable = swap_chain->pvr_jtable;
	IMG_BOOL status = IMG_TRUE;
	emgddc_flip_queue_item_t *flip_item;
	unsigned long max_index;

	EMGD_TRACE_ENTER;


	if (swap_chain->devinfo->flush_commands) {
		EMGD_TRACE_EXIT;
		return status;
	}

	/* Get the first item to process in the circular queue: */
	flip_item = &swap_chain->flip_queue[swap_chain->remove_index];
	max_index = swap_chain->buffer_count - 1;

	while (flip_item->valid) {
		if (flip_item->flipped) {
			if (!flip_item->cmd_completed) {
				EMGD_DEBUG("Calling pfnPVRSRVCmdComplete() for buffer "
					"offset=0x%lx", flip_item->buffer->offset);
				pvr_jtable->pfnPVRSRVCmdComplete(flip_item->cmd_complete,
					IMG_TRUE);
				flip_item->cmd_completed = EMGD_TRUE;
			}

			flip_item->swap_interval--;
			EMGD_DEBUG("Swap interval is %lu for buffer offset=0x%lx",
				flip_item->swap_interval, flip_item->buffer->offset);

			if (flip_item->swap_interval == 0) {
				/* We're done with this item in the queue.  Prepare for
				 * processing the next item:
				 */
				flip_item->cmd_completed = EMGD_FALSE;
				flip_item->flipped = EMGD_FALSE;
				flip_item->valid = EMGD_FALSE;

				/* Point to the next item in the circular queue: */
				swap_chain->remove_index++;
				if (swap_chain->remove_index > max_index) {
					swap_chain->remove_index = 0;
				}
			} else {
				/* Wait for more vblanks before doing more queue processing: */
				break;
			}
		} else {
			EMGD_DEBUG("Flipping to buffer offset=0x%lx",
				flip_item->buffer->offset);
			emgddc_flip(swap_chain, flip_item->buffer);
			flip_item->flipped = EMGD_TRUE;
			/* Wait for more vblanks before doing more queue processing: */
			break;
		}

		/* Get the next item in the circular queue: */
		flip_item = &swap_chain->flip_queue[swap_chain->remove_index];
	}


	EMGD_TRACE_EXIT;

	return status;

} /* emgddc_process_flip_queue_for_vblank() */


/**
 * This is called by a HAL-implemented, Linux interrupt handler.  It is called
 * when a VBlank interrupt occurs.  All device-specific functionality was
 * implemented by the HAL, and only 3DD-specific functionality needs to be
 * provided by this function.
 *
 * @param pdevinfo (IN) Pointer to the devinfo that had a VBlank interrupt.
 * @return Non-zero for success, zeron for failure.
 */
static int emgddc_process_vblank(void* pdevinfo)
{
	emgddc_devinfo_t *devinfo = (emgddc_devinfo_t *) pdevinfo;
	emgddc_swapchain_t *swap_chain;
	unsigned long lock_flags;

	EMGD_TRACE_ENTER;


	if ((devinfo != global_devinfo[0]) && (devinfo != global_devinfo[1])) {
		return 0;
	}

	spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);

	swap_chain = devinfo->flipable_swapchains;
	while (swap_chain != NULL) {
		(void) emgddc_process_flip_queue_for_vblank(swap_chain);
		swap_chain = swap_chain->next;
	}

	spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);

	return 1;

} /* emgddc_process_vblank() */


/**
 * Called by PVR services to flip a buffer.  When interrupts are supported, the
 * flip may be queued (in a circular buffer) to happen later.  When interrupts
 * are not supported, the flip always happens immediately
 *
 * @param cmd_cookie_h (IN) An opaque pointer to a PVR service data structure
 *   that must be handed back when the flip is "completed" (i.e. PVR services
 *   is told that the flip occured).
 * @param data_size (IN) Size of the flip command and all clipping rectangles
 * (which isn't supported).  This is only used for consistency-checking.
 * @param data (IN) A pointer to information about what to flip.
 */
static IMG_BOOL emgddc_process_flip(IMG_HANDLE cmd_cookie_h,
	IMG_UINT32 data_size,
	IMG_VOID *data)
{
	DISPLAYCLASS_FLIP_COMMAND *flip_cmd;
	emgddc_devinfo_t *devinfo;
	emgddc_buffer_t *buffers;
	emgddc_swapchain_t *swap_chain;
	unsigned long max_index;
	emgddc_flip_queue_item_t* flip_item;
	unsigned long lock_flags;
	int must_flip = 0;
	int must_complete = 0;
	igd_context_t *context;

	EMGD_TRACE_ENTER;


	/*
	 * Unpack the flip command and look for errors:
	 */

	if (!cmd_cookie_h || !data) {
		EMGD_ERROR_EXIT("NULL parameter(s)");
		return IMG_FALSE;
	}

	flip_cmd = (DISPLAYCLASS_FLIP_COMMAND *) data;

	/* Note: the data_size actually accounts for both the flip command and
	 * all of the clipping rectangles.  As such, the only error is if
	 * data_size is smaller than the sizeof the flip command:
	 */
	if (flip_cmd == IMG_NULL ||
		(sizeof(DISPLAYCLASS_FLIP_COMMAND) > data_size)) {
		EMGD_ERROR_EXIT("Invalid flip_cmd (0x%p)", flip_cmd);
		return IMG_FALSE;
	}

	devinfo = (emgddc_devinfo_t *) flip_cmd->hExtDevice;
	buffers = (emgddc_buffer_t *) flip_cmd->hExtBuffer;
	swap_chain = (emgddc_swapchain_t *) flip_cmd->hExtSwapChain;

	if (!is_valid_swap_chain(devinfo, swap_chain, 1)) {
		/* Note: Hardware video decode creates pixmap swap chains,
		 * and when they are being destroyed (at
		 * the end of video playback), something tries to flip these
		 * non-flipable swap chains.  The only way to avoid a hang is to
		 * "complete" the flip command.
		 */
		printk(KERN_ERR "[EMGD] %s() given invalid device handle (0x%p) and/or "
			"swap chain handle (0x%p)\n",__FUNCTION__, devinfo, swap_chain);
		swap_chain->pvr_jtable->pfnPVRSRVCmdComplete(cmd_cookie_h, IMG_TRUE);
		EMGD_TRACE_EXIT;
		return IMG_TRUE;
	}

	spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);

	if ((devinfo->flipping_disabled == EMGD_TRUE) ||
		(swap_chain->valid != EMGD_TRUE)) {
		/* We won't flip, but must tell PVR services that the flip occured: */
		EMGD_DEBUG("Something (e.g. a mode change) has invalidated\n"
			"this swap chain.  As such buffer flips are not allowed.\n"
			"If a mode change caused this problem, this swap chain\n"
			"needs to be destroyed, and a new one created.");
		swap_chain->pvr_jtable->pfnPVRSRVCmdComplete(cmd_cookie_h, IMG_TRUE);
		spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);
		EMGD_TRACE_EXIT;
		return IMG_TRUE;
	}

	context = devinfo->priv->context;
	if (context->device_context.power_state != IGD_POWERSTATE_D0) {
		/* If device is in a suspended state, but PVR services asks the driver
		 * to perform a buffer flip, basically ignore it except for telling PVR
		 * services that we did the flip:
		 */
		EMGD_DEBUG("Device in suspended state--completing command");
		swap_chain->pvr_jtable->pfnPVRSRVCmdComplete(cmd_cookie_h, IMG_TRUE);
		spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);
		EMGD_TRACE_EXIT;
		return IMG_TRUE;
	}

	/* Get the first item to add to the circular queue: */
	flip_item = &swap_chain->flip_queue[swap_chain->insert_index];
	max_index = swap_chain->buffer_count - 1;


	/*
	 * Decide what needs to be done:
	 */
	if ((flip_cmd->ui32SwapInterval == 0) ||
		(devinfo->flush_commands == EMGD_TRUE)) {
		/* Perform and complete the flip now: */
		must_flip = 1;
		must_complete = 1;
	} else {
		/* PVR services only calls emgddc_process_flip() when the 3DD completes
		 * the previous flip.  Thus, the circular flip_item queue should never
		 * overflow.  However, just in case, check whether the flip_item
		 * already contains a valid/queued flip (i.e. we've overflowed the
		 * circular queue).
		 */
		if (flip_item->valid == EMGD_FALSE) {
			if (swap_chain->insert_index == swap_chain->remove_index) {
			/* Perform the flip now, but queue it for completion: */
				must_flip = 1;
			}
			/* else - queue the flip for later: */
		} else {
			/* Just in case we overflow the circular queue, generate an error */
			swap_chain->pvr_jtable->pfnPVRSRVCmdComplete(cmd_cookie_h,IMG_TRUE);
			spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);
			EMGD_ERROR_EXIT("Overflowed the circular flip_item queue");
			return IMG_FALSE;
		}
	}


	/*
	 * Do what needs to be done:
	 */
	if (must_flip) {
		/* Perform the flip now: */
		EMGD_DEBUG("Flipping to buffer offset=0x%lx", buffers->offset);
		emgddc_flip(swap_chain, buffers);
	}
	if (must_complete) {
		/* Tell the PVR services that the flip occured: */
		EMGD_DEBUG("Calling pfnPVRSRVCmdComplete() for buffer offset=0x%lx",
			buffers->offset);
		swap_chain->pvr_jtable->pfnPVRSRVCmdComplete(cmd_cookie_h,IMG_TRUE);
	} else {
		/* Queue the flip for later completion: */
		EMGD_DEBUG("Queueing buffer offset=0x%lx", buffers->offset);
		if (must_flip) {
			flip_item->flipped = EMGD_TRUE;
		}
		flip_item->cmd_complete = cmd_cookie_h;
		flip_item->swap_interval = (unsigned long) flip_cmd->ui32SwapInterval;
		flip_item->valid = EMGD_TRUE;
		flip_item->buffer = buffers;

		swap_chain->insert_index++;
		if (swap_chain->insert_index > max_index) {
			swap_chain->insert_index = 0;
		}
	}

	spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);

	EMGD_TRACE_EXIT;
	return IMG_TRUE;
} /* emgddc_process_flip() */


/**
 * For a given devinfo, unmap's the frame buffer, and frees the devinfo and
 * it's surfaces.
 *
 * @param dev (IN) The drm_device for this driver connection.
 */
void emgddc_free_a_devinfo(emgddc_devinfo_t *devinfo)
{
	igd_context_t *context = devinfo->priv->context;

	EMGD_TRACE_ENTER;

	if (devinfo->priv->hal_running) {
		/* Un-register (if needed) the interrupt connection with the HAL: */
		if (devinfo->interrupt_h) {
			context->dispatch.unregister_vblank_callback(devinfo->interrupt_h);
			devinfo->interrupt_h = NULL;
		}

		if (devinfo->system_buffer.virt_addr) {
			context->dispatch.gmm_unmap(devinfo->system_buffer.virt_addr);
		}
	}
	OS_FREE(devinfo);

	EMGD_TRACE_EXIT;
} /* emgddc_free_a_devinfo() */


/**
 * Frees all devinfo structures and their surfaces.  This is called during
 * de-init time, or when init fails.
 */
void emgddc_free_all_devinfos(void)
{
	emgddc_devinfo_t *devinfo;

	EMGD_TRACE_ENTER;

	/* Free the primary display's structures: */
	devinfo = global_devinfo[0];
	emgddc_free_a_devinfo(devinfo);
	global_devinfo[0] = NULL;

	/* Free the secondary display's structures, if applicable: */
	if (NULL == (devinfo = global_devinfo[1])) {
		emgddc_free_a_devinfo(devinfo);
		global_devinfo[1] = NULL;
	}

	EMGD_TRACE_EXIT;
} /* emgddc_free_all_devinfos() */


/**
 * Initialize the "static" (i.e. doesn't vary with alter_display) portion of
 * a devinfo structure.
 *
 * @param dev (IN) The drm_device for this driver connection.
 * @param devinfo (IN/OUT) The devinfo to initialize.
 * @param port_number (IN) Which devinfo (0 for primary, 1 for secondary/DIH).
 * @return emgd_error_t value (e.g. EMGD_OK, EMGD_ERROR_OUT_OF_MEMORY).
 */
static emgd_error_t emgddc_init_devinfo(struct drm_device *dev,
	emgddc_devinfo_t *devinfo, int which_devinfo)
{
	drm_emgd_priv_t *priv = dev->dev_private;
	PVRSRV_DC_DISP2SRV_KMJTABLE	*pvr_jtable;
	PFN_CMD_PROC cmd_proc_list[EMGDDC_COMMAND_COUNT];
	IMG_UINT32 sync_count_list[EMGDDC_COMMAND_COUNT][2];

	EMGD_TRACE_ENTER;


	/*
	 * Initialize the static/display-independent devinfo values:
	 */
	devinfo->which_devinfo = which_devinfo;
	devinfo->priv = priv;
	devinfo->drm_device = dev;
	devinfo->flipable_swapchains = NULL;
	devinfo->swap_chain_id_counter = 0;
	devinfo->display_info.ui32MaxSwapChainBuffers = 5;
	devinfo->display_info.ui32MaxSwapChains = 1024 * 1024;
	/* Note: change from zero if we support interrupts and see the need: */
	devinfo->display_info.ui32MaxSwapInterval = 2;
	devinfo->display_info.ui32MinSwapInterval = 0;
	EMGD_DEBUG("Maximum number of swap chains: %lu",
		devinfo->display_info.ui32MaxSwapChains);
	EMGD_DEBUG("Maximum number of swap chain buffers: %lu",
		devinfo->display_info.ui32MaxSwapChainBuffers);
	strncpy(devinfo->display_info.szDisplayName, DISPLAY_DEVICE_NAME,
		MAX_DISPLAY_NAME_SIZE);
	devinfo->flush_commands = EMGD_FALSE;
	spin_lock_init(&devinfo->swap_chain_lock);


	/*
	 * Get the PVR services jump table, which this 3rd-party display driver can
	 * use to call PVR services:
	 */
	if (!PVRGetDisplayClassJTable(&devinfo->pvr_jtable)) {
		EMGD_ERROR_EXIT("Can not get PVR services jump table");
		return EMGD_ERROR_INIT_FAILURE;
	}
	pvr_jtable = &devinfo->pvr_jtable;


	/*
	 * Setup the jump table that PVR services uses to call this 3rd-party
	 * display driver:
	 */
	devinfo->dc_jtable.ui32TableSize = sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
	devinfo->dc_jtable.pfnOpenDCDevice = OpenDCDevice;
	devinfo->dc_jtable.pfnCloseDCDevice = CloseDCDevice;
	devinfo->dc_jtable.pfnEnumDCFormats = EnumDCFormats;
	devinfo->dc_jtable.pfnEnumDCDims = EnumDCDims;
	devinfo->dc_jtable.pfnGetDCSystemBuffer = GetDCSystemBuffer;
	devinfo->dc_jtable.pfnGetDCInfo = GetDCInfo;
	devinfo->dc_jtable.pfnGetBufferAddr = GetDCBufferAddr;
	devinfo->dc_jtable.pfnCreateDCSwapChain = CreateDCSwapChain;
	devinfo->dc_jtable.pfnDestroyDCSwapChain = DestroyDCSwapChain;
	devinfo->dc_jtable.pfnSetDCDstRect = SetDCDstRect;
	devinfo->dc_jtable.pfnSetDCSrcRect = SetDCSrcRect;
	devinfo->dc_jtable.pfnSetDCDstColourKey = SetDCDstColourKey;
	devinfo->dc_jtable.pfnSetDCSrcColourKey = SetDCSrcColourKey;
	devinfo->dc_jtable.pfnGetDCBuffers = GetDCBuffers;
	devinfo->dc_jtable.pfnSwapToDCBuffer = SwapToDCBuffer;
	devinfo->dc_jtable.pfnSwapToDCSystem = SwapToDCSystem;
	devinfo->dc_jtable.pfnSetDCState = SetDCState;


	/*
	 * Register this device with PVR services:
	 */
	if (pvr_jtable->pfnPVRSRVRegisterDCDevice(&devinfo->dc_jtable,
		&devinfo->device_id ) != PVRSRV_OK) {
		EMGD_ERROR_EXIT("Device registration failed");
		return EMGD_ERROR_DEVICE_REGISTER_FAILED;
	}

	EMGD_DEBUG("Device ID: %d", (int)devinfo->device_id);


	/*
	 * Tell PVR services about the function to process swap-chain buffer
	 * flips:
	 */
	cmd_proc_list[DC_FLIP_COMMAND] = emgddc_process_flip;

	/* FIXME:  Not sure what these are for: */
	sync_count_list[DC_FLIP_COMMAND][0] = 0; 
	sync_count_list[DC_FLIP_COMMAND][1] = 2; 

	if (pvr_jtable->pfnPVRSRVRegisterCmdProcList(devinfo->device_id,
		&cmd_proc_list[0],
		sync_count_list,
		EMGDDC_COMMAND_COUNT) != PVRSRV_OK) {
		EMGD_ERROR_EXIT("Can't register callback\n");
		return EMGD_ERROR_CANT_REGISTER_CALLBACK;
	}


	EMGD_TRACE_EXIT;
	return EMGD_OK;

} /* emgddc_init_devinfo() */


/** 
 * Initializes the display/device-specific values of the devinfo structure for
 * a specified display (primary or secondary).  This function is called during
 * initialization time, and whenever the EMGD driver does a potential mode
 * change, via alter_displays().
 *
 * @param devinfo (IN/OUT) The devinfo to initialize for the display.
 * @param display (IN) The specified display (primary or secondary).
 * @param port_number (IN) The port number of the specified display.
 * @return emgd_error_t value (e.g. EMGD_OK, EMGD_ERROR_OUT_OF_MEMORY).
 */
static emgd_error_t init_display(emgddc_devinfo_t *devinfo,
	igd_display_h display,
	unsigned short port_number)
{
	drm_emgd_priv_t *priv = devinfo->priv;
	igd_context_t *context = priv->context;
	unsigned long dc = priv->dc;
	igd_framebuffer_info_t fb_info;
	DISPLAY_FORMAT *display_format_list;
	igd_display_info_t pt_info;
	unsigned long *fb_list_pfs;
	DISPLAY_DIMS *display_dim_list;
	igd_display_info_t *mode_list = NULL;
	igd_display_info_t *mode;
	int mode_flags = IGD_QUERY_LIVE_MODES;
	emgddc_buffer_t *buffer = &(devinfo->system_buffer);
	int i = 1, j, ret;

	EMGD_TRACE_ENTER;

	EMGD_DEBUG("Parameters:");
	EMGD_DEBUG(" devinfo=0x%p", devinfo);
	EMGD_DEBUG(" devinfo->which_devinfo=%d", devinfo->which_devinfo);
	EMGD_DEBUG(" display=0x%p", display);
	EMGD_DEBUG(" port_number=%u", port_number);

	/* Clear the following lists, in case we are re-initializing: */
	OS_MEMSET(devinfo->display_format_list, 0,
			sizeof(devinfo->display_format_list));
	OS_MEMSET(devinfo->display_dim_list, 0,
			sizeof(devinfo->display_dim_list));
	display_format_list = devinfo->display_format_list;
	display_dim_list = devinfo->display_dim_list;


	/* Call get_display() to get some of the following info: */
	ret = context->dispatch.get_display(display, port_number,
		&fb_info, &pt_info, 0);
	if (0 != ret) {
		EMGD_ERROR_EXIT("get_display() returned %d", ret);
		return EMGD_ERROR_GENERIC;
	}


	/*
	 * Obtain the following addresses:
	 *
	 * - buffer->offset = Framebuffer GTT address
	 * - virt_addr = gmm_map(framebuffer offset);
	 */
	buffer->offset = fb_info.fb_base_offset;
	if(NULL == buffer->virt_addr){
		buffer->virt_addr = context->dispatch.gmm_map(fb_info.fb_base_offset);
	}
	EMGD_DEBUG("buffer->virt_addr = 0x%p", buffer->virt_addr);
	/* This is the offset of the allocated framebuffer (e.g. the
	 * 1024x768 surface of gmm-managed memory):
	 */
	EMGD_DEBUG("fb_info->fb_base_offset = 0x%lx", fb_info.fb_base_offset);


	/* Register (if we haven't) with the HAL to be able to use interrupts: */
	if (!devinfo->interrupt_h) {
		devinfo->interrupt_h =
			context->dispatch.register_vblank_callback(emgddc_process_vblank,
				devinfo, port_number);
		if (!devinfo->interrupt_h) {
			/* This should not happen, but just in case, provide an error: */
			printk(KERN_ERR "Cannot establish the ability to perform VBlank "
				"interrupts for port number %u.", port_number);
		}
	}


	/*
	 * Obtain and translate pixel formats:
	 */
	/* Note: We always care about the current pixel format: */
	devinfo->emgd_pf = fb_info.pixel_format;
	devinfo->pvr_pf = emgd2pvr_pf(fb_info.pixel_format);
	display_format_list[0].pixelformat = emgd2pvr_pf(fb_info.pixel_format);
	EMGD_DEBUG("FB's native EMGD pixel format = 0x%08lx", devinfo->emgd_pf);
	EMGD_DEBUG("FB's PVR pixel format = %u",display_format_list[0].pixelformat);
	if (!priv->xserver_running) {
		/* In addition, provide an entire list of pixel formats: */
		ret = context->dispatch.get_pixelformats(display, &fb_list_pfs, NULL,
			NULL, NULL, NULL);
		if (0 != ret) {
			EMGD_ERROR_EXIT("get_pixelformats() returned %d", ret);
			return EMGD_ERROR_GENERIC;
		}
		while (*fb_list_pfs) {
			if ((devinfo->pvr_pf != emgd2pvr_pf(*fb_list_pfs)) &&
				(PVRSRV_PIXEL_FORMAT_UNKNOWN != emgd2pvr_pf(*fb_list_pfs))) {
				display_format_list[i].pixelformat = emgd2pvr_pf(*fb_list_pfs);
				EMGD_DEBUG("  Add'l (%d) PVR pixel format = "
					"%u", i, emgd2pvr_pf(*fb_list_pfs));
			}
			fb_list_pfs++;
			if (++i >= EMGDDC_MAXFORMATS) {
				/* Don't write past the end of the array */
				EMGD_ERROR("Reached end of display_format_list!  Consider "
						"increasing EMGDDC_MAXFORMATS.");
				break;
			}
		}
	}
	devinfo->num_formats = i;
	EMGD_DEBUG("Total number of translated pixel formats = %d", i);


	/*
	 * Obtain the possible dimensions, from the EMGD modes:
	 */
	/* Note: we always care about the current dimensions: */
	devinfo->width = fb_info.width;
	devinfo->height = fb_info.height;
	devinfo->byte_stride = fb_info.screen_pitch;
	EMGD_DEBUG("FB's width = %ld, height = %ld, stride = %ld",
		devinfo->width, devinfo->height, devinfo->byte_stride);
	display_dim_list[0].ui32Width = fb_info.width;
	display_dim_list[0].ui32Height = fb_info.height;
	display_dim_list[0].ui32ByteStride = fb_info.screen_pitch;
	i = 1;
	if (!priv->xserver_running) {
		/* In addition, provide an entire list of dimensions: */
		ret = context->dispatch.query_mode_list((igd_driver_h) context, dc,
			&mode_list, mode_flags);
		if (0 != ret) {
			EMGD_ERROR_EXIT("query_mode_list() returned %d", ret);
			return EMGD_ERROR_GENERIC;
		}
		mode = mode_list;
		while (mode && (mode->width != IGD_TIMING_TABLE_END)) {
			int seen = 0;
			for (j = i - 1 ; j >= 0 ; j--) {
				if ((display_dim_list[j].ui32Width == mode->width) &&
					(display_dim_list[j].ui32Height == mode->height)) {
					seen = 1;
				}
			}
			if (!seen) {
				display_dim_list[i].ui32Width = mode->width;
				display_dim_list[i].ui32Height = mode->height;
				display_dim_list[i].ui32ByteStride =
					(mode->width * IGD_PF_BPP(devinfo->emgd_pf) + 7) >>3;
				EMGD_DEBUG("  Add'l width = %d, height = %d, stride = %ld",
					mode->width, mode->height,
					display_dim_list[i].ui32ByteStride);
				if (++i >= EMGDDC_MAXDIMS) {
					/* Don't write past the end of the array */
					EMGD_ERROR("Reached end of display_dim_list!  Consider "
							"increasing EMGDDC_MAXDIMS.");
					break;
				}
			}
			mode++;
		}
	}
	devinfo->num_dims = i;
	EMGD_DEBUG("Total number of dimensions = %d", i);


	/*
	 * Obtain the size of the frame buffer, which will also be the size of any
	 * swap chain buffers:
	 */
	devinfo->fb_size = devinfo->height * devinfo->byte_stride;
	EMGD_DEBUG("Frame buffer size = %lu = %luMB = 0x%lx", devinfo->fb_size,
		devinfo->fb_size / (1024 * 1024), devinfo->fb_size);


	/*
	 * Initialize the igd_surface_t structure for the frame buffer, in order
	 * allow the buffer flipping code to flip back to the frame buffer:
	 */
	buffer->priv = priv;
	buffer->offset = fb_info.fb_base_offset;
	buffer->pitch = devinfo->byte_stride;
	buffer->width = devinfo->width;
	buffer->height = devinfo->height;
	buffer->pixel_format = fb_info.pixel_format;
	buffer->size = devinfo->fb_size;


	EMGD_TRACE_EXIT;
	return EMGD_OK;

} /* init_display() */

/** 
 * Loops through the avaiable displays, invalidating the  associated flip-chains
 * This function is called from igd_alter_displays so as to resolve any race 
 * conditions that may occur due to performing a flip during a mode-set.
 *
 * @param display     (IN) The display whose flipchains are to be invalidated.
 */
static int emgddc_invalidate_flip_chains(int display) {

	emgddc_devinfo_t * devinfo;
	emgddc_swapchain_t *swap_chain;
	unsigned long lock_flags;
	igd_surface_t surf;
	igd_display_h display_handle;
	igd_context_t *context;
	int i;
	int ret;

	EMGD_TRACE_ENTER;
	EMGD_DEBUG("Parameters:");
	EMGD_DEBUG("display=0x%1x",display);

	for (i = 0; i < MAX_DISPLAYS; i++) {
		if (display & (1 << i)) {
			devinfo = global_devinfo[i];

			if (devinfo == NULL) {
				EMGD_DEBUG("Skipping NULL display at index %d", i);
				continue;
			}


			/* Mode changes invalidate flip-able swap chains.  We can't destroy
			 * them behind the back of PVR services, but we can and should
			 * ignore all pending and future buffer flips for existing swap
			 * chains.
			 * Note: new swap chains will be valid, and be able to perform flips.
			 */
			/* Obtain the lock, to hold-off future interrupt handling for a bit */
			spin_lock_irqsave(&devinfo->swap_chain_lock, lock_flags);
			swap_chain = devinfo->flipable_swapchains;
			while (swap_chain != NULL) {
				swap_chain->valid = EMGD_FALSE;

				flush_flip_queue(swap_chain);
				swap_chain = swap_chain->next;
			}
			/* Now that we've invalidated pending flips, release the lock */
			spin_unlock_irqrestore(&devinfo->swap_chain_lock, lock_flags);

			/* Reset the frame-buffer information to point to the system
			 * buffer */
			context = devinfo->priv->context;
			display_handle = (devinfo->which_devinfo == 1)?
								devinfo->priv->secondary:devinfo->priv->primary;

			surf.offset = devinfo->system_buffer.offset;
			surf.pitch = devinfo->system_buffer.pitch;
			surf.width = devinfo->system_buffer.width;
			surf.height = devinfo->system_buffer.height;
			surf.pixel_format = devinfo->system_buffer.pixel_format;
			surf.flags = IGD_SURFACE_RENDER | IGD_SURFACE_DISPLAY;

			ret = context->dispatch.set_surface(display_handle,
					IGD_PRIORITY_NORMAL, IGD_BUFFER_DISPLAY, &surf, NULL, 0);

			if (ret) {
				EMGD_ERROR("set_surface() returned %d for display at index %d",
						ret, i);
			}
		}
	}

	EMGD_TRACE_EXIT;

	return 0;
}


/**
 * [Re-]Initializes the 3DD's display/device-specific values for both devinfo
 * structures.  This function is called during initializatio time, and whenever
 * the EMGD driver does a potential mode change, via alter_displays().
 *
 * @param dev (IN) The drm_device associated with this driver.
 * @return emgd_error_t value (e.g. EMGD_OK, EMGD_ERROR_OUT_OF_MEMORY).
 */
static int emgddc_reinit_3dd(struct drm_device *dev)
{
	emgddc_devinfo_t *devinfo;
	drm_emgd_priv_t *priv = dev->dev_private;
	int ret;

	EMGD_TRACE_ENTER;

	if (0 == priv->dc) {
		EMGD_DEBUG("not ready to re-init, because no dc has been set");
		if (!priv->hal_running) {
			/* Ensure that devinfo->interrupt_h is NULL: */
			if (NULL != (devinfo = global_devinfo[0])) {
				devinfo->interrupt_h = NULL;
			}
			if (NULL != (devinfo = global_devinfo[1])) {
				devinfo->interrupt_h = NULL;
			}
		}
		return EMGD_OK;
	}

	EMGD_DEBUG("The DC is 0x%08lx", priv->dc);
	EMGD_DEBUG("IGD_DC_CLONE(priv->dc) is %d", IGD_DC_CLONE(priv->dc));
	EMGD_DEBUG("IGD_DC_EXTENDED(priv->dc) is %d", IGD_DC_EXTENDED(priv->dc));

	/* Always initialize the primary devinfo: */
	ret = init_display(global_devinfo[0], priv->primary,
		priv->primary_port_number);
	if (ret != EMGD_OK) {
		return ret;
	}

	/* Initialize the secondary devinfo if we're in DIH/extended mode: */
	if (IGD_DC_EXTENDED(priv->dc)) {
		EMGD_DEBUG("Detected that we're in DIH/EXTENDED mode");

		/* Allocate the devinfo if it hasn't already been allocated: */
		if (NULL == (devinfo = global_devinfo[1])) {
			EMGD_DEBUG("Allocating devinfo structure for DIH/EXTENDED mode");
			devinfo = (emgddc_devinfo_t *) OS_ALLOC(sizeof(emgddc_devinfo_t));
			if (!devinfo) {
				EMGD_ERROR_EXIT("Can not allocate emgddc_devinfo_t structure");
				return EMGD_ERROR_OUT_OF_MEMORY;
			}
			OS_MEMSET(devinfo, 0, sizeof(emgddc_devinfo_t));

			/* Perform static/display-independent initialization: */
			ret = emgddc_init_devinfo(dev, devinfo, 1);
			if (ret != EMGD_OK) {
				EMGD_ERROR_EXIT("CAN NOT support DIH/EXTENDED mode!");
				emgddc_free_a_devinfo(devinfo);
				return ret;
			}
		}

		/* Perform dynamic/display-dependent initialization: */
		ret = init_display(devinfo, priv->secondary,
			priv->secondary_port_number);
		if (ret != EMGD_OK) {
			EMGD_ERROR_EXIT("CAN NOT support DIH/EXTENDED mode!");
			emgddc_free_a_devinfo(devinfo);
			global_devinfo[1] = NULL;
			return ret;
		}

		/* Remember the devinfo, for other functions that aren't passed it: */
		global_devinfo[1] = devinfo;
	}


	EMGD_TRACE_EXIT;
	return EMGD_OK;

} /* emgddc_reinit_3dd() */


/**
 * Master initialization function.  This is called when the EMGD DRM module
 * tells PVR services to start, which calls this function.
 *
 * @param dev (IN) The drm_device for this driver connection.
 * @return emgd_error_t value (e.g. EMGD_OK, EMGD_ERROR_OUT_OF_MEMORY).
 */
emgd_error_t emgddc_init(struct drm_device *dev)
{
	emgddc_devinfo_t *devinfo;
	drm_emgd_priv_t *priv = dev->dev_private;
	int ret;

	EMGD_TRACE_ENTER;


	/* Exit early if trying to initialize again: */
	devinfo = global_devinfo[0];
	if (devinfo != NULL) {
		EMGD_TRACE_EXIT;
		return EMGD_OK;
	}

	/* Always allocate the primary display's devinfo structure: */
	devinfo = (emgddc_devinfo_t *) OS_ALLOC(sizeof(emgddc_devinfo_t));
	if (!devinfo) {
		EMGD_ERROR_EXIT("Can not allocate emgddc_devinfo_t structure");
		return EMGD_ERROR_OUT_OF_MEMORY;
	}
	OS_MEMSET(devinfo, 0, sizeof(emgddc_devinfo_t));
	global_devinfo[0] = devinfo;

	/* Perform static/display-independent initialization: */
	ret = emgddc_init_devinfo(dev, devinfo, 0);
	if (ret != EMGD_OK) {
		emgddc_free_all_devinfos();
		return ret;
	}

	/* Perform dynamic/display-dependent initialization (if we're in
	 * DIH/Extended mode, the secondary devinfo will also be allocated and
	 * initialized):
	 */
	priv->reinit_3dd = emgddc_reinit_3dd;

	/* Used inside igd_alter_displays */
	priv->invalidate_flip_chains = emgddc_invalidate_flip_chains;

	ret = emgddc_reinit_3dd(dev);
	if (ret != EMGD_OK) {
		emgddc_free_all_devinfos();
		return ret;
	}

	/* Remember the devinfo, for other functions that aren't passed it: */
	global_devinfo[0] = devinfo;


	EMGD_TRACE_EXIT;
	return EMGD_OK;

} /* emgddc_init() */


/**
 * Master de-initialization function.  This is called when the EMGD DRM module
 * is being unloaded (it tells PVR services to exit, which calls this
 * function).
 *
 * @return emgd_error_t value (e.g. EMGD_OK, EMGD_ERROR_OUT_OF_MEMORY).
 */
emgd_error_t emgddc_deinit(void)
{
	emgddc_devinfo_t *devinfo;
	int i;
	emgd_error_t ret = EMGD_OK;

	EMGD_TRACE_ENTER;


	for (i = 0 ; i < 2 ; i++) {
		devinfo = global_devinfo[i];
		if (devinfo == NULL) {
			continue;
		}


		/* Unhook and unregister from PVR services: */
		if (devinfo->pvr_jtable.pfnPVRSRVRemoveCmdProcList(devinfo->device_id,
			EMGDDC_COMMAND_COUNT) != PVRSRV_OK) {
			ret = EMGD_ERROR_GENERIC;
		}
		if (devinfo->pvr_jtable.pfnPVRSRVRemoveDCDevice(devinfo->device_id) !=
			PVRSRV_OK) {
			ret = EMGD_ERROR_GENERIC;
		}


		/* Delete any/all swap chains now: */
		/* Note: If we ever support interrupts, there may be a race condition
		 * of pending flips.  This was placed here, after the PVR services
		 * thinks the driver has gone away, so that no flips should come by
		 * this time.
		 */
		while (devinfo->flipable_swapchains) {
			if (PVRSRV_OK != DestroyDCSwapChain(devinfo,
				devinfo->flipable_swapchains)) {
				ret = EMGD_ERROR_GENERIC;
			}
		}
		while (devinfo->pixmap_swapchains) {
			if (PVRSRV_OK != DestroyDCSwapChain(devinfo,
				devinfo->pixmap_swapchains)) {
				ret = EMGD_ERROR_GENERIC;
			}
		}


		emgddc_free_a_devinfo(devinfo);
		global_devinfo[i] = NULL;
	}


	EMGD_TRACE_EXIT;
	return ret;

} /* emgddc_deinit() */


/*
 * Potentially perform a mode change.
 *
 * If the X server is running, PVR services (including this function) can't
 * do mode changes.  To avoid having to switch modes, ensure that the
 * current pixel format and dimension is specified:
 *
 * If the X server is NOT running, PVR services (including this function)
 * can do mode changes.  In this case, if the pixel format and/or
 * dimensions don't match the current mode, perform a mode change (as long
 * as valid values are provided):
 */
static PVRSRV_ERROR do_mode_change(igd_context_t *context,
		emgddc_devinfo_t *devinfo,
		drm_emgd_priv_t *priv,
		DISPLAY_SURF_ATTRIBUTES *dst_surf_attrib)
{
	struct drm_device* drm_dev;
	int err = PVRSRV_OK;

	drm_dev = devinfo->drm_device;

	if (priv->xserver_running) {
		if ((dst_surf_attrib->pixelformat != devinfo->pvr_pf) ||
			(dst_surf_attrib->sDims.ui32ByteStride != devinfo->byte_stride) ||
			(dst_surf_attrib->sDims.ui32Width != devinfo->width) ||
			(dst_surf_attrib->sDims.ui32Height != devinfo->height)) {
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	} else if ((dst_surf_attrib->pixelformat != devinfo->pvr_pf) ||
			(dst_surf_attrib->sDims.ui32ByteStride != devinfo->byte_stride)||
			(dst_surf_attrib->sDims.ui32Width != devinfo->width) ||
			(dst_surf_attrib->sDims.ui32Height != devinfo->height)) {
		unsigned long emgd_pf = pvr2emgd_pf(dst_surf_attrib->pixelformat);
		igd_display_info_t *mode_list = NULL;
		igd_display_info_t *mode = NULL;
		igd_display_info_t *desired_mode = NULL;
		int mode_flags = IGD_QUERY_LIVE_MODES;
		unsigned long byte_stride;
		igd_framebuffer_info_t primary_fb_info;
		igd_framebuffer_info_t secondary_fb_info;
		igd_display_h primary;
		igd_display_h secondary;

		printk("[EMGD] A mode change is requested.  The following new values\n"
			"[EMGD] will be checked, and if good, take effect:\n");
		printk("[EMGD]   pixel format = %u (PVR) = 0x%08lx (EMGD)\n",
			dst_surf_attrib->pixelformat, emgd_pf);
		printk("[EMGD]   width = %lu, height = %lu\n",
			dst_surf_attrib->sDims.ui32Width,
			dst_surf_attrib->sDims.ui32Height);
		printk("[EMGD]   stride = %lu\n",dst_surf_attrib->sDims.ui32ByteStride);


		/* Check the pixel format: */
		if (IGD_PF_UNKNOWN == emgd_pf) {
			printk(KERN_ERR "Unknown pixel format %u\n",
				dst_surf_attrib->pixelformat);
			return PVRSRV_ERROR_INVALID_PARAMS;
		}


		/* Check the width, height, and stride: */
		EMGD_DEBUG("Calling query_mode_list()");
		err = context->dispatch.query_mode_list(context, priv->dc,
			&mode_list, mode_flags);
		if (err) {
			printk(KERN_ERR "The query_mode_list() function returned %d.", err);
			return PVRSRV_ERROR_FAILED_DEPENDENCIES;
		}
		EMGD_DEBUG("Comparing the mode list with the desired width, height, "
				"and stride...");
		mode = mode_list;
		while (mode && (mode->width != IGD_TIMING_TABLE_END)) {
			byte_stride =  IGD_PF_PIXEL_BYTES(emgd_pf, mode->width);
			EMGD_DEBUG(" ... Found a mode with width=%d, height=%d, "
					"refresh=%d;", mode->width, mode->height, mode->refresh);
			if ((mode->width == dst_surf_attrib->sDims.ui32Width) &&
				(mode->height == dst_surf_attrib->sDims.ui32Height) &&
				(byte_stride == dst_surf_attrib->sDims.ui32ByteStride)) {
				EMGD_DEBUG("     ... This mode is a match!");
				desired_mode = mode;
				break;
			}
			mode++;
		}
		if (NULL == desired_mode) {
			printk(KERN_ERR "No mode matching the desired width (%lu), height "
				"(%lu), and stride (%lu) was found.",
				dst_surf_attrib->sDims.ui32Width,
				dst_surf_attrib->sDims.ui32Height,
				dst_surf_attrib->sDims.ui32ByteStride);
			return PVRSRV_ERROR_FAILED_DEPENDENCIES;
		} else {
			/* Must set this in order to get the timings setup: */
			desired_mode->flags |= IGD_DISPLAY_ENABLE;
		}


		/* Make the mode change by calling alter_displays(): */
		primary_fb_info.width = desired_mode->width;
		primary_fb_info.height = desired_mode->height;
		primary_fb_info.pixel_format = emgd_pf;
		primary_fb_info.flags = 0;
		primary_fb_info.allocated = 0;
		memcpy(&secondary_fb_info, &primary_fb_info,
				sizeof(igd_framebuffer_info_t));

		EMGD_DEBUG("Calling alter_displays()");
		err = context->dispatch.alter_displays(context,
			&primary, desired_mode, &primary_fb_info,
			&secondary, desired_mode, &secondary_fb_info, priv->dc, 0);
		if (err) {
			printk(KERN_ERR "The alter_displays() function returned %d.", err);
			return PVRSRV_ERROR_FAILED_DEPENDENCIES;
		}


		/* Update the private copy, like emgd_alter_displays() would do: */
		priv->primary = primary;
		priv->secondary = secondary;
		priv->primary_port_number = (priv->dc & 0xf0) >> 4;
		priv->secondary_port_number = (priv->dc & 0xf00000) >> 20;


		/* Re-initialize the display values: */
		err = priv->reinit_3dd(drm_dev);
		if (err != EMGD_OK) {
			printk(KERN_ERR "The reinit_3dd() function returned %d.", err);
			return PVRSRV_ERROR_FAILED_DEPENDENCIES;
		}
	} /* end of mode-change code */

	return err;
} /* do_mode_change() */
