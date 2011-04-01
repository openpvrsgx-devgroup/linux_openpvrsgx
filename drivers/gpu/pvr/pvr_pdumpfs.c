/*
 * Copyright (c) 2010-2011 by Luc Verhaegen <libv@codethink.co.uk>
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

#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>

#include "img_defs.h"
#include "services_headers.h"
#include "pvr_pdump.h"
#include "pvr_pdumpfs.h"

static struct mutex pdumpfs_mutex[1];

enum pdumpfs_mode {
	PDUMPFS_MODE_DISABLED,
	PDUMPFS_MODE_STANDARD,
	PDUMPFS_MODE_FULL,
};

static enum pdumpfs_mode pdumpfs_mode =
#if defined(CONFIG_PVR_PDUMP_MODE_STANDARD)
	PDUMPFS_MODE_STANDARD
#elif defined(CONFIG_PVR_PDUMP_MODE_FULL)
	PDUMPFS_MODE_FULL
#else
	PDUMPFS_MODE_DISABLED
#endif
	;

#define FRAME_PAGE_COUNT 2040

struct pdumpfs_frame {
	struct pdumpfs_frame *next;

	u32 pid;
	u32 number;

	size_t offset;
	int page_count;
	unsigned long pages[FRAME_PAGE_COUNT];
};

#define MAX_FRAME_COUNT_HARD 1024
static u32 frame_count_max = CONFIG_PVR_PDUMP_INITIAL_MAX_FRAME_COUNT;
static u32 frame_count;

static struct pdumpfs_frame *frame_init;
static struct pdumpfs_frame *frame_stream;
static struct pdumpfs_frame *frame_current;

static struct pdumpfs_frame *frame_current_debugfs;
static int frame_current_open_count;
static int frame_stream_open_count;

static loff_t stream_start;
static loff_t stream_f_pos;
static loff_t stream_end;

static struct pdumpfs_frame *
frame_create(void)
{
	struct pdumpfs_frame *frame =
		kmalloc(sizeof(struct pdumpfs_frame), GFP_KERNEL);
	if (!frame)
		return NULL;

	memset(frame, 0, sizeof(struct pdumpfs_frame));

	return frame;
}

static void
frame_destroy(struct pdumpfs_frame *frame)
{
	int i;

	if (!frame)
		return;

	for (i = 0; i < frame->page_count; i++)
		free_page(frame->pages[i]);

	kfree(frame);
}

static void
frame_destroy_all(void)
{
	/* detach from possible clones */
	if (frame_current_debugfs &&
	    ((frame_current_debugfs == frame_init) ||
	     (frame_current_debugfs == frame_current) ||
	     frame_current_debugfs->next))
		frame_current_debugfs = NULL;

	frame_current = NULL;

	frame_destroy(frame_init);
	frame_init = NULL;

	frame_destroy(frame_current_debugfs);
	frame_current_debugfs = NULL;

	while (frame_stream) {
		struct pdumpfs_frame *frame = frame_stream;

		frame_stream = frame->next;

		frame_destroy(frame);
		frame_count--;
	}

	stream_start = 0;
	stream_f_pos = 0;
	stream_end = 0;
}

static void
frame_cull_first(void)
{
	struct pdumpfs_frame *frame = frame_stream;

	frame_stream = frame->next;
	frame->next = NULL;

	if (frame_stream_open_count)
		stream_start += frame->offset;
	else
		stream_end -= frame->offset;

	/*
	 * we cannot have frames vanish in the middle of reading
	 * them through debugfs
	 */
	if (frame != frame_current_debugfs)
		frame_destroy(frame);

	frame_count--;
}

static void
frame_cull(void)
{

	if (!frame_stream_open_count) {
		while (frame_count > frame_count_max)
			frame_cull_first();
	} else {
		while (((stream_start + frame_stream->offset) < stream_f_pos) ||
		       (frame_count > MAX_FRAME_COUNT_HARD))
			frame_cull_first();
	}
}

static int
frame_new(u32 pid, u32 number)
{
	struct pdumpfs_frame *frame = frame_create();

	if (!frame) {
		pr_err("%s: Failed to create frame.\n", __func__);
		return -ENOMEM;
	}

	frame->pid = pid;
	frame->number = number;

	if (!frame_init)
		frame_init = frame;
	else {
		if (frame_current != frame_init)
			frame_current->next = frame;
		else
			frame_stream = frame;
		frame_count++;
	}

	frame_current = frame;

	frame_cull();

	return 0;
}

void
pdumpfs_frame_set(u32 pid, u32 frame)
{
	mutex_lock(pdumpfs_mutex);

	frame_new(pid, frame);

	mutex_unlock(pdumpfs_mutex);
}

bool
pdumpfs_capture_enabled(void)
{
	bool ret;

	mutex_lock(pdumpfs_mutex);

	if ((pdumpfs_mode == PDUMPFS_MODE_FULL) &&
	    (frame_current != frame_init)) /* simulator bails otherwise */
		ret = true;
	else
		ret = false;

	mutex_unlock(pdumpfs_mutex);

	return ret;
}

bool
pdumpfs_flags_check(u32 flags)
{
	bool ret;

	if (flags & PDUMP_FLAGS_NEVER)
		return false;

	mutex_lock(pdumpfs_mutex);

	if (pdumpfs_mode == PDUMPFS_MODE_DISABLED)
		ret = false;
	else if ((pdumpfs_mode == PDUMPFS_MODE_FULL) &&
		 (frame_current != frame_init)) /* simulator bails otherwise */
		ret = true;
	else if (flags & PDUMP_FLAGS_CONTINUOUS)
		ret = true;
	else
		ret = false;

	mutex_unlock(pdumpfs_mutex);

	return ret;
}

static size_t
pdumpfs_frame_write_low(void *buffer, size_t size, bool from_user)
{
	struct pdumpfs_frame *frame = frame_current;
	size_t ret = 0;

	while (size) {
		size_t count = size;
		size_t offset = frame->offset & ~PAGE_MASK;
		unsigned long page;

		if (!offset) {
			if (frame->page_count >= FRAME_PAGE_COUNT) {
				pr_err("%s: Frame size overrun.\n", __func__);
				return -ENOMEM;
			}

			page = __get_free_page(GFP_KERNEL);
			if (!page) {
				pr_err("%s: failed to get free page.\n",
				       __func__);
				return -ENOMEM;
			}

			frame->pages[frame->page_count] = page;
			frame->page_count++;
		} else
			page =
				frame->pages[frame->page_count - 1];

		if (count > (PAGE_SIZE - offset))
			count = PAGE_SIZE - offset;

		if (from_user) {
			if (copy_from_user(((u8 *) page) + offset,
					   (void __user __force *) buffer,
					   count))
				return -EINVAL;
		} else
			memcpy(((u8 *) page) + offset, buffer, count);

		buffer += count;
		size -= count;
		ret += count;
		frame->offset += count;
	}
	return ret;
}

static size_t
pdumpfs_frame_write(void *buffer, size_t size, bool from_user)
{
	size_t ret;

	if ((frame_current->offset + size) > (PAGE_SIZE * FRAME_PAGE_COUNT)) {
		u32 pid = OSGetCurrentProcessIDKM();
		struct task_struct *task;

		pr_err("Frame overrun!!!\n");

		ret = frame_new(pid, -1);
		if (ret)
			return ret;

		rcu_read_lock();
		task = pid_task(find_vpid(pid), PIDTYPE_PID);

#define FRAME_OVERRUN_MESSAGE "-- Starting forced frame caused by "
		pdumpfs_frame_write_low(FRAME_OVERRUN_MESSAGE,
					sizeof(FRAME_OVERRUN_MESSAGE), false);
		pdumpfs_frame_write_low(task->comm, strlen(task->comm), false);
		pdumpfs_frame_write_low("\r\n", 2, false);

		pr_err("%s: Frame size overrun, caused by %d (%s)\n",
		       __func__, pid, task->comm);

		rcu_read_unlock();
	}

	return pdumpfs_frame_write_low(buffer, size, from_user);
}

enum PVRSRV_ERROR
pdumpfs_write_data(void *buffer, size_t size, bool from_user)
{
	mutex_lock(pdumpfs_mutex);

	size = pdumpfs_frame_write(buffer, size, from_user);
	if ((size > 0) && (frame_current != frame_init))
		stream_end += size;

	mutex_unlock(pdumpfs_mutex);

	if ((size >= 0) || (size == -ENOMEM))
		return PVRSRV_OK;
	else
		return PVRSRV_ERROR_GENERIC;
}

void
pdumpfs_write_string(char *string)
{
	size_t size;

	mutex_lock(pdumpfs_mutex);

	size = pdumpfs_frame_write(string, strlen(string), false);
	if ((size > 0) && (frame_current != frame_init))
		stream_end += size;

	mutex_unlock(pdumpfs_mutex);
}

/*
 * DebugFS entries.
 */
static const struct {
	char *name;
	enum pdumpfs_mode mode;
} pdumpfs_modes[] = {
	{"disabled",   PDUMPFS_MODE_DISABLED},
	{"standard",   PDUMPFS_MODE_STANDARD},
	{"full",       PDUMPFS_MODE_FULL},
	{NULL, PDUMPFS_MODE_DISABLED}
};

static ssize_t
pdumpfs_mode_read(struct file *filp, char __user *buf, size_t size,
		  loff_t *f_pos)
{
	char tmp[16];
	int i;

	tmp[0] = 0;

	mutex_lock(pdumpfs_mutex);

	for (i = 0; pdumpfs_modes[i].name; i++)
		if (pdumpfs_modes[i].mode == pdumpfs_mode)
			snprintf(tmp, sizeof(tmp), "%s\n",
				 pdumpfs_modes[i].name);

	mutex_unlock(pdumpfs_mutex);

	if (strlen(tmp) < *f_pos)
		return 0;

	if ((strlen(tmp) + 1) < (*f_pos + size))
		size = strlen(tmp) + 1 - *f_pos;

	if (copy_to_user(buf, tmp + *f_pos, size))
		return -EFAULT;

	*f_pos += size;
	return size;
}

static ssize_t
pdumpfs_mode_write(struct file *filp, const char __user *buf, size_t size,
		   loff_t *f_pos)
{
	static char tmp[16];
	int i;

	if (*f_pos > sizeof(tmp))
		return -EINVAL;

	if (size > (sizeof(tmp) - *f_pos))
		size = sizeof(tmp) - *f_pos;

	if (copy_from_user(tmp + *f_pos, buf, size))
		return -EFAULT;

	*f_pos += size;

	mutex_lock(pdumpfs_mutex);

	for (i = 0; pdumpfs_modes[i].name; i++)
		if (!strnicmp(tmp, pdumpfs_modes[i].name,
			      strlen(pdumpfs_modes[i].name))) {
			pdumpfs_mode = pdumpfs_modes[i].mode;
			mutex_unlock(pdumpfs_mutex);
			return size;
		}

	mutex_unlock(pdumpfs_mutex);
	return -EINVAL;
}

static const struct file_operations pdumpfs_mode_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = pdumpfs_mode_read,
	.write = pdumpfs_mode_write,
};

static ssize_t
pdumpfs_modes_possible_read(struct file *filp, char __user *buf, size_t size,
			    loff_t *f_pos)
{
	unsigned int i, skip = *f_pos, pos = 0;

	for (i = 0; pdumpfs_modes[i].name; i++) {
		if (i) { /* space */
			if (skip)
				skip--;
			else if (size > pos) {
				if (copy_to_user(buf + pos, " ", 1))
					return -EFAULT;
				pos++;
			}
		}

		if (size) {
			int len = strlen(pdumpfs_modes[i].name);

			if (skip >= len) {
				skip -= len;
			} else if (size > pos) {
				len = min(len - skip, size - pos);

				if (copy_to_user(buf + pos,
						 pdumpfs_modes[i].name + skip,
						 len))
					return -EFAULT;

				skip = 0;
				pos += len;
			}
		}
	}

	*f_pos += pos;
	return pos;
}

static const struct file_operations pdumpfs_modes_possible_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = pdumpfs_modes_possible_read,
};

static ssize_t
pdumpfs_frame_count_max_read(struct file *filp, char __user *buf, size_t size,
			     loff_t *f_pos)
{
	char tmp[16];

	tmp[0] = 0;

	mutex_lock(pdumpfs_mutex);
	snprintf(tmp, sizeof(tmp), "%d", frame_count_max);
	mutex_unlock(pdumpfs_mutex);

	if (strlen(tmp) < *f_pos)
		return 0;

	if ((strlen(tmp) + 1) < (*f_pos + size))
		size = strlen(tmp) + 1 - *f_pos;

	if (copy_to_user(buf, tmp + *f_pos, size))
		return -EFAULT;

	*f_pos += size;
	return size;
}

static ssize_t
pdumpfs_frame_count_max_write(struct file *filp, const char __user *buf,
			      size_t size, loff_t *f_pos)
{
	static char tmp[16];
	unsigned long result = 0;

	if (*f_pos > sizeof(tmp))
		return -EINVAL;

	if (size > (sizeof(tmp) - *f_pos))
		size = sizeof(tmp) - *f_pos;

	if (copy_from_user(tmp + *f_pos, buf, size))
		return -EFAULT;

	tmp[size] = 0;

	mutex_lock(pdumpfs_mutex);

	if (!strict_strtoul(tmp, 0, &result)) {
		if (result > 1024)
			result = 1024;
		if (!result)
			result = 1;
		frame_count_max = result;
	}

	mutex_unlock(pdumpfs_mutex);

	*f_pos += size;
	return size;
}

static const struct file_operations pdumpfs_frame_count_max_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = pdumpfs_frame_count_max_read,
	.write = pdumpfs_frame_count_max_write,
};

static ssize_t
pdumpfs_frame_read_single(struct pdumpfs_frame *frame, char __user *buf,
			  size_t size, loff_t f_pos)
{
	int page;
	size_t offset;

	if (f_pos >= frame->offset)
		return 0;

	if (size > (frame->offset - f_pos))
		size = frame->offset - f_pos;

	page = f_pos / PAGE_SIZE;
	offset = f_pos % PAGE_SIZE;

	if (size > (PAGE_SIZE - offset))
		size = PAGE_SIZE - offset;

	if (copy_to_user(buf, ((u8 *) frame->pages[page]) + offset, size))
		return -EFAULT;

	return size;
}

static loff_t
pdumpfs_llseek_helper(struct file *filp, loff_t offset, int whence, loff_t max)
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

static loff_t
pdumpfs_init_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t f_pos;

	mutex_lock(pdumpfs_mutex);

	f_pos = pdumpfs_llseek_helper(filp, offset, whence, frame_init->offset);

	mutex_unlock(pdumpfs_mutex);

	return f_pos;
}

static ssize_t
pdumpfs_init_read(struct file *filp, char __user *buf, size_t size,
		  loff_t *f_pos)
{
	mutex_lock(pdumpfs_mutex);

	size = pdumpfs_frame_read_single(frame_init,
					 buf, size, *f_pos);

	mutex_unlock(pdumpfs_mutex);

	if (size > 0)
		*f_pos += size;
	return size;
}

static const struct file_operations pdumpfs_init_fops = {
	.owner = THIS_MODULE,
	.llseek = pdumpfs_init_llseek,
	.read = pdumpfs_init_read,
};

/*
 * We need to make sure that our frame doesn't vanish while we are still
 * reading it: so reference this frame again.
 */
static int
pdumpfs_current_open(struct inode *inode, struct file *filp)
{
	mutex_lock(pdumpfs_mutex);

	if (!frame_current_open_count)
		frame_current_debugfs = frame_current;

	frame_current_open_count++;

	mutex_unlock(pdumpfs_mutex);
	return 0;
}

static int
pdumpfs_current_release(struct inode *inode, struct file *filp)
{
	mutex_lock(pdumpfs_mutex);

	frame_current_open_count--;

	if (!frame_current_open_count) {
		if ((frame_current_debugfs != frame_init) &&
		    (frame_current_debugfs != frame_current) &&
		    !frame_current_debugfs->next)
			frame_destroy(frame_current_debugfs);
		frame_current_debugfs = NULL;
	}

	mutex_unlock(pdumpfs_mutex);
	return 0;
}

static loff_t
pdumpfs_current_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t f_pos;

	mutex_lock(pdumpfs_mutex);

	f_pos = pdumpfs_llseek_helper(filp, offset, whence,
				      frame_current_debugfs->offset);

	mutex_unlock(pdumpfs_mutex);

	return f_pos;
}

static ssize_t
pdumpfs_current_read(struct file *filp, char __user *buf, size_t size,
		     loff_t *f_pos)
{
	mutex_lock(pdumpfs_mutex);

	if (frame_current_debugfs->offset)
		size = pdumpfs_frame_read_single(frame_current_debugfs,
						 buf, size, *f_pos);
	else
		size = 0;

	mutex_unlock(pdumpfs_mutex);

	if (size > 0)
		*f_pos += size;
	return size;
}

static const struct file_operations pdumpfs_current_fops = {
	.owner = THIS_MODULE,
	.llseek = pdumpfs_current_llseek,
	.read = pdumpfs_current_read,
	.open = pdumpfs_current_open,
	.release = pdumpfs_current_release,
};

/*
 * So we can track when we can alter stream offsets.
 */
static int
pdumpfs_stream_open(struct inode *inode, struct file *filp)
{
	int ret;

	mutex_lock(pdumpfs_mutex);

	if (frame_stream_open_count)
		ret = -EUSERS;
	else {
		frame_stream_open_count++;
		ret = 0;
	}

	mutex_unlock(pdumpfs_mutex);
	return ret;
}

static int
pdumpfs_stream_release(struct inode *inode, struct file *filp)
{
	mutex_lock(pdumpfs_mutex);

	frame_stream_open_count--;

	/* fix the damage done while it was open */
	if (!frame_stream_open_count) {
		stream_end -= stream_start;
		stream_start = 0;
		stream_f_pos = 0;
	}

	mutex_unlock(pdumpfs_mutex);
	return 0;
}

static ssize_t
pdumpfs_stream_buffer_clear(char __user *buf, size_t size)
{
	char tmp[80];

	memset(tmp, '.', sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = '\n';

	if (size >= sizeof(tmp)) {
		int i;

		for (i = 0; (i  + sizeof(tmp)) <= size; i += sizeof(tmp))
			if (copy_to_user(buf + i, tmp, sizeof(tmp)))
				return -EFAULT;
		return i;
	} else {
		if (copy_to_user(buf, tmp + sizeof(tmp) - size, size))
			return -EFAULT;
		return size;
	}
}

static ssize_t
pdumpfs_stream_buffer_fill(struct pdumpfs_frame *frame,
			   char __user *buf, size_t offset, size_t size)
{
	int page = offset / PAGE_SIZE;

	if (size > (frame->offset - offset))
		size = frame->offset - offset;

	offset %= PAGE_SIZE;

	if (size > (PAGE_SIZE - offset))
		size = PAGE_SIZE - offset;

	if (copy_to_user(buf, ((u8 *) frame->pages[page]) + offset, size))
		return -EFAULT;

	stream_f_pos += size;

	return size;
}

static loff_t
pdumpfs_stream_llseek(struct file *filp, loff_t offset, int whence)
{
	loff_t f_pos;

	mutex_lock(pdumpfs_mutex);

	switch (whence) {
	case SEEK_SET:
		if ((offset > stream_end) || (offset < stream_start))
			f_pos = -EINVAL;
		else
			f_pos = offset;
		break;
	case SEEK_CUR:
		if (((filp->f_pos + offset) > stream_end) ||
		    ((filp->f_pos + offset) < stream_start))
			f_pos = -EINVAL;
		else
			f_pos = filp->f_pos + offset;
		break;
	case SEEK_END:
		if ((offset > 0) ||
		    (offset < (stream_start - stream_end)))
			f_pos = -EINVAL;
		else
			f_pos = stream_end + offset;
		break;
	default:
		f_pos = -EINVAL;
		break;
	}

	if (f_pos >= 0) {
		filp->f_pos = f_pos;
		stream_f_pos = f_pos;
	}

	mutex_unlock(pdumpfs_mutex);

	return f_pos;
}

static ssize_t
pdumpfs_stream_read(struct file *filp, char __user *buf, size_t size,
		    loff_t *f_pos)
{
	size_t ret = 0;

	mutex_lock(pdumpfs_mutex);

	if ((stream_end <= 0) || (*f_pos >= stream_end))
		ret = 0;
	else if (*f_pos < stream_start) {
		if (size > (stream_start - *f_pos))
			size = stream_start - *f_pos;
		ret = pdumpfs_stream_buffer_clear(buf, size);
	} else {
		loff_t start = stream_start;
		struct pdumpfs_frame *frame = frame_stream;

		/* skip frames that are before our offset */
		while ((start + frame->offset) <= *f_pos) {
			start += frame->offset;
			frame = frame->next;
		}

		ret = pdumpfs_stream_buffer_fill(frame, buf, *f_pos - start,
						 size);
	}

	if (ret > 0)
		*f_pos += ret;
	mutex_unlock(pdumpfs_mutex);
	return ret;
}

static const struct file_operations pdumpfs_stream_fops = {
	.owner = THIS_MODULE,
	.llseek = pdumpfs_stream_llseek,
	.read = pdumpfs_stream_read,
	.open = pdumpfs_stream_open,
	.release = pdumpfs_stream_release,
};

static struct dentry *pdumpfs_dir;

static void
pdumpfs_file_create(const char *name, mode_t mode,
		    const struct file_operations *fops)
{
	struct dentry *tmp = NULL;

	tmp = debugfs_create_file(name, mode, pdumpfs_dir, NULL, fops);
	if (!tmp)
		pr_err("%s: failed to create pvr/%s file.\n", __func__, name);
}

static int
pdumpfs_fs_init(void)
{
	if (!pvr_debugfs_dir) {
		pr_err("%s: debugfs pvr/ directory does not exist.\n",
		       __func__);
		return -ENOENT;
	}

	pdumpfs_dir = debugfs_create_dir("pdump", pvr_debugfs_dir);
	if (!pdumpfs_dir) {
		pr_err("%s: failed to create top level directory.\n",
		       __func__);
		return -ENOENT;
	}

	pdumpfs_file_create("mode", S_IRUSR | S_IWUSR,
			    &pdumpfs_mode_fops);
	pdumpfs_file_create("modes_possible", S_IRUSR,
			    &pdumpfs_modes_possible_fops);

	pdumpfs_file_create("frame_count_max", S_IRUSR | S_IWUSR,
			    &pdumpfs_frame_count_max_fops);

	pdumpfs_file_create("init_frame", S_IRUSR,
			    &pdumpfs_init_fops);
	pdumpfs_file_create("current_frame", S_IRUSR,
			    &pdumpfs_current_fops);
	pdumpfs_file_create("stream_frames", S_IRUSR,
			    &pdumpfs_stream_fops);

	return 0;
}

static void
pdumpfs_fs_destroy(void)
{
	if (pdumpfs_dir)
		debugfs_remove_recursive(pdumpfs_dir);
}

int
pdumpfs_init(void)
{
	int ret;

	mutex_init(pdumpfs_mutex);

	ret = frame_new(0, 0);
	if (ret < 0)
		return ret;

	pdumpfs_fs_init();

	return 0;
}

void
pdumpfs_cleanup(void)
{
	pdumpfs_fs_destroy();

	frame_destroy_all();
}
