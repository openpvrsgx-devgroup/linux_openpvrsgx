/**********************************************************************
 Copyright (c) Imagination Technologies Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
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
#include "linkage.h"

#include "lists.h"
DECLARE_LIST_ANY_VA(PVRSRV_DEVICE_NODE);


static struct proc_dir_entry * dir;

#ifndef PVR_PROC_USE_SEQ_FILE
static off_t procDumpSysNodes(IMG_CHAR *buf, size_t size, off_t off);
static off_t procDumpVersion(IMG_CHAR *buf, size_t size, off_t off);
#endif


static const IMG_CHAR PVRProcDirRoot[] = "pvr";


#ifdef PVR_PROC_USE_SEQ_FILE

#define PVR_PROC_SEQ_START_TOKEN (void*)1
static IMG_INT pvr_proc_open(struct inode *inode,struct file *file);
static void *pvr_proc_seq_start (struct seq_file *m, loff_t *pos);
static void pvr_proc_seq_stop (struct seq_file *m, void *v);
static void *pvr_proc_seq_next (struct seq_file *m, void *v, loff_t *pos);
static int pvr_proc_seq_show (struct seq_file *m, void *v);

static struct file_operations pvr_proc_operations =
{
	.open		= pvr_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static struct seq_operations pvr_proc_seq_operations =
{
	.start =	pvr_proc_seq_start,
	.next =		pvr_proc_seq_next,
	.stop =		pvr_proc_seq_stop,
	.show =		pvr_proc_seq_show,
};

static struct proc_dir_entry* g_pProcQueue;
static struct proc_dir_entry* g_pProcVersion;
static struct proc_dir_entry* g_pProcSysNodes;

static PVR_PROC_SEQ_HANDLERS *g_pProcQueueHandlers;
static PVR_PROC_SEQ_HANDLERS *g_pProcVersionHandlers;
static PVR_PROC_SEQ_HANDLERS *g_pProcSysNodesHandlers;

#ifdef DEBUG
static struct proc_dir_entry* g_pProcDebugLevel;
#endif

#ifdef PVR_MANUAL_POWER_CONTROL
static struct proc_dir_entry* g_pProcPowerLevel;
#endif


static void ProcSeqShowVersion(struct seq_file *sfile,void* el);

static void ProcSeqShowSysNodes(struct seq_file *sfile,void* el);
static void* ProcSeqOff2ElementSysNodes(struct seq_file * sfile, loff_t off);

#endif

off_t printAppend(IMG_CHAR * buffer, size_t size, off_t off, const IMG_CHAR * format, ...)
{
    IMG_INT n;
    size_t space = size - (size_t)off;
    va_list ap;

    PVR_ASSERT(space >= 0);

    va_start (ap, format);

    n = vsnprintf (buffer+off, space, format, ap);

    va_end (ap);

    if (n >= (IMG_INT)space || n < 0)
    {

        buffer[size - 1] = 0;
        return (off_t)(size - 1);
    }
    else
    {
        return (off + (off_t)n);
    }
}


#ifdef PVR_PROC_USE_SEQ_FILE

void* ProcSeq1ElementOff2Element(struct seq_file *sfile, loff_t off)
{

	if(!off)
		return (void*)2;
	return NULL;
}


void* ProcSeq1ElementHeaderOff2Element(struct seq_file *sfile, loff_t off)
{
	if(!off)
	{
		return PVR_PROC_SEQ_START_TOKEN;
	}


	if(off == 1)
		return (void*)2;

	return NULL;
}


static IMG_INT pvr_proc_open(struct inode *inode,struct file *file)
{
	IMG_INT ret = seq_open(file, &pvr_proc_seq_operations);

	struct seq_file *seq = (struct seq_file*)file->private_data;


	seq->private = PDE_DATA(inode);
	return ret;
}

static void *pvr_proc_seq_start (struct seq_file *proc_seq_file, loff_t *pos)
{
	PVR_PROC_SEQ_HANDLERS *handlers = (PVR_PROC_SEQ_HANDLERS*)proc_seq_file->private;
	if(handlers->startstop != NULL)
		handlers->startstop(proc_seq_file, IMG_TRUE);
	return handlers->off2element(proc_seq_file, *pos);
}

static void pvr_proc_seq_stop (struct seq_file *proc_seq_file, void *v)
{
	PVR_PROC_SEQ_HANDLERS *handlers = (PVR_PROC_SEQ_HANDLERS*)proc_seq_file->private;
	if(handlers->startstop != NULL)
		handlers->startstop(proc_seq_file, IMG_FALSE);
}

static void *pvr_proc_seq_next (struct seq_file *proc_seq_file, void *v, loff_t *pos)
{
	PVR_PROC_SEQ_HANDLERS *handlers = (PVR_PROC_SEQ_HANDLERS*)proc_seq_file->private;
	(*pos)++;
	if( handlers->next != NULL)
		return handlers->next( proc_seq_file, v, *pos );
	return handlers->off2element(proc_seq_file, *pos);
}

static int pvr_proc_seq_show (struct seq_file *proc_seq_file, void *v)
{
	PVR_PROC_SEQ_HANDLERS *handlers = (PVR_PROC_SEQ_HANDLERS*)proc_seq_file->private;
	handlers->show( proc_seq_file,v );
    return 0;
}



static struct proc_dir_entry* CreateProcEntryInDirSeq(
									   struct proc_dir_entry *pdir,
									   const IMG_CHAR * name,
								   IMG_VOID* data,
									   pvr_next_proc_seq_t next_handler,
									   pvr_show_proc_seq_t show_handler,
									   pvr_off2element_proc_seq_t off2element_handler,
									   pvr_startstop_proc_seq_t startstop_handler,
									   write_proc_t whandler,
									   PVR_PROC_SEQ_HANDLERS **handlers
									   )
{
	PVR_PROC_SEQ_HANDLERS *seq_handlers;
	struct proc_dir_entry * file;
	mode_t mode;

    if (!dir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntryInDirSeq: cannot make proc entry /proc/%s/%s: no parent", PVRProcDirRoot, name));
        return NULL;
    }

	mode = S_IFREG;

    if (show_handler)
    {
		mode |= S_IRUGO;
    }

    if (whandler)
    {
		mode |= S_IWUSR;
    }

    seq_handlers =  kmalloc(sizeof(PVR_PROC_SEQ_HANDLERS), GFP_KERNEL);
    if(seq_handlers)
    {
	    seq_handlers->next = next_handler;
	    seq_handlers->show = show_handler;
	    seq_handlers->off2element = off2element_handler;
	    seq_handlers->startstop = startstop_handler;
	    seq_handlers->data = data;
    }

    file = proc_create_data(name, mode, pdir, &pvr_proc_operations,
			    seq_handlers);

    if (file) {
	    *handlers = seq_handlers;
	    return file;
    } else
	    kfree(seq_handlers)

    PVR_DPF((PVR_DBG_ERROR, "CreateProcEntryInDirSeq: cannot make proc entry /proc/%s/%s: no memory", PVRProcDirRoot, name));
    return 0;
}


struct proc_dir_entry* CreateProcReadEntrySeq (
								const IMG_CHAR * name,
								IMG_VOID* data,
								pvr_next_proc_seq_t next_handler,
								pvr_show_proc_seq_t show_handler,
								pvr_off2element_proc_seq_t off2element_handler,
								pvr_startstop_proc_seq_t startstop_handler,
								PVR_PROC_SEQ_HANDLERS **handlers
							   )
{
	return CreateProcEntrySeq(name,
				  data,
				  next_handler,
				  show_handler,
				  off2element_handler,
				  startstop_handler,
				  NULL,
				  handlers);
}

struct proc_dir_entry* CreateProcEntrySeq (
											const IMG_CHAR * name,
											IMG_VOID* data,
											pvr_next_proc_seq_t next_handler,
											pvr_show_proc_seq_t show_handler,
											pvr_off2element_proc_seq_t off2element_handler,
											pvr_startstop_proc_seq_t startstop_handler,
											write_proc_t whandler,
											PVR_PROC_SEQ_HANDLERS **handlers
	)
{
	return CreateProcEntryInDirSeq(
								   dir,
								   name,
								   data,
								   next_handler,
								   show_handler,
								   off2element_handler,
								   startstop_handler,
								   NULL,
								   handlers
								  );
}



struct proc_dir_entry* CreatePerProcessProcEntrySeq (
									  const IMG_CHAR * name,
								  IMG_VOID* data,
									  pvr_next_proc_seq_t next_handler,
									  pvr_show_proc_seq_t show_handler,
									  pvr_off2element_proc_seq_t off2element_handler,
									  pvr_startstop_proc_seq_t startstop_handler,
									  write_proc_t whandler
									 )
{
    PVRSRV_ENV_PER_PROCESS_DATA *psPerProc;
    IMG_UINT32 ui32PID;

    if (!dir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntrySeq: /proc/%s doesn't exist", PVRProcDirRoot));
        return NULL;
    }

    ui32PID = OSGetCurrentProcessIDKM();

    psPerProc = PVRSRVPerProcessPrivateData(ui32PID);
    if (!psPerProc)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntrySeq: no per process data"));

        return NULL;
    }

    if (!psPerProc->psProcDir)
    {
        IMG_CHAR dirname[16];
        IMG_INT ret;

        ret = snprintf(dirname, sizeof(dirname), "%lu", ui32PID);

		if (ret <=0 || ret >= (IMG_INT)sizeof(dirname))
		{
			PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: couldn't generate per process proc directory name \"%u\"", ui32PID));
			return NULL;
		}
		else
		{
			psPerProc->psProcDir = proc_mkdir(dirname, dir);
			if (!psPerProc->psProcDir)
			{
				PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: couldn't create per process proc directory /proc/%s/%u",
						PVRProcDirRoot, ui32PID));
				return NULL;
			}
		}
    }

    return CreateProcEntryInDirSeq(psPerProc->psProcDir, name, data, next_handler,
				   show_handler,off2element_handler,startstop_handler,whandler, NULL);
}


IMG_VOID RemoveProcEntrySeq(struct proc_dir_entry* proc_entry, const char *name,
			    PVR_PROC_SEQ_HANDLERS *handlers)
{
    if (dir)
    {
        PVR_DPF((PVR_DBG_MESSAGE, "Removing /proc/%s/%s", PVRProcDirRoot, name));

        remove_proc_entry(name, dir);
	kfree(handlers);
    }
}

IMG_VOID RemovePerProcessProcEntrySeq(struct proc_dir_entry* proc_entry)
{
    PVRSRV_ENV_PER_PROCESS_DATA *psPerProc;

    psPerProc = LinuxTerminatingProcessPrivateData();
    if (!psPerProc)
    {
        psPerProc = PVRSRVFindPerProcessPrivateData();
        if (!psPerProc)
        {
            PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: can't "
                                    "remove %s, no per process data", proc_entry->name));
            return;
        }
    }

    WARN_ON_ONCE(psPerProc->psProcDir != NULL);
}

#endif

static int pvr_read_proc(struct seq_file *m, void *v)
{
    pvr_read_proc_t *pprn = (pvr_read_proc_t *)m->private;

    pprn (m, v);

    return 0;
}


static IMG_INT CreateProcEntryInDir(struct proc_dir_entry *pdir, const IMG_CHAR * name, read_proc_t rhandler, write_proc_t whandler, IMG_VOID *data)
{
    struct proc_dir_entry * file;
    mode_t mode;

    if (!pdir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntryInDir: parent directory doesn't exist"));

        return -ENOMEM;
    }

    mode = S_IFREG;

    if (rhandler)
    {
	mode |= S_IRUGO;
    }

    if (whandler)
    {
	mode |= S_IWUSR;
    }

    file = proc_create_data(name, mode, pdir, &pvr_proc_operations, data);

    if (file)
    {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
        file->owner = THIS_MODULE;
#endif

		PVR_DPF((PVR_DBG_MESSAGE, "Created proc entry %s in %s", name, pdir->name));

        return 0;
    }

    PVR_DPF((PVR_DBG_ERROR, "CreateProcEntry: cannot create proc entry %s in %s", name, pdir->name));

    return -ENOMEM;
}


IMG_INT CreateProcEntry(const IMG_CHAR * name, read_proc_t rhandler, write_proc_t whandler, IMG_VOID *data)
{
    return CreateProcEntryInDir(dir, name, rhandler, whandler, data);
}


IMG_INT CreatePerProcessProcEntry(const IMG_CHAR * name, read_proc_t rhandler, write_proc_t whandler, IMG_VOID *data)
{
    PVRSRV_ENV_PER_PROCESS_DATA *psPerProc;
    IMG_UINT32 ui32PID;

    if (!dir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: /proc/%s doesn't exist", PVRProcDirRoot));

        return -ENOMEM;
    }

    ui32PID = OSGetCurrentProcessIDKM();

    psPerProc = PVRSRVPerProcessPrivateData(ui32PID);
    if (!psPerProc)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: no per process data"));

        return -ENOMEM;
    }

    if (!psPerProc->psProcDir)
    {
        IMG_CHAR dirname[16];
        IMG_INT ret;

        ret = snprintf(dirname, sizeof(dirname), "%lu", ui32PID);

		if (ret <=0 || ret >= (IMG_INT)sizeof(dirname))
		{
			PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: couldn't generate per process proc directory name \"%u\"", ui32PID));

					return -ENOMEM;
		}
		else
		{
			psPerProc->psProcDir = proc_mkdir(dirname, dir);
			if (!psPerProc->psProcDir)
			{
			PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: couldn't create per process proc directory /proc/%s/%u", PVRProcDirRoot, ui32PID));

			return -ENOMEM;
			}
		}
    }

    return CreateProcEntryInDir(psPerProc->psProcDir, name, rhandler, whandler, data);
}

static int pvr_read_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, pvr_read_proc, PDE_DATA(inode));
}

static struct file_operations pvr_read_proc_operations =
{
	.open		= pvr_read_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

IMG_INT CreateProcReadEntry(const IMG_CHAR * name, pvr_read_proc_t handler)
{
    struct proc_dir_entry * file;

    if (!dir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcReadEntry: cannot make proc entry /proc/%s/%s: no parent", PVRProcDirRoot, name));

        return -ENOMEM;
    }

    file = proc_create_data(name, S_IFREG | S_IRUGO, dir, &pvr_read_proc_operations, (IMG_VOID *)handler);

    if (file)
    {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30))
        file->owner = THIS_MODULE;
#endif
        return 0;
    }

    PVR_DPF((PVR_DBG_ERROR, "CreateProcReadEntry: cannot make proc entry /proc/%s/%s: no memory", PVRProcDirRoot, name));

    return -ENOMEM;
}


IMG_INT CreateProcEntries(IMG_VOID)
{
    dir = proc_mkdir (PVRProcDirRoot, NULL);

    if (!dir)
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: cannot make /proc/%s directory", PVRProcDirRoot));

        return -ENOMEM;
    }

#ifdef PVR_PROC_USE_SEQ_FILE
    g_pProcQueue = CreateProcReadEntrySeq("queue", NULL, NULL, ProcSeqShowQueue, ProcSeqOff2ElementQueue, NULL, &g_pProcQueueHandlers);
    g_pProcVersion = CreateProcReadEntrySeq("version", NULL, NULL, ProcSeqShowVersion, ProcSeq1ElementHeaderOff2Element, NULL, &g_pProcVersionHandlers);
    g_pProcSysNodes = CreateProcReadEntrySeq("nodes", NULL, NULL, ProcSeqShowSysNodes, ProcSeqOff2ElementSysNodes, NULL, &g_pProcSysNodesHandlers);

	if(!g_pProcQueue || !g_pProcVersion || !g_pProcSysNodes)
#else
    if (CreateProcReadEntry("queue", QueuePrintQueues) ||
		CreateProcReadEntry("version", procDumpVersion) ||
		CreateProcReadEntry("nodes", procDumpSysNodes))
#endif
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: couldn't make /proc/%s files", PVRProcDirRoot));

        return -ENOMEM;
    }


#ifdef DEBUG

#ifdef PVR_PROC_USE_SEQ_FILE
	g_pProcDebugLevel = CreateProcEntrySeq("debug_level", NULL, NULL,
											ProcSeqShowDebugLevel,
											ProcSeq1ElementOff2Element, NULL,
										    PVRDebugProcSetLevel);
	if(!g_pProcDebugLevel)
#else
	if (CreateProcEntry ("debug_level", PVRDebugProcGetLevel, PVRDebugProcSetLevel, 0))
#endif
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: couldn't make /proc/%s/debug_level", PVRProcDirRoot));

        return -ENOMEM;
    }

#ifdef PVR_MANUAL_POWER_CONTROL
#ifdef PVR_PROC_USE_SEQ_FILE
	g_pProcPowerLevel = CreateProcEntrySeq("power_control", NULL, NULL,
											ProcSeqShowPowerLevel,
											ProcSeq1ElementOff2Element, NULL,
										    PVRProcSetPowerLevel);
	if(!g_pProcPowerLevel)
#else
	if (CreateProcEntry("power_control", PVRProcGetPowerLevel, PVRProcSetPowerLevel, 0))
#endif
    {
        PVR_DPF((PVR_DBG_ERROR, "CreateProcEntries: couldn't make /proc/%s/power_control", PVRProcDirRoot));

        return -ENOMEM;
    }
#endif
#endif

    return 0;
}


IMG_VOID RemoveProcEntry(const IMG_CHAR * name)
{
    if (dir)
    {
        remove_proc_entry(name, dir);
        PVR_DPF((PVR_DBG_MESSAGE, "Removing /proc/%s/%s", PVRProcDirRoot, name));
    }
}


IMG_VOID RemovePerProcessProcEntry(const IMG_CHAR *name)
{
    PVRSRV_ENV_PER_PROCESS_DATA *psPerProc;

    psPerProc = LinuxTerminatingProcessPrivateData();
    if (!psPerProc)
    {
        psPerProc = PVRSRVFindPerProcessPrivateData();
        if (!psPerProc)
        {
            PVR_DPF((PVR_DBG_ERROR, "CreatePerProcessProcEntries: can't "
                                    "remove %s, no per process data", name));
            return;
        }
    }

    if (psPerProc->psProcDir)
    {
        remove_proc_entry(name, psPerProc->psProcDir);

        PVR_DPF((PVR_DBG_MESSAGE, "Removing proc entry %s from %s", name, psPerProc->psProcDir->name));
    }
}


IMG_VOID RemovePerProcessProcDir(PVRSRV_ENV_PER_PROCESS_DATA *psPerProc)
{
	WARN_ON_ONCE(psPerProc->psProcDir != NULL);
}

IMG_VOID RemoveProcEntries(IMG_VOID)
{
#ifdef DEBUG

#ifdef PVR_PROC_USE_SEQ_FILE
    RemoveProcEntrySeq( g_pProcDebugLevel );
#else
    RemoveProcEntry("debug_level");
#endif

#ifdef PVR_MANUAL_POWER_CONTROL
#ifdef PVR_PROC_USE_SEQ_FILE
    RemoveProcEntrySeq( g_pProcPowerLevel );
#else
    RemoveProcEntry("power_control");
#endif
#endif

#endif

#ifdef PVR_PROC_USE_SEQ_FILE
    RemoveProcEntrySeq(g_pProcQueue, "queue", g_pProcQueueHandlers);
    RemoveProcEntrySeq(g_pProcVersion, "version", g_pProcVersionHandlers);
    RemoveProcEntrySeq(g_pProcSysNodes, "nodes", g_pProcSysNodesHandlers);
#else
    RemoveProcEntry("queue");
    RemoveProcEntry("version");
    RemoveProcEntry("nodes");
#endif

    remove_proc_entry(PVRProcDirRoot, NULL);
}


#ifdef PVR_PROC_USE_SEQ_FILE

static void ProcSeqShowVersion(struct seq_file *sfile,void* el)
{
	SYS_DATA * psSysData;
	IMG_CHAR *pszSystemVersionString = "None";

	if(el == PVR_PROC_SEQ_START_TOKEN)
	{
		seq_printf( sfile,
						"Version %s (%s) %s\n",
						PVRVERSION_STRING,
						PVR_BUILD_TYPE, PVR_BUILD_DIR);
		return;
	}

    SysAcquireData(&psSysData);

    if(psSysData->pszVersionString)
	{
	pszSystemVersionString = psSysData->pszVersionString;
    }

	seq_printf( sfile, "System Version String: %s\n", pszSystemVersionString);
}

#else

static off_t procDumpVersion(IMG_CHAR *buf, size_t size, off_t off)
{
    SYS_DATA *psSysData;

    if (off == 0)
    {
	return printAppend(buf, size, 0,
						"Version %s (%s) %s\n",
						PVRVERSION_STRING,
						PVR_BUILD_TYPE, PVR_BUILD_DIR);
    }

    SysAcquireData(&psSysData)

    if (off == 1)
    {
        IMG_CHAR *pszSystemVersionString = "None";

        if(psSysData->pszVersionString)
        {
            pszSystemVersionString = psSysData->pszVersionString;
        }

        if(strlen(pszSystemVersionString)
            + strlen("System Version String: \n")
            + 1 > size)
        {
            return 0;
        }
        return printAppend(buf, size, 0,
                            "System Version String: %s\n",
                            pszSystemVersionString);
    }

    return END_OF_FILE;
}

#endif


static const IMG_CHAR *deviceTypeToString(PVRSRV_DEVICE_TYPE deviceType)
{
    switch (deviceType)
    {
        default:
        {
            static IMG_CHAR text[10];

            sprintf(text, "?%x", (IMG_UINT)deviceType);

            return text;
        }
    }
}


static const IMG_CHAR *deviceClassToString(PVRSRV_DEVICE_CLASS deviceClass)
{
    switch (deviceClass)
    {
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
	    static IMG_CHAR text[10];

	    sprintf(text, "?%x", (IMG_UINT)deviceClass);
	    return text;
	}
    }
}

IMG_VOID* DecOffPsDev_AnyVaCb(PVRSRV_DEVICE_NODE *psNode, va_list va)
{
	off_t *pOff = va_arg(va, off_t*);
	if (--(*pOff))
	{
		return IMG_NULL;
	}
	else
	{
		return psNode;
	}
}

#ifdef PVR_PROC_USE_SEQ_FILE

static void ProcSeqShowSysNodes(struct seq_file *sfile,void* el)
{
	SYS_DATA * psSysData;
    PVRSRV_DEVICE_NODE	*psDevNode = (PVRSRV_DEVICE_NODE*)el;

	if(el == PVR_PROC_SEQ_START_TOKEN)
	{
		seq_printf( sfile,
						"Registered nodes\n"
						"Addr     Type     Class    Index Ref pvDev     Size Res\n");
		return;
	}

    SysAcquireData(&psSysData);

	seq_printf( sfile,
				  "%p %-8s %-8s %4d  %2lu  %p  %3lu  %p\n",
				  psDevNode,
				  deviceTypeToString(psDevNode->sDevId.eDeviceType),
				  deviceClassToString(psDevNode->sDevId.eDeviceClass),
				  psDevNode->sDevId.eDeviceClass,
				  psDevNode->ui32RefCount,
				  psDevNode->pvDevice,
				  psDevNode->ui32pvDeviceSize,
				  psDevNode->hResManContext);

}

static void* ProcSeqOff2ElementSysNodes(struct seq_file * sfile, loff_t off)
{
    SYS_DATA *psSysData;
    PVRSRV_DEVICE_NODE	*psDevNode;
	if(!off)
	{
		return PVR_PROC_SEQ_START_TOKEN;
	}

    SysAcquireData(&psSysData);


	psDevNode = (PVRSRV_DEVICE_NODE*)
				List_PVRSRV_DEVICE_NODE_Any_va(psSysData->psDeviceNodeList,
													DecOffPsDev_AnyVaCb,
													&off);


	return (void*)psDevNode;
}

#else

static
off_t procDumpSysNodes(IMG_CHAR *buf, size_t size, off_t off)
{
    SYS_DATA 			*psSysData;
    PVRSRV_DEVICE_NODE	*psDevNode;
    off_t				len;


    if (size < 80)
    {
		return 0;
    }

    if (off == 0)
    {
		return printAppend(buf, size, 0,
						"Registered nodes\n"
						"Addr     Type     Class    Index Ref pvDev     Size Res\n");
    }

    SysAcquireData(&psSysData);


	psDevNode = (PVRSRV_DEVICE_NODE*)
				List_PVRSRV_DEVICE_NODE_Any_va(psSysData->psDeviceNodeList,
													DecOffPsDev_AnyVaCb,
													&off);

    if (!psDevNode)
    {
		return END_OF_FILE;
    }

    len = printAppend(buf, size, 0,
				  "%p %-8s %-8s %4d  %2lu  %p  %3lu  %p\n",
				  psDevNode,
				  deviceTypeToString(psDevNode->sDevId.eDeviceType),
				  deviceClassToString(psDevNode->sDevId.eDeviceClass),
				  psDevNode->sDevId.eDeviceClass,
				  psDevNode->ui32RefCount,
				  psDevNode->pvDevice,
				  psDevNode->ui32pvDeviceSize,
				  psDevNode->hResManContext);
    return (len);
}

#endif

