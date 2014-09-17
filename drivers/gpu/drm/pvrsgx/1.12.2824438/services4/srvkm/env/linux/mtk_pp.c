#include <linux/sched.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "mtk_pp.h"

#define ENABLE_AEE_WHEN_LOCKUP

#if defined(ENABLE_AEE_WHEN_LOCKUP)
#include <linux/workqueue.h>
#include <linux/aee.h>
#endif

#if defined(MTK_DEBUG) && defined(MTK_DEBUG_PROC_PRINT)

static struct proc_dir_entry *g_MTKPP_proc;
static MTK_PROC_PRINT_DATA *g_MTKPPdata[MTKPP_ID_SIZE];

static spinlock_t g_MTKPP_4_SGXDumpDebugInfo_lock;
static unsigned long g_MTKPP_4_SGXDumpDebugInfo_irqflags;
static void *g_MTKPP_4_SGXDumpDebugInfo_current;

#if defined(ENABLE_AEE_WHEN_LOCKUP)

typedef struct MTKPP_WORKQUEUE_t 
{	
	int cycle;  
	struct workqueue_struct     *psWorkQueue;   
} MTKPP_WORKQUEUE;
MTKPP_WORKQUEUE g_MTKPP_workqueue;

typedef struct MTKPP_WORKQUEUE_WORKER_t 
{      
	struct work_struct          sWork;  
} MTKPP_WORKQUEUE_WORKER; 
MTKPP_WORKQUEUE_WORKER g_MTKPP_worker;

#endif

static void MTKPP_InitLock(MTK_PROC_PRINT_DATA *data)
{
	spin_lock_init(&data->lock);
}

static void MTKPP_Lock(MTK_PROC_PRINT_DATA *data)
{
	spin_lock_irqsave(&data->lock, data->irqflags);
}
static void MTKPP_UnLock(MTK_PROC_PRINT_DATA *data)
{
	spin_unlock_irqrestore(&data->lock, data->irqflags);
}

static void MTKPP_PrintQueueBuffer(MTK_PROC_PRINT_DATA *data, const char *fmt, ...) MTK_PP_FORMAT_PRINTF(2,3);

static void MTKPP_PrintQueueBuffer2(MTK_PROC_PRINT_DATA *data, const char *fmt, ...) MTK_PP_FORMAT_PRINTF(2,3);

static void MTKPP_PrintRingBuffer(MTK_PROC_PRINT_DATA *data, const char *fmt, ...) MTK_PP_FORMAT_PRINTF(2,3);

static int MTKPP_PrintTime(char *buf, int n)
{
	/* copy & modify from ./kernel/printk.c */
	unsigned long long t;
	unsigned long nanosec_rem;

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	
	return snprintf(buf, n, "[%5lu.%06lu] ", (unsigned long) t, nanosec_rem / 1000);
}

static void MTKPP_PrintQueueBuffer(MTK_PROC_PRINT_DATA *data, const char *fmt, ...)
{
	va_list args;
	char *buf;
	int len;

	MTKPP_Lock(data);
	
	if ((data->current_line >= data->line_array_size)
		|| (data->current_data >= (data->data_array_size - 128)))
	{
		// out of buffer, ignore input
		MTKPP_UnLock(data);
		return;
	}

	/* move to next line */
	buf = data->line[data->current_line++] = data->data + data->current_data;
	
	/* print string */
	va_start(args, fmt);
	len = vsnprintf(buf, (data->data_array_size - data->current_data), fmt, args);
	va_end(args);
	
	data->current_data += len + 1;
	
	MTKPP_UnLock(data);
}

static void MTKPP_PrintQueueBuffer2(MTK_PROC_PRINT_DATA *data, const char *fmt, ...)
{
	va_list args;
	char *buf;
	int len;

	MTKPP_Lock(data);
	
	if ((data->current_line >= data->line_array_size)
		|| (data->current_data >= (data->data_array_size - 128)))
	{
		/* buffer full, ignore the coming input */
		MTKPP_UnLock(data);
		return;
	}

	/* move to next line */
	buf = data->line[data->current_line++] = data->data + data->current_data;

	/* add the current time stamp */
	len = MTKPP_PrintTime(buf, (data->data_array_size - data->current_data));
	buf += len;
	data->current_data += len;

	/* print string */
	va_start(args, fmt);
	len = vsnprintf(buf, (data->data_array_size - data->current_data), fmt, args);
	va_end(args);
	
	data->current_data += len + 1 ;
	
	MTKPP_UnLock(data);
}

static void MTKPP_PrintRingBuffer(MTK_PROC_PRINT_DATA *data, const char *fmt, ...)
{
	va_list args;
	char *buf;
	int len, s;

	MTKPP_Lock(data);
	
	if ((data->current_line >= data->line_array_size)
		|| (data->current_data >= (data->data_array_size - 128)))
	{
		/* buffer full, move the pointer to the head */
		data->current_line = 0;
		data->current_data = 0;
	}

	/* move to next line */
	buf = data->line[data->current_line++] = data->data + data->current_data;

	/* add the current time stamp */
	len = MTKPP_PrintTime(buf, (data->data_array_size - data->current_data));
	buf += len;
	data->current_data += len;

	/* print string */
	va_start(args, fmt);
	len = vsnprintf(buf, (data->data_array_size - data->current_data), fmt, args);
	va_end(args);
	
	data->current_data += len + 1 ;

	/* clear data which are overlaid by the new log */
	buf += len; s = data->current_line;
	while (s < data->line_array_size 
		&& data->line[s] != NULL 
		&& data->line[s] <= buf)
	{
		data->line[s++] = NULL;
	}
	
	MTKPP_UnLock(data);
}

static MTK_PROC_PRINT_DATA *MTKPP_AllocStruct(int type)
{
	MTK_PROC_PRINT_DATA *data;

	data = vmalloc(sizeof(MTK_PROC_PRINT_DATA));
	if (data == NULL)
	{
		_MTKPP_DEBUG_LOG("%s: vmalloc fail", __func__);
		goto err_out;
	}
	
	MTKPP_InitLock(data);

	switch (type)
	{
		case MTKPP_BUFFERTYPE_QUEUEBUFFER:
			data->pfn_print = MTKPP_PrintQueueBuffer;
			break;
		case MTKPP_BUFFERTYPE_RINGBUFFER:
			data->pfn_print = MTKPP_PrintRingBuffer;
			break;
		default:
			_MTKPP_DEBUG_LOG("%s: unknow flags: %d", __func__, type);
			goto err_out2;
			break;
	}

	data->data = NULL;
	data->line = NULL;
	data->data_array_size = 0;
	data->line_array_size = 0;
	data->current_data = 0;
	data->current_line = 0;
	data->type = type;

	return data;
		
err_out2:
	vfree(data);
err_out:	
	return NULL;

}

static void MTKPP_FreeStruct(MTK_PROC_PRINT_DATA **data)
{
	vfree(*data);
	*data = NULL;
}

static void MTKPP_AllocData(MTK_PROC_PRINT_DATA *data, int data_size, int max_line)
{
	MTKPP_Lock(data);
		
	data->data = (char *)kmalloc(sizeof(char)*data_size, GFP_ATOMIC);
	if (data->data == NULL)
	{
		//_MTKPP_DEBUG_LOG("%s, kmalloc data fail, size = %d", __func__, data_size);
		goto err_alloc_struct;
	}
	data->line = (char **)kmalloc(sizeof(char*)*max_line, GFP_ATOMIC);
	if (data->line == NULL)
	{
		//_MTKPP_DEBUG_LOG("%s, kmalloc line fail, size = %d", __func__, data_size);
		goto err_alloc_data;
	}

	data->data_array_size = data_size;
	data->line_array_size = max_line;
	
	MTKPP_UnLock(data);

	return;
	
err_alloc_data:
	kfree(data->data);
err_alloc_struct:	
	MTKPP_UnLock(data);
	return;
	
}

static void MTKPP_FreeData(MTK_PROC_PRINT_DATA *data)
{
	MTKPP_Lock(data);
	
	kfree(data->line);
	kfree(data->data);
	
	data->line = NULL;
	data->data = NULL;
	data->data_array_size = 0;
	data->line_array_size = 0;
	data->current_data = 0;
	data->current_line = 0;
		
	MTKPP_UnLock(data);
}

static void MTKPP_CleanData(MTK_PROC_PRINT_DATA *data)
{
	MTKPP_Lock(data);

	memset(data->line, 0, sizeof(char*)*data->line_array_size);
	data->current_data = 0;
	data->current_line = 0;
	
	MTKPP_UnLock(data);
}

static void* MTKPP_SeqStart(struct seq_file *s, loff_t *pos)
{
	loff_t *spos;
	
	spos = kmalloc(sizeof(loff_t), GFP_KERNEL);
	
	spin_lock_irqsave(&g_MTKPP_4_SGXDumpDebugInfo_lock, g_MTKPP_4_SGXDumpDebugInfo_irqflags);

	if (*pos >= MTKPP_ID_SIZE)	
	{
		return NULL;
	}

	if (spos == NULL)
	{	
		return NULL;
	}

	*spos = *pos;
	return spos;
}

static void* MTKPP_SeqNext(struct seq_file *s, void *v, loff_t *pos)
{
	loff_t *spos = (loff_t *) v;
	*pos = ++(*spos);
	
	return (*pos < MTKPP_ID_SIZE) ? spos : NULL;
}

static void MTKPP_SeqStop(struct seq_file *s, void *v)
{
	spin_unlock_irqrestore(&g_MTKPP_4_SGXDumpDebugInfo_lock, g_MTKPP_4_SGXDumpDebugInfo_irqflags);
	
	kfree(v);
}

static int MTKPP_SeqShow(struct seq_file *sfile, void *v)
{
	MTK_PROC_PRINT_DATA *data;
	int off, i;
	loff_t *spos = (loff_t *) v;

    off = *spos;	
	data = g_MTKPPdata[off];
		
	seq_printf(sfile, "\n" "===== buffer_id = %d =====\n", off);

	MTKPP_Lock(data);
	
	switch (data->type)
	{
		case MTKPP_BUFFERTYPE_QUEUEBUFFER:
			seq_printf(sfile, "data_size = %d/%d\n", data->current_data, data->data_array_size);
			seq_printf(sfile, "data_line = %d/%d\n", data->current_line, data->line_array_size);
			for (i = 0; i < data->current_line; ++i)
			{
				seq_printf(sfile, "%s\n", data->line[i]);
			}
			break;
		case MTKPP_BUFFERTYPE_RINGBUFFER:
			seq_printf(sfile, "data_size = %d\n", data->data_array_size);
			seq_printf(sfile, "data_line = %d\n", data->line_array_size);
			for (i = data->current_line; i < data->line_array_size; ++i)
			{
				if (data->line[i] != NULL)
				{
					seq_printf(sfile, "%s\n", data->line[i]);
				}
			}
			for (i = 0; i < data->current_line; ++i)
			{
				if (data->line[i] != NULL)
				{
					seq_printf(sfile, "%s\n", data->line[i]);
				}
			}
			break;
		default:
			break;
	}
	
	MTKPP_UnLock(data);
	
	return 0;
}

static struct seq_operations g_MTKPP_seq_ops = {
    .start = MTKPP_SeqStart,
    .next  = MTKPP_SeqNext,
    .stop  = MTKPP_SeqStop,
    .show  = MTKPP_SeqShow
};

static int MTKPP_ProcOpen(struct inode *inode, struct file *file)
{
    return seq_open(file, &g_MTKPP_seq_ops);
}

static struct file_operations g_MTKPP_proc_ops = {
    .open    = MTKPP_ProcOpen,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = seq_release
};

#if defined(ENABLE_AEE_WHEN_LOCKUP)

static IMG_VOID MTKPP_WORKR_Handle(struct work_struct *_psWork)
{	
	struct MTKPP_WORKQUEUE_WORKER_t* psWork = 
	container_of(_psWork, MTKPP_WORKQUEUE_WORKER, sWork);  

	/* avoid the build warnning */
	psWork = psWork;

	aee_kernel_exception("gpulog", "aee dump gpulog");
}

#endif

void MTKPP_Init(void)
{
	int i;
	struct {
		MTKPP_ID uid;
		MTKPP_BUFFERTYPE type;
		int data_size;
		int max_line;
	} mtk_pp_register_tabls[] =
	{
		/* buffer is allocated in MTK_PP_4_SGXOSTimer_register */
		{MTKPP_ID_SGXDumpDebugInfo, MTKPP_BUFFERTYPE_QUEUEBUFFER,   0,                  0}, 
		{MTKPP_ID_DEVMEM,           MTKPP_BUFFERTYPE_RINGBUFFER,    1024 * 1024 * 2,    1024 * 64},
		{MTKPP_ID_SYNC,             MTKPP_BUFFERTYPE_RINGBUFFER,    1024 * 8,           128},
		{MTKPP_ID_MUTEX,            MTKPP_BUFFERTYPE_RINGBUFFER,    1024 * 32,          512},
	};

	for (i = 0; i < MTKPP_ID_SIZE; ++i)
	{
		if (i != mtk_pp_register_tabls[i].uid)
		{
			_MTKPP_DEBUG_LOG("%s: index(%d) != tabel_uid(%d)", __func__, i, mtk_pp_register_tabls[i].uid);
			goto err_out;
		}
		
		g_MTKPPdata[i] = MTKPP_AllocStruct(mtk_pp_register_tabls[i].type);

		if (g_MTKPPdata[i] == NULL)
		{
			_MTKPP_DEBUG_LOG("%s: alloc struct fail: flags = %d", __func__, mtk_pp_register_tabls[i].type);
			goto err_out;
		}

		if (mtk_pp_register_tabls[i].data_size > 0)
		{
			MTKPP_AllocData(
				g_MTKPPdata[i],
				mtk_pp_register_tabls[i].data_size,
				mtk_pp_register_tabls[i].max_line
				);
			
			MTKPP_CleanData(g_MTKPPdata[i]);
		}
	}
	
	g_MTKPP_proc = create_proc_entry("gpulog", 0, NULL);
	g_MTKPP_proc->proc_fops = &g_MTKPP_proc_ops;
	
	g_MTKPP_4_SGXDumpDebugInfo_current = NULL;
	spin_lock_init(&g_MTKPP_4_SGXDumpDebugInfo_lock);
	
#if defined(ENABLE_AEE_WHEN_LOCKUP)
	g_MTKPP_workqueue.psWorkQueue = alloc_ordered_workqueue("mwp", WQ_FREEZABLE | WQ_MEM_RECLAIM);
	INIT_WORK(&g_MTKPP_worker.sWork, MTKPP_WORKR_Handle);
#endif

	return;
	
err_out:	
	return;
}

void MTKPP_Deinit(void)
{
	int i;
	
	remove_proc_entry("gpulog", NULL);
	
	for (i = (MTKPP_ID_SIZE - 1); i >= 0; --i)
	{
		MTKPP_FreeData(g_MTKPPdata[i]);
		MTKPP_FreeStruct(&g_MTKPPdata[i]);
	}
}

MTK_PROC_PRINT_DATA *MTKPP_GetData(MTKPP_ID id)
{
	return (id >= 0 && id < MTKPP_ID_SIZE) ? 
		g_MTKPPdata[id] : NULL;
}

MTK_PROC_PRINT_DATA *MTKPP_4_SGXDumpDebugInfo_GetData()
{
	return (g_MTKPP_4_SGXDumpDebugInfo_current == (void *)current) ? 
		g_MTKPPdata[MTKPP_ID_SGXDumpDebugInfo] : NULL;
}

void MTKPP_4_SGXDumpDebugInfo_Aquire(void)
{
	int i = 0;
	int allocate_size[] = {
		1024 * 1024 * 4,    // 4 MB
		1024 * 1024 * 2,    // 2 MB
		1024 * 1024 * 1,    // 1 MB
		1024 * 512          // 512 KB
	};

	MTK_PROC_PRINT_DATA *ppdata = g_MTKPPdata[MTKPP_ID_SGXDumpDebugInfo];
	
	spin_lock_irqsave(&g_MTKPP_4_SGXDumpDebugInfo_lock, g_MTKPP_4_SGXDumpDebugInfo_irqflags);

	g_MTKPP_4_SGXDumpDebugInfo_current = (void *)current;

	/* try allocate by different size */
	while (ppdata->data == NULL && i < ARRAY_SIZE(allocate_size))
	{
		MTKPP_AllocData(ppdata, allocate_size[i++], 1024 * 64);

		if (ppdata->data != NULL)
		{
			MTKPP_CleanData(ppdata);
			break;
		}
	}

	if (ppdata->data == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: fail to allocate 512 KB for gpulog", __func__));
	}

	/* print for reocrding start time */
	MTKPP_PrintQueueBuffer2(ppdata, "=== start ===");
}

void MTKPP_4_SGXDumpDebugInfo_Release(void)
{
	g_MTKPP_4_SGXDumpDebugInfo_current = NULL;
	
	spin_unlock_irqrestore(&g_MTKPP_4_SGXDumpDebugInfo_lock, g_MTKPP_4_SGXDumpDebugInfo_irqflags);

#if defined(ENABLE_AEE_WHEN_LOCKUP)
	queue_work(g_MTKPP_workqueue.psWorkQueue, &g_MTKPP_worker.sWork);
#endif
}

#endif
