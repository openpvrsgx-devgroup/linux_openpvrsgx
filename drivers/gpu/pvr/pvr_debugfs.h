/*
 * Copyright (c) 2010-2011 Imre Deak <imre.deak@nokia.com>
 * Copyright (c) 2010-2011 Luc Verhaegen <libv@codethink.co.uk>
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

#ifndef _PVR_DEBUGFS_H_
#define _PVR_DEBUGFS_H_ 1

#ifndef CONFIG_DEBUG_FS
#error Error: debugfs header included but CONFIG_DEBUG_FS is not defined!
#endif

int pvr_debugfs_init(void);
void pvr_debugfs_cleanup(void);

void pvr_hwrec_dump(void);

#endif /* _PVR_DEBUGFS_H_ */
