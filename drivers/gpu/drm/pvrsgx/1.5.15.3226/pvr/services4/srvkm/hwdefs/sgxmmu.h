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

#if !defined(__SGXMMU_KM_H__)
#define __SGXMMU_KM_H__

#define SGX_MMU_PAGE_SHIFT				(12)
#define SGX_MMU_PAGE_SIZE				(1UL<<SGX_MMU_PAGE_SHIFT)
#define SGX_MMU_PAGE_MASK				(SGX_MMU_PAGE_SIZE - 1UL)

#define SGX_MMU_PD_SHIFT				(10)
#define SGX_MMU_PD_SIZE					(1UL<<SGX_MMU_PD_SHIFT)
#define SGX_MMU_PD_MASK					(0xFFC00000UL)

#if defined(SGX_FEATURE_36BIT_MMU)
	#define SGX_MMU_PDE_ADDR_MASK			(0xFFFFFF00UL)
	#define SGX_MMU_PDE_ADDR_ALIGNSHIFT		(4)
#else
	#define SGX_MMU_PDE_ADDR_MASK			(0xFFFFF000UL)
	#define SGX_MMU_PDE_ADDR_ALIGNSHIFT		(0)
#endif
#define SGX_MMU_PDE_VALID				(0x00000001UL)
#define SGX_MMU_PDE_PAGE_SIZE_4K		(0x00000000UL)
#if defined(SGX_FEATURE_VARIABLE_MMU_PAGE_SIZE)
	#define SGX_MMU_PDE_PAGE_SIZE_16K		(0x00000002UL)
	#define SGX_MMU_PDE_PAGE_SIZE_64K		(0x00000004UL)
	#define SGX_MMU_PDE_PAGE_SIZE_256K		(0x00000006UL)
	#define SGX_MMU_PDE_PAGE_SIZE_1M		(0x00000008UL)
	#define SGX_MMU_PDE_PAGE_SIZE_4M		(0x0000000AUL)
	#define SGX_MMU_PDE_PAGE_SIZE_MASK		(0x0000000EUL)
#else
	#define SGX_MMU_PDE_WRITEONLY			(0x00000002UL)
	#define SGX_MMU_PDE_READONLY			(0x00000004UL)
	#define SGX_MMU_PDE_CACHECONSISTENT		(0x00000008UL)
	#define SGX_MMU_PDE_EDMPROTECT			(0x00000010UL)
#endif

#define SGX_MMU_PT_SHIFT				(10)
#define SGX_MMU_PT_SIZE					(1UL<<SGX_MMU_PT_SHIFT)
#define SGX_MMU_PT_MASK					(0x003FF000UL)

#if defined(SGX_FEATURE_36BIT_MMU)
	#define SGX_MMU_PTE_ADDR_MASK			(0xFFFFFF00UL)
	#define SGX_MMU_PTE_ADDR_ALIGNSHIFT		(4)
#else
	#define SGX_MMU_PTE_ADDR_MASK			(0xFFFFF000UL)
	#define SGX_MMU_PTE_ADDR_ALIGNSHIFT		(0)
#endif
#define SGX_MMU_PTE_VALID				(0x00000001UL)
#define SGX_MMU_PTE_WRITEONLY			(0x00000002UL)
#define SGX_MMU_PTE_READONLY			(0x00000004UL)
#define SGX_MMU_PTE_CACHECONSISTENT		(0x00000008UL)
#define SGX_MMU_PTE_EDMPROTECT			(0x00000010UL)

#endif

