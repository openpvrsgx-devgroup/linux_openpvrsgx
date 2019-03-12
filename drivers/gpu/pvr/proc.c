/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful but, except
 * as otherwise stated in writing, without any warranty; without even the
 * implied warranty of merchantability or fitness for a particular purpose.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK
 *
 ******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "services_headers.h"

#include "queue.h"
#include "resman.h"
#include "pvrmmap.h"
#include "pvr_debug.h"
#include "pvrversion.h"
#include "proc.h"
#include "perproc.h"
#include "env_perproc.h"

static const char PVRProcDirRoot[] = "pvr";

/* The proc entry for our /proc/pvr directory */
static struct proc_dir_entry *dir;

struct pvr_proc_dir_entry {
	struct proc_dir_entry	*pdir;
	char			*name;
	struct seq_operations	*rhandlers;
	file_ops_write_t	 whandler;
	void			*data;
	struct list_head	 list;
};

/* Keep the list of entries to be able to free memory */
static LIST_HEAD(ppde_list);

static int pvr_proc_open(struct inode *inode, struct file *file);
static ssize_t pvr_proc_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations pvr_proc_ops =
{
	.owner   = THIS_MODULE,
	.open    = pvr_proc_open,
	.read    = seq_read,
	.write   = pvr_proc_write,
	.llseek  = seq_lseek,
	.release = seq_release,
};

static void *ProcNodesSeqStart (struct seq_file *s, loff_t *pos);
static void *ProcNodesSeqNext  (struct seq_file *s, void *v, loff_t *pos);
static void  ProcNodesSeqStop  (struct seq_file *s, void *v);
static int   ProcNodesSeqShow  (struct seq_file *s, void *v);

static struct seq_operations pvr_proc_nodes_ops = {
	.start = ProcNodesSeqStart,
	.next  = ProcNodesSeqNext,
	.stop  = ProcNodesSeqStop,
	.show  = ProcNodesSeqShow,
};

static void *ProcVersionSeqStart (struct seq_file *s, loff_t *pos);
static void *ProcVersionSeqNext  (struct seq_file *s, void *v, loff_t *pos);
static void  ProcVersionSeqStop  (struct seq_file *s, void *v);
static int   ProcVersionSeqShow  (struct seq_file *s, void *v);

static struct seq_operations pvr_proc_version_ops = {
	.start = ProcVersionSeqStart,
	.next  = ProcVersionSeqNext,
	.stop  = ProcVersionSeqStop,
	.show  = ProcVersionSeqShow,
};

void *pvr_proc_file_get_data(struct file *file)
{
	struct pvr_proc_dir_entry *ppde = PDE_DATA(file_inode(file));
	return ppde->data;
}

static int pvr_proc_open(struct inode *inode, struct file *file)
{
	struct pvr_proc_dir_entry *ppde = PDE_DATA(inode);
	return seq_open(file, ppde->rhandlers);
}

static ssize_t pvr_proc_write(struct file *file,
			      const char __user *buffer,
			      size_t count,
			      loff_t *ppos)
{
	struct pvr_proc_dir_entry *ppde = PDE_DATA(file_inode(file));

	if (!ppde->whandler)
		return -EACCES;

	return ppde->whandler(file, buffer, count, ppde->data);
}

static int CreateProcEntryInDir(struct proc_dir_entry *pdir,
				const char *name,
				struct seq_operations *rhandlers,
				file_ops_write_t whandler,
				void *data)
{
	struct proc_dir_entry *file;
	struct pvr_proc_dir_entry *ppde;
	mode_t mode;

	if (!pdir) {
		PVR_DPF(PVR_DBG_ERROR,
			"%s: parent directory doesn't exist", __func__);

		return -ENOENT;
	}

	/* S_IFREG - regular file */
	mode = S_IFREG;

	if (rhandlers)
		mode |= 0444;
	else {
		/* Write-only proc files handling is not implemented here */
		PVR_DPF(PVR_DBG_WARNING,
			"%s: cannot make proc entry %s in %s: no read handler",
			__func__, name, pdir->name);
		return -EINVAL;
	}

	if (whandler)
		mode |= 0200;

	ppde = kmalloc(sizeof(*ppde), GFP_KERNEL);
	if (!ppde)
		goto no_ppde;

	ppde->name = kstrdup(name, GFP_KERNEL);
	if (!ppde->name)
		goto no_name;

	file = proc_create_data(name, mode, dir, &pvr_proc_ops, ppde);
	if (!file)
		goto no_file;

	INIT_LIST_HEAD(&ppde->list);
	list_add(&ppde->list, &ppde_list);
	ppde->rhandlers = rhandlers;
	ppde->whandler = whandler;
	ppde->data = data;
	ppde->pdir = pdir;

	PVR_DPF(PVR_DBG_MESSAGE, "Created proc entry %s in %s", name,
		pdir->name);

	return 0;

no_file:
	kfree(ppde->name);
no_name:
	kfree(ppde);
no_ppde:
	PVR_DPF(PVR_DBG_ERROR,
		"%s: cannot make proc entry %s in %s: no memory",
		__func__, name, pdir->name);

	return -ENOMEM;
}

int CreateProcEntry(const char *name, struct seq_operations *rhandlers,
		    file_ops_write_t whandler, void *data)
{
	return CreateProcEntryInDir(dir, name, rhandlers, whandler, data);
}

static struct proc_dir_entry *
ProcessProcDirCreate(u32 pid)
{
	struct PVRSRV_ENV_PER_PROCESS_DATA *psPerProc;
	char dirname[16];
	int ret;

	psPerProc = PVRSRVPerProcessPrivateData(pid);
	if (!psPerProc) {
		pr_err("%s: no per process data for %d\n", __func__, pid);
		return NULL;
	}

	if (psPerProc->psProcDir)
		return psPerProc->psProcDir;

	ret = snprintf(dirname, sizeof(dirname), "%u", pid);
	if (ret <= 0 || ret >= sizeof(dirname)) {
		pr_err("%s: couldn't generate per process proc dir for %d\n",
		       __func__, pid);
		return NULL;
	}

	psPerProc->psProcDir = proc_mkdir(dirname, dir);
	if (!psPerProc->psProcDir)
		pr_err("%s: couldn't create /proc/%s/%u\n",
		       __func__, PVRProcDirRoot, pid);

	return psPerProc->psProcDir;
}

static struct proc_dir_entry *
ProcessProcDirGet(u32 pid)
{
	struct PVRSRV_ENV_PER_PROCESS_DATA *psPerProc;

	psPerProc = PVRSRVPerProcessPrivateData(pid);
	if (!psPerProc) {
		pr_err("%s: no per process data for %d\n", __func__, pid);
		return NULL;
	}

	if (!psPerProc->psProcDir) {
		pr_err("%s: couldn't retrieve /proc/%s/%u\n", __func__,
		       PVRProcDirRoot, pid);
		return NULL;
	}

	return psPerProc->psProcDir;
}

int CreatePerProcessProcEntry(u32 pid, const char *name,
			      struct seq_operations *rhandlers,
			      void *data)
{
	if (!dir) {
		PVR_DPF(PVR_DBG_ERROR,
			 "CreatePerProcessProcEntries: /proc/%s doesn't exist",
			 PVRProcDirRoot);

		return -ENOENT;
	}

	if (pid) {
		struct proc_dir_entry *pid_dir = ProcessProcDirCreate(pid);

		if (!pid_dir)
			return -ENOMEM;

		return CreateProcEntryInDir(pid_dir, name, rhandlers, NULL, data);
	} else
		return CreateProcEntryInDir(dir, name, rhandlers, NULL, data);
}

int CreateProcReadEntry(const char *name,
			struct seq_operations *rhandlers)
{
	return CreateProcEntry(name, rhandlers, NULL, NULL);
}

int CreateProcEntries(void)
{
	dir = proc_mkdir(PVRProcDirRoot, NULL);

	if (!dir) {
		PVR_DPF(PVR_DBG_ERROR,
			 "CreateProcEntries: cannot make /proc/%s directory",
			 PVRProcDirRoot);

		return -ENOMEM;
	}

	if (CreateProcReadEntry("queue", &pvr_proc_queue_ops) ||
	    CreateProcReadEntry("version", &pvr_proc_version_ops) ||
	    CreateProcReadEntry("nodes", &pvr_proc_nodes_ops)) {
		PVR_DPF(PVR_DBG_ERROR,
			 "CreateProcEntries: couldn't make /proc/%s files",
			 PVRProcDirRoot);

		return -ENOMEM;
	}
#ifdef CONFIG_PVR_DEBUG_EXTRA
	if (CreateProcEntry
	    ("debug_level", &pvr_proc_debug_level_ops, PVRDebugProcSetLevel, NULL)) {
		PVR_DPF(PVR_DBG_ERROR,
			"CreateProcEntries: couldn't make /proc/%s/debug_level",
			 PVRProcDirRoot);

		return -ENOMEM;
	}
#endif

	return 0;
}

void RemoveProcEntryFromDir(struct proc_dir_entry *pdir, const char *name)
{
	struct pvr_proc_dir_entry *ppde;

	if (!pdir)
		return;

	list_for_each_entry(ppde, &ppde_list, list) {
		if (ppde->pdir == pdir && !strcmp(ppde->name, name)) {
			PVR_DPF(PVR_DBG_MESSAGE, "Removing proc entry %s from %s",
				name, pdir->name);
			remove_proc_entry(name, dir);
			list_del(&ppde->list);
			kfree(ppde->name);
			kfree(ppde);
			return;
		}
	}

	PVR_DPF(PVR_DBG_WARNING,
		"Removing proc entry %s from %s failed: entry not found",
		name, pdir->name);
}

void RemoveProcEntry(const char *name)
{
	RemoveProcEntryFromDir(dir, name);
}

void RemovePerProcessProcEntry(u32 pid, const char *name)
{
	if (pid)
		RemoveProcEntryFromDir(ProcessProcDirGet(pid), name);
	else
		RemoveProcEntry(name);
}

void RemovePerProcessProcDir(struct PVRSRV_ENV_PER_PROCESS_DATA *psPerProc)
{
	if (psPerProc->psProcDir) {
		while (psPerProc->psProcDir->subdir) {
			PVR_DPF(PVR_DBG_WARNING,
				 "Belatedly removing /proc/%s/%s/%s",
				 PVRProcDirRoot, psPerProc->psProcDir->name,
				 psPerProc->psProcDir->subdir->name);

			RemoveProcEntry(psPerProc->psProcDir->subdir->name);
		}
		RemoveProcEntry(psPerProc->psProcDir->name);
	}
}

void RemoveProcEntries(void)
{
	struct pvr_proc_dir_entry *ppde, *next;
#ifdef CONFIG_PVR_DEBUG_EXTRA
	RemoveProcEntry("debug_level");
#endif
	RemoveProcEntry("queue");
	RemoveProcEntry("nodes");
	RemoveProcEntry("version");

	list_for_each_entry_safe(ppde, next, &ppde_list, list) {
		if (ppde->pdir != dir)
			continue;

		PVR_DPF(PVR_DBG_WARNING, "Belatedly removing /proc/%s/%s",
			PVRProcDirRoot, ppde->name);

		remove_proc_entry(ppde->name, dir);
		list_del(&ppde->list);
		kfree(ppde->name);
		kfree(ppde);
	}

	remove_proc_entry(PVRProcDirRoot, NULL);
}

static int ProcVersionSeqShow(struct seq_file *s, void *v)
{
	struct SYS_DATA *psSysData;
	char *pszSystemVersionString = "None";

	if (!v)
		return 0;

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return -EIO;

	seq_printf(s, "Version %s (%s) %s\n",
		   PVRVERSION_STRING, PVR_BUILD_TYPE, PVR_BUILD_DIR);

	if (psSysData->pszVersionString)
		pszSystemVersionString = psSysData->pszVersionString;

	seq_printf(s, "System Version String: %s\n", pszSystemVersionString);

	return 0;
}

static void *ProcVersionSeqStart(struct seq_file *s, loff_t *pos)
{
	return NULL + (*pos == 0);
}

static void *ProcVersionSeqNext(struct seq_file *s, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void ProcVersionSeqStop(struct seq_file *s, void *v)
{
	/* Nothing to do */
}

static const char *deviceTypeToString(enum PVRSRV_DEVICE_TYPE deviceType)
{
	switch (deviceType) {
	default:
		{
			static char text[10];
			sprintf(text, "?%x", deviceType);
			return text;
		}
	}
}

static const char *deviceClassToString(enum PVRSRV_DEVICE_CLASS deviceClass)
{
	switch (deviceClass) {
	case PVRSRV_DEVICE_CLASS_3D:
		{
			return "3D";
		}
	case PVRSRV_DEVICE_CLASS_DISPLAY:
		{
			return "display";
		}
	case PVRSRV_DEVICE_CLASS_BUFFER:
		{
			return "buffer";
		}
	default:
		{
			static char text[10];

			sprintf(text, "?%x", deviceClass);
			return text;
		}
	}
}

static int ProcNodesSeqShow(struct seq_file *s, void *v)
{
	struct PVRSRV_DEVICE_NODE *psDevNode = v;

	if (v == SEQ_START_TOKEN) {
		seq_puts(s, "Registered nodes\n"
			 "Addr     Type     Class    Index Ref pvDev     "
			 "Size Res\n");
		return 0;
	}

	seq_printf(s, "%p %-8s %-8s %4d  %2u  %p  %3u  %p\n",
		   psDevNode,
		   deviceTypeToString(psDevNode->sDevId.eDeviceType),
		   deviceClassToString(psDevNode->sDevId.eDeviceClass),
		   psDevNode->sDevId.eDeviceClass,
		   psDevNode->ui32RefCount,
		   psDevNode->pvDevice,
		   psDevNode->ui32pvDeviceSize,
		   psDevNode->hResManContext);

	return 0;
}

static void *ProcNodesSeqStart(struct seq_file *s, loff_t *pos)
{
	struct SYS_DATA *psSysData;
	struct PVRSRV_DEVICE_NODE *psDevNode;
	off_t i;

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return NULL;

	if (*pos == 0)
		return SEQ_START_TOKEN;

	psDevNode = psSysData->psDeviceNodeList;

	for (i = 1; i < *pos && psDevNode; i++)
		psDevNode = psDevNode->psNext;

	return psDevNode;
}

static void *ProcNodesSeqNext(struct seq_file *s, void *v, loff_t *pos)
{
	struct SYS_DATA *psSysData;

	if (++(*pos) == 1) {
		if (SysAcquireData(&psSysData) != PVRSRV_OK)
			return NULL;

		return psSysData->psDeviceNodeList;
	}

	return ((struct PVRSRV_DEVICE_NODE *)v)->psNext;
}

static void ProcNodesSeqStop(struct seq_file *s, void *v)
{
	/* Nothing to do */
}
