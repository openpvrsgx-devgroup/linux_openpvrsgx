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

#ifndef __SERVICES_PROC_H__
#define __SERVICES_PROC_H__

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
#include <asm/exec.h>
#include <asm/cmpxchg.h>
#include <asm/auxvec.h>
#include <asm/switch_to.h>
#else
#include <asm/system.h>
#endif
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define END_OF_FILE (off_t) -1

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
typedef int (pvr_read_proc_t)(struct seq_file *, void *v);

typedef int (read_proc_t)(char *page, char **start, off_t off,
			  int count, int *eof, void *data);
typedef int (write_proc_t)(struct file *file, const char __user *buffer,
			   unsigned long count, void *data);
#else
typedef off_t (pvr_read_proc_t)(IMG_CHAR *, size_t, off_t);
#endif

#ifdef PVR_PROC_USE_SEQ_FILE
#define PVR_PROC_SEQ_START_TOKEN (void*)1
typedef void* (pvr_next_proc_seq_t)(struct seq_file *,void*,loff_t);
typedef void* (pvr_off2element_proc_seq_t)(struct seq_file *, loff_t);
typedef void (pvr_show_proc_seq_t)(struct seq_file *,void*);
typedef void (pvr_startstop_proc_seq_t)(struct seq_file *, IMG_BOOL start);

typedef struct _PVR_PROC_SEQ_HANDLERS_ {
	pvr_next_proc_seq_t *next;
	pvr_show_proc_seq_t *show;
	pvr_off2element_proc_seq_t *off2element;
	pvr_startstop_proc_seq_t *startstop;
	IMG_VOID *data;
} PVR_PROC_SEQ_HANDLERS;


void* ProcSeq1ElementOff2Element(struct seq_file *sfile, loff_t off);

void* ProcSeq1ElementHeaderOff2Element(struct seq_file *sfile, loff_t off);


#endif

off_t printAppend(IMG_CHAR * buffer, size_t size, off_t off, const IMG_CHAR * format, ...)
	__attribute__((format(printf, 4, 5)));

IMG_INT CreateProcEntries(IMG_VOID);

IMG_INT CreateProcReadEntry (const IMG_CHAR * name, pvr_read_proc_t handler);

IMG_INT CreateProcEntry(const IMG_CHAR * name, read_proc_t rhandler, write_proc_t whandler, IMG_VOID *data);

IMG_INT CreatePerProcessProcEntry(const IMG_CHAR * name, read_proc_t rhandler, write_proc_t whandler, IMG_VOID *data);

IMG_VOID RemoveProcEntry(const IMG_CHAR * name);

IMG_VOID RemovePerProcessProcEntry(const IMG_CHAR * name);

IMG_VOID RemoveProcEntries(IMG_VOID);

#ifdef PVR_PROC_USE_SEQ_FILE
struct proc_dir_entry* CreateProcReadEntrySeq (
								const IMG_CHAR* name,
								IMG_VOID* data,
								pvr_next_proc_seq_t next_handler,
								pvr_show_proc_seq_t show_handler,
								pvr_off2element_proc_seq_t off2element_handler,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
								pvr_startstop_proc_seq_t startstop_handler,
								PVR_PROC_SEQ_HANDLERS **handlers
#else
								pvr_startstop_proc_seq_t startstop_handler
#endif
							   );

struct proc_dir_entry* CreateProcEntrySeq (
								const IMG_CHAR* name,
								IMG_VOID* data,
								pvr_next_proc_seq_t next_handler,
								pvr_show_proc_seq_t show_handler,
								pvr_off2element_proc_seq_t off2element_handler,
								pvr_startstop_proc_seq_t startstop_handler,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
								write_proc_t whandler,
								PVR_PROC_SEQ_HANDLERS **handlers
#else
								write_proc_t whandler
#endif
							   );

struct proc_dir_entry* CreatePerProcessProcEntrySeq (
								const IMG_CHAR* name,
								IMG_VOID* data,
								pvr_next_proc_seq_t next_handler,
								pvr_show_proc_seq_t show_handler,
								pvr_off2element_proc_seq_t off2element_handler,
								pvr_startstop_proc_seq_t startstop_handler,
								write_proc_t whandler
							   );


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
IMG_VOID RemoveProcEntrySeq(struct proc_dir_entry* proc_entry, const char *name,
			    PVR_PROC_SEQ_HANDLERS *handlers);
#else
IMG_VOID RemoveProcEntrySeq(struct proc_dir_entry* proc_entry);
#endif
IMG_VOID RemovePerProcessProcEntrySeq(struct proc_dir_entry* proc_entry);

#endif

#endif
