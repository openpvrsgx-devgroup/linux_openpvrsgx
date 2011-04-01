/*
 * Copyright (C) 2011 Nokia Corporation
 * Author: Imre Deak <imre.deak@nokia.com>
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

#include <linux/vmalloc.h>

#include "img_types.h"
#include "resman.h"
#include "handle.h"
#include "pvr_trace_cmd.h"
#include "perproc.h"
#include "pvr_bridge_km.h"

/* Need to be log2 size. */
#define PVR_TBUF_SIZE	(1 << (CONFIG_PVR_TRACE_CMD_BUF_SHIFT + PAGE_SHIFT))

#define PVR_TRCMD_INDENT	3

static struct pvr_trcmd_buf {
	int		read_idx;
	int		write_idx;
	char		data[PVR_TBUF_SIZE];
} tbuf;

DEFINE_MUTEX(pvr_trcmd_mutex);	/* protects tbuf */

struct tbuf_frame {
	unsigned short size;
	unsigned short type;
	unsigned long pid;
	unsigned long long time;
	char pname[16];
};

struct trcmd_desc {
	const char *name;
	size_t (*print)(char *dst, size_t dst_size, const void *tbuf);
};

static size_t prn_syn(const char *name, char *dst, size_t dst_size,
		      const struct pvr_trcmd_syn *ts)
{
	size_t len;

	if (!ts->addr)
		return 0;

	len =  scnprintf(dst, dst_size, "%*s%s", PVR_TRCMD_INDENT, "", name);
	len += scnprintf(&dst[len], dst_size - len, " addr:%08lx", ts->addr);
	len += scnprintf(&dst[len], dst_size - len,
			 " rop/c:%8lu/%8lu wop/c:%8lu/%8lu\n",
			 ts->rd_pend, ts->rd_comp, ts->wr_pend, ts->wr_comp);

	return len;
}

static size_t trcmd_prn_syn(char *dst, size_t dst_size, const void *tbuf)
{
	const struct pvr_trcmd_syn *ts = tbuf;

	return prn_syn("syn     ", dst, dst_size, ts);
}

static size_t trcmd_prn_sgxkick(char *dst, size_t dst_size, const void *tbuf)
{
	const struct pvr_trcmd_sgxkick *d = tbuf;
	size_t len;
	int i;

	len  = prn_syn("tatq_syn", dst, dst_size, &d->tatq_syn);
	len += prn_syn("3dtq_syn", &dst[len], dst_size - len, &d->_3dtq_syn);
	for (i = 0; i < SGX_MAX_SRC_SYNCS; i++) {
		char sname[10];

		snprintf(sname, sizeof(sname), "src_syn%d", i);
		len += prn_syn(sname, &dst[len], dst_size - len,
			       &d->src_syn[i]);
	}
	len += prn_syn("dst_syn ", &dst[len], dst_size - len, &d->dst_syn);
	len += prn_syn("ta3d_syn", &dst[len], dst_size - len, &d->ta3d_syn);
	len += scnprintf(&dst[len], dst_size - len, "%*sctx  %08lx\n",
			 PVR_TRCMD_INDENT, "", d->ctx);

	return len;
}

static size_t trcmd_prn_sgxtfer(char *dst, size_t dst_size, const void *tbuf)
{
	const struct pvr_trcmd_sgxtransfer *d = tbuf;
	size_t len;

	len  = prn_syn("ta_syn  ", dst, dst_size, &d->ta_syn);
	len += prn_syn("3d_syn  ", &dst[len], dst_size - len, &d->_3d_syn);
	len += prn_syn("src_syn ", &dst[len], dst_size - len, &d->src_syn);
	len += prn_syn("dst_syn ", &dst[len], dst_size - len, &d->dst_syn);
	len += scnprintf(&dst[len], dst_size - len, "%*sctx  %08lx\n",
			 PVR_TRCMD_INDENT, "", d->ctx);

	return len;
}

static struct trcmd_desc trcmd_desc_table[] = {
	[PVR_TRCMD_SGX_FIRSTKICK]    = { "sgx_first_kick", trcmd_prn_sgxkick },
	[PVR_TRCMD_SGX_KICK]	     = { "sgx_kick", trcmd_prn_sgxkick },
	[PVR_TRCMD_TFER_KICK]	     = { "sgx_tfer_kick", trcmd_prn_sgxtfer },
	[PVR_TRCMD_SGX_QBLT_FLPREQ]  = { "sgx_qblt_flip", NULL },
	[PVR_TRCMD_SGX_QBLT_UPDREQ]  = { "sgx_qblt_update", NULL },
	[PVR_TRCMD_SGX_QBLT_SYNCHK]  = { "sgx_qblt_synchk", trcmd_prn_syn },
	[PVR_TRCMD_SGX_QBLT_SYNREQ]  = { "sgx_qblt_synreq", trcmd_prn_syn },
	[PVR_TRCMD_SGX_QBLT_SYNCOMP] = { "sgx_qblt_syn_comp", trcmd_prn_syn },
	[PVR_TRCMD_SGX_QBLT_FLPCOMP] = { "sgx_qblt_flip_comp", NULL },
	[PVR_TRCMD_SGX_QBLT_UPDCOMP] = { "sgx_qblt_update_comp", NULL }
};

/* Modular add */
static inline int tbuf_idx_add(int val, int delta)
{
	val += delta;
	val &= PVR_TBUF_SIZE - 1;

	return val;
}

static size_t prn_frame(const struct tbuf_frame *f, char *dst, size_t dst_size)
{
	const struct trcmd_desc *desc;
	unsigned long long sec;
	unsigned long usec_frac;
	size_t len;

	desc = &trcmd_desc_table[f->type];

	sec = f->time;
	usec_frac = do_div(sec, 1000000000) / 1000;

	len = scnprintf(dst, dst_size, "[%5llu.%06lu] %s[%ld]: %s\n",
			sec, usec_frac, f->pname, f->pid, desc->name);
	if (desc->print)
		len += desc->print(&dst[len], dst_size - len, (void *)(f + 1));

	return len;
}

int pvr_trcmd_create_snapshot(u8 **snapshot_ret, size_t *snapshot_size)
{
	u8 *snapshot;
	int read_idx;
	size_t size;
	size_t tail_size;

	read_idx = tbuf.read_idx;
	size = tbuf_idx_add(tbuf.write_idx, -read_idx);
	snapshot = vmalloc(size);
	if (!snapshot)
		return -ENOMEM;

	tail_size = min_t(size_t, size, PVR_TBUF_SIZE - read_idx);
	memcpy(snapshot, &tbuf.data[read_idx], tail_size);
	memcpy(&snapshot[tail_size], tbuf.data, size - tail_size);

	*snapshot_ret = snapshot;
	*snapshot_size = size;

	return 0;
}

void pvr_trcmd_destroy_snapshot(void *snapshot)
{
	vfree(snapshot);
}

size_t pvr_trcmd_print(char *dst, size_t dst_size, const u8 *snapshot,
		       size_t snapshot_size, loff_t *snapshot_ofs)
{
	size_t dst_len;

	if (*snapshot_ofs >= snapshot_size)
		return 0;
	dst_len = 0;

	snapshot_size -= *snapshot_ofs;

	while (snapshot_size) {
		const struct tbuf_frame *f;
		size_t this_len;

		if (WARN_ON_ONCE(snapshot_size < 4))
			break;

		f = (struct tbuf_frame *)&snapshot[*snapshot_ofs];
		if (WARN_ON_ONCE(!f->size || f->size > snapshot_size ||
				 f->type >= ARRAY_SIZE(trcmd_desc_table)))
			break;

		if (f->type != PVR_TRCMD_PAD)
			this_len = prn_frame(f, &dst[dst_len],
					     dst_size - dst_len);
		else
			this_len = 0;

		if (dst_len + this_len + 1 == dst_size) {
			/* drop the last printed frame */
			dst[dst_len] = '\0';

			break;
		}

		*snapshot_ofs += f->size;
		dst_len += this_len;
		snapshot_size -= f->size;
	}

	return dst_len;
}

static void *tbuf_get_space(size_t size)
{
	void *ret;
	int buf_idx;

	while (1) {
		if (tbuf_idx_add(tbuf.read_idx - 1, -tbuf.write_idx) < size) {
			/*
			 * Trace buffer overflow, discard the frame that will
			 * be overwritten by the next write.
			 */
			struct tbuf_frame *f =
				(void *)&tbuf.data[tbuf.read_idx];

			buf_idx = tbuf.read_idx;
			tbuf.read_idx = tbuf_idx_add(tbuf.read_idx,
							  f->size);
		} else if (PVR_TBUF_SIZE - tbuf.write_idx < size) {
			struct tbuf_frame *f =
				(void *)&tbuf.data[tbuf.write_idx];
			/*
			 * Not enough space until the end of trace buffer,
			 * rewind to the beginning. Frames are sizeof(long)
			 * aligned, thus we are guaranteed to have space for
			 * the following two fields.
			 */
			f->size = PVR_TBUF_SIZE - tbuf.write_idx;
			f->type = PVR_TRCMD_PAD;
			tbuf.write_idx = 0;
		} else {
			break;
		}
	}
	ret = &tbuf.data[tbuf.write_idx];
	tbuf.write_idx = tbuf_idx_add(tbuf.write_idx, size);

	return ret;
}

void *pvr_trcmd_alloc(unsigned type, int pid, const char *pname, size_t size)
{
	struct tbuf_frame *f;
	size_t total_size;

	size = ALIGN(size, __alignof__(*f));
	total_size = sizeof(*f) + size;
	f = tbuf_get_space(total_size);
	f->size = total_size;
	f->type = type;
	f->pid = pid;
	f->time = cpu_clock(smp_processor_id());
	strlcpy(f->pname, pname, sizeof(f->pname));

	return f + 1;
}

void pvr_trcmd_set_syn(struct pvr_trcmd_syn *ts,
		     const struct PVRSRV_KERNEL_SYNC_INFO *si)
{
	struct PVRSRV_SYNC_DATA *sd = si->psSyncData;

	ts->rd_pend = sd->ui32ReadOpsPending;
	ts->rd_comp = sd->ui32ReadOpsComplete;
	ts->wr_pend = sd->ui32WriteOpsPending;
	ts->wr_comp = sd->ui32WriteOpsComplete;
	ts->addr    = si->sWriteOpsCompleteDevVAddr.uiAddr - 4;
}
