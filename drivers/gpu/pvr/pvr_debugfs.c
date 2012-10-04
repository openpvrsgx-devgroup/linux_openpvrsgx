/*
 * Copyright (c) 2010-2011 Imre Deak <imre.deak@nokia.com>
 * Copyright (c) 2010-2011 Luc Verhaegen <libv@codethink.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 *
 * Debugfs interface living in pvr/ subdirectory.
 *
 */
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include "img_types.h"
#include "servicesext.h"
#include "services.h"
#include "sgxinfokm.h"
#include "syscommon.h"
#include "pvr_bridge_km.h"
#include "sgxutils.h"
#include "pvr_debugfs.h"
#include "mmu.h"
#include "bridged_support.h"
#include "mm.h"
#include "pvr_trace_cmd.h"

struct dentry *pvr_debugfs_dir;
static u32 pvr_reset;

/*
 *
 */
static struct PVRSRV_DEVICE_NODE *get_sgx_node(void)
{
	struct SYS_DATA *sysdata;
	struct PVRSRV_DEVICE_NODE *node;

	if (SysAcquireData(&sysdata) != PVRSRV_OK)
		return NULL;

	for (node = sysdata->psDeviceNodeList; node; node = node->psNext)
		if (node->sDevId.eDeviceType == PVRSRV_DEVICE_TYPE_SGX)
			break;

	return node;
}

static int pvr_debugfs_reset(void *data, u64 val)
{
	struct PVRSRV_DEVICE_NODE *node;
	enum PVRSRV_ERROR err;
	int r = 0;

	if (val != 1)
		return 0;

	pvr_lock();

	if (pvr_is_disabled()) {
		r = -ENODEV;
		goto exit;
	}

	node = get_sgx_node();
	if (!node) {
		r =  -ENODEV;
		goto exit;
	}

	err = PVRSRVSetDevicePowerStateKM(node->sDevId.ui32DeviceIndex,
					  PVRSRV_POWER_STATE_D0);
	if (err != PVRSRV_OK) {
		r = -EIO;
		goto exit;
	}

	HWRecoveryResetSGX(node, __func__);

	SGXTestActivePowerEvent(node);
exit:
	pvr_unlock();

	return r;
}

static int pvr_debugfs_reset_wrapper(void *data, u64 val)
{
	u32 *var = data;

	if (var == &pvr_reset)
		return pvr_debugfs_reset(data, val);

	BUG();

	return -EFAULT;
}

DEFINE_SIMPLE_ATTRIBUTE(pvr_debugfs_reset_fops, NULL,
			pvr_debugfs_reset_wrapper, "%llu\n");

#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
/*
 *
 */
#define SGXMK_TRACE_BUFFER_SIZE					512
#define SGXMK_TRACE_BUF_STR_LEN					80

struct edm_buf_info {
	size_t len;
	char data[1];
};

size_t
edm_trace_print(struct PVRSRV_SGXDEV_INFO *sdev, char *dst, size_t dst_len)
{
	u32 *buf_start;
	u32 *buf_end;
	u32 *buf;
	size_t p = 0;
	size_t wr_ofs;
	int i;

	if (!sdev->psKernelEDMStatusBufferMemInfo)
		return 0;

	buf = sdev->psKernelEDMStatusBufferMemInfo->pvLinAddrKM;

	if (dst)
		p += scnprintf(dst + p, dst_len - p,
			      "Last SGX microkernel status code: 0x%x\n", *buf);
	else
		printk(KERN_DEBUG "Last SGX microkernel status code: 0x%x\n",
				*buf);
	buf++;
	wr_ofs = *buf;
	buf++;

	buf_start = buf;
	buf_end = buf + SGXMK_TRACE_BUFFER_SIZE * 4;

	buf += wr_ofs * 4;

	/* Dump the status values */
	for (i = 0; i < SGXMK_TRACE_BUFFER_SIZE; i++) {
		if (dst)
			p += scnprintf(dst + p, dst_len - p,
				      "%3d %08X %08X %08X %08X\n",
				      i, buf[2], buf[3], buf[1], buf[0]);
		else
			printk(KERN_DEBUG "%3d %08X %08X %08X %08X\n",
				      i, buf[2], buf[3], buf[1], buf[0]);
		buf += 4;
		if (buf >= buf_end)
			buf = buf_start;
	}

	return p > dst_len ? dst_len : p;
}

static struct edm_buf_info *
pvr_edm_buffer_create(struct PVRSRV_SGXDEV_INFO *sgx_info)
{
	struct edm_buf_info *bi;
	size_t size;

	/* Take a snapshot of the EDM trace buffer */
	size = SGXMK_TRACE_BUFFER_SIZE * SGXMK_TRACE_BUF_STR_LEN;
	bi = vmalloc(sizeof(*bi) + size);
	if (!bi) {
		pr_err("%s: vmalloc failed!\n", __func__);
		return NULL;
	}

	bi->len = edm_trace_print(sgx_info, bi->data, size);

	return bi;
}

static void
pvr_edm_buffer_destroy(struct edm_buf_info *edm)
{
	vfree(edm);
}

static int pvr_debugfs_edm_open(struct inode *inode, struct file *file)
{
	struct PVRSRV_DEVICE_NODE *node;

	node = get_sgx_node();

	file->private_data = pvr_edm_buffer_create(node->pvDevice);
	if (!file->private_data)
		return -ENOMEM;

	return 0;
}

static int pvr_debugfs_edm_release(struct inode *inode, struct file *file)
{
	pvr_edm_buffer_destroy(file->private_data);

	return 0;
}

static ssize_t pvr_debugfs_edm_read(struct file *file, char __user *buffer,
				size_t count, loff_t *ppos)
{
	struct edm_buf_info *bi = file->private_data;

	return simple_read_from_buffer(buffer, count, ppos, bi->data, bi->len);
}

static const struct file_operations pvr_debugfs_edm_fops = {
	.owner		= THIS_MODULE,
	.open		= pvr_debugfs_edm_open,
	.read		= pvr_debugfs_edm_read,
	.release	= pvr_debugfs_edm_release,
};
#endif /* PVRSRV_USSE_EDM_STATUS_DEBUG */

#ifdef CONFIG_PVR_TRACE_CMD

static void *trcmd_str_buf;
static u8 *trcmd_snapshot;
static size_t trcmd_snapshot_size;
static int trcmd_open_cnt;

static int pvr_dbg_trcmd_open(struct inode *inode, struct file *file)
{
	int r;

	if (trcmd_open_cnt)
		return -EBUSY;

	trcmd_open_cnt++;

	trcmd_str_buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!trcmd_str_buf) {
		trcmd_open_cnt--;

		return -ENOMEM;
	}

	pvr_trcmd_lock();

	r = pvr_trcmd_create_snapshot(&trcmd_snapshot, &trcmd_snapshot_size);
	if (r < 0) {
		pvr_trcmd_unlock();
		kfree(trcmd_str_buf);
		trcmd_open_cnt--;

		return r;
	}

	pvr_trcmd_unlock();

	return 0;
}

static int pvr_dbg_trcmd_release(struct inode *inode, struct file *file)
{
	pvr_trcmd_destroy_snapshot(trcmd_snapshot);
	kfree(trcmd_str_buf);
	trcmd_open_cnt--;

	return 0;
}

static ssize_t pvr_dbg_trcmd_read(struct file *file, char __user *buffer,
				  size_t count, loff_t *ppos)
{
	ssize_t ret;

	ret = pvr_trcmd_print(trcmd_str_buf, max_t(size_t, PAGE_SIZE, count),
			      trcmd_snapshot, trcmd_snapshot_size, ppos);
	if (copy_to_user(buffer, trcmd_str_buf, ret))
		return -EFAULT;

	return ret;
}

static const struct file_operations pvr_dbg_trcmd_fops = {
	.owner		= THIS_MODULE,
	.open		= pvr_dbg_trcmd_open,
	.release	= pvr_dbg_trcmd_release,
	.read		= pvr_dbg_trcmd_read,
};
#endif

/*
 * llseek helper.
 */
static loff_t
pvr_debugfs_llseek_helper(struct file *filp, loff_t offset, int whence,
			  loff_t max)
{
	loff_t f_pos;

	switch (whence) {
	case SEEK_SET:
		if ((offset > max) || (offset < 0))
			f_pos = -EINVAL;
		else
			f_pos = offset;
		break;
	case SEEK_CUR:
		if (((filp->f_pos + offset) > max) ||
		    ((filp->f_pos + offset) < 0))
			f_pos = -EINVAL;
		else
			f_pos = filp->f_pos + offset;
		break;
	case SEEK_END:
		if ((offset > 0) ||
		    (offset < -max))
			f_pos = -EINVAL;
		else
			f_pos = max + offset;
		break;
	default:
		f_pos = -EINVAL;
		break;
	}

	if (f_pos >= 0)
		filp->f_pos = f_pos;

	return f_pos;
}

/*
 * One shot register dump.
 *
 * Only in D0 can we read all registers. Our driver currently only does either
 * D0 or D3. In D3 any register read results in a SIGBUS. There is a possibility
 * that in D1 or possibly D2 all registers apart from [0xA08:0xA4C] can be read.
 */
static int
pvr_debugfs_regs_open(struct inode *inode, struct file *filp)
{
	struct PVRSRV_DEVICE_NODE *node;
	struct PVRSRV_SGXDEV_INFO *dev;
	enum PVRSRV_ERROR error;
	u32 *regs;
	int i, ret = 0;

	regs = (u32 *) __get_free_page(GFP_KERNEL);
	if (!regs)
		return -ENOMEM;

	pvr_lock();

	if (pvr_is_disabled()) {
		ret = -ENODEV;
		goto exit;
	}

	node = get_sgx_node();
	if (!node) {
		ret = -ENODEV;
		goto exit;
	}
	dev = node->pvDevice;

	error = PVRSRVSetDevicePowerStateKM(node->sDevId.ui32DeviceIndex,
					    PVRSRV_POWER_STATE_D0);
	if (error != PVRSRV_OK) {
		ret = -EIO;
		goto exit;
	}

	for (i = 0; i < 1024; i++)
		regs[i] = readl(dev->pvRegsBaseKM + 4 * i);

	filp->private_data = regs;

	SGXTestActivePowerEvent(node);

 exit:
	pvr_unlock();

	return ret;
}

static int
pvr_debugfs_regs_release(struct inode *inode, struct file *filp)
{
	free_page((unsigned long) filp->private_data);

	return 0;
}

#define REGS_DUMP_LINE_SIZE 17
#define REGS_DUMP_FORMAT "0x%03X 0x%08X\n"

static loff_t
pvr_debugfs_regs_llseek(struct file *filp, loff_t offset, int whence)
{
	return pvr_debugfs_llseek_helper(filp, offset, whence,
					 1024 * REGS_DUMP_LINE_SIZE);
}

static ssize_t
pvr_debugfs_regs_read(struct file *filp, char __user *buf, size_t size,
		      loff_t *f_pos)
{
	char tmp[REGS_DUMP_LINE_SIZE + 1];
	u32 *regs = filp->private_data;
	int i;

	if ((*f_pos < 0) || (size < (sizeof(tmp) - 1)))
		return 0;

	i = ((int) *f_pos) / (sizeof(tmp) - 1);
	if (i >= 1024)
		return 0;

	size = snprintf(tmp, sizeof(tmp), REGS_DUMP_FORMAT, i * 4, regs[i]);

	if (size > 0) {
		if (copy_to_user(buf, tmp + *f_pos - (i * (sizeof(tmp) - 1)),
				 size))
			return -EFAULT;

		*f_pos += size;
		return size;
	} else
		return 0;
}

static const struct file_operations pvr_debugfs_regs_fops = {
	.owner = THIS_MODULE,
	.llseek = pvr_debugfs_regs_llseek,
	.read = pvr_debugfs_regs_read,
	.open = pvr_debugfs_regs_open,
	.release = pvr_debugfs_regs_release,
};


/*
 *
 * HW Recovery dumping support.
 *
 */
static struct mutex hwrec_mutex[1];
static struct timeval hwrec_time;
static int hwrec_open_count;
static DECLARE_WAIT_QUEUE_HEAD(hwrec_wait_queue);
static int hwrec_event;

/* add extra locking to keep us from overwriting things during dumping. */
static int hwrec_event_open_count;
static int hwrec_event_file_lock;

/* While these could get moved into PVRSRV_SGXDEV_INFO, the more future-proof
 * way of handling hw recovery events is by providing 1 single hwrecovery dump
 * at a time, and adding a hwrec_info debugfs file with: process information,
 * general driver information, and the instance of the (then multicore) pvr
 * where the hwrec event happened.
 */
static u32 *hwrec_registers;

#ifdef CONFIG_PVR_DEBUG
static size_t hwrec_mem_size;
#define HWREC_MEM_PAGES (4 * PAGE_SIZE)
static unsigned long hwrec_mem_pages[HWREC_MEM_PAGES];
#endif /* CONFIG_PVR_DEBUG */

#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
static struct edm_buf_info *hwrec_edm_buf;
#endif

static void
hwrec_registers_dump(struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	int i;

	if (!hwrec_registers) {
		hwrec_registers = (u32 *) __get_free_page(GFP_KERNEL);
		if (!hwrec_registers) {
			pr_err("%s: failed to get free page.\n", __func__);
			return;
		}
	}

	for (i = 0; i < 1024; i++)
		hwrec_registers[i] = readl(psDevInfo->pvRegsBaseKM + 4 * i);
}

static void
hwrec_pages_free(size_t *size, unsigned long *pages)
{
	int i;

	if (!(*size))
		return;

	for (i = 0; (i * PAGE_SIZE) < *size; i++) {
		free_page(pages[i]);
		pages[i] = 0;
	}

	*size = 0;
}

static int
hwrec_pages_write(u8 *buffer, size_t size, size_t *current_size,
		  unsigned long *pages, int array_size)
{
	size_t ret = 0;

	while (size) {
		size_t count = size;
		size_t offset = *current_size & ~PAGE_MASK;
		int page = *current_size / PAGE_SIZE;

		if (!offset) {
			if (((*current_size) / PAGE_SIZE) >= array_size) {
				pr_err("%s: Size overrun!\n", __func__);
				return -ENOMEM;
			}

			pages[page] = __get_free_page(GFP_KERNEL);
			if (!pages[page]) {
				pr_err("%s: failed to get free page.\n",
				       __func__);
				return -ENOMEM;
			}
		}

		if (count > (PAGE_SIZE - offset))
			count = PAGE_SIZE - offset;

		memcpy(((u8 *) pages[page]) + offset, buffer, count);

		buffer += count;
		size -= count;
		ret += count;
		*current_size += count;
	}

	return ret;
}

#ifdef CONFIG_PVR_DEBUG
static void
hwrec_mem_free(void)
{
	hwrec_pages_free(&hwrec_mem_size, hwrec_mem_pages);
}

int
hwrec_mem_write(u8 *buffer, size_t size)
{
	return hwrec_pages_write(buffer, size, &hwrec_mem_size,
				 hwrec_mem_pages, ARRAY_SIZE(hwrec_mem_pages));
}

int
hwrec_mem_print(char *format, ...)
{
	char tmp[25];
	va_list ap;
	size_t size;

	va_start(ap, format);
	size = vscnprintf(tmp, sizeof(tmp), format, ap);
	va_end(ap);

	return hwrec_mem_write(tmp, size);
}
#endif /* CONFIG_PVR_DEBUG */

/*
 * Render status buffer dumping.
 */
static size_t hwrec_status_size;
static unsigned long hwrec_status_pages[1024];

static int
hwrec_status_write(char *buffer, size_t size)
{
	return hwrec_pages_write(buffer, size, &hwrec_status_size,
				 hwrec_status_pages,
				 ARRAY_SIZE(hwrec_status_pages));
}

static void
hwrec_status_free(void)
{
	hwrec_pages_free(&hwrec_status_size, hwrec_status_pages);
}

static int
hwrec_status_print(char *format, ...)
{
	char tmp[25];
	va_list ap;
	size_t size;

	va_start(ap, format);
	size = vscnprintf(tmp, sizeof(tmp), format, ap);
	va_end(ap);

	return hwrec_status_write(tmp, size);
}

#define BUF_DESC_CORRUPT	(1 << 31)

static void add_uniq_items(struct render_state_buf_list *dst,
			   const struct render_state_buf_list *src)
{
	int i;

	for (i = 0; i < src->cnt; i++) {
		const struct render_state_buf_info *sbinf = &src->info[i];
		int j;

		for (j = 0; j < dst->cnt; j++) {
			if (sbinf->buf_id == dst->info[j].buf_id) {
				if (memcmp(sbinf, &dst->info[j],
					   sizeof(*sbinf)))
					dst->info[j].type |= BUF_DESC_CORRUPT;
				break;
			}
		}
		if (j == dst->cnt) {
			/* Bound for cnt is guaranteed by the caller */
			dst->info[dst->cnt] = *sbinf;
			dst->cnt++;
		}
	}
}

static struct render_state_buf_list *create_merged_uniq_list(
		struct render_state_buf_list **bl_set, int set_size)
{
	int i;
	struct render_state_buf_list *dbl;
	size_t size;

	/*
	 * Create a buf list big enough to contain all elements from each
	 * list in bl_set.
	 */
	size = offsetof(struct render_state_buf_list, info[0]);
	for (i = 0; i < set_size; i++) {
		if (!bl_set[i])
			continue;
		size += bl_set[i]->cnt * sizeof(bl_set[i]->info[0]);
	}
	if (!size)
		return NULL;
	dbl = kmalloc(size, GFP_KERNEL);
	if (!dbl)
		return NULL;

	dbl->cnt = 0;
	for (i = 0; i < set_size; i++) {
		if (bl_set[i])
			add_uniq_items(dbl, bl_set[i]);
	}

	return dbl;
}

static void *vmap_buf(struct PVRSRV_PER_PROCESS_DATA *proc,
			u32 handle, off_t offset, size_t size)
{
	struct PVRSRV_KERNEL_MEM_INFO *minfo;
	struct LinuxMemArea *mem_area;
	enum PVRSRV_ERROR err;
	unsigned start_ofs;
	unsigned end_ofs;
	int pg_cnt;
	struct page **pages;
	void *map = NULL;
	int i;

	if (offset & PAGE_MASK)
		return NULL;

	err = PVRSRVLookupHandle(proc->psHandleBase, (void **)&minfo,
				(void *)handle, PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (err != PVRSRV_OK)
		return NULL;
	if (minfo->pvLinAddrKM)
		return minfo->pvLinAddrKM;

	err = PVRSRVLookupOSMemHandle(proc->psHandleBase, (void *)&mem_area,
					(void *)handle);
	if (err != PVRSRV_OK)
		return NULL;

	start_ofs = offset & PAGE_MASK;
	end_ofs = PAGE_ALIGN(offset + size);
	pg_cnt = (end_ofs - start_ofs) >> PAGE_SHIFT;
	pages = kmalloc(pg_cnt * sizeof(pages[0]), GFP_KERNEL);
	if (!pages)
		return NULL;
	for (i = 0; i < pg_cnt; i++) {
		unsigned pfn;

		pfn = LinuxMemAreaToCpuPFN(mem_area, start_ofs);
		if (!pfn_valid(pfn))
			goto err;
		pages[i] = pfn_to_page(pfn);
		start_ofs += PAGE_SIZE;
	}
	map = vmap(pages, pg_cnt, VM_MAP, PAGE_KERNEL);
	map += offset;
err:
	kfree(pages);

	return map;
}

static void vunmap_buf(struct PVRSRV_PER_PROCESS_DATA *proc,
			u32 handle, void *map)
{
	struct PVRSRV_KERNEL_MEM_INFO *minfo;
	enum PVRSRV_ERROR err;

	err = PVRSRVLookupHandle(proc->psHandleBase, (void **)&minfo,
				(void *)handle, PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (err != PVRSRV_OK)
		return;
	if (minfo->pvLinAddrKM)
		return;
	vunmap((void *)(((unsigned long)map) & PAGE_MASK));
}

static void dump_buf(void *start, size_t size, u32 type)
{
	char *corr = "";

	if (type & BUF_DESC_CORRUPT) {
		type &= ~BUF_DESC_CORRUPT;
		corr = "(corrupt)";
	}
	hwrec_status_print("<type %d%s size %d>\n", type, corr, size);
	hwrec_status_write(start, size);
}

static struct render_state_buf_list *get_state_buf_list(
			struct PVRSRV_PER_PROCESS_DATA *proc,
			u32 handle, off_t offset)
{
	struct PVRSRV_KERNEL_MEM_INFO *container;
	struct render_state_buf_list *buf;
	enum PVRSRV_ERROR err;

	err = PVRSRVLookupHandle(proc->psHandleBase, (void **)&container,
				(void *)handle, PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (err != PVRSRV_OK)
		return NULL;
	if (!container->pvLinAddrKM)
		return NULL;
	if (offset + sizeof(*buf) > container->ui32AllocSize)
		return NULL;

	buf = container->pvLinAddrKM + offset;

	if (buf->cnt > ARRAY_SIZE(buf->info))
		return NULL;

	return buf;
}

static void dump_state_buf_list(struct PVRSRV_PER_PROCESS_DATA *proc,
				struct render_state_buf_list *bl)
{
	int i;

	if (!bl->cnt)
		return;

	pr_info("Dumping %d render state buffers\n", bl->cnt);
	for (i = 0; i < bl->cnt; i++) {
		struct render_state_buf_info *binfo;
		void *map;

		binfo = &bl->info[i];

		map = vmap_buf(proc, binfo->buf_id, binfo->offset, binfo->size);
		if (!map)
			continue;
		dump_buf(map, binfo->size, binfo->type);

		vunmap_buf(proc, binfo->buf_id, map);
	}
}

static void dump_sgx_state_bufs(struct PVRSRV_PER_PROCESS_DATA *proc,
				struct PVRSRV_SGXDEV_INFO *dev_info)
{
	struct SGXMKIF_HOST_CTL __iomem *hctl = dev_info->psSGXHostCtl;
	struct render_state_buf_list *bl_set[2] = { NULL };
	struct render_state_buf_list *mbl;
	u32 handle_ta;
	u32 handle_3d;

	if (!proc)
		return;

	handle_ta = readl(&hctl->render_state_buf_ta_handle);
	handle_3d = readl(&hctl->render_state_buf_3d_handle);
	bl_set[0] = get_state_buf_list(proc, handle_ta,
					dev_info->state_buf_ofs);
	/*
	 * The two buf list can be the same if the TA and 3D phases used the
	 * same context at the time of the HWrec. In this case just ignore
	 * one of them.
	 */
	if (handle_ta != handle_3d)
		bl_set[1] = get_state_buf_list(proc, handle_3d,
					dev_info->state_buf_ofs);
	mbl = create_merged_uniq_list(bl_set, ARRAY_SIZE(bl_set));
	if (!mbl)
		return;

	dump_state_buf_list(proc, mbl);
	kfree(mbl);
}

void
pvr_hwrec_dump(struct PVRSRV_PER_PROCESS_DATA *proc_data,
	       struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	mutex_lock(hwrec_mutex);

	if (hwrec_open_count || hwrec_event_file_lock) {
		pr_err("%s: previous hwrec dump is still locked!\n", __func__);
		mutex_unlock(hwrec_mutex);
		return;
	}

	do_gettimeofday(&hwrec_time);
	pr_info("HW Recovery dump generated at %010ld%06ld\n",
		hwrec_time.tv_sec, hwrec_time.tv_usec);

	hwrec_registers_dump(psDevInfo);

#ifdef CONFIG_PVR_DEBUG
	hwrec_mem_free();
	mmu_hwrec_mem_dump(psDevInfo);
#endif /* CONFIG_PVR_DEBUG */

#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
	if (hwrec_edm_buf)
		pvr_edm_buffer_destroy(hwrec_edm_buf);
	hwrec_edm_buf = pvr_edm_buffer_create(psDevInfo);
#endif

	hwrec_status_free();
	dump_sgx_state_bufs(proc_data, psDevInfo);

	hwrec_event = 1;

	mutex_unlock(hwrec_mutex);

	wake_up_interruptible(&hwrec_wait_queue);
}

/*
 * helpers.
 */
static int
hwrec_file_open(struct inode *inode, struct file *filp)
{
	mutex_lock(hwrec_mutex);

	hwrec_open_count++;

	mutex_unlock(hwrec_mutex);
	return 0;
}

static int
hwrec_file_release(struct inode *inode, struct file *filp)
{
	mutex_lock(hwrec_mutex);

	hwrec_open_count--;

	mutex_unlock(hwrec_mutex);
	return 0;
}

/*
 * Provides a hwrec timestamp for unique dumping.
 */
static ssize_t
hwrec_time_read(struct file *filp, char __user *buf, size_t size,
		loff_t *f_pos)
{
	char tmp[20];

	mutex_lock(hwrec_mutex);
	snprintf(tmp, sizeof(tmp), "%010ld%06ld",
		 hwrec_time.tv_sec, hwrec_time.tv_usec);
	mutex_unlock(hwrec_mutex);

	return simple_read_from_buffer(buf, size, f_pos, tmp, strlen(tmp));
}

static const struct file_operations hwrec_time_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = hwrec_time_read,
	.open = hwrec_file_open,
	.release = hwrec_file_release,
};

/*
 * Blocks the reader until a HWRec happens.
 */
static int
hwrec_event_open(struct inode *inode, struct file *filp)
{
	int ret;

	mutex_lock(hwrec_mutex);

	if (hwrec_event_open_count)
		ret = -EUSERS;
	else {
		hwrec_event_open_count++;
		ret = 0;
	}

	mutex_unlock(hwrec_mutex);

	return ret;
}

static int
hwrec_event_release(struct inode *inode, struct file *filp)
{
	mutex_lock(hwrec_mutex);

	hwrec_event_open_count--;

	mutex_unlock(hwrec_mutex);

	return 0;
}


static ssize_t
hwrec_event_read(struct file *filp, char __user *buf, size_t size,
		 loff_t *f_pos)
{
	int ret = 0;

	mutex_lock(hwrec_mutex);

	hwrec_event_file_lock = 0;

	mutex_unlock(hwrec_mutex);

	ret = wait_event_interruptible(hwrec_wait_queue, hwrec_event);
	if (!ret) {
		mutex_lock(hwrec_mutex);

		hwrec_event = 0;
		hwrec_event_file_lock = 1;

		mutex_unlock(hwrec_mutex);
	}

	return ret;
}

static const struct file_operations hwrec_event_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = hwrec_event_read,
	.open = hwrec_event_open,
	.release = hwrec_event_release,
};

/*
 * Reads out all readable registers.
 */
static loff_t
hwrec_regs_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t f_pos;

	mutex_lock(hwrec_mutex);

	if (hwrec_registers)
		f_pos = pvr_debugfs_llseek_helper(filp, offset, whence,
						  1024 * REGS_DUMP_LINE_SIZE);
	else
		f_pos = 0;

	mutex_unlock(hwrec_mutex);

	return f_pos;
}

static ssize_t
hwrec_regs_read(struct file *filp, char __user *buf, size_t size,
		loff_t *f_pos)
{
	char tmp[REGS_DUMP_LINE_SIZE + 1];
	int i;

	if ((*f_pos < 0) || (size < (sizeof(tmp) - 1)))
		return 0;

	i = ((int) *f_pos) / (sizeof(tmp) - 1);
	if (i >= 1024)
		return 0;

	mutex_lock(hwrec_mutex);

	if (!hwrec_registers)
		size = 0;
	else
		size = snprintf(tmp, sizeof(tmp), REGS_DUMP_FORMAT, i * 4,
				hwrec_registers[i]);

	mutex_unlock(hwrec_mutex);

	if (size > 0) {
		if (copy_to_user(buf, tmp + *f_pos - (i * (sizeof(tmp) - 1)),
				 size))
			return -EFAULT;

		*f_pos += size;
		return size;
	} else
		return 0;
}

static const struct file_operations hwrec_regs_fops = {
	.owner = THIS_MODULE,
	.llseek = hwrec_regs_llseek,
	.read = hwrec_regs_read,
	.open = hwrec_file_open,
	.release = hwrec_file_release,
};

#ifdef CONFIG_PVR_DEBUG
/*
 * Provides a full context dump: page directory, page tables, and all mapped
 * pages.
 */
static loff_t
hwrec_mem_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t f_pos;

	mutex_lock(hwrec_mutex);

	if (hwrec_mem_size)
		f_pos = pvr_debugfs_llseek_helper(filp, offset, whence,
						  hwrec_mem_size);
	else
		f_pos = 0;

	mutex_unlock(hwrec_mutex);

	return f_pos;
}

static ssize_t
hwrec_mem_read(struct file *filp, char __user *buf, size_t size,
	       loff_t *f_pos)
{
	mutex_lock(hwrec_mutex);

	if ((*f_pos >= 0) && (*f_pos < hwrec_mem_size)) {
		int page, offset;

		size = min(size, (size_t) hwrec_mem_size - (size_t) *f_pos);

		page = (*f_pos) / PAGE_SIZE;
		offset = (*f_pos) & ~PAGE_MASK;

		size = min(size, (size_t) PAGE_SIZE - offset);

		if (copy_to_user(buf,
				 ((u8 *) hwrec_mem_pages[page]) + offset,
				 size)) {
			mutex_unlock(hwrec_mutex);
			return -EFAULT;
		}
	} else
		size = 0;

	mutex_unlock(hwrec_mutex);

	*f_pos += size;
	return size;
}

static const struct file_operations hwrec_mem_fops = {
	.owner = THIS_MODULE,
	.llseek = hwrec_mem_llseek,
	.read = hwrec_mem_read,
	.open = hwrec_file_open,
	.release = hwrec_file_release,
};
#endif /* CONFIG_PVR_DEBUG */

/*
 * Read out edm trace created before HW recovery reset.
 */
#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
static loff_t
hwrec_edm_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t f_pos;

	mutex_lock(hwrec_mutex);

	if (hwrec_edm_buf)
		f_pos = pvr_debugfs_llseek_helper(filp, offset, whence,
						  hwrec_edm_buf->len);
	else
		f_pos = 0;

	mutex_unlock(hwrec_mutex);

	return f_pos;
}

static ssize_t
hwrec_edm_read(struct file *filp, char __user *buf, size_t size,
	       loff_t *f_pos)
{
	ssize_t ret;

	mutex_lock(hwrec_mutex);

	if (hwrec_edm_buf)
		ret = simple_read_from_buffer(buf, size, f_pos,
					      hwrec_edm_buf->data,
					      hwrec_edm_buf->len);
	else
		ret = 0;

	mutex_unlock(hwrec_mutex);

	return ret;
}

static const struct file_operations hwrec_edm_fops = {
	.owner = THIS_MODULE,
	.llseek = hwrec_edm_llseek,
	.read = hwrec_edm_read,
	.open = hwrec_file_open,
	.release = hwrec_file_release,
};
#endif /* PVRSRV_USSE_EDM_STATUS_DEBUG */

/*
 * Provides a dump of the TA and 3D status buffers.
 */
static loff_t
hwrec_status_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t f_pos;

	mutex_lock(hwrec_mutex);

	if (hwrec_status_size)
		f_pos = pvr_debugfs_llseek_helper(filp, offset, whence,
						  hwrec_status_size);
	else
		f_pos = 0;

	mutex_unlock(hwrec_mutex);

	return f_pos;
}

static ssize_t
hwrec_status_read(struct file *filp, char __user *buf, size_t size,
	       loff_t *f_pos)
{
	mutex_lock(hwrec_mutex);

	if ((*f_pos >= 0) && (*f_pos < hwrec_status_size)) {
		int page, offset;

		size = min(size, (size_t) hwrec_status_size - (size_t) *f_pos);

		page = (*f_pos) / PAGE_SIZE;
		offset = (*f_pos) & ~PAGE_MASK;

		size = min(size, (size_t) PAGE_SIZE - offset);

		if (copy_to_user(buf,
				 ((u8 *) hwrec_status_pages[page]) + offset,
				 size)) {
			mutex_unlock(hwrec_mutex);
			return -EFAULT;
		}
	} else
		size = 0;

	mutex_unlock(hwrec_mutex);

	*f_pos += size;
	return size;
}

static const struct file_operations hwrec_status_fops = {
	.owner = THIS_MODULE,
	.llseek = hwrec_status_llseek,
	.read = hwrec_status_read,
	.open = hwrec_file_open,
	.release = hwrec_file_release,
};

/*
 *
 */
int pvr_debugfs_init(void)
{
	mutex_init(hwrec_mutex);

	pvr_debugfs_dir = debugfs_create_dir("pvr", NULL);
	if (!pvr_debugfs_dir)
		return -ENODEV;

	if (!debugfs_create_file("reset_sgx", S_IWUSR, pvr_debugfs_dir,
				 &pvr_reset, &pvr_debugfs_reset_fops)) {
		debugfs_remove(pvr_debugfs_dir);
		return -ENODEV;
	}

#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
	if (!debugfs_create_file("edm_trace", S_IRUGO, pvr_debugfs_dir, NULL,
				 &pvr_debugfs_edm_fops)) {
		debugfs_remove_recursive(pvr_debugfs_dir);
		return -ENODEV;
	}
#endif
#ifdef CONFIG_PVR_TRACE_CMD
	if (!debugfs_create_file("command_trace", S_IRUGO, pvr_debugfs_dir,
				 NULL, &pvr_dbg_trcmd_fops)) {
		debugfs_remove_recursive(pvr_debugfs_dir);
		return -ENODEV;
	}
#endif

	if (!debugfs_create_file("registers", S_IRUSR, pvr_debugfs_dir, NULL,
				 &pvr_debugfs_regs_fops)) {
		debugfs_remove(pvr_debugfs_dir);
		return -ENODEV;
	}

	if (!debugfs_create_file("hwrec_event", S_IRUSR, pvr_debugfs_dir, NULL,
				 &hwrec_event_fops)) {
		debugfs_remove_recursive(pvr_debugfs_dir);
		return -ENODEV;
	}

	if (!debugfs_create_file("hwrec_time", S_IRUSR, pvr_debugfs_dir, NULL,
				 &hwrec_time_fops)) {
		debugfs_remove_recursive(pvr_debugfs_dir);
		return -ENODEV;
	}

	if (!debugfs_create_file("hwrec_regs", S_IRUSR, pvr_debugfs_dir, NULL,
				 &hwrec_regs_fops)) {
		debugfs_remove_recursive(pvr_debugfs_dir);
		return -ENODEV;
	}

#ifdef CONFIG_PVR_DEBUG
	if (!debugfs_create_file("hwrec_mem", S_IRUSR, pvr_debugfs_dir, NULL,
				 &hwrec_mem_fops)) {
		debugfs_remove_recursive(pvr_debugfs_dir);
		return -ENODEV;
	}
#endif /* CONFIG_PVR_DEBUG */

#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
	if (!debugfs_create_file("hwrec_edm", S_IRUSR, pvr_debugfs_dir, NULL,
				 &hwrec_edm_fops)) {
		debugfs_remove_recursive(pvr_debugfs_dir);
		return -ENODEV;
	}
#endif

	if (!debugfs_create_file("hwrec_status", S_IRUSR, pvr_debugfs_dir, NULL,
				 &hwrec_status_fops)) {
		debugfs_remove_recursive(pvr_debugfs_dir);
		return -ENODEV;
	}

	return 0;
}

void pvr_debugfs_cleanup(void)
{
	debugfs_remove_recursive(pvr_debugfs_dir);

	if (hwrec_registers)
		free_page((u32) hwrec_registers);

#ifdef CONFIG_PVR_DEBUG
	hwrec_mem_free();
#endif /* CONFIG_PVR_DEBUG */

#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
	if (hwrec_edm_buf)
		pvr_edm_buffer_destroy(hwrec_edm_buf);
#endif

	hwrec_status_free();
}
