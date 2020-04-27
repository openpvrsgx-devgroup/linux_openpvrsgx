/*
 * ALSA SoC driver for OMAP4/5 AESS (Audio Engine Sub-System)
 *
 * Copyright (C) 2010-2013 Texas Instruments
 *
 * Authors: Liam Girdwood <lrg@ti.com>
 *          Misael Lopez Cruz <misael.lopez@ti.com>
 *          Sebastien Guiriec <s-guiriec@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * Contact: Misael Lopez Cruz <misael.lopez@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/debugfs.h>

#include <linux/omap-dma.h>

#include <sound/soc.h>

#include "omap-aess-priv.h"

/* TODO: size in bytes of debug options */
#define OMAP_AESS_DBG_FLAG1_SIZE	0
#define OMAP_AESS_DBG_FLAG2_SIZE	0
#define OMAP_AESS_DBG_FLAG3_SIZE	0

#define OMAP_AESS_DBG_DMA_NAME		"fifo7"

#ifdef CONFIG_DEBUG_FS

struct omap_aess_debug {
	/* its intended we can switch on/off individual debug items */
	u32 format1; /* TODO: match flag names here to debug format flags */
	u32 format2;
	u32 format3;

	/* AESS DMA buffer */
	u32 buffer_bytes;
	u32 circular;
	u32 buffer_msecs;  /* size of buffer in secs */
	u32 elem_bytes;
	dma_addr_t buffer_addr;
	wait_queue_head_t wait;
	size_t reader_offset;
	size_t dma_offset;
	int complete;
	char *buffer;
	int dma_ch;
	int dma_req;
	struct dma_chan *chan;

	/* debugfs */
	struct dentry *test_root;
	struct dentry *d_data;

	struct dentry *d_cmem;
	struct dentry *d_pmem;
	struct dentry *d_smem;
	struct dentry *d_dmem;
};

/* TODO: Need to move to dmaengine API */
#if 0
static int aess_dbg_get_dma_pos(struct omap_aess *aess)
{
	struct omap_aess_debug *debug = aess->debug;

	return omap_get_dma_dst_pos(debug->dma_ch) - debug->buffer_addr;
}

static void aess_dbg_dma_irq(int ch, u16 stat, void *data)
{
}

static int aess_dbg_start_dma(struct omap_aess *aess, int circular)
{
	struct omap_aess_debug *debug = aess->debug;
	struct omap_dma_channel_params dma_params;
	int err;

	/* start the DMA in either :-
	 *
	 * 1) circular buffer mode where the DMA will restart when it get to
	 *    the end of the buffer.
	 * 2) default mode, where DMA stops at the end of the buffer.
	 */

	debug->dma_req = aess->dma_lines[7];
	err = omap_request_dma(debug->dma_req, "AESS debug",
			       aess_dbg_dma_irq, aess, &debug->dma_ch);
	if (debug->circular) {
		/*
		 * Link channel with itself so DMA doesn't need any
		 * reprogramming while looping the buffer
		 */
		omap_dma_link_lch(debug->dma_ch, debug->dma_ch);
	}

	memset(&dma_params, 0, sizeof(dma_params));
	dma_params.data_type = OMAP_DMA_DATA_TYPE_S32;
	dma_params.trigger = debug->dma_req;
	dma_params.sync_mode = OMAP_DMA_SYNC_FRAME;
	dma_params.src_amode = OMAP_DMA_AMODE_DOUBLE_IDX;
	dma_params.dst_amode = OMAP_DMA_AMODE_POST_INC;
	dma_params.src_or_dst_synch = OMAP_DMA_SRC_SYNC;
	dma_params.src_start =
			aess->fw_info.map[OMAP_AESS_DMEM_DEBUG_FIFO_ID].offset +
			aess->dmem_l3;
	dma_params.dst_start = debug->buffer_addr;
	dma_params.src_port = OMAP_DMA_PORT_MPUI;
	dma_params.src_ei = 1;
	dma_params.src_fi = 1 - debug->elem_bytes;

	 /* 128 bytes shifted into words */
	dma_params.elem_count = debug->elem_bytes >> 2;
	dma_params.frame_count = debug->buffer_bytes / debug->elem_bytes;
	omap_set_dma_params(debug->dma_ch, &dma_params);

	omap_enable_dma_irq(debug->dma_ch, OMAP_DMA_FRAME_IRQ);
	omap_set_dma_src_burst_mode(debug->dma_ch, OMAP_DMA_DATA_BURST_16);
	omap_set_dma_dest_burst_mode(debug->dma_ch, OMAP_DMA_DATA_BURST_16);

	debug->reader_offset = 0;

	pm_runtime_get_sync(aess->dev);
	omap_start_dma(debug->dma_ch);
	return 0;
}

static void aess_dbg_stop_dma(struct omap_aess *aess)
{
	struct omap_aess_debug *debug = aess->debug;

	/* Since we are using self linking, there is a
	chance that the DMA as re-enabled the channel just after disabling it */
	while (omap_get_dma_active_status(debug->dma_ch))
		omap_stop_dma(debug->dma_ch);

	if (debug->circular)
		omap_dma_unlink_lch(debug->dma_ch, debug->dma_ch);

	omap_free_dma(debug->dma_ch);
	pm_runtime_put_sync(aess->dev);
}

static ssize_t aess_copy_to_user(struct omap_aess *aess, char __user *user_buf,
				 size_t count)
{
	struct omap_aess_debug *debug = aess->debug;

	/* check for reader buffer wrap */
	if (debug->reader_offset + count > debug->buffer_bytes) {
		size_t size = debug->buffer_bytes - debug->reader_offset;

		/* wrap */
		if (copy_to_user(user_buf, debug->buffer + debug->reader_offset,
				 size))
			return -EFAULT;

		/* need to just return if non circular */
		if (!debug->circular) {
			debug->complete = 1;
			return count;
		}

		if (copy_to_user(user_buf + size, debug->buffer, count - size))
			return -EFAULT;

		debug->reader_offset = count - size;
		return count;
	} else {
		/* no wrap */
		if (copy_to_user(user_buf, debug->buffer + debug->reader_offset,
				 count))
			return -EFAULT;

		debug->reader_offset += count;

		if (!debug->circular &&
		    debug->reader_offset == debug->buffer_bytes)
			debug->complete = 1;

		return count;
	}
}

static ssize_t aess_read_data(struct file *file, char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct omap_aess *aess = file->private_data;
	struct omap_aess_debug *debug = aess->debug;
	DECLARE_WAITQUEUE(wait, current);
	int dma_offset, bytes;
	ssize_t ret = 0;

	add_wait_queue(&debug->wait, &wait);
	do {
		set_current_state(TASK_INTERRUPTIBLE);
		/* TODO: Check if really needed. Or adjust sleep delay
		 * If not delay trace is not working */
		msleep_interruptible(1);

		/* is DMA finished ? */
		if (debug->complete)
			break;

		dma_offset = aess_dbg_get_dma_pos(aess);


		/* get maximum amount of debug bytes we can read */
		if (dma_offset >= debug->reader_offset) {
			/* dma ptr is ahead of reader */
			bytes = dma_offset - debug->reader_offset;
		} else {
			/* dma ptr is behind reader */
			bytes = dma_offset + debug->buffer_bytes -
				debug->reader_offset;
		}

		if (count > bytes)
			count = bytes;

		if (count > 0) {
			ret = aess_copy_to_user(aess, user_buf, count);
			break;
		}

		if (file->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}

		schedule();

	} while (1);

	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&debug->wait, &wait);

	return ret;
}
#else /* legacy DMA code */
static int aess_dbg_start_dma(struct omap_aess *aess, int circular)
{
	struct omap_aess_debug *debug = aess->debug;

	if (debug->chan) {
		dev_err(aess->dev, "debug DMA already in use\n");
		return -EBUSY;
	}

	debug->chan = dma_request_slave_channel(aess->dev,
						OMAP_AESS_DBG_DMA_NAME);
	if (!debug->chan) {
		dev_err(aess->dev, "Failed to request debug DMA\n");
		return -ENODEV;
	}

	return 0;
}

static ssize_t aess_read_data(struct file *file, char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	return 0;
}

static void aess_dbg_stop_dma(struct omap_aess *aess)
{
	struct omap_aess_debug *debug = aess->debug;

	if (!debug->chan)
		return;

	dma_release_channel(debug->chan);
	debug->chan = NULL;
}
#endif /* legacy DMA code */

static int aess_open_data(struct inode *inode, struct file *file)
{
	struct omap_aess *aess = inode->i_private;
	struct omap_aess_debug *debug = aess->debug;

	/* adjust debug word size based on any user params */
	if (debug->format1)
		debug->elem_bytes += OMAP_AESS_DBG_FLAG1_SIZE;
	if (debug->format2)
		debug->elem_bytes += OMAP_AESS_DBG_FLAG2_SIZE;
	if (debug->format3)
		debug->elem_bytes += OMAP_AESS_DBG_FLAG3_SIZE;

	debug->buffer_bytes = debug->elem_bytes * 4 * debug->buffer_msecs;

	debug->buffer = dma_alloc_wc(aess->dev, debug->buffer_bytes,
					&debug->buffer_addr, GFP_KERNEL);
	if (debug->buffer == NULL) {
		dev_err(aess->dev, "can't alloc %d bytes for trace DMA buffer\n",
			debug->buffer_bytes);
		return -ENOMEM;
	}

	file->private_data = inode->i_private;
	debug->complete = 0;
	aess_dbg_start_dma(aess, debug->circular);

	return 0;
}

static int aess_release_data(struct inode *inode, struct file *file)
{
	struct omap_aess *aess = inode->i_private;
	struct omap_aess_debug *debug = aess->debug;

	aess_dbg_stop_dma(aess);

	dma_free_wc(aess->dev, debug->buffer_bytes, debug->buffer,
		    debug->buffer_addr);
	return 0;
}

static const struct file_operations omap_aess_fops = {
	.open = aess_open_data,
	.read = aess_read_data,
	.release = aess_release_data,
};


static int aess_open_mem(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t aess_read_mem(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos, void *mem, int size)
{
	struct omap_aess *aess = file->private_data;
	ssize_t ret = 0;

	pm_runtime_get_sync(aess->dev);
	set_current_state(TASK_INTERRUPTIBLE);

	if (*ppos >= size)
		goto out;

	if (*ppos + count > size)
		count = size - *ppos;

	if (copy_to_user(user_buf, mem + *ppos, count)) {
		ret = -EFAULT;
		goto out;
	}
	*ppos += count;
	ret = count;
out:
	__set_current_state(TASK_RUNNING);
	pm_runtime_put_sync(aess->dev);
	return ret;
}

static loff_t aess_llseek(struct file *file, loff_t off, int whence, int size)
{
	loff_t newpos;

	switch (whence) {
	case SEEK_SET:
		newpos = off;
		break;
	case SEEK_CUR:
		newpos = file->f_pos + off;
		break;
	case SEEK_END:
		newpos = size;
		break;
	default: /* can't happen */
		return -EINVAL;
	}

	if (newpos < 0)
		return -EINVAL;

	if (newpos > size)
		newpos = size;

	file->f_pos = newpos;
	return newpos;
}

static loff_t aess_llseek_cmem(struct file *file, loff_t off, int whence)
{
	struct omap_aess *aess = file->private_data;

	return aess_llseek(file, off, whence, aess->hdr.cmem_size);
}

static ssize_t aess_read_cmem(struct file *file, char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct omap_aess *aess = file->private_data;

	return aess_read_mem(file, user_buf, count, ppos,
			     (void __force *)aess->io_base[OMAP_AESS_BANK_CMEM],
			     aess->hdr.cmem_size);
}

static const struct file_operations omap_aess_cmem_fops = {
	.open = aess_open_mem,
	.read = aess_read_cmem,
	.llseek = aess_llseek_cmem,
};

static loff_t aess_llseek_pmem(struct file *file, loff_t off, int whence)
{
	struct omap_aess *aess = file->private_data;

	return aess_llseek(file, off, whence, aess->hdr.pmem_size);
}

static ssize_t aess_read_pmem(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct omap_aess *aess = file->private_data;

	return aess_read_mem(file, user_buf, count, ppos,
			     (void __force *)aess->io_base[OMAP_AESS_BANK_PMEM],
			     aess->hdr.pmem_size);
}

static const struct file_operations omap_aess_pmem_fops = {
	.open = aess_open_mem,
	.read = aess_read_pmem,
	.llseek = aess_llseek_pmem,
};

static loff_t aess_llseek_smem(struct file *file, loff_t off, int whence)
{
	struct omap_aess *aess = file->private_data;

	return aess_llseek(file, off, whence, aess->hdr.smem_size);
}

static ssize_t aess_read_smem(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct omap_aess *aess = file->private_data;

	return aess_read_mem(file, user_buf, count, ppos,
			     (void __force *)aess->io_base[OMAP_AESS_BANK_SMEM],
			     aess->hdr.smem_size);
}

static const struct file_operations omap_aess_smem_fops = {
	.open = aess_open_mem,
	.read = aess_read_smem,
	.llseek = aess_llseek_smem,
};

static loff_t aess_llseek_dmem(struct file *file, loff_t off, int whence)
{
	struct omap_aess *aess = file->private_data;

	return aess_llseek(file, off, whence, aess->hdr.dmem_size);
}

static ssize_t aess_read_dmem(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct omap_aess *aess = file->private_data;

	return aess_read_mem(file, user_buf, count, ppos,
			     (void __force *)aess->io_base[OMAP_AESS_BANK_DMEM],
			     aess->hdr.dmem_size);
}

static const struct file_operations omap_aess_dmem_fops = {
	.open = aess_open_mem,
	.read = aess_read_dmem,
	.llseek = aess_llseek_dmem,
};

#define aess_debugfs_failure(aess, file) \
		dev_err(aess->dev, "Failed to create %s debugfs file\n", file)


void aess_init_debugfs(struct omap_aess *aess)
{
	struct omap_aess_debug *debug;

	debug = devm_kzalloc(aess->dev, sizeof(struct omap_aess_debug),
			     GFP_KERNEL);
	if (!debug) {
		dev_err(aess->dev, "Failed to allocate memory for debug\n");
		return;
	}

	debug->test_root = debugfs_create_dir("test", aess->debugfs_root);
	if (!debug->test_root) {
		dev_err(aess->dev, "Failed to create debugfs directory\n");
		return;
	}

	/* Test related files */
	debugfs_create_u32("format1", 0644, debug->test_root,
					    &debug->format1);
	debugfs_create_u32("format2", 0644, debug->test_root,
					    &debug->format2);

	debugfs_create_u32("format3", 0644, debug->test_root,
					    &debug->format3);

	debugfs_create_u32("element_bytes", 0604,
						 debug->test_root,
						 &debug->elem_bytes);

	debugfs_create_u32("msecs", 0644, debug->test_root,
					   &debug->buffer_msecs);

	debugfs_create_u32("circular", 0644, debug->test_root,
					    &debug->circular);

	debug->d_data = debugfs_create_file("_data", 0644, debug->test_root,
					    aess, &omap_aess_fops);
	if (!debug->d_data)
		aess_debugfs_failure(aess, "_data");

	/* AESS information */
	debugfs_create_u32("opp_level", 0604, aess->debugfs_root,
					  &aess->opp.level);

	debug->d_pmem = debugfs_create_file("pmem", 0644, aess->debugfs_root,
					    aess, &omap_aess_pmem_fops);
	if (!debug->d_pmem)
		aess_debugfs_failure(aess, "PMEM");

	debug->d_smem = debugfs_create_file("smem", 0644, aess->debugfs_root,
					    aess, &omap_aess_smem_fops);
	if (!debug->d_smem)
		aess_debugfs_failure(aess, "SMEM");

	debug->d_dmem = debugfs_create_file("dmem", 0644, aess->debugfs_root,
					    aess, &omap_aess_dmem_fops);
	if (!debug->d_dmem)
		aess_debugfs_failure(aess, "DMEM");

	debug->d_cmem = debugfs_create_file("cmem", 0644, aess->debugfs_root,
					    aess, &omap_aess_cmem_fops);
	if (!debug->d_cmem)
		aess_debugfs_failure(aess, "CMEM");

	init_waitqueue_head(&debug->wait);

	aess->debug = debug;
}

void aess_cleanup_debugfs(struct omap_aess *aess)
{
	if (aess->debug)
		debugfs_remove_recursive(aess->debug->test_root);
}

#else

void aess_init_debugfs(struct omap_aess *aess)
{
}

void aess_cleanup_debugfs(struct omap_aess *aess)
{
}
#endif
