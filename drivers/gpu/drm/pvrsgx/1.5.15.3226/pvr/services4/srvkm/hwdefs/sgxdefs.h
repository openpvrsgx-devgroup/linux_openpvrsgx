/**********************************************************************
 Copyright (c) Imagination Technologies Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ******************************************************************************/

#ifndef _SGXDEFS_H_
#define	_SGXDEFS_H_

#include "sgxerrata.h"
#include "sgxfeaturedefs.h"

#if defined(SGX520)
#include "sgx520defs.h"
#else
#if defined(SGX530)
#include "sgx530defs.h"
#else
#if defined(SGX535)
#include "sgx535defs.h"
#else
#if defined(SGX535_V1_1)
#include "sgx535defs.h"
#else
#if defined(SGX540)
#include "sgx540defs.h"
#else
#if defined(SGX541)
#include "sgx541defs.h"
#else
#if defined(SGX543)
#include "sgx543defs.h"
#else
#if defined(SGX545)
#include "sgx545defs.h"
#else
#if defined(SGX531)
#include "sgx531defs.h"
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#if defined(SGX_FEATURE_MP)
#if defined(SGX541)
#if SGX_CORE_REV == 100
#include "sgx541_100mpdefs.h"
#else
#include "sgx541mpdefs.h"
#endif
#else
#include "sgxmpdefs.h"
#endif
#endif

#endif

