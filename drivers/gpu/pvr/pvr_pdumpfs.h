/*
 * Copyright (c) 2010 by Luc Verhaegen <libv@codethink.co.uk>
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

#ifndef _PVR_PDUMPFS_H_
#define _PVR_PDUMPFS_H_

void pdumpfs_frame_set(u32 pid, u32 frame);
bool pdumpfs_capture_enabled(void);
bool pdumpfs_flags_check(u32 flags);
enum PVRSRV_ERROR pdumpfs_write_data(void *buffer, size_t size, bool from_user);
void pdumpfs_write_string(char *string);

int pdumpfs_init(void);
void pdumpfs_cleanup(void);

#endif /* _PVR_PDUMPFS_H_ */
