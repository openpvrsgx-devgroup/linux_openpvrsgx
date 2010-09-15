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

#ifndef __BRIDGED_SGX_BRIDGE_H__
#define __BRIDGED_SGX_BRIDGE_H__

void SetSGXDispatchTableEntry(void);

int SGXGetClientInfoBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_GETCLIENTINFO *psGetClientInfoIN,
       struct PVRSRV_BRIDGE_OUT_GETCLIENTINFO *psGetClientInfoOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXReleaseClientInfoBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_RELEASECLIENTINFO *psReleaseClientInfoIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXGetInternalDevInfoBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_GETINTERNALDEVINFO *psSGXGetInternalDevInfoIN,
       struct PVRSRV_BRIDGE_OUT_GETINTERNALDEVINFO *psSGXGetInternalDevInfoOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXDoKickBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_DOKICK *psDoKickIN,
	struct PVRSRV_BRIDGE_RETURN *psRetOUT, size_t in_size,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXScheduleProcessQueuesBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGX_SCHEDULE_PROCESS_QUEUES *psScheduleProcQIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXSubmitTransferBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SUBMITTRANSFER *psSubmitTransferIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXGetMiscInfoBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGXGETMISCINFO *psSGXGetMiscInfoIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXReadDiffCountersBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGX_READ_DIFF_COUNTERS *psSGXReadDiffCountersIN,
       struct PVRSRV_BRIDGE_OUT_SGX_READ_DIFF_COUNTERS
       *psSGXReadDiffCountersOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXReadHWPerfCBBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGX_READ_HWPERF_CB *psSGXReadHWPerfCBIN,
       struct PVRSRV_BRIDGE_OUT_SGX_READ_HWPERF_CB *psSGXReadHWPerfCBOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXDevInitPart2BW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGXDEVINITPART2 *psSGXDevInitPart2IN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXRegisterHWRenderContextBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_RENDER_CONTEXT
       *psSGXRegHWRenderContextIN,
       struct PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_RENDER_CONTEXT
       *psSGXRegHWRenderContextOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXUnregisterHWRenderContextBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_RENDER_CONTEXT
       *psSGXUnregHWRenderContextIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXRegisterHWTransferContextBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_TRANSFER_CONTEXT
       *psSGXRegHWTransferContextIN,
       struct PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_TRANSFER_CONTEXT
       *psSGXRegHWTransferContextOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXUnregisterHWTransferContextBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_TRANSFER_CONTEXT
       *psSGXUnregHWTransferContextIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXFlushHWRenderTargetBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGX_FLUSH_HW_RENDER_TARGET
       *psSGXFlushHWRenderTargetIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGX2DQueryBlitsCompleteBW(struct file *filp, u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_2DQUERYBLTSCOMPLETE *ps2DQueryBltsCompleteIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXFindSharedPBDescBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGXFINDSHAREDPBDESC *psSGXFindSharedPBDescIN,
       struct PVRSRV_BRIDGE_OUT_SGXFINDSHAREDPBDESC *psSGXFindSharedPBDescOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXUnrefSharedPBDescBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGXUNREFSHAREDPBDESC *psSGXUnrefSharedPBDescIN,
       struct PVRSRV_BRIDGE_OUT_SGXUNREFSHAREDPBDESC
       *psSGXUnrefSharedPBDescOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXAddSharedPBDescBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGXADDSHAREDPBDESC *psSGXAddSharedPBDescIN,
       struct PVRSRV_BRIDGE_OUT_SGXADDSHAREDPBDESC *psSGXAddSharedPBDescOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXGetInfoForSrvinitBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SGXINFO_FOR_SRVINIT *psSGXInfoForSrvinitIN,
       struct PVRSRV_BRIDGE_OUT_SGXINFO_FOR_SRVINIT *psSGXInfoForSrvinitOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

#if defined(PDUMP)
int SGXPDumpBufferArrayBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_PDUMP_BUFFER_ARRAY *psPDumpBufferArrayIN,
       void *psBridgeOut, struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXPDump3DSignatureRegistersBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_PDUMP_3D_SIGNATURE_REGISTERS
       *psPDump3DSignatureRegistersIN,
       void *psBridgeOut, struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXPDumpCounterRegistersBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_PDUMP_COUNTER_REGISTERS
       *psPDumpCounterRegistersIN,
       void *psBridgeOut, struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXPDumpTASignatureRegistersBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_PDUMP_TA_SIGNATURE_REGISTERS
       *psPDumpTASignatureRegistersIN,
       void *psBridgeOut, struct PVRSRV_PER_PROCESS_DATA *psPerProc);

int SGXPDumpHWPerfCBBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_PDUMP_HWPERFCB *psPDumpHWPerfCBIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc);

#endif
#endif
