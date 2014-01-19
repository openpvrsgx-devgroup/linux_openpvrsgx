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

#include <linux/version.h>

#include <linux/spinlock.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include "img_defs.h"
#include "pvr_debug.h"
#include "mutils.h"

#if defined(SUPPORT_LINUX_X86_PAT)
#define	PAT_LINUX_X86_WC	1

#define	PAT_X86_ENTRY_BITS	8

#define	PAT_X86_BIT_PWT		1U
#define	PAT_X86_BIT_PCD		2U
#define	PAT_X86_BIT_PAT		4U
#define	PAT_X86_BIT_MASK	(PAT_X86_BIT_PAT | PAT_X86_BIT_PCD | PAT_X86_BIT_PWT)

static IMG_BOOL g_write_combining_available = IMG_FALSE;

#define	PROT_TO_PAT_INDEX(v, B) ((v & _PAGE_ ## B) ? PAT_X86_BIT_ ## B : 0)

static inline IMG_UINT
pvr_pat_index(pgprotval_t prot_val)
{
	IMG_UINT ret = 0;
	pgprotval_t val = prot_val & _PAGE_CACHE_MASK;

	ret |= PROT_TO_PAT_INDEX(val, PAT);
	ret |= PROT_TO_PAT_INDEX(val, PCD);
	ret |= PROT_TO_PAT_INDEX(val, PWT);

	return ret;
}

static inline IMG_UINT
pvr_pat_entry(u64 pat, IMG_UINT index)
{
	return (IMG_UINT)(pat >> (index * PAT_X86_ENTRY_BITS)) & PAT_X86_BIT_MASK;
}

static IMG_VOID
PVRLinuxX86PATProbe(IMG_VOID)
{

	if (cpu_has_pat)
	{
		u64 pat;
		IMG_UINT pat_index;
		IMG_UINT pat_entry;

		PVR_TRACE(("%s: PAT available", __FUNCTION__));

		rdmsrl(MSR_IA32_CR_PAT, pat);
		PVR_TRACE(("%s: Top 32 bits of PAT: 0x%.8x", __FUNCTION__, (IMG_UINT)(pat >> 32)));
		PVR_TRACE(("%s: Bottom 32 bits of PAT: 0x%.8x", __FUNCTION__, (IMG_UINT)(pat)));

		pat_index = pvr_pat_index(_PAGE_CACHE_WC);
		PVR_TRACE(("%s: PAT index for write combining: %u", __FUNCTION__, pat_index));

		pat_entry = pvr_pat_entry(pat, pat_index);
		PVR_TRACE(("%s: PAT entry for write combining: 0x%.2x (should be 0x%.2x)", __FUNCTION__, pat_entry, PAT_LINUX_X86_WC));

#if defined(SUPPORT_LINUX_X86_WRITECOMBINE)
		g_write_combining_available = (IMG_BOOL)(pat_entry == PAT_LINUX_X86_WC);
#endif
	}
#if defined(DEBUG)
#if defined(SUPPORT_LINUX_X86_WRITECOMBINE)
	if (g_write_combining_available)
	{
		PVR_TRACE(("%s: Write combining available via PAT", __FUNCTION__));
	}
	else
	{
		PVR_TRACE(("%s: Write combining not available", __FUNCTION__));
	}
#else
	PVR_TRACE(("%s: Write combining disabled in driver build", __FUNCTION__));
#endif
#endif
}

pgprot_t
pvr_pgprot_writecombine(pgprot_t prot)
{


    return (g_write_combining_available) ?
		__pgprot((pgprot_val(prot) & ~_PAGE_CACHE_MASK) | _PAGE_CACHE_WC) : pgprot_noncached(prot);
}
#endif

IMG_VOID
PVRLinuxMUtilsInit(IMG_VOID)
{
#if defined(SUPPORT_LINUX_X86_PAT)
	PVRLinuxX86PATProbe();
#endif
}

