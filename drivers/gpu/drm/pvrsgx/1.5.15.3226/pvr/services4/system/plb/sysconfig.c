/* -*- syscommon-c -*-
 *-----------------------------------------------------------------------------
 * Filename: syscommon.c
 * $Revision: 1.4 $
 *-----------------------------------------------------------------------------
 * Copyright (c) 2002-2010, Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *-----------------------------------------------------------------------------
 * Description: platform detection, and sharing of correct platform interface.
 *
 *-----------------------------------------------------------------------------
 */

#include "sysconfig.h"
#include "sysplb.h"

/* --------------------------------------------------------------------------*/
/**
* @Synopsis Poulsbo platform interface.
*/
/* --------------------------------------------------------------------------*/
SYS_PLATFORM_INTERFACE gpsSysPlatformInterfacePlb = {
        VS_PRODUCT_NAME_PLB,
        SYS_SGX_DEV_DEVICE_ID_PLB,
	SGX_REGS_OFFSET_PLB,
	MSVDX_REGS_OFFSET_PLB,
	SYS_SGX_CLOCK_SPEED_PLB,
	SYS_SGX_ACTIVE_POWER_LATENCY_MS_PLB
};

