/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
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

#if !defined(__SYSINFO_H__)
#define __SYSINFO_H__

#define MAX_HW_TIME_US				500000
#define WAIT_TRY_COUNT				10000

enum SYS_DEVICE_TYPE {
	SYS_DEVICE_SGX				= 0,
	SYS_DEVICE_FORCE_I16			= 0x7fff
};

#define SYS_DEVICE_COUNT			3

#define PRM_REG32(offset)			(offset)
#define CM_REG32(offset)			(offset)

#define CM_FCLKEN_SGX				CM_REG32(0xB00)
#define	CM_FCLKEN_SGX_EN_3D			0x00000002

#define CM_ICLKEN_SGX				CM_REG32(0xB10)
#define	CM_ICLKEN_SGX_EN_SGX			0x00000001

#define CM_IDLEST_SGX				CM_REG32(0xB20)
#define	CM_IDLEST_SGX_ST_SGX			0x00000001

#define CM_CLKSEL_SGX				CM_REG32(0xB40)
#define	CM_CLKSEL_SGX_MASK			0x0000000f
#define	CM_CLKSEL_SGX_L3DIV3			0x00000000
#define	CM_CLKSEL_SGX_L3DIV4			0x00000001
#define	CM_CLKSEL_SGX_L3DIV6			0x00000002
#define	CM_CLKSEL_SGX_96M			0x00000003

#define CM_SLEEPDEP_SGX				CM_REG32(0xB44)
#define CM_CLKSTCTRL_SGX			CM_REG32(0xB48)
#define CM_CLKSTCTRL_SGX_AUTOSTATE		0x00008001

#define CM_CLKSTST_SGX				CM_REG32(0xB4C)
#define CM_CLKSTST_SGX_STATUS_VALID		0x00000001

#define RM_RSTST_SGX				PRM_REG32(0xB58)
#define RM_RSTST_SGX_RST_MASK			0x0000000F
#define RM_RSTST_SGX_COREDOMAINWKUP_RST		0x00000008
#define RM_RSTST_SGX_DOMAINWKUP_RST		0x00000004
#define RM_RSTST_SGX_GLOBALWARM_RST		0x00000002
#define RM_RSTST_SGX_GLOBALCOLD_RST		0x00000001

#define PM_WKDEP_SGXi				PRM_REG32(0xBC8)
#define PM_WKDEP_SGX_EN_WAKEUP			0x00000010
#define PM_WKDEP_SGX_EN_MPU			0x00000002
#define PM_WKDEP_SGX_EN_CORE			0x00000001

#define PM_PWSTCTRL_SGX				PRM_REG32(0xBE0)
#define	PM_PWSTCTRL_SGX_POWERSTATE_MASK		0x00000003
#define	PM_PWSTCTRL_SGX_OFF			0x00000000
#define	PM_PWSTCTRL_SGX_RETENTION		0x00000001
#define	PM_PWSTCTRL_SGX_ON			0x00000003

#define PM_PWSTST_SGX				PRM_REG32(0xBE4)
#define	PM_PWSTST_SGX_INTRANSITION		0x00100000
#define	PM_PWSTST_SGX_CLKACTIVITY		0x00080000
#define	PM_PWSTST_SGX_POWERSTATE_MASK		0x00000003
#define	PM_PWSTST_SGX_OFF			0x00000003
#define	PM_PWSTST_SGX_RETENTION			0x00000001
#define	PM_PWSTST_SGX_ON			0x00000000

#define PM_PREPWSTST_SGX			PRM_REG32(0xBE8)

#endif
