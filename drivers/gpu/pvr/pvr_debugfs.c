/*
 * Copyright (c) 2010-2011 Imre Deak <imre.deak@nokia.com>
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

#include "img_types.h"
#include "servicesext.h"
#include "services.h"
#include "sgxinfokm.h"
#include "syscommon.h"
#include "pvr_bridge_km.h"
#include "sgxutils.h"
#include "pvr_debugfs.h"

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
 */
int pvr_debugfs_init(void)
{
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

	return 0;
}

void pvr_debugfs_cleanup(void)
{
	debugfs_remove_recursive(debugfs_dir);
}
