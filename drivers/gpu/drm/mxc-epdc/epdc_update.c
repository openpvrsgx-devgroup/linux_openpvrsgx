// SPDX-License-Identifier: GPL-2.0+
// Copyright (C) 2020 Andreas Kemnade
//
/*
 * based on the EPDC framebuffer driver
 * Copyright (C) 2010-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 */

#include <linux/bits.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <asm/cacheflush.h>
#include "mxc_epdc.h"
#include "epdc_hw.h"
#include "epdc_regs.h"
#include "epdc_waveform.h"

#define EPDC_V2_NUM_LUTS	64
#define EPDC_V2_MAX_NUM_UPDATES 64
#define INVALID_LUT	     (-1)
#define DRY_RUN_NO_LUT	  100

#define MERGE_OK	0
#define MERGE_FAIL      1
#define MERGE_BLOCK     2

struct update_desc_list {
	struct list_head list;
	struct mxcfb_update_data upd_data;/* Update parameters */
	u32 epdc_offs;		/* Added to buffer ptr to resolve alignment */
	u32 epdc_stride;	/* Depends on rotation & whether we skip PxP */
	u32 update_order;	/* Numeric ordering value for update */
};

/* This structure represents a list node containing both
 * a memory region allocated as an output buffer for the PxP
 * update processing task, and the update description (mode, region, etc.)
 */
struct update_data_list {
	struct list_head list;
	dma_addr_t phys_addr;	/* Pointer to phys address of processed Y buf */
	void *virt_addr;
	struct update_desc_list *update_desc;
	int lut_num;		/* Assigned before update is processed into working buffer */
	u64 collision_mask;	/* Set when update creates collision */
				/* Mask of the LUTs the update collides with */
};

/********************************************************
 * Start Low-Level EPDC Functions
 ********************************************************/

static inline void epdc_lut_complete_intr(struct mxc_epdc *priv, u32 lut_num, bool enable)
{
	if (priv->rev < 20) {
		if (enable)
			epdc_write(priv, EPDC_IRQ_MASK_SET, BIT(lut_num));
		else
			epdc_write(priv, EPDC_IRQ_MASK_CLEAR, BIT(lut_num));
	} else {
		if (enable) {
			if (lut_num < 32)
				epdc_write(priv, EPDC_IRQ_MASK1_SET, BIT(lut_num));
			else
				epdc_write(priv, EPDC_IRQ_MASK2_SET, BIT(lut_num - 32));
		} else {
			if (lut_num < 32)
				epdc_write(priv, EPDC_IRQ_MASK1_CLEAR, BIT(lut_num));
			else
				epdc_write(priv, EPDC_IRQ_MASK2_CLEAR, BIT(lut_num - 32));
		}
	}
}

static inline void epdc_working_buf_intr(struct mxc_epdc *priv, bool enable)
{
	if (enable)
		epdc_write(priv, EPDC_IRQ_MASK_SET, EPDC_IRQ_WB_CMPLT_IRQ);
	else
		epdc_write(priv, EPDC_IRQ_MASK_CLEAR, EPDC_IRQ_WB_CMPLT_IRQ);
}

static inline void epdc_clear_working_buf_irq(struct mxc_epdc *priv)
{
	epdc_write(priv, EPDC_IRQ_CLEAR,
		   EPDC_IRQ_WB_CMPLT_IRQ | EPDC_IRQ_LUT_COL_IRQ);
}

static inline void epdc_eof_intr(struct mxc_epdc *priv, bool enable)
{
	if (enable)
		epdc_write(priv, EPDC_IRQ_MASK_SET, EPDC_IRQ_FRAME_END_IRQ);
	else
		epdc_write(priv, EPDC_IRQ_MASK_CLEAR, EPDC_IRQ_FRAME_END_IRQ);
}

static inline void epdc_clear_eof_irq(struct mxc_epdc *priv)
{
	epdc_write(priv, EPDC_IRQ_CLEAR, EPDC_IRQ_FRAME_END_IRQ);
}

static inline bool epdc_signal_eof(struct mxc_epdc *priv)
{
	return (epdc_read(priv, EPDC_IRQ_MASK) & epdc_read(priv, EPDC_IRQ)
		& EPDC_IRQ_FRAME_END_IRQ) ? true : false;
}


static void epdc_set_update_area(struct mxc_epdc *priv, u32 addr,
				 u32 x, u32 y, u32 width, u32 height,
				 u32 stride)
{
	u32 val;

	epdc_write(priv, EPDC_UPD_ADDR, addr);
	val = (y << EPDC_UPD_CORD_YCORD_OFFSET) | x;
	epdc_write(priv, EPDC_UPD_CORD, val);
	val = (height << EPDC_UPD_SIZE_HEIGHT_OFFSET) | width;
	epdc_write(priv, EPDC_UPD_SIZE, val);
	epdc_write(priv, EPDC_UPD_STRIDE, stride);
}

static bool is_free_list_full(struct mxc_epdc *priv)
{
	int count = 0;
	struct update_data_list *plist;

	/* Count buffers in free buffer list */
	list_for_each_entry(plist, &priv->upd_buf_free_list, list)
		count++;

	/* Check to see if all buffers are in this list */
	return (count == priv->max_num_updates);
}

static inline bool epdc_is_lut_complete(struct mxc_epdc *priv, u32 lut_num)
{
	u32 val;
	bool is_compl;

	if (priv->rev < 20) {
		val = epdc_read(priv, EPDC_IRQ);
		is_compl = val & BIT(lut_num) ? true : false;
	} else if (lut_num < 32) {
		val = epdc_read(priv, EPDC_IRQ1);
		is_compl = val & BIT(lut_num) ? true : false;
	} else {
		val = epdc_read(priv, EPDC_IRQ2);
		is_compl = val & BIT(lut_num - 32) ? true : false;
	}

	return is_compl;
}
static inline void epdc_clear_lut_complete_irq(struct mxc_epdc *priv, u32 lut_num)
{
	if (priv->rev < 20)
		epdc_write(priv, EPDC_IRQ_CLEAR, BIT(lut_num));
	else if (lut_num < 32)
		epdc_write(priv, EPDC_IRQ1_CLEAR, BIT(lut_num));
	else
		epdc_write(priv, EPDC_IRQ2_CLEAR, BIT(lut_num - 32));
}

static inline bool epdc_is_working_buffer_busy(struct mxc_epdc *priv)
{
	u32 val = epdc_read(priv, EPDC_STATUS);
	bool is_busy = (val & EPDC_STATUS_WB_BUSY) ? true : false;

	return is_busy;
}

static inline bool epdc_is_working_buffer_complete(struct mxc_epdc *priv)
{
	u32 val = epdc_read(priv, EPDC_IRQ);
	bool is_compl = (val & EPDC_IRQ_WB_CMPLT_IRQ) ? true : false;

	return is_compl;
}

static inline bool epdc_is_lut_cancelled(struct mxc_epdc *priv)
{
	u32 val = epdc_read(priv, EPDC_STATUS);
	bool is_void = (val & EPDC_STATUS_UPD_VOID) ? true : false;

	return is_void;
}

static inline bool epdc_is_collision(struct mxc_epdc *priv)
{
	u32 val = epdc_read(priv, EPDC_IRQ);

	return (val & EPDC_IRQ_LUT_COL_IRQ) ? true : false;
}

static inline u64 epdc_get_colliding_luts(struct mxc_epdc *priv)
{
	u64 val = (u64)(epdc_read(priv, EPDC_STATUS_COL));

	val |= (u64)epdc_read(priv, EPDC_STATUS_COL2) << 32;
	return val;
}

static irqreturn_t mxc_epdc_irq_handler(int irq, void *dev_id)
{
	struct mxc_epdc *priv = dev_id;
	u32 ints_fired, luts1_ints_fired, luts2_ints_fired;

	/*
	 * If we just completed one-time panel init, bypass
	 * queue handling, clear interrupt and return
	 */
	if (priv->in_init) {
		if (epdc_is_working_buffer_complete(priv)) {
			epdc_working_buf_intr(priv, false);
			epdc_clear_working_buf_irq(priv);
			dev_dbg(priv->drm.dev, "Cleared WB for init update\n");
		}

		if (epdc_is_lut_complete(priv, 0)) {
			epdc_lut_complete_intr(priv, 0, false);
			epdc_clear_lut_complete_irq(priv, 0);
			priv->in_init = false;
			dev_dbg(priv->drm.dev, "Cleared LUT complete for init update\n");
		}

		return IRQ_HANDLED;
	}

	ints_fired = epdc_read(priv, EPDC_IRQ_MASK) & epdc_read(priv, EPDC_IRQ);

	luts1_ints_fired = epdc_read(priv, EPDC_IRQ_MASK1) & epdc_read(priv, EPDC_IRQ1);
	luts2_ints_fired = epdc_read(priv, EPDC_IRQ_MASK2) & epdc_read(priv, EPDC_IRQ2);

	if (!(ints_fired || luts1_ints_fired || luts2_ints_fired))
		return IRQ_HANDLED;

	if (epdc_read(priv, EPDC_IRQ) & EPDC_IRQ_TCE_UNDERRUN_IRQ) {
		dev_err(priv->drm.dev,
			"TCE underrun! Will continue to update panel\n");
		/* Clear TCE underrun IRQ */
		epdc_write(priv, EPDC_IRQ_CLEAR, EPDC_IRQ_TCE_UNDERRUN_IRQ);
	}

	/* Check if we are waiting on EOF to sync a new update submission */
	if (epdc_signal_eof(priv)) {
		epdc_eof_intr(priv, false);
		epdc_clear_eof_irq(priv);
	}

	/*
	 * Workaround for EPDC v2.0/v2.1 errata: Must read collision status
	 * before clearing IRQ, or else collision status for bits 16:63
	 * will be automatically cleared.  So we read it here, and there is
	 * no conflict with using it in epdc_intr_work_func since the
	 * working buffer processing flow is strictly sequential (i.e.,
	 * only one WB processing done at a time, so the data grabbed
	 * here should be up-to-date and accurate when the WB processing
	 * completes.  Also, note that there is no impact to other versions
	 * of EPDC by reading LUT status here.
	 */
	if (priv->cur_update != NULL)
		priv->epdc_colliding_luts = epdc_get_colliding_luts(priv);

	/* Clear the interrupt mask for any interrupts signalled */
	epdc_write(priv, EPDC_IRQ_MASK_CLEAR, ints_fired);
	epdc_write(priv, EPDC_IRQ_MASK1_CLEAR, luts1_ints_fired);
	epdc_write(priv, EPDC_IRQ_MASK2_CLEAR, luts2_ints_fired);

	dev_dbg(priv->drm.dev,
		"EPDC interrupts fired = 0x%x, LUTS1 fired = 0x%x, LUTS2 fired = 0x%x\n",
		ints_fired, luts1_ints_fired, luts2_ints_fired);

	queue_work(priv->epdc_intr_workqueue,
		&priv->epdc_intr_work);


	return IRQ_HANDLED;
}

static void cleanup_update_list(void *data)
{
	struct mxc_epdc *priv = (struct mxc_epdc *)data;
	struct update_data_list *plist, *temp_list;

	list_for_each_entry_safe(plist, temp_list, &priv->upd_buf_free_list,
			list) {
		list_del(&plist->list);
		kfree(plist);
	}
}

static void epdc_done_work_func(struct work_struct *work)
{
	struct mxc_epdc *priv =
		container_of(work, struct mxc_epdc,
			epdc_done_work.work);
	mxc_epdc_powerdown(priv);
}

static int epdc_submit_merge(struct update_desc_list *upd_desc_list,
				struct update_desc_list *update_to_merge,
				struct mxc_epdc *priv)
{
	struct mxcfb_update_data *a, *b;
	struct drm_rect *arect, *brect;

	a = &upd_desc_list->upd_data;
	b = &update_to_merge->upd_data;
	arect = &upd_desc_list->upd_data.update_region;
	brect = &update_to_merge->upd_data.update_region;

	if (a->update_mode != b->update_mode)
		a->update_mode = UPDATE_MODE_FULL;

	if (a->waveform_mode != b->waveform_mode)
		a->waveform_mode = WAVEFORM_MODE_AUTO;

	if ((arect->x1 > brect->x2) ||
	    (brect->x1 > arect->x2) ||
	    (arect->y1 > brect->y2) ||
	    (brect->y1 > arect->y2))
		return MERGE_FAIL;

	arect->x1 = min(arect->x1, brect->x1);
	arect->x2 = max(arect->x2, brect->x2);
	arect->y1 = min(arect->y1, brect->y1);
	arect->y2 = max(arect->y2, brect->y2);

	/* Merged update should take on the earliest order */
	upd_desc_list->update_order =
		(upd_desc_list->update_order > update_to_merge->update_order) ?
		upd_desc_list->update_order : update_to_merge->update_order;

	return MERGE_OK;
}

static void epdc_from_rgb_clear_lower_nibble(struct drm_rect *clip, void *vaddr, int pitch, u8 *dst, int dst_pitch)
{
	unsigned int x, y;

	dst += clip->y1 * dst_pitch;

	for (y = clip->y1; y < clip->y2; y++, dst += dst_pitch) {
		u32 *src;
		src = vaddr + (y * pitch);
		src += clip->x1;
		for (x = clip->x1; x < clip->x2; x++) {
			u8 r = (*src & 0x00ff0000) >> 16;
			u8 g = (*src & 0x0000ff00) >> 8;
			u8 b =  *src & 0x000000ff;

			/* ITU BT.601: Y = 0.299 R + 0.587 G + 0.114 B */
			u8 gray = (3 * r + 6 * g + b) / 10;

			/*
			 * done in Tolino 3.0.x kernels via PXP_LUT_AA
			 * needed for 5 bit waveforms 
			 */

			dst[x] = gray & 0xF0;
			src++;
		}
	}
}

/* found by experimentation, reduced number of levels of gray */
static void epdc_from_rgb_shift(struct drm_rect *clip, void *vaddr, int pitch, u8 *dst, int dst_pitch)
{
	unsigned int x, y;

	dst += clip->y1 * dst_pitch;

	for (y = clip->y1; y < clip->y2; y++, dst += dst_pitch) {
		u32 *src;
		src = vaddr + (y * pitch);
		src += clip->x1;
		for (x = clip->x1; x < clip->x2; x++) {
			u8 r = (*src & 0x00ff0000) >> 16;
			u8 g = (*src & 0x0000ff00) >> 8;
			u8 b =  *src & 0x000000ff;

			/* ITU BT.601: Y = 0.299 R + 0.587 G + 0.114 B */
			u8 gray = (3 * r + 6 * g + b) / 10;
			dst[x] = (gray >> 2) | 0xC0;
			src++;
		}
	}
}

static void epdc_submit_update(struct mxc_epdc *priv,
			       u32 lut_num, u32 waveform_mode, u32 update_mode,
			       bool use_dry_run, bool use_test_mode, u32 np_val)
{
	u32 reg_val = 0;

	if (use_test_mode) {
		reg_val |=
		    ((np_val << EPDC_UPD_FIXED_FIXNP_OFFSET) &
		     EPDC_UPD_FIXED_FIXNP_MASK) | EPDC_UPD_FIXED_FIXNP_EN;

		epdc_write(priv, EPDC_UPD_FIXED, reg_val);

		reg_val = EPDC_UPD_CTRL_USE_FIXED;
	} else {
		epdc_write(priv, EPDC_UPD_FIXED, reg_val);
	}

	if (waveform_mode == WAVEFORM_MODE_AUTO)
		reg_val |= EPDC_UPD_CTRL_AUTOWV;
	else
		reg_val |= ((waveform_mode <<
				EPDC_UPD_CTRL_WAVEFORM_MODE_OFFSET) &
				EPDC_UPD_CTRL_WAVEFORM_MODE_MASK);

	reg_val |= (use_dry_run ? EPDC_UPD_CTRL_DRY_RUN : 0) |
	    ((lut_num << EPDC_UPD_CTRL_LUT_SEL_OFFSET) &
	     EPDC_UPD_CTRL_LUT_SEL_MASK) |
	    update_mode;
	epdc_write(priv, EPDC_UPD_CTRL, reg_val);
}

static inline bool epdc_is_lut_active(struct mxc_epdc *priv, u32 lut_num)
{
	u32 val;
	bool is_active;

	if (lut_num < 32) {
		val = epdc_read(priv, EPDC_STATUS_LUTS);
		is_active = val & BIT(lut_num) ? true : false;
	} else {
		val = epdc_read(priv, EPDC_STATUS_LUTS2);
		is_active = val & BIT(lut_num - 32) ? true : false;
	}

	return is_active;
}

static inline bool epdc_any_luts_active(struct mxc_epdc *priv)
{
	bool any_active;

	if (priv->rev < 20)
		any_active = epdc_read(priv, EPDC_STATUS_LUTS) ? true : false;
	else
		any_active = (epdc_read(priv, EPDC_STATUS_LUTS) |
			epdc_read(priv, EPDC_STATUS_LUTS2))	? true : false;

	return any_active;
}

static inline bool epdc_any_luts_available(struct mxc_epdc *priv)
{
	bool luts_available =
	    (epdc_read(priv, EPDC_STATUS_NEXTLUT) &
	     EPDC_STATUS_NEXTLUT_NEXT_LUT_VALID) ? true : false;
	return luts_available;
}

static inline int epdc_get_next_lut(struct mxc_epdc *priv)
{
	u32 val =
	    epdc_read(priv, EPDC_STATUS_NEXTLUT) &
	    EPDC_STATUS_NEXTLUT_NEXT_LUT_MASK;
	return val;
}

static int epdc_choose_next_lut(struct mxc_epdc *priv, int *next_lut)
{
	u64 luts_status, unprocessed_luts, used_luts;
	/* Available LUTs are reduced to 16 in 5-bit waveform mode */
	bool format_p5n = ((epdc_read(priv, EPDC_FORMAT) &
	EPDC_FORMAT_BUF_PIXEL_FORMAT_MASK) ==
	EPDC_FORMAT_BUF_PIXEL_FORMAT_P5N);

	luts_status = epdc_read(priv, EPDC_STATUS_LUTS);
	if ((priv->rev < 20) || format_p5n)
		luts_status &= 0xFFFF;
	else
		luts_status |= ((u64)epdc_read(priv, EPDC_STATUS_LUTS2) << 32);

	if (priv->rev < 20) {
		unprocessed_luts = epdc_read(priv, EPDC_IRQ) & 0xFFFF;
	} else {
		unprocessed_luts = epdc_read(priv, EPDC_IRQ1) |
			((u64)epdc_read(priv, EPDC_IRQ2) << 32);
		if (format_p5n)
			unprocessed_luts &= 0xFFFF;
	}

	/*
	 * Note on unprocessed_luts: There is a race condition
	 * where a LUT completes, but has not been processed by
	 * IRQ handler workqueue, and then a new update request
	 * attempts to use that LUT.  We prevent that here by
	 * ensuring that the LUT we choose doesn't have its IRQ
	 * bit set (indicating it has completed but not yet been
	 * processed).
	 */
	used_luts = luts_status | unprocessed_luts;

	/*
	 * Selecting a LUT to minimize incidence of TCE Underrun Error
	 * --------------------------------------------------------
	 * We want to find the lowest order LUT that is of greater
	 * order than all other active LUTs.  If highest order LUT
	 * is active, then we want to choose the lowest order
	 * available LUT.
	 *
	 * NOTE: For EPDC version 2.0 and later, TCE Underrun error
	 *       bug is fixed, so it doesn't matter which LUT is used.
	 */

	if ((priv->rev < 20) || format_p5n) {
		*next_lut = fls64(used_luts);
		if (*next_lut > 15)
			*next_lut = ffz(used_luts);
	} else {
		if ((u32)used_luts != ~0UL)
			*next_lut = ffz((u32)used_luts);
		else if ((u32)(used_luts >> 32) != ~0UL)
			*next_lut = ffz((u32)(used_luts >> 32)) + 32;
		else
			*next_lut = INVALID_LUT;
	}

	if (used_luts & 0x8000)
		return 1;
	else
		return 0;
}




static void epdc_submit_work_func(struct work_struct *work)
{
	struct update_data_list *next_update, *temp_update;
	struct update_desc_list *next_desc, *temp_desc;
	struct mxc_epdc *priv =
		container_of(work, struct mxc_epdc, epdc_submit_work);
	struct update_data_list *upd_data_list = NULL;
	struct drm_rect adj_update_region, *upd_region;
	bool end_merge = false;
	u32 update_addr;
	uint8_t *update_addr_virt;
	int ret;

	/* Protect access to buffer queues and to update HW */
	mutex_lock(&priv->queue_mutex);

	/*
	 * Are any of our collision updates able to go now?
	 * Go through all updates in the collision list and check to see
	 * if the collision mask has been fully cleared
	 */
	list_for_each_entry_safe(next_update, temp_update,
				&priv->upd_buf_collision_list, list) {

		if (next_update->collision_mask != 0)
			continue;

		dev_dbg(priv->drm.dev, "A collision update is ready to go!\n");

		/* Force waveform mode to auto for resubmitted collisions */
		next_update->update_desc->upd_data.waveform_mode =
			WAVEFORM_MODE_AUTO;

		/*
		 * We have a collision cleared, so select it for resubmission.
		 * If an update is already selected, attempt to merge.
		 */
		if (!upd_data_list) {
			upd_data_list = next_update;
			list_del_init(&next_update->list);
		} else {
			switch (epdc_submit_merge(upd_data_list->update_desc,
						next_update->update_desc,
						priv)) {
			case MERGE_OK:
				dev_dbg(priv->drm.dev,
					"Update merged [collision]\n");
				list_del_init(&next_update->update_desc->list);
				kfree(next_update->update_desc);
				next_update->update_desc = NULL;
				list_del_init(&next_update->list);
				/* Add to free buffer list */
				list_add_tail(&next_update->list,
					 &priv->upd_buf_free_list);
				break;
			case MERGE_FAIL:
				dev_dbg(priv->drm.dev,
					"Update not merged [collision]\n");
				break;
			case MERGE_BLOCK:
				dev_dbg(priv->drm.dev,
					"Merge blocked [collision]\n");
				end_merge = true;
				break;
			}

			if (end_merge) {
				end_merge = false;
				break;
			}
		}
	}

	/*
	 * If we didn't find a collision update ready to go, we
	 * need to get a free buffer and match it to a pending update.
	 */

	/*
	 * Can't proceed if there are no free buffers (and we don't
	 * already have a collision update selected)
	 */
	if (!upd_data_list &&
		list_empty(&priv->upd_buf_free_list)) {
		mutex_unlock(&priv->queue_mutex);
		return;
	}

	list_for_each_entry_safe(next_desc, temp_desc,
			&priv->upd_pending_list, list) {

		dev_dbg(priv->drm.dev, "Found a pending update!\n");

		if (!upd_data_list) {
			if (list_empty(&priv->upd_buf_free_list))
				break;
			upd_data_list =
				list_entry(priv->upd_buf_free_list.next,
					struct update_data_list, list);
			list_del_init(&upd_data_list->list);
			upd_data_list->update_desc = next_desc;
			list_del_init(&next_desc->list);
		} else {
			switch (epdc_submit_merge(upd_data_list->update_desc,
					next_desc, priv)) {
			case MERGE_OK:
				dev_dbg(priv->drm.dev,
					"Update merged [queue]\n");
				list_del_init(&next_desc->list);
				kfree(next_desc);
				break;
			case MERGE_FAIL:
				dev_dbg(priv->drm.dev,
					"Update not merged [queue]\n");
				break;
			case MERGE_BLOCK:
				dev_dbg(priv->drm.dev,
					"Merge blocked [collision]\n");
				end_merge = true;
				break;
			}

			if (end_merge)
				break;
		}
	}

	/* Is update list empty? */
	if (!upd_data_list) {
		mutex_unlock(&priv->queue_mutex);
		return;
	}

	if ((!priv->powered)
		|| priv->powering_down)
		mxc_epdc_powerup(priv);

	/*
	 * Set update buffer pointer to the start of
	 * the update region in the frame buffer.
	 */
	upd_region = &upd_data_list->update_desc->upd_data.update_region;
	update_addr = priv->epdc_mem_phys +
		((upd_region->y1 * priv->epdc_mem_width) +
		upd_region->x1);
	update_addr_virt = (u8 *)(priv->epdc_mem_virt) +
		((upd_region->y1 * priv->epdc_mem_width) +
		upd_region->x1);
	upd_data_list->update_desc->epdc_stride = priv->epdc_mem_width;

	adj_update_region = upd_data_list->update_desc->upd_data.update_region;
	/*
	 * Is the working buffer idle?
	 * If the working buffer is busy, we must wait for the resource
	 * to become free. The IST will signal this event.
	 */
	if (priv->cur_update != NULL) {
		dev_dbg(priv->drm.dev, "working buf busy!\n");

		/* Initialize event signalling an update resource is free */
		init_completion(&priv->update_res_free);

		priv->waiting_for_wb = true;

		/* Leave spinlock while waiting for WB to complete */
		mutex_unlock(&priv->queue_mutex);
		wait_for_completion(&priv->update_res_free);
		mutex_lock(&priv->queue_mutex);
	}

	/*
	 * If there are no LUTs available,
	 * then we must wait for the resource to become free.
	 * The IST will signal this event.
	 */
	if (!epdc_any_luts_available(priv)) {
		dev_dbg(priv->drm.dev, "no luts available!\n");

		/* Initialize event signalling an update resource is free */
		init_completion(&priv->update_res_free);

		priv->waiting_for_lut = true;

		/* Leave spinlock while waiting for LUT to free up */
		mutex_unlock(&priv->queue_mutex);
		wait_for_completion(&priv->update_res_free);
		mutex_lock(&priv->queue_mutex);
	}

	ret = epdc_choose_next_lut(priv, &upd_data_list->lut_num);

	/* LUTs are available, so we get one here */
	priv->cur_update = upd_data_list;

	/* Reset mask for LUTS that have completed during WB processing */
	priv->luts_complete_wb = 0;

	/* Mark LUT with order */
	priv->lut_update_order[upd_data_list->lut_num] =
			upd_data_list->update_desc->update_order;

	epdc_lut_complete_intr(priv, upd_data_list->lut_num,
				       true);

	/* Enable Collision and WB complete IRQs */
	epdc_working_buf_intr(priv, true);

	epdc_write(priv, EPDC_TEMP,
		   mxc_epdc_fb_get_temp_index(priv, TEMP_USE_AMBIENT));

	/* Program EPDC update to process buffer */

	epdc_set_update_area(priv, update_addr,
			     adj_update_region.x1, adj_update_region.y1,
			     drm_rect_width(&adj_update_region),
			     drm_rect_height(&adj_update_region),
			     upd_data_list->update_desc->epdc_stride);

	if (priv->wv_modes_update &&
		(upd_data_list->update_desc->upd_data.waveform_mode
			== WAVEFORM_MODE_AUTO)) {
		mxc_epdc_set_update_waveform(priv, &priv->wv_modes);
		priv->wv_modes_update = false;
	}

	epdc_submit_update(priv, upd_data_list->lut_num,
			   upd_data_list->update_desc->upd_data.waveform_mode,
			   upd_data_list->update_desc->upd_data.update_mode,
			   false,
			   false, 0);

	/* Release buffer queues */
	mutex_unlock(&priv->queue_mutex);
}

void mxc_epdc_flush_updates(struct mxc_epdc *priv)
{
	int ret;

	if (priv->in_init)
		return;

	/* Grab queue lock to prevent any new updates from being submitted */
	mutex_lock(&priv->queue_mutex);

	/*
	 * 3 places to check for updates that are active or pending:
	 *   1) Updates in the pending list
	 *   2) Update buffers in use (e.g., PxP processing)
	 *   3) Active updates to panel - We can key off of EPDC
	 *      power state to know if we have active updates.
	 */
	if (!list_empty(&priv->upd_pending_list) ||
		!is_free_list_full(priv) ||
		(priv->updates_active == true)) {
		/* Initialize event signalling updates are done */
		init_completion(&priv->updates_done);
		priv->waiting_for_idle = true;

		mutex_unlock(&priv->queue_mutex);
		/* Wait for any currently active updates to complete */
		ret = wait_for_completion_timeout(&priv->updates_done,
						msecs_to_jiffies(8000));
		if (!ret)
			dev_err(priv->drm.dev,
				"Flush updates timeout! ret = 0x%x\n", ret);

		mutex_lock(&priv->queue_mutex);
		priv->waiting_for_idle = false;
	}

	mutex_unlock(&priv->queue_mutex);
}

void mxc_epdc_draw_mode0(struct mxc_epdc *priv)
{
	u32 *upd_buf_ptr;
	int i;
	u32 xres, yres;

	upd_buf_ptr = (u32 *)priv->epdc_mem_virt;

	epdc_working_buf_intr(priv, true);
	epdc_lut_complete_intr(priv, 0, true);

	/* Use unrotated (native) width/height */
	xres = priv->epdc_mem_width;
	yres = priv->epdc_mem_height;

	/* Program EPDC update to process buffer */
	epdc_set_update_area(priv, priv->epdc_mem_phys, 0, 0, xres, yres, 0);
	epdc_submit_update(priv, 0, priv->wv_modes.mode_init, UPDATE_MODE_FULL,
		false, true, 0xFF);

	dev_dbg(priv->drm.dev, "Mode0 update - Waiting for LUT to complete...\n");

	/* Will timeout after ~4-5 seconds */

	for (i = 0; i < 40; i++) {
		if (!epdc_is_lut_active(priv, 0)) {
			dev_dbg(priv->drm.dev, "Mode0 init complete\n");
			return;
		}
		msleep(100);
	}

	dev_err(priv->drm.dev, "Mode0 init failed!\n");
}


int mxc_epdc_send_single_update(struct drm_rect *clip, int pitch, void *vaddr,
				struct mxc_epdc *priv)
{
	struct update_desc_list *upd_desc;

	if (priv->rev < 30) 
		epdc_from_rgb_clear_lower_nibble(clip, vaddr, pitch,
						 (u8 *)priv->epdc_mem_virt,
						 priv->epdc_mem_width);
	else
		epdc_from_rgb_shift(clip, vaddr, pitch,
				    (u8 *)priv->epdc_mem_virt,
				    priv->epdc_mem_width);

	/* Has EPDC HW been initialized? */
	if (!priv->hw_ready) {
		/* Throw message if we are not mid-initialization */
		if (!priv->hw_initializing)
			dev_err(priv->drm.dev, "Display HW not properly initialized. Aborting update.\n");
		return -EPERM;
	}

	mutex_lock(&priv->queue_mutex);

	if (priv->waiting_for_idle) {
		dev_dbg(priv->drm.dev, "EPDC not active. Update request abort.\n");
		mutex_unlock(&priv->queue_mutex);
		return -EPERM;
	}


	/*
	 * Create new update data structure, fill it with new update
	 * data and add it to the list of pending updates
	 */
	upd_desc = kzalloc(sizeof(struct update_desc_list), GFP_KERNEL);
	if (!upd_desc) {
		mutex_unlock(&priv->queue_mutex);
		return -ENOMEM;
	}
	/* Initialize per-update marker list */
	upd_desc->upd_data.update_region = *clip;
	upd_desc->upd_data.waveform_mode = WAVEFORM_MODE_AUTO;
	upd_desc->upd_data.temp = TEMP_USE_AMBIENT;
	upd_desc->upd_data.update_mode = UPDATE_MODE_PARTIAL;
	upd_desc->update_order = priv->order_cnt++;
	list_add_tail(&upd_desc->list, &priv->upd_pending_list);

	/* Queued update scheme processing */

	mutex_unlock(&priv->queue_mutex);

	/* Signal workqueue to handle new update */
	queue_work(priv->epdc_submit_workqueue,
		   &priv->epdc_submit_work);

	return 0;
}

static void epdc_intr_work_func(struct work_struct *work)
{
	struct mxc_epdc *priv =
		container_of(work, struct mxc_epdc, epdc_intr_work);
	struct update_data_list *collision_update;
	u64 temp_mask;
	u32 lut;
	bool ignore_collision = false;
	int i;
	bool wb_lut_done = false;
	bool free_update = true;
	u32 epdc_luts_active, epdc_wb_busy, epdc_luts_avail, epdc_lut_cancelled;
	u32 epdc_collision;
	u64 epdc_irq_stat;
	bool epdc_waiting_on_wb;
	u32 coll_coord, coll_size;

	/* Protect access to buffer queues and to update HW */
	mutex_lock(&priv->queue_mutex);

	/* Capture EPDC status one time to limit exposure to race conditions */
	epdc_luts_active = epdc_any_luts_active(priv);
	epdc_wb_busy = epdc_is_working_buffer_busy(priv);
	epdc_lut_cancelled = epdc_is_lut_cancelled(priv);
	epdc_luts_avail = epdc_any_luts_available(priv);
	epdc_collision = epdc_is_collision(priv);

	epdc_irq_stat = (u64)epdc_read(priv, EPDC_IRQ1) |
			((u64)epdc_read(priv, EPDC_IRQ2) << 32);
	epdc_waiting_on_wb = (priv->cur_update != NULL) ? true : false;

	/* Free any LUTs that have completed */
	for (i = 0; i < priv->num_luts; i++) {
		if ((epdc_irq_stat & (1ULL << i)) == 0)
			continue;

		dev_dbg(priv->drm.dev, "LUT %d completed\n", i);

		/* Disable IRQ for completed LUT */
		epdc_lut_complete_intr(priv, i, false);

		/*
		 * Go through all updates in the collision list and
		 * unmask any updates that were colliding with
		 * the completed LUT.
		 */
		list_for_each_entry(collision_update,
				    &priv->upd_buf_collision_list, list) {
			collision_update->collision_mask =
			    collision_update->collision_mask & ~BIT(i);
		}

		epdc_clear_lut_complete_irq(priv, i);

		priv->luts_complete_wb |= 1ULL << i;

		priv->lut_update_order[i] = 0;

		/* Signal completion if submit workqueue needs a LUT */
		if (priv->waiting_for_lut) {
			complete(&priv->update_res_free);
			priv->waiting_for_lut = false;
		}

		/*
		 * Detect race condition where WB and its LUT complete
		 * (i.e. full update completes) in one swoop
		 */
		if (epdc_waiting_on_wb &&
			(i == priv->cur_update->lut_num))
			wb_lut_done = true;
	}

	/* Check to see if all updates have completed */
	if (list_empty(&priv->upd_pending_list) &&
		is_free_list_full(priv) &&
		!epdc_waiting_on_wb &&
		!epdc_luts_active) {

		priv->updates_active = false;

		if (priv->pwrdown_delay != FB_POWERDOWN_DISABLE) {
			/*
			 * Set variable to prevent overlapping
			 * enable/disable requests
			 */
			priv->powering_down = true;

			/* Schedule task to disable EPDC HW until next update */
			schedule_delayed_work(&priv->epdc_done_work,
				msecs_to_jiffies(priv->pwrdown_delay));

			/* Reset counter to reduce chance of overflow */
			priv->order_cnt = 0;
		}

		if (priv->waiting_for_idle)
			complete(&priv->updates_done);
	}

	/* Is Working Buffer busy? */
	if (epdc_wb_busy) {
		/* Can't submit another update until WB is done */
		mutex_unlock(&priv->queue_mutex);
		return;
	}

	/*
	 * Were we waiting on working buffer?
	 * If so, update queues and check for collisions
	 */
	if (epdc_waiting_on_wb) {
		dev_dbg(priv->drm.dev, "\nWorking buffer completed\n");

		/* Signal completion if submit workqueue was waiting on WB */
		if (priv->waiting_for_wb) {
			complete(&priv->update_res_free);
			priv->waiting_for_wb = false;
		}

		if (epdc_lut_cancelled && !epdc_collision) {
			/*
			 * Note: The update may be cancelled (void) if all
			 * pixels collided. In that case we handle it as a
			 * collision, not a cancel.
			 */

			/* Clear LUT status (might be set if no AUTOWV used) */

			/*
			 * Disable and clear IRQ for the LUT used.
			 * Even though LUT is cancelled in HW, the LUT
			 * complete bit may be set if AUTOWV not used.
			 */
			epdc_lut_complete_intr(priv,
					priv->cur_update->lut_num, false);
			epdc_clear_lut_complete_irq(priv,
					priv->cur_update->lut_num);

			priv->lut_update_order[priv->cur_update->lut_num] = 0;

			/* Signal completion if submit workqueue needs a LUT */
			if (priv->waiting_for_lut) {
				complete(&priv->update_res_free);
				priv->waiting_for_lut = false;
			}
		} else if (epdc_collision) {
			/* Check list of colliding LUTs, and add to our collision mask */
			priv->cur_update->collision_mask =
			    priv->epdc_colliding_luts;

			/* Clear collisions that completed since WB began */
			priv->cur_update->collision_mask &=
				~priv->luts_complete_wb;

			dev_dbg(priv->drm.dev, "Collision mask = 0x%llx\n",
			       priv->epdc_colliding_luts);

			/*
			 * For EPDC 2.0 and later, minimum collision bounds
			 * are provided by HW.  Recompute new bounds here.
			 */

			coll_coord = epdc_read(priv, EPDC_UPD_COL_CORD);
			coll_size = epdc_read(priv, EPDC_UPD_COL_SIZE);
			drm_rect_init(&priv->cur_update->update_desc->upd_data.update_region,
				(coll_coord & EPDC_UPD_COL_CORD_XCORD_MASK)
					>> EPDC_UPD_COL_CORD_XCORD_OFFSET,
				(coll_coord & EPDC_UPD_COL_CORD_YCORD_MASK)
					>> EPDC_UPD_COL_CORD_YCORD_OFFSET,
				((coll_size & EPDC_UPD_COL_SIZE_WIDTH_MASK)
					>> EPDC_UPD_COL_SIZE_WIDTH_OFFSET),
				((coll_size & EPDC_UPD_COL_SIZE_HEIGHT_MASK)
					>> EPDC_UPD_COL_SIZE_HEIGHT_OFFSET));

			/*
			 * If we collide with newer updates, then
			 * we don't need to re-submit the update. The
			 * idea is that the newer updates should take
			 * precedence anyways, so we don't want to
			 * overwrite them.
			 */
			for (temp_mask = priv->cur_update->collision_mask, lut = 0;
				temp_mask != 0;
				lut++, temp_mask = temp_mask >> 1) {
				if (!(temp_mask & 0x1))
					continue;

				if (priv->lut_update_order[lut] >=
					priv->cur_update->update_desc->update_order) {
					dev_dbg(priv->drm.dev,
						"Ignoring collision with newer update.\n");
					ignore_collision = true;
					break;
				}
			}

			if (!ignore_collision) {
				free_update = false;

				/* Move to collision list */
				list_add_tail(&priv->cur_update->list,
					 &priv->upd_buf_collision_list);
			}
		}

		/* Do we need to free the current update descriptor? */
		if (free_update) {
			/* Free update descriptor */
			kfree(priv->cur_update->update_desc);

			/* Add to free buffer list */
			list_add_tail(&priv->cur_update->list,
				 &priv->upd_buf_free_list);

			/* Check to see if all updates have completed */
			if (list_empty(&priv->upd_pending_list) &&
				is_free_list_full(priv) &&
				!epdc_luts_active) {

				priv->updates_active = false;

				if (priv->pwrdown_delay !=
						FB_POWERDOWN_DISABLE) {
					/*
					 * Set variable to prevent overlapping
					 * enable/disable requests
					 */
					priv->powering_down = true;

					/* Schedule EPDC disable */
					schedule_delayed_work(&priv->epdc_done_work,
						msecs_to_jiffies(priv->pwrdown_delay));

					/* Reset counter to reduce chance of overflow */
					priv->order_cnt = 0;
				}

				if (priv->waiting_for_idle)
					complete(&priv->updates_done);
			}
		}

		/* Clear current update */
		priv->cur_update = NULL;

		/* Clear IRQ for working buffer */
		epdc_working_buf_intr(priv, false);
		epdc_clear_working_buf_irq(priv);
	}

	/* Schedule task to submit collision and pending update */
	if (!priv->powering_down)
		queue_work(priv->epdc_submit_workqueue,
			&priv->epdc_submit_work);

	/* Release buffer queues */
	mutex_unlock(&priv->queue_mutex);
}


int mxc_epdc_init_update(struct mxc_epdc *priv)
{
	struct update_data_list *upd_list;
	int ret;
	int irq;
	int i;
	/*
	 * Initialize lists for pending updates,
	 * active update requests, update collisions,
	 * and freely available updates.
	 */
	priv->num_luts = EPDC_V2_NUM_LUTS;
	priv->max_num_updates = EPDC_V2_MAX_NUM_UPDATES;

	INIT_LIST_HEAD(&priv->upd_pending_list);
	INIT_LIST_HEAD(&priv->upd_buf_queue);
	INIT_LIST_HEAD(&priv->upd_buf_free_list);
	INIT_LIST_HEAD(&priv->upd_buf_collision_list);

	devm_add_action_or_reset(priv->drm.dev, cleanup_update_list, priv);
	/* Allocate update buffers and add them to the list */
	for (i = 0; i < priv->max_num_updates; i++) {
		upd_list = kzalloc(sizeof(*upd_list), GFP_KERNEL);
		if (upd_list == NULL)
			return -ENOMEM;

		/* Add newly allocated buffer to free list */
		list_add(&upd_list->list, &priv->upd_buf_free_list);
	}



	/* Initialize marker list */
	INIT_LIST_HEAD(&priv->full_marker_list);

	/* Initialize all LUTs to inactive */
	priv->lut_update_order =
		devm_kzalloc(priv->drm.dev, priv->num_luts * sizeof(u32 *), GFP_KERNEL);
	if (!priv->lut_update_order)
		return -ENOMEM;

	for (i = 0; i < priv->num_luts; i++)
		priv->lut_update_order[i] = 0;

	INIT_DELAYED_WORK(&priv->epdc_done_work, epdc_done_work_func);
	priv->epdc_submit_workqueue = alloc_workqueue("EPDC Submit",
					WQ_MEM_RECLAIM | WQ_HIGHPRI |
					WQ_CPU_INTENSIVE | WQ_UNBOUND, 1);
	INIT_WORK(&priv->epdc_submit_work, epdc_submit_work_func);
	priv->epdc_intr_workqueue = alloc_workqueue("EPDC Interrupt",
					WQ_MEM_RECLAIM | WQ_HIGHPRI |
					WQ_CPU_INTENSIVE | WQ_UNBOUND, 1);
	INIT_WORK(&priv->epdc_intr_work, epdc_intr_work_func);

	mutex_init(&priv->queue_mutex);

	/* Retrieve EPDC IRQ num */
	irq = platform_get_irq(to_platform_device(priv->drm.dev), 0);
	if (irq < 0) {
		dev_err(priv->drm.dev, "cannot get IRQ resource\n");
		return -ENODEV;
	}
	priv->epdc_irq = irq;

	/* Register IRQ handler */
	ret = devm_request_irq(priv->drm.dev, priv->epdc_irq,
				mxc_epdc_irq_handler, 0, "epdc", priv);
	if (ret) {
		dev_err(priv->drm.dev, "request_irq (%d) failed with error %d\n",
			priv->epdc_irq, ret);
		return ret;
	}

	return 0;

}

