/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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

#ifndef _MNEMEDEFS_KM_H_
#define _MNEMEDEFS_KM_H_

#define MNE_CR_CTRL                         0x0D00
#define MNE_CR_CTRL_BYP_CC_N_MASK           0x00010000U
#define MNE_CR_CTRL_BYP_CC_N_SHIFT          16
#define MNE_CR_CTRL_BYP_CC_N_SIGNED         0
#define MNE_CR_CTRL_BYP_CC_MASK             0x00008000U
#define MNE_CR_CTRL_BYP_CC_SHIFT            15
#define MNE_CR_CTRL_BYP_CC_SIGNED           0
#define MNE_CR_CTRL_USE_INVAL_REQ_MASK      0x00007800U
#define MNE_CR_CTRL_USE_INVAL_REQ_SHIFT     11
#define MNE_CR_CTRL_USE_INVAL_REQ_SIGNED    0
#define MNE_CR_CTRL_BYPASS_ALL_MASK         0x00000400U
#define MNE_CR_CTRL_BYPASS_ALL_SHIFT        10
#define MNE_CR_CTRL_BYPASS_ALL_SIGNED       0
#define MNE_CR_CTRL_BYPASS_MASK             0x000003E0U
#define MNE_CR_CTRL_BYPASS_SHIFT            5
#define MNE_CR_CTRL_BYPASS_SIGNED           0
#define MNE_CR_CTRL_PAUSE_MASK              0x00000010U
#define MNE_CR_CTRL_PAUSE_SHIFT             4
#define MNE_CR_CTRL_PAUSE_SIGNED            0
#define MNE_CR_USE_INVAL                    0x0D04
#define MNE_CR_USE_INVAL_ADDR_MASK          0xFFFFFFFFU
#define MNE_CR_USE_INVAL_ADDR_SHIFT         0
#define MNE_CR_USE_INVAL_ADDR_SIGNED        0
#define MNE_CR_STAT                         0x0D08
#define MNE_CR_STAT_PAUSED_MASK             0x00000400U
#define MNE_CR_STAT_PAUSED_SHIFT            10
#define MNE_CR_STAT_PAUSED_SIGNED           0
#define MNE_CR_STAT_READS_MASK              0x000003FFU
#define MNE_CR_STAT_READS_SHIFT             0
#define MNE_CR_STAT_READS_SIGNED            0
#define MNE_CR_STAT_STATS                   0x0D0C
#define MNE_CR_STAT_STATS_RST_MASK          0x000FFFF0U
#define MNE_CR_STAT_STATS_RST_SHIFT         4
#define MNE_CR_STAT_STATS_RST_SIGNED        0
#define MNE_CR_STAT_STATS_SEL_MASK          0x0000000FU
#define MNE_CR_STAT_STATS_SEL_SHIFT         0
#define MNE_CR_STAT_STATS_SEL_SIGNED        0
#define MNE_CR_STAT_STATS_OUT               0x0D10
#define MNE_CR_STAT_STATS_OUT_VALUE_MASK    0xFFFFFFFFU
#define MNE_CR_STAT_STATS_OUT_VALUE_SHIFT   0
#define MNE_CR_STAT_STATS_OUT_VALUE_SIGNED  0
#define MNE_CR_EVENT_STATUS                 0x0D14
#define MNE_CR_EVENT_STATUS_INVAL_MASK      0x00000001U
#define MNE_CR_EVENT_STATUS_INVAL_SHIFT     0
#define MNE_CR_EVENT_STATUS_INVAL_SIGNED    0
#define MNE_CR_EVENT_CLEAR                  0x0D18
#define MNE_CR_EVENT_CLEAR_INVAL_MASK       0x00000001U
#define MNE_CR_EVENT_CLEAR_INVAL_SHIFT      0
#define MNE_CR_EVENT_CLEAR_INVAL_SIGNED     0
#define MNE_CR_CTRL_INVAL                   0x0D20
#define MNE_CR_CTRL_INVAL_PREQ_PDS_MASK     0x00000008U
#define MNE_CR_CTRL_INVAL_PREQ_PDS_SHIFT    3
#define MNE_CR_CTRL_INVAL_PREQ_PDS_SIGNED   0
#define MNE_CR_CTRL_INVAL_PREQ_USEC_MASK    0x00000004U
#define MNE_CR_CTRL_INVAL_PREQ_USEC_SHIFT   2
#define MNE_CR_CTRL_INVAL_PREQ_USEC_SIGNED  0
#define MNE_CR_CTRL_INVAL_PREQ_CACHE_MASK   0x00000002U
#define MNE_CR_CTRL_INVAL_PREQ_CACHE_SHIFT  1
#define MNE_CR_CTRL_INVAL_PREQ_CACHE_SIGNED 0
#define MNE_CR_CTRL_INVAL_ALL_MASK          0x00000001U
#define MNE_CR_CTRL_INVAL_ALL_SHIFT         0
#define MNE_CR_CTRL_INVAL_ALL_SIGNED        0

#endif 

