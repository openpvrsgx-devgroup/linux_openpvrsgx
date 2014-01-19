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

#ifndef __SGXSCRIPT_H__
#define __SGXSCRIPT_H__

#if defined (__cplusplus)
extern "C" {
#endif

#define	SGX_MAX_INIT_COMMANDS	64
#define	SGX_MAX_DEINIT_COMMANDS	16

typedef	enum _SGX_INIT_OPERATION
{
	SGX_INIT_OP_ILLEGAL = 0,
	SGX_INIT_OP_WRITE_HW_REG,
#if defined(PDUMP)
	SGX_INIT_OP_PDUMP_HW_REG,
#endif
	SGX_INIT_OP_HALT
} SGX_INIT_OPERATION;

typedef union _SGX_INIT_COMMAND
{
	SGX_INIT_OPERATION eOp;
	struct {
		SGX_INIT_OPERATION eOp;
		IMG_UINT32 ui32Offset;
		IMG_UINT32 ui32Value;
	} sWriteHWReg;
#if defined(PDUMP)
	struct {
		SGX_INIT_OPERATION eOp;
		IMG_UINT32 ui32Offset;
		IMG_UINT32 ui32Value;
	} sPDumpHWReg;
#endif
#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)
	struct {
		SGX_INIT_OPERATION eOp;
	} sWorkaroundBRN22997;
#endif
} SGX_INIT_COMMAND;

typedef struct _SGX_INIT_SCRIPTS_
{
	SGX_INIT_COMMAND asInitCommandsPart1[SGX_MAX_INIT_COMMANDS];
	SGX_INIT_COMMAND asInitCommandsPart2[SGX_MAX_INIT_COMMANDS];
	SGX_INIT_COMMAND asDeinitCommands[SGX_MAX_DEINIT_COMMANDS];
} SGX_INIT_SCRIPTS;

#if defined(__cplusplus)
}
#endif

#endif

