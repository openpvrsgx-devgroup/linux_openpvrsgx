/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2010-2011 Texas Instruments Incorporated,
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2010-2011 Texas Instruments Incorporated,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef _ABE_DM_ADDR_H_
#define _ABE_DM_ADDR_H_
#define D_atcDescriptors_ADDR                               0
#define D_atcDescriptors_ADDR_END                           511
#define D_atcDescriptors_sizeof                             512
#define stack_ADDR                                          512
#define stack_ADDR_END                                      623
#define stack_sizeof                                        112
#define D_version_ADDR                                      624
#define D_version_ADDR_END                                  627
#define D_version_sizeof                                    4
#define D_IOdescr_ADDR                                      628
#define D_IOdescr_ADDR_END                                  1267
#define D_IOdescr_sizeof                                    640
#define d_zero_ADDR                                         1268
#define d_zero_ADDR_END                                     1271
#define d_zero_sizeof                                       4
#define dbg_trace1_ADDR                                     1272
#define dbg_trace1_ADDR_END                                 1272
#define dbg_trace1_sizeof                                   1
#define dbg_trace2_ADDR                                     1273
#define dbg_trace2_ADDR_END                                 1273
#define dbg_trace2_sizeof                                   1
#define dbg_trace3_ADDR                                     1274
#define dbg_trace3_ADDR_END                                 1274
#define dbg_trace3_sizeof                                   1
#define D_multiFrame_ADDR                                   1276
#define D_multiFrame_ADDR_END                               1675
#define D_multiFrame_sizeof                                 400
#define D_idleTask_ADDR                                     1676
#define D_idleTask_ADDR_END                                 1677
#define D_idleTask_sizeof                                   2
#define D_typeLengthCheck_ADDR                              1678
#define D_typeLengthCheck_ADDR_END                          1679
#define D_typeLengthCheck_sizeof                            2
#define D_maxTaskBytesInSlot_ADDR                           1680
#define D_maxTaskBytesInSlot_ADDR_END                       1681
#define D_maxTaskBytesInSlot_sizeof                         2
#define D_rewindTaskBytes_ADDR                              1682
#define D_rewindTaskBytes_ADDR_END                          1683
#define D_rewindTaskBytes_sizeof                            2
#define D_pCurrentTask_ADDR                                 1684
#define D_pCurrentTask_ADDR_END                             1685
#define D_pCurrentTask_sizeof                               2
#define D_pFastLoopBack_ADDR                                1686
#define D_pFastLoopBack_ADDR_END                            1687
#define D_pFastLoopBack_sizeof                              2
#define D_pNextFastLoopBack_ADDR                            1688
#define D_pNextFastLoopBack_ADDR_END                        1691
#define D_pNextFastLoopBack_sizeof                          4
#define D_ppCurrentTask_ADDR                                1692
#define D_ppCurrentTask_ADDR_END                            1693
#define D_ppCurrentTask_sizeof                              2
#define D_slotCounter_ADDR                                  1696
#define D_slotCounter_ADDR_END                              1697
#define D_slotCounter_sizeof                                2
#define D_loopCounter_ADDR                                  1700
#define D_loopCounter_ADDR_END                              1703
#define D_loopCounter_sizeof                                4
#define D_RewindFlag_ADDR                                   1704
#define D_RewindFlag_ADDR_END                               1705
#define D_RewindFlag_sizeof                                 2
#define D_Slot23_ctrl_ADDR                                  1708
#define D_Slot23_ctrl_ADDR_END                              1711
#define D_Slot23_ctrl_sizeof                                4
#define D_McuIrqFifo_ADDR                                   1712
#define D_McuIrqFifo_ADDR_END                               1775
#define D_McuIrqFifo_sizeof                                 64
#define D_PingPongDesc_ADDR                                 1776
#define D_PingPongDesc_ADDR_END                             1807
#define D_PingPongDesc_sizeof                               32
#define D_PP_MCU_IRQ_ADDR                                   1808
#define D_PP_MCU_IRQ_ADDR_END                               1809
#define D_PP_MCU_IRQ_sizeof                                 2
#define D_SRC44P1_MMDL_STRUCT_ADDR                          1812
#define D_SRC44P1_MMDL_STRUCT_ADDR_END                      1829
#define D_SRC44P1_MMDL_STRUCT_sizeof                        18
#define D_SRC44P1_TONES_STRUCT_ADDR                         1832
#define D_SRC44P1_TONES_STRUCT_ADDR_END                     1849
#define D_SRC44P1_TONES_STRUCT_sizeof                       18
#define D_ctrlPortFifo_ADDR                                 1856
#define D_ctrlPortFifo_ADDR_END                             1871
#define D_ctrlPortFifo_sizeof                               16
#define D_Idle_State_ADDR                                   1872
#define D_Idle_State_ADDR_END                               1875
#define D_Idle_State_sizeof                                 4
#define D_Stop_Request_ADDR                                 1876
#define D_Stop_Request_ADDR_END                             1879
#define D_Stop_Request_sizeof                               4
#define D_Ref0_ADDR                                         1880
#define D_Ref0_ADDR_END                                     1881
#define D_Ref0_sizeof                                       2
#define D_DebugRegister_ADDR                                1884
#define D_DebugRegister_ADDR_END                            2023
#define D_DebugRegister_sizeof                              140
#define D_Gcount_ADDR                                       2024
#define D_Gcount_ADDR_END                                   2025
#define D_Gcount_sizeof                                     2
#define D_fastCounter_ADDR                                  2028
#define D_fastCounter_ADDR_END                              2031
#define D_fastCounter_sizeof                                4
#define D_slowCounter_ADDR                                  2032
#define D_slowCounter_ADDR_END                              2035
#define D_slowCounter_sizeof                                4
#define D_aUplinkRouting_ADDR                               2036
#define D_aUplinkRouting_ADDR_END                           2067
#define D_aUplinkRouting_sizeof                             32
#define D_VirtAudioLoop_ADDR                                2068
#define D_VirtAudioLoop_ADDR_END                            2071
#define D_VirtAudioLoop_sizeof                              4
#define D_AsrcVars_DL_VX_ADDR                               2072
#define D_AsrcVars_DL_VX_ADDR_END                           2103
#define D_AsrcVars_DL_VX_sizeof                             32
#define D_AsrcVars_UL_VX_ADDR                               2104
#define D_AsrcVars_UL_VX_ADDR_END                           2135
#define D_AsrcVars_UL_VX_sizeof                             32
#define D_CoefAddresses_VX_ADDR                             2136
#define D_CoefAddresses_VX_ADDR_END                         2167
#define D_CoefAddresses_VX_sizeof                           32
#define D_AsrcVars_MM_EXT_IN_ADDR                           2168
#define D_AsrcVars_MM_EXT_IN_ADDR_END                       2199
#define D_AsrcVars_MM_EXT_IN_sizeof                         32
#define D_CoefAddresses_MM_ADDR                             2200
#define D_CoefAddresses_MM_ADDR_END                         2231
#define D_CoefAddresses_MM_sizeof                           32
#define D_TraceBufAdr_ADDR                                  2232
#define D_TraceBufAdr_ADDR_END                              2233
#define D_TraceBufAdr_sizeof                                2
#define D_TraceBufOffset_ADDR                               2234
#define D_TraceBufOffset_ADDR_END                           2235
#define D_TraceBufOffset_sizeof                             2
#define D_TraceBufLength_ADDR                               2236
#define D_TraceBufLength_ADDR_END                           2237
#define D_TraceBufLength_sizeof                             2
#define D_maxTaskBytesInSlot_saved_ADDR                     2240
#define D_maxTaskBytesInSlot_saved_ADDR_END                 2243
#define D_maxTaskBytesInSlot_saved_sizeof                   4
#define D_Pempty_ADDR                                       2244
#define D_Pempty_ADDR_END                                   2323
#define D_Pempty_sizeof                                     80
#define D_ECHO_REF_48_16_WRAP_ADDR                          2324
#define D_ECHO_REF_48_16_WRAP_ADDR_END                      2331
#define D_ECHO_REF_48_16_WRAP_sizeof                        8
#define D_ECHO_REF_48_8_WRAP_ADDR                           2332
#define D_ECHO_REF_48_8_WRAP_ADDR_END                       2339
#define D_ECHO_REF_48_8_WRAP_sizeof                         8
#define D_BT_UL_16_48_WRAP_ADDR                             2340
#define D_BT_UL_16_48_WRAP_ADDR_END                         2347
#define D_BT_UL_16_48_WRAP_sizeof                           8
#define D_BT_UL_8_48_WRAP_ADDR                              2348
#define D_BT_UL_8_48_WRAP_ADDR_END                          2355
#define D_BT_UL_8_48_WRAP_sizeof                            8
#define D_BT_DL_48_16_WRAP_ADDR                             2356
#define D_BT_DL_48_16_WRAP_ADDR_END                         2363
#define D_BT_DL_48_16_WRAP_sizeof                           8
#define D_BT_DL_48_8_WRAP_ADDR                              2364
#define D_BT_DL_48_8_WRAP_ADDR_END                          2371
#define D_BT_DL_48_8_WRAP_sizeof                            8
#define D_VX_DL_16_48_WRAP_ADDR                             2372
#define D_VX_DL_16_48_WRAP_ADDR_END                         2379
#define D_VX_DL_16_48_WRAP_sizeof                           8
#define D_VX_DL_8_48_WRAP_ADDR                              2380
#define D_VX_DL_8_48_WRAP_ADDR_END                          2387
#define D_VX_DL_8_48_WRAP_sizeof                            8
#define D_VX_UL_48_16_WRAP_ADDR                             2388
#define D_VX_UL_48_16_WRAP_ADDR_END                         2395
#define D_VX_UL_48_16_WRAP_sizeof                           8
#define D_VX_UL_48_8_WRAP_ADDR                              2396
#define D_VX_UL_48_8_WRAP_ADDR_END                          2403
#define D_VX_UL_48_8_WRAP_sizeof                            8
#define D_AsrcVars_BT_UL_ADDR                               2404
#define D_AsrcVars_BT_UL_ADDR_END                           2435
#define D_AsrcVars_BT_UL_sizeof                             32
#define D_AsrcVars_BT_DL_ADDR                               2436
#define D_AsrcVars_BT_DL_ADDR_END                           2467
#define D_AsrcVars_BT_DL_sizeof                             32
#define D_BT_DL_48_8_OPP100_WRAP_ADDR                       2468
#define D_BT_DL_48_8_OPP100_WRAP_ADDR_END                   2475
#define D_BT_DL_48_8_OPP100_WRAP_sizeof                     8
#define D_BT_DL_48_16_OPP100_WRAP_ADDR                      2476
#define D_BT_DL_48_16_OPP100_WRAP_ADDR_END                  2483
#define D_BT_DL_48_16_OPP100_WRAP_sizeof                    8
#define D_VX_DL_8_48_FIR_WRAP_ADDR                          2484
#define D_VX_DL_8_48_FIR_WRAP_ADDR_END                      2491
#define D_VX_DL_8_48_FIR_WRAP_sizeof                        8
#define D_BT_UL_8_48_FIR_WRAP_ADDR                          2492
#define D_BT_UL_8_48_FIR_WRAP_ADDR_END                      2499
#define D_BT_UL_8_48_FIR_WRAP_sizeof                        8
#define D_tasksList_ADDR                                    2500
#define D_tasksList_ADDR_END                                4899
#define D_tasksList_sizeof                                  2400
#define D_HW_TEST_ADDR                                      4900
#define D_HW_TEST_ADDR_END                                  4939
#define D_HW_TEST_sizeof                                    40
#define D_TraceBufAdr_HAL_ADDR                              4940
#define D_TraceBufAdr_HAL_ADDR_END                          4943
#define D_TraceBufAdr_HAL_sizeof                            4
#define D_CHECK_LIST_SMEM_8K_ADDR                           4944
#define D_CHECK_LIST_SMEM_8K_ADDR_END                       5071
#define D_CHECK_LIST_SMEM_8K_sizeof                         128
#define D_CHECK_LIST_SMEM_16K_ADDR                          5072
#define D_CHECK_LIST_SMEM_16K_ADDR_END                      5199
#define D_CHECK_LIST_SMEM_16K_sizeof                        128
#define D_CHECK_LIST_LEFT_IDX_ADDR                          5200
#define D_CHECK_LIST_LEFT_IDX_ADDR_END                      5201
#define D_CHECK_LIST_LEFT_IDX_sizeof                        2
#define D_CHECK_LIST_RIGHT_IDX_ADDR                         5202
#define D_CHECK_LIST_RIGHT_IDX_ADDR_END                     5203
#define D_CHECK_LIST_RIGHT_IDX_sizeof                       2
#define D_BT_DL_48_8_FIR_WRAP_ADDR                          5204
#define D_BT_DL_48_8_FIR_WRAP_ADDR_END                      5211
#define D_BT_DL_48_8_FIR_WRAP_sizeof                        8
#define D_BT_DL_48_8_FIR_OPP100_WRAP_ADDR                   5212
#define D_BT_DL_48_8_FIR_OPP100_WRAP_ADDR_END               5219
#define D_BT_DL_48_8_FIR_OPP100_WRAP_sizeof                 8
#define D_VX_UL_48_8_FIR_WRAP_ADDR                          5220
#define D_VX_UL_48_8_FIR_WRAP_ADDR_END                      5227
#define D_VX_UL_48_8_FIR_WRAP_sizeof                        8
#define D_DEBUG_FW_TASK_ADDR                                5632
#define D_DEBUG_FW_TASK_ADDR_END                            5887
#define D_DEBUG_FW_TASK_sizeof                              256
#define D_DEBUG_FIFO_ADDR                                   5888
#define D_DEBUG_FIFO_ADDR_END                               5983
#define D_DEBUG_FIFO_sizeof                                 96
#define D_DEBUG_FIFO_HAL_ADDR                               5984
#define D_DEBUG_FIFO_HAL_ADDR_END                           6015
#define D_DEBUG_FIFO_HAL_sizeof                             32
#define D_FwMemInit_ADDR                                    6016
#define D_FwMemInit_ADDR_END                                6975
#define D_FwMemInit_sizeof                                  960
#define D_FwMemInitDescr_ADDR                               6976
#define D_FwMemInitDescr_ADDR_END                           6991
#define D_FwMemInitDescr_sizeof                             16
#define D_BT_DL_FIFO_ADDR                                   7168
#define D_BT_DL_FIFO_ADDR_END                               7647
#define D_BT_DL_FIFO_sizeof                                 480
#define D_BT_UL_FIFO_ADDR                                   7680
#define D_BT_UL_FIFO_ADDR_END                               8159
#define D_BT_UL_FIFO_sizeof                                 480
#define D_MM_EXT_OUT_FIFO_ADDR                              8192
#define D_MM_EXT_OUT_FIFO_ADDR_END                          8671
#define D_MM_EXT_OUT_FIFO_sizeof                            480
#define D_MM_EXT_IN_FIFO_ADDR                               8704
#define D_MM_EXT_IN_FIFO_ADDR_END                           9183
#define D_MM_EXT_IN_FIFO_sizeof                             480
#define D_MM_UL2_FIFO_ADDR                                  9216
#define D_MM_UL2_FIFO_ADDR_END                              9695
#define D_MM_UL2_FIFO_sizeof                                480
#define D_DMIC_UL_FIFO_ADDR                                 9728
#define D_DMIC_UL_FIFO_ADDR_END                             10207
#define D_DMIC_UL_FIFO_sizeof                               480
#define D_MM_UL_FIFO_ADDR                                   10240
#define D_MM_UL_FIFO_ADDR_END                               10719
#define D_MM_UL_FIFO_sizeof                                 480
#define D_MM_DL_FIFO_ADDR                                   10752
#define D_MM_DL_FIFO_ADDR_END                               11231
#define D_MM_DL_FIFO_sizeof                                 480
#define D_TONES_DL_FIFO_ADDR                                11264
#define D_TONES_DL_FIFO_ADDR_END                            11743
#define D_TONES_DL_FIFO_sizeof                              480
#define D_VIB_DL_FIFO_ADDR                                  11776
#define D_VIB_DL_FIFO_ADDR_END                              12255
#define D_VIB_DL_FIFO_sizeof                                480
#define D_DEBUG_HAL_TASK_ADDR                               12288
#define D_DEBUG_HAL_TASK_ADDR_END                           14335
#define D_DEBUG_HAL_TASK_sizeof                             2048
#define D_McPDM_DL_FIFO_ADDR                                14336
#define D_McPDM_DL_FIFO_ADDR_END                            14815
#define D_McPDM_DL_FIFO_sizeof                              480
#define D_McPDM_UL_FIFO_ADDR                                14848
#define D_McPDM_UL_FIFO_ADDR_END                            15327
#define D_McPDM_UL_FIFO_sizeof                              480
#define D_VX_UL_FIFO_ADDR                                   15360
#define D_VX_UL_FIFO_ADDR_END                               15839
#define D_VX_UL_FIFO_sizeof                                 480
#define D_VX_DL_FIFO_ADDR                                   15872
#define D_VX_DL_FIFO_ADDR_END                               16351
#define D_VX_DL_FIFO_sizeof                                 480
#define D_PING_ADDR                                         16384
#define D_PING_ADDR_END                                     40959
#define D_PING_sizeof                                       24576
#define D_PONG_ADDR                                         40960
#define D_PONG_ADDR_END                                     65535
#define D_PONG_sizeof                                       24576
#endif /* _ABEDM_ADDR_H_ */
