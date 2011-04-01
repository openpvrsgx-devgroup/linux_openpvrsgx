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

#ifndef __PVR_TRACE_CMD_H__
#define __PVR_TRACE_CMD_H__

#include <linux/mutex.h>

#include "servicesint.h"
#include "sgxapi_km.h"

enum pvr_trcmd_type {
	PVR_TRCMD_PAD,
	PVR_TRCMD_SGX_FIRSTKICK,
	PVR_TRCMD_SGX_KICK,
	PVR_TRCMD_TFER_KICK,
	PVR_TRCMD_SGX_QBLT_SYNREQ,
	PVR_TRCMD_SGX_QBLT_SYNCHK,
	PVR_TRCMD_SGX_QBLT_FLPREQ,
	PVR_TRCMD_SGX_QBLT_UPDREQ,
	PVR_TRCMD_SGX_QBLT_SYNCOMP,
	PVR_TRCMD_SGX_QBLT_FLPCOMP,
	PVR_TRCMD_SGX_QBLT_UPDCOMP,
};

struct pvr_trcmd_buf;

struct pvr_trcmd_syn {
	unsigned long	rd_pend;
	unsigned long	rd_comp;
	unsigned long	wr_pend;
	unsigned long	wr_comp;
	unsigned long	addr;
};

struct pvr_trcmd_sgxkick {
	struct pvr_trcmd_syn	tatq_syn;
	struct pvr_trcmd_syn	_3dtq_syn;
	struct pvr_trcmd_syn	src_syn[SGX_MAX_SRC_SYNCS];
	struct pvr_trcmd_syn	dst_syn;
	struct pvr_trcmd_syn	ta3d_syn;
	unsigned long		ctx;
};

struct pvr_trcmd_sgxtransfer {
	struct pvr_trcmd_syn	ta_syn;
	struct pvr_trcmd_syn	_3d_syn;
	struct pvr_trcmd_syn	src_syn;
	struct pvr_trcmd_syn	dst_syn;
	unsigned long		ctx;
};

#ifdef CONFIG_PVR_TRACE_CMD

void *pvr_trcmd_alloc(unsigned type, int pid, const char *pname, size_t size);

extern struct mutex pvr_trcmd_mutex;

static inline void pvr_trcmd_lock(void)
{
	mutex_lock(&pvr_trcmd_mutex);
}

static inline void pvr_trcmd_unlock(void)
{
	mutex_unlock(&pvr_trcmd_mutex);
}

int pvr_trcmd_create_snapshot(u8 **snapshot_ret, size_t *snapshot_size);
void pvr_trcmd_destroy_snapshot(void *snapshot);

size_t pvr_trcmd_print(char *dst, size_t dst_size, const u8 *snapshot,
		       size_t snapshot_size, loff_t *snapshot_ofs);

void pvr_trcmd_set_syn(struct pvr_trcmd_syn *ts,
		       const struct PVRSRV_KERNEL_SYNC_INFO *si);

static inline void pvr_trcmd_set_data(unsigned long *a, unsigned long val)
{
	*a = val;
}

static inline void pvr_trcmd_clear_syn(struct pvr_trcmd_syn *ts)
{
	memset(ts, 0, sizeof(*ts));
}

#else

static inline void *
pvr_trcmd_alloc(unsigned type, int pid, const char *pname, size_t size)
{
	return NULL;
}

static inline void pvr_trcmd_lock(void)
{
}

static inline void pvr_trcmd_unlock(void)
{
}

static inline int
pvr_trcmd_create_snapshot(u8 **snapshot_ret, size_t *snapshot_size)
{
	return 0;
}

static inline void pvr_trcmd_destroy_snapshot(void *snapshot)
{
}

static inline size_t
pvr_trcmd_print(char *dst, size_t dst_size, const u8 *snapshot,
		size_t snapshot_size, loff_t *snapshot_ofs)
{
	return 0;
}

static inline void
pvr_trcmd_set_syn(struct pvr_trcmd_syn *ts,
		  const struct PVRSRV_KERNEL_SYNC_INFO *si)
{
}

static inline void pvr_trcmd_set_data(unsigned long *a, unsigned long val)
{
}

static inline void pvr_trcmd_clear_syn(struct pvr_trcmd_syn *ts)
{
}

#endif		/* CONFIG_PVR_SYNC_CNT */

#endif
