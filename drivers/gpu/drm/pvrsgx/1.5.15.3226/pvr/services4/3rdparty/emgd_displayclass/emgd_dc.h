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

#ifndef EMGD_DC_H
#define EMGD_DC_H

#include <drm/drmP.h>
#include "memory.h"
#include "io.h"
#include "emgd_shared.h"
#include "kerneldisplay.h"


#define EMGDDC_MAXFORMATS 20
#define EMGDDC_MAXDIMS 20


#define DISPLAY_DEVICE_NAME "Intel EMGD Display Driver"
#define	DRVNAME	"emgd_dc"
#define	DEVNAME	DRVNAME
#define	DRIVER_PREFIX DRVNAME


#define	EMGD_TRACE_STUB \
	EMGD_DEBUG("Inside the stubbed %s() function", __FUNCTION__)


typedef struct _emgddc_devinfo  emgddc_devinfo_t;


typedef enum tag_emgd_bool
{
	EMGD_FALSE = 0,
	EMGD_TRUE  = 1,
} emgd_bool;


/**
 * A pair of equivalent pixel formats, one in the EMGD format, one in the PVR
 * format.  This structure is used to translate between the two formats.
 */
typedef struct _emgddc_pixelformat_translator
{
	/** An EMGD-formatted pixel format. */
	unsigned long emgd_pf;

	/** A PVR-formatted pixel format. */
	PVRSRV_PIXEL_FORMAT pvr_pf;
}  emgddc_pixelformat_translator_t;


/** Information about a given swap chain buffer. */
typedef struct _emgddc_buffer
{
	/** A copy of the drm_emgd_private associated with this buffer. */
	drm_emgd_priv_t *priv;

	/** The GMM offset of this buffer's memory. */
	unsigned long offset;

	/** This buffer's EMGD-formatted pixel format. */
	unsigned long pixel_format;

	/** This buffer's width. */
	unsigned long width;

	/** This buffer's height. */
	unsigned long height;

	/**
	 * This buffer's stride (i.e. number of bytes between the same point on
	 * successive scanlines).
	 */
	unsigned long pitch;

	/** This buffer's size. */
	unsigned long size;

	/** Is this buffer the framebuffer (a.k.a. system buffer). */
	int is_fb;

	/**
	 * The memory-mapped virtual address of the buffer (obtained from
	 * gmm_map())
	 */
	IMG_CPU_VIRTADDR virt_addr;

	/** The PVR sync data for this buffer */
	PVRSRV_SYNC_DATA *sync_data;

	unsigned long is_contiguous;

	/**
	 * The PVR next buffer in the swap chain (the last buffer points to the
	 * first buffer
	 */
	struct _emgddc_buffer *next;
} emgddc_buffer_t;


/** Information for queueing a flip for a given swap chain buffer. */
typedef struct _emgddc_flip_queue_item
{
	/**
	 * A handle to a PVR services data structure that must be passed to
	 * PVRSRVCmdComplete(), in order to "complete" a flip (i.e. when PVR
	 * services knows that the 3DD has caused the flip to occur, and as such,
	 * when its queue-processing code can call emgddc_process_flip() to
	 * start/queue another flip).  "Completing" a flip does not mean that the
	 * flip has reached the end of its swap_interval and been "cleared."
	 */
	IMG_HANDLE cmd_complete;

	/**
	 * The number of VBlanks after a flip occurs, before the next flip can
	 * start.
	 */
	unsigned long swap_interval;

	/** Whether the corresponding buffer has a flip in progress. */
	emgd_bool valid;

	/**
	 * Whether the corresponding buffer completed a flip.  This can only be
	 * EMGD_TRUE if valid is also EMGD_TRUE.
	 */
	emgd_bool flipped;

	/**
	 * Whether the corresponding buffer told PVR services that it completed a
	 * flip.  This can only be EMGD_TRUE if flipped is also EMGD_TRUE.
	 */
	emgd_bool cmd_completed;

	/** Which buffer is associated with this flip_item. */
	emgddc_buffer_t *buffer;

} emgddc_flip_queue_item_t;


/** Information about a given swap chain. */
typedef struct _emgddc_swapchain
{
	/**
	 * Pointer to the parent devinfo, so that emgddc_flip() can lookup which
	 * device (and therefore, which igd_display_t) to use.
	 */
	emgddc_devinfo_t *devinfo;

	/** Whether this swap chain is valid for buffer flipping. */
	emgd_bool valid;

	/** Number of buffers in this swap chain. */
	unsigned long buffer_count;

	/** An array of the buffers in this swap chain. */
	emgddc_buffer_t *buffers;

	/** An array of the structures that keep track of queued flips.  Each flip
	 * item corresponds to the identically-numbered buffer in this swap chain.
	 */
	emgddc_flip_queue_item_t	*flip_queue;

	/** Index into both the buffers and flip_queue arrays.  Used when queueing
	 * (or inserting) flips.  Is incremented until it gets too large, and then
	 * is zero'd-out.
	 */
	unsigned long insert_index;

	/** Index into both the buffers and flip_queue arrays.  Used when flushing
	 * (or removing from) the flip queue.  Is incremented until it gets too
	 * large, and then is zero'd-out.
	 */
	unsigned long remove_index;

#ifdef SUPPORT_FB_EVENTS
	emgd_bool blanked;
#endif /* SUPPORT_FB_EVENTS */

	/** Type of swap chain */
	unsigned long flags;

	/** A copy of emgddc_devinfo_t.pvr_jtable. */
	PVRSRV_DC_DISP2SRV_KMJTABLE	*pvr_jtable;

	/** Next swap chain in the list */
	struct _emgddc_swapchain *next;
} emgddc_swapchain_t;


/** Information about the device. */
struct _emgddc_devinfo
{
	/*
	 * The following values are display-dependent, and are dynamically
	 * re-initialized after an EMGD alter_displays() call:
	 */

	/** The size (in bytes) of the framebuffer and future swap chain buffers. */
	unsigned long fb_size;

	/** The width (in pixels) of the framebuffer. */
	unsigned long width;

	/** The height (in pixels) of the framebuffer. */
	unsigned long height;

	/** The stride (in bytes) from one row of the framebuffer to another. */
	unsigned long byte_stride;

	/* The number of dimensions for the primary display: */
	unsigned long num_dims;

	/* The supported dimensions for the primary display: */
	DISPLAY_DIMS display_dim_list[EMGDDC_MAXDIMS];

	/** The one EMGD-formatted pixel format (before translation) being used. */
	unsigned long emgd_pf;

	/** The one PVR-formatted pixel format (after translation) being used. */
	PVRSRV_PIXEL_FORMAT pvr_pf;

	/** The number of pixel formats for the primary display. */
	unsigned long num_formats;

	/** The supported PVR pixel formats for the primary display. */
	DISPLAY_FORMAT display_format_list[EMGDDC_MAXFORMATS];

	/** The frame buffer (a.k.a. "system buffer" or "front buffer"). */
	emgddc_buffer_t system_buffer;


	/*
	 * The following values are display-independent, are statically
	 * initialized at start-up time, and some may be altered during run time:
	 */

	/** 0 for the primary devinfo, 1 for the secondary/DIH mode devinfo */
	int which_devinfo;

	/** A copy of the drm_emgd_private associated with this devinfo. */
	drm_emgd_priv_t *priv;

	/** Private copy of the drm_device associated with this devinfo. */
	struct drm_device *drm_device;

	/**
	 * The first of potentially multiple swap chains that support full-screen
	 * buffer flipping.
	 */
	emgddc_swapchain_t *flipable_swapchains;

	/** The first of potentially many swap chains that support pixmaps. */
	emgddc_swapchain_t *pixmap_swapchains;

	/** Counter to provide unique IDs to each swap chain. */
	unsigned long swap_chain_id_counter;

	/** Static information about swap chains, plus the driver's string name. */
	DISPLAY_INFO display_info;

	/** HAL handle associated with this devinfo for dealing with interrupts. */
	emgd_vblank_callback_h interrupt_h;

	/** Handle very rare case of not being able to [re-]enable interrupts. */
	emgd_bool flipping_disabled;

	/** If EMGD_TRUE, don't queue buffer flips, but flip/complete right away. */
	emgd_bool flush_commands;

	/**
	 * A counter so that the state of flush_commands must be set to EMGD_TRUE
	 * and EMGD_FALSE an equal number of times before the state actually
	 * changes.
	 */
	unsigned long set_flush_state_ref_count;

	/**
	 * A spinlock to prevent contention between regular code and
	 * interrupt-handling code.
	 */
	spinlock_t swap_chain_lock;

#ifdef SUPPORT_FB_EVENTS
	struct notifier_block lin_notif_block;
#endif /* SUPPORT_FB_EVENTS */

	/** PVR services functions. */
	PVRSRV_DC_DISP2SRV_KMJTABLE	pvr_jtable;

	/** Functions we provide to PVR services. */
	PVRSRV_DC_SRV2DISP_KMJTABLE	dc_jtable;

	/** This device's numeric ID, obtained when registering with PVR services */
	unsigned long device_id;

};


typedef enum _emgd_error
{
	EMGD_OK                             =  0,
	EMGD_ERROR_GENERIC                  =  1,
	EMGD_ERROR_OUT_OF_MEMORY            =  2,
	EMGD_ERROR_TOO_FEW_BUFFERS          =  3,
	EMGD_ERROR_INVALID_PARAMS           =  4,
	EMGD_ERROR_INIT_FAILURE             =  5,
	EMGD_ERROR_CANT_REGISTER_CALLBACK   =  6,
	EMGD_ERROR_INVALID_DEVICE           =  7,
	EMGD_ERROR_DEVICE_REGISTER_FAILED   =  8
} emgd_error_t;


#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif


emgd_error_t emgddc_init(struct drm_device * dev);
emgd_error_t emgddc_deinit(void);

void emgddc_flip(emgddc_swapchain_t *swap_chain, emgddc_buffer_t *buffer);

#endif
