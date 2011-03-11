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

static enum pdumpfs_mode pdumpfs_mode = PDUMPFS_MODE_DISABLED;
static u32 pdumpfs_frame_number;

void
pdumpfs_frame_set(u32 frame)
{
	mutex_lock(pdumpfs_mutex);

	pdumpfs_frame_number = frame;

	mutex_unlock(pdumpfs_mutex);
}

bool
pdumpfs_capture_enabled(void)
{
	bool ret;

	mutex_lock(pdumpfs_mutex);

	if (pdumpfs_mode == PDUMPFS_MODE_FULL)
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

	if (pdumpfs_mode == PDUMPFS_MODE_FULL)
		ret = true;
	else if ((pdumpfs_mode == PDUMPFS_MODE_STANDARD) &&
		 (flags & PDUMP_FLAGS_CONTINUOUS))
		ret = true;
	else
		ret = false;

	mutex_unlock(pdumpfs_mutex);

	return ret;
}

enum PVRSRV_ERROR
pdumpfs_write_data(void *buffer, size_t size, bool from_user)
{
	mutex_lock(pdumpfs_mutex);

	mutex_unlock(pdumpfs_mutex);

	return PVRSRV_OK;
}

void
pdumpfs_write_string(char *string)
{
	mutex_lock(pdumpfs_mutex);

	mutex_unlock(pdumpfs_mutex);
}

int
pdumpfs_init(void)
{
	mutex_init(pdumpfs_mutex);

	return 0;
}

void
pdumpfs_cleanup(void)
{

}
