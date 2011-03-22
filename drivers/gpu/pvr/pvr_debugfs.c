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

static struct dentry *debugfs_dir;
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

	HWRecoveryResetSGX(node);

	SGXTestActivePowerEvent(node);
exit:
	pvr_unlock();

	return r;
}

static int pvr_debugfs_set(void *data, u64 val)
{
	u32 *var = data;

	if (var == &pvr_reset)
		return pvr_debugfs_reset(data, val);

	BUG();
}

DEFINE_SIMPLE_ATTRIBUTE(pvr_debugfs_fops, NULL, pvr_debugfs_set, "%llu\n");

/*
 *
 */
struct edm_buf_info {
	size_t len;
	char data[1];
};

static int pvr_debugfs_edm_open(struct inode *inode, struct file *file)
{
	struct PVRSRV_DEVICE_NODE *node;
	struct PVRSRV_SGXDEV_INFO *sgx_info;
	struct edm_buf_info *bi;
	size_t size;

	/* Take a snapshot of the EDM trace buffer */
	size = SGXMK_TRACE_BUFFER_SIZE * SGXMK_TRACE_BUF_STR_LEN;
	bi = vmalloc(sizeof(*bi) + size);
	if (!bi)
		return -ENOMEM;

	node = get_sgx_node();
	sgx_info = node->pvDevice;
	bi->len = snprint_edm_trace(sgx_info, bi->data, size);
	file->private_data = bi;

	return 0;
}

static int pvr_debugfs_edm_release(struct inode *inode, struct file *file)
{
	vfree(file->private_data);

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
hwrec_pages_free(size_t *size, u32 *pages)
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
hwrec_pages_write(u8 *buffer, size_t size, size_t *current_size, u32 *pages,
		  int array_size)
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

void
pvr_hwrec_dump(struct PVRSRV_SGXDEV_INFO *psDevInfo)
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

static loff_t
hwrec_llseek_helper(struct file *filp, loff_t offset, int whence, loff_t max)
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
#define HWREC_REGS_LINE_SIZE 17

static loff_t
hwrec_regs_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t f_pos;

	mutex_lock(hwrec_mutex);

	if (hwrec_registers)
		f_pos = hwrec_llseek_helper(filp, offset, whence,
					    1024 * HWREC_REGS_LINE_SIZE);
	else
		f_pos = 0;

	mutex_unlock(hwrec_mutex);

	return f_pos;
}

static ssize_t
hwrec_regs_read(struct file *filp, char __user *buf, size_t size,
		loff_t *f_pos)
{
	char tmp[HWREC_REGS_LINE_SIZE + 1];
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
		size = snprintf(tmp, sizeof(tmp), "0x%03X 0x%08X\n", i * 4,
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
		f_pos = hwrec_llseek_helper(filp, offset, whence,
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
 *
 */
int pvr_debugfs_init(void)
{
	mutex_init(hwrec_mutex);

	debugfs_dir = debugfs_create_dir("pvr", NULL);
	if (!debugfs_dir)
		return -ENODEV;

	if (!debugfs_create_file("reset_sgx", S_IWUSR, debugfs_dir, &pvr_reset,
				 &pvr_debugfs_fops)) {
		debugfs_remove(debugfs_dir);
		return -ENODEV;
	}

	if (!debugfs_create_file("edm_trace", S_IRUGO, debugfs_dir, NULL,
				 &pvr_debugfs_edm_fops)) {
		debugfs_remove_recursive(debugfs_dir);
		return -ENODEV;
	}

	if (!debugfs_create_file("hwrec_event", S_IRUSR, debugfs_dir, NULL,
				 &hwrec_event_fops)) {
		debugfs_remove_recursive(debugfs_dir);
		return -ENODEV;
	}

	if (!debugfs_create_file("hwrec_time", S_IRUSR, debugfs_dir, NULL,
				 &hwrec_time_fops)) {
		debugfs_remove_recursive(debugfs_dir);
		return -ENODEV;
	}

	if (!debugfs_create_file("hwrec_regs", S_IRUSR, debugfs_dir, NULL,
				 &hwrec_regs_fops)) {
		debugfs_remove_recursive(debugfs_dir);
		return -ENODEV;
	}

#ifdef CONFIG_PVR_DEBUG
	if (!debugfs_create_file("hwrec_mem", S_IRUSR, debugfs_dir, NULL,
				 &hwrec_mem_fops)) {
		debugfs_remove_recursive(debugfs_dir);
		return -ENODEV;
	}
#endif /* CONFIG_PVR_DEBUG */

	return 0;
}

void pvr_debugfs_cleanup(void)
{
	debugfs_remove_recursive(debugfs_dir);

	if (hwrec_registers)
		free_page((u32) hwrec_registers);

#ifdef CONFIG_PVR_DEBUG
	hwrec_mem_free();
#endif /* CONFIG_PVR_DEBUG */

}
