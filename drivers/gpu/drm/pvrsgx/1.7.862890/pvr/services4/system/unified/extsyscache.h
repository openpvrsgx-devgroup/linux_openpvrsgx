#include "sgxdefs.h"

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
/*
	byte offsets to ext cache control registers from the
	base of the external register bank
	- TO BE SPECIFIED IN SYSTEM PORT!
*/
/* global invalidate */
/* This address must be the physical address of the cache controller rounded
   down to the nearest page*/
#define SYS_EXT_SYS_CACHE_GBL_INV_REG_OFFSET	0x80000000


/* per address invalidate (data written in the address) */
/* This is the offset within the page to the cache controller which we want
   to write */
#define SYS_EXT_SYS_CACHE_ADDR_INV_REG_OFFSET	0x0

/*
	FIXME: these two defines should really live in sgxconfig.h as a heap define
	not possible right now due to .h include issues
*/

/* This address must not live within the same virtal address as
   any of the heaps, but must be within a shared heap */

#if SGX_FEATURE_ADDRESS_SPACE_SIZE == 32
#define SGX_EXT_SYSTEM_CACHE_REGS_DEVVADDR_BASE			(0xF8FFE000)
#endif

#if SGX_FEATURE_ADDRESS_SPACE_SIZE == 28
#define SGX_EXT_SYSTEM_CACHE_REGS_DEVVADDR_BASE			(0x0F3FE000)
#endif

/* assume 4k is enough range to cover all control registers */
#define SGX_EXT_SYSTEM_CACHE_REGS_SIZE					 0x00001000

#define INVALIDATE_EXT_SYSTEM_CACHE_INLINE(RegA)									\
	mov		RegA, HSH(SGX_EXT_SYSTEM_CACHE_REGS_DEVVADDR_BASE);						\
	stad.bpcache	[RegA, HSH(SYS_EXT_SYS_CACHE_ADDR_INV_REG_OFFSET>>2)], HSH(0);	\
	idf		drc0, st;																\
	wdf		drc0;
#endif /* SUPPORT_EXTERNAL_SYSTEM_CACHE */
