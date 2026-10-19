/*
 * Copyright (C) 2012 Texas Instruments, Inc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <asm/page.h>
#include <linux/slab.h>
#include "services_headers.h"
#include "pdump_km.h"
#include "sysfs.h"

/* sysfs structures */
struct pvrsrv_attribute {
	struct attribute attr;
	int sgx_version;
	int sgx_revision;
};

static struct pvrsrv_attribute PVRSRVAttr = {
	.attr.name = "egl.cfg",
	.attr.mode = S_IRUGO,
#if defined(SGX544)
	.sgx_version = 544,
	.sgx_revision = 112,
#else
	.sgx_version = 540,
	.sgx_revision = 120,
#endif
};

/* sysfs read function */
static ssize_t PVRSRVEglCfgShow(struct kobject *kobj, struct attribute *attr,
								char *buffer) {
	struct pvrsrv_attribute *pvrsrv_attr;

	pvrsrv_attr = container_of(attr, struct pvrsrv_attribute, attr);
	return snprintf(buffer, PAGE_SIZE, "0 0 android\n0 1 POWERVR_SGX%d_%d",
			pvrsrv_attr->sgx_version, pvrsrv_attr->sgx_revision);
}

/* sysfs write function unsupported*/
static ssize_t PVRSRVEglCfgStore(struct kobject *kobj, struct attribute *attr,
					const char *buffer, size_t size) {
	PVR_DPF((PVR_DBG_ERROR, "PVRSRVEglCfgStore not implemented"));
	return 0;
}

static struct attribute *pvrsrv_sysfs_attrs[] = {
	&PVRSRVAttr.attr,
	NULL
};

static const struct sysfs_ops pvrsrv_sysfs_ops = {
	.show = PVRSRVEglCfgShow,
	.store = PVRSRVEglCfgStore,
};

static struct kobj_type pvrsrv_ktype = {
	.sysfs_ops = &pvrsrv_sysfs_ops,
	.default_attrs = pvrsrv_sysfs_attrs,
};

/* create sysfs entry /sys/egl/egl.cfg to determine
   which gfx libraries to load */

int PVRSRVCreateSysfsEntry(void)
{
	struct kobject *egl_cfg_kobject;
	int r;

	egl_cfg_kobject = kzalloc(sizeof(*egl_cfg_kobject), GFP_KERNEL);
	r = kobject_init_and_add(egl_cfg_kobject, &pvrsrv_ktype, NULL, "egl");

	if (r) {
		PVR_DPF((PVR_DBG_ERROR,
			"Failed to create egl.cfg sysfs entry"));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	return PVRSRV_OK;
}
