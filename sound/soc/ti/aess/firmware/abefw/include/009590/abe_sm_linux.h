/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2010-2013 Texas Instruments Incorporated,
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
 * Copyright(c) 2010-2013 Texas Instruments Incorporated,
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
#define OMAP_ABE_INIT_SM_ADDR                              0x0
#define OMAP_ABE_INIT_SM_SIZE                              0xD00
#define OMAP_ABE_S_DATA0_ADDR                              0xD00
#define OMAP_ABE_S_DATA0_SIZE                              0x8
#define OMAP_ABE_S_TEMP_ADDR                               0xD08
#define OMAP_ABE_S_TEMP_SIZE                               0x8
#define OMAP_ABE_S_PHOENIXOFFSET_ADDR                      0xD10
#define OMAP_ABE_S_PHOENIXOFFSET_SIZE                      0x8
#define OMAP_ABE_S_GTARGET1_ADDR                           0xD18
#define OMAP_ABE_S_GTARGET1_SIZE                           0x38
#define OMAP_ABE_S_GTARGET_DL1_ADDR                        0xD50
#define OMAP_ABE_S_GTARGET_DL1_SIZE                        0x10
#define OMAP_ABE_S_GTARGET_DL2_ADDR                        0xD60
#define OMAP_ABE_S_GTARGET_DL2_SIZE                        0x10
#define OMAP_ABE_S_GTARGET_ECHO_ADDR                       0xD70
#define OMAP_ABE_S_GTARGET_ECHO_SIZE                       0x8
#define OMAP_ABE_S_GTARGET_SDT_ADDR                        0xD78
#define OMAP_ABE_S_GTARGET_SDT_SIZE                        0x8
#define OMAP_ABE_S_GTARGET_VXREC_ADDR                      0xD80
#define OMAP_ABE_S_GTARGET_VXREC_SIZE                      0x10
#define OMAP_ABE_S_GTARGET_UL_ADDR                         0xD90
#define OMAP_ABE_S_GTARGET_UL_SIZE                         0x10
#define OMAP_ABE_S_GTARGET_BTUL_ADDR                       0xDA0
#define OMAP_ABE_S_GTARGET_BTUL_SIZE                       0x8
#define OMAP_ABE_S_GCURRENT_ADDR                           0xDA8
#define OMAP_ABE_S_GCURRENT_SIZE                           0x90
#define OMAP_ABE_S_GAIN_ONE_ADDR                           0xE38
#define OMAP_ABE_S_GAIN_ONE_SIZE                           0x8
#define OMAP_ABE_S_TONES_ADDR                              0xE40
#define OMAP_ABE_S_TONES_SIZE                              0x60
#define OMAP_ABE_S_VX_DL_ADDR                              0xEA0
#define OMAP_ABE_S_VX_DL_SIZE                              0x60
#define OMAP_ABE_S_MM_UL2_ADDR                             0xF00
#define OMAP_ABE_S_MM_UL2_SIZE                             0x60
#define OMAP_ABE_S_MM_DL_ADDR                              0xF60
#define OMAP_ABE_S_MM_DL_SIZE                              0x60
#define OMAP_ABE_S_DL1_M_OUT_ADDR                          0xFC0
#define OMAP_ABE_S_DL1_M_OUT_SIZE                          0x60
#define OMAP_ABE_S_DL2_M_OUT_ADDR                          0x1020
#define OMAP_ABE_S_DL2_M_OUT_SIZE                          0x60
#define OMAP_ABE_S_ECHO_M_OUT_ADDR                         0x1080
#define OMAP_ABE_S_ECHO_M_OUT_SIZE                         0x60
#define OMAP_ABE_S_SDT_M_OUT_ADDR                          0x10E0
#define OMAP_ABE_S_SDT_M_OUT_SIZE                          0x60
#define OMAP_ABE_S_VX_UL_ADDR                              0x1140
#define OMAP_ABE_S_VX_UL_SIZE                              0x60
#define OMAP_ABE_S_VX_UL_M_ADDR                            0x11A0
#define OMAP_ABE_S_VX_UL_M_SIZE                            0x60
#define OMAP_ABE_S_BT_DL_ADDR                              0x1200
#define OMAP_ABE_S_BT_DL_SIZE                              0x60
#define OMAP_ABE_S_BT_UL_ADDR                              0x1260
#define OMAP_ABE_S_BT_UL_SIZE                              0x60
#define OMAP_ABE_S_BT_DL_8K_ADDR                           0x12C0
#define OMAP_ABE_S_BT_DL_8K_SIZE                           0x18
#define OMAP_ABE_S_BT_DL_16K_ADDR                          0x12D8
#define OMAP_ABE_S_BT_DL_16K_SIZE                          0x28
#define OMAP_ABE_S_BT_UL_8K_ADDR                           0x1300
#define OMAP_ABE_S_BT_UL_8K_SIZE                           0x10
#define OMAP_ABE_S_BT_UL_16K_ADDR                          0x1310
#define OMAP_ABE_S_BT_UL_16K_SIZE                          0x20
#define OMAP_ABE_S_SDT_F_ADDR                              0x1330
#define OMAP_ABE_S_SDT_F_SIZE                              0x60
#define OMAP_ABE_S_SDT_F_DATA_ADDR                         0x1390
#define OMAP_ABE_S_SDT_F_DATA_SIZE                         0x48
#define OMAP_ABE_S_MM_DL_OSR_ADDR                          0x13D8
#define OMAP_ABE_S_MM_DL_OSR_SIZE                          0xC0
#define OMAP_ABE_S_24_ZEROS_ADDR                           0x1498
#define OMAP_ABE_S_24_ZEROS_SIZE                           0xC0
#define OMAP_ABE_S_DMIC1_ADDR                              0x1558
#define OMAP_ABE_S_DMIC1_SIZE                              0x60
#define OMAP_ABE_S_DMIC2_ADDR                              0x15B8
#define OMAP_ABE_S_DMIC2_SIZE                              0x60
#define OMAP_ABE_S_DMIC3_ADDR                              0x1618
#define OMAP_ABE_S_DMIC3_SIZE                              0x60
#define OMAP_ABE_S_AMIC_ADDR                               0x1678
#define OMAP_ABE_S_AMIC_SIZE                               0x60
#define OMAP_ABE_S_DMIC1_L_ADDR                            0x16D8
#define OMAP_ABE_S_DMIC1_L_SIZE                            0x60
#define OMAP_ABE_S_DMIC1_R_ADDR                            0x1738
#define OMAP_ABE_S_DMIC1_R_SIZE                            0x60
#define OMAP_ABE_S_DMIC2_L_ADDR                            0x1798
#define OMAP_ABE_S_DMIC2_L_SIZE                            0x60
#define OMAP_ABE_S_DMIC2_R_ADDR                            0x17F8
#define OMAP_ABE_S_DMIC2_R_SIZE                            0x60
#define OMAP_ABE_S_DMIC3_L_ADDR                            0x1858
#define OMAP_ABE_S_DMIC3_L_SIZE                            0x60
#define OMAP_ABE_S_DMIC3_R_ADDR                            0x18B8
#define OMAP_ABE_S_DMIC3_R_SIZE                            0x60
#define OMAP_ABE_S_BT_UL_L_ADDR                            0x1918
#define OMAP_ABE_S_BT_UL_L_SIZE                            0x60
#define OMAP_ABE_S_BT_UL_R_ADDR                            0x1978
#define OMAP_ABE_S_BT_UL_R_SIZE                            0x60
#define OMAP_ABE_S_AMIC_L_ADDR                             0x19D8
#define OMAP_ABE_S_AMIC_L_SIZE                             0x60
#define OMAP_ABE_S_AMIC_R_ADDR                             0x1A38
#define OMAP_ABE_S_AMIC_R_SIZE                             0x60
#define OMAP_ABE_S_ECHOREF_L_ADDR                          0x1A98
#define OMAP_ABE_S_ECHOREF_L_SIZE                          0x60
#define OMAP_ABE_S_ECHOREF_R_ADDR                          0x1AF8
#define OMAP_ABE_S_ECHOREF_R_SIZE                          0x60
#define OMAP_ABE_S_MM_DL_L_ADDR                            0x1B58
#define OMAP_ABE_S_MM_DL_L_SIZE                            0x60
#define OMAP_ABE_S_MM_DL_R_ADDR                            0x1BB8
#define OMAP_ABE_S_MM_DL_R_SIZE                            0x60
#define OMAP_ABE_S_MM_UL_ADDR                              0x1C18
#define OMAP_ABE_S_MM_UL_SIZE                              0x3C0
#define OMAP_ABE_S_AMIC_96K_ADDR                           0x1FD8
#define OMAP_ABE_S_AMIC_96K_SIZE                           0xC0
#define OMAP_ABE_S_DMIC0_96K_ADDR                          0x2098
#define OMAP_ABE_S_DMIC0_96K_SIZE                          0xC0
#define OMAP_ABE_S_DMIC1_96K_ADDR                          0x2158
#define OMAP_ABE_S_DMIC1_96K_SIZE                          0xC0
#define OMAP_ABE_S_DMIC2_96K_ADDR                          0x2218
#define OMAP_ABE_S_DMIC2_96K_SIZE                          0xC0
#define OMAP_ABE_S_UL_VX_UL_48_8K_ADDR                     0x22D8
#define OMAP_ABE_S_UL_VX_UL_48_8K_SIZE                     0x60
#define OMAP_ABE_S_UL_VX_UL_48_16K_ADDR                    0x2338
#define OMAP_ABE_S_UL_VX_UL_48_16K_SIZE                    0x60
#define OMAP_ABE_S_UL_MIC_48K_ADDR                         0x2398
#define OMAP_ABE_S_UL_MIC_48K_SIZE                         0x60
#define OMAP_ABE_S_VOICE_8K_UL_ADDR                        0x23F8
#define OMAP_ABE_S_VOICE_8K_UL_SIZE                        0x18
#define OMAP_ABE_S_VOICE_8K_DL_ADDR                        0x2410
#define OMAP_ABE_S_VOICE_8K_DL_SIZE                        0x10
#define OMAP_ABE_S_MCPDM_OUT1_ADDR                         0x2420
#define OMAP_ABE_S_MCPDM_OUT1_SIZE                         0xC0
#define OMAP_ABE_S_MCPDM_OUT2_ADDR                         0x24E0
#define OMAP_ABE_S_MCPDM_OUT2_SIZE                         0xC0
#define OMAP_ABE_S_MCPDM_OUT3_ADDR                         0x25A0
#define OMAP_ABE_S_MCPDM_OUT3_SIZE                         0xC0
#define OMAP_ABE_S_VOICE_16K_UL_ADDR                       0x2660
#define OMAP_ABE_S_VOICE_16K_UL_SIZE                       0x28
#define OMAP_ABE_S_VOICE_16K_DL_ADDR                       0x2688
#define OMAP_ABE_S_VOICE_16K_DL_SIZE                       0x20
#define OMAP_ABE_S_XINASRC_DL_VX_ADDR                      0x26A8
#define OMAP_ABE_S_XINASRC_DL_VX_SIZE                      0x140
#define OMAP_ABE_S_XINASRC_UL_VX_ADDR                      0x27E8
#define OMAP_ABE_S_XINASRC_UL_VX_SIZE                      0x140
#define OMAP_ABE_S_XINASRC_MM_EXT_IN_ADDR                  0x2928
#define OMAP_ABE_S_XINASRC_MM_EXT_IN_SIZE                  0x140
#define OMAP_ABE_S_VX_REC_ADDR                             0x2A68
#define OMAP_ABE_S_VX_REC_SIZE                             0x60
#define OMAP_ABE_S_VX_REC_L_ADDR                           0x2AC8
#define OMAP_ABE_S_VX_REC_L_SIZE                           0x60
#define OMAP_ABE_S_VX_REC_R_ADDR                           0x2B28
#define OMAP_ABE_S_VX_REC_R_SIZE                           0x60
#define OMAP_ABE_S_DL2_M_L_ADDR                            0x2B88
#define OMAP_ABE_S_DL2_M_L_SIZE                            0x60
#define OMAP_ABE_S_DL2_M_R_ADDR                            0x2BE8
#define OMAP_ABE_S_DL2_M_R_SIZE                            0x60
#define OMAP_ABE_S_DL2_M_LR_EQ_DATA_ADDR                   0x2C48
#define OMAP_ABE_S_DL2_M_LR_EQ_DATA_SIZE                   0xC8
#define OMAP_ABE_S_DL1_M_EQ_DATA_ADDR                      0x2D10
#define OMAP_ABE_S_DL1_M_EQ_DATA_SIZE                      0xC8
#define OMAP_ABE_S_EARP_48_96_LP_DATA_ADDR                 0x2DD8
#define OMAP_ABE_S_EARP_48_96_LP_DATA_SIZE                 0x78
#define OMAP_ABE_S_IHF_48_96_LP_DATA_ADDR                  0x2E50
#define OMAP_ABE_S_IHF_48_96_LP_DATA_SIZE                  0x78
#define OMAP_ABE_S_VX_UL_8_TEMP_ADDR                       0x2EC8
#define OMAP_ABE_S_VX_UL_8_TEMP_SIZE                       0x10
#define OMAP_ABE_S_VX_UL_16_TEMP_ADDR                      0x2ED8
#define OMAP_ABE_S_VX_UL_16_TEMP_SIZE                      0x20
#define OMAP_ABE_S_VX_DL_8_48_LP_DATA_ADDR                 0x2EF8
#define OMAP_ABE_S_VX_DL_8_48_LP_DATA_SIZE                 0x68
#define OMAP_ABE_S_VX_DL_8_48_HP_DATA_ADDR                 0x2F60
#define OMAP_ABE_S_VX_DL_8_48_HP_DATA_SIZE                 0x38
#define OMAP_ABE_S_VX_DL_16_48_LP_DATA_ADDR                0x2F98
#define OMAP_ABE_S_VX_DL_16_48_LP_DATA_SIZE                0x68
#define OMAP_ABE_S_VX_DL_16_48_HP_DATA_ADDR                0x3000
#define OMAP_ABE_S_VX_DL_16_48_HP_DATA_SIZE                0x28
#define OMAP_ABE_S_VX_UL_48_8_LP_DATA_ADDR                 0x3028
#define OMAP_ABE_S_VX_UL_48_8_LP_DATA_SIZE                 0x68
#define OMAP_ABE_S_VX_UL_48_8_HP_DATA_ADDR                 0x3090
#define OMAP_ABE_S_VX_UL_48_8_HP_DATA_SIZE                 0x38
#define OMAP_ABE_S_VX_UL_48_16_LP_DATA_ADDR                0x30C8
#define OMAP_ABE_S_VX_UL_48_16_LP_DATA_SIZE                0x68
#define OMAP_ABE_S_VX_UL_48_16_HP_DATA_ADDR                0x3130
#define OMAP_ABE_S_VX_UL_48_16_HP_DATA_SIZE                0x28
#define OMAP_ABE_S_BT_UL_8_48_LP_DATA_ADDR                 0x3158
#define OMAP_ABE_S_BT_UL_8_48_LP_DATA_SIZE                 0x68
#define OMAP_ABE_S_BT_UL_8_48_HP_DATA_ADDR                 0x31C0
#define OMAP_ABE_S_BT_UL_8_48_HP_DATA_SIZE                 0x38
#define OMAP_ABE_S_BT_UL_16_48_LP_DATA_ADDR                0x31F8
#define OMAP_ABE_S_BT_UL_16_48_LP_DATA_SIZE                0x68
#define OMAP_ABE_S_BT_UL_16_48_HP_DATA_ADDR                0x3260
#define OMAP_ABE_S_BT_UL_16_48_HP_DATA_SIZE                0x28
#define OMAP_ABE_S_BT_DL_48_8_LP_DATA_ADDR                 0x3288
#define OMAP_ABE_S_BT_DL_48_8_LP_DATA_SIZE                 0x68
#define OMAP_ABE_S_BT_DL_48_8_HP_DATA_ADDR                 0x32F0
#define OMAP_ABE_S_BT_DL_48_8_HP_DATA_SIZE                 0x38
#define OMAP_ABE_S_BT_DL_48_16_LP_DATA_ADDR                0x3328
#define OMAP_ABE_S_BT_DL_48_16_LP_DATA_SIZE                0x68
#define OMAP_ABE_S_BT_DL_48_16_HP_DATA_ADDR                0x3390
#define OMAP_ABE_S_BT_DL_48_16_HP_DATA_SIZE                0x28
#define OMAP_ABE_S_ECHO_REF_48_8_LP_DATA_ADDR              0x33B8
#define OMAP_ABE_S_ECHO_REF_48_8_LP_DATA_SIZE              0x68
#define OMAP_ABE_S_ECHO_REF_48_8_HP_DATA_ADDR              0x3420
#define OMAP_ABE_S_ECHO_REF_48_8_HP_DATA_SIZE              0x38
#define OMAP_ABE_S_ECHO_REF_48_16_LP_DATA_ADDR             0x3458
#define OMAP_ABE_S_ECHO_REF_48_16_LP_DATA_SIZE             0x68
#define OMAP_ABE_S_ECHO_REF_48_16_HP_DATA_ADDR             0x34C0
#define OMAP_ABE_S_ECHO_REF_48_16_HP_DATA_SIZE             0x28
#define OMAP_ABE_S_XINASRC_ECHO_REF_ADDR                   0x34E8
#define OMAP_ABE_S_XINASRC_ECHO_REF_SIZE                   0x140
#define OMAP_ABE_S_ECHO_REF_16K_ADDR                       0x3628
#define OMAP_ABE_S_ECHO_REF_16K_SIZE                       0x28
#define OMAP_ABE_S_ECHO_REF_8K_ADDR                        0x3650
#define OMAP_ABE_S_ECHO_REF_8K_SIZE                        0x18
#define OMAP_ABE_S_DL1_EQ_ADDR                             0x3668
#define OMAP_ABE_S_DL1_EQ_SIZE                             0x60
#define OMAP_ABE_S_DL2_EQ_ADDR                             0x36C8
#define OMAP_ABE_S_DL2_EQ_SIZE                             0x60
#define OMAP_ABE_S_DL1_GAIN_OUT_ADDR                       0x3728
#define OMAP_ABE_S_DL1_GAIN_OUT_SIZE                       0x60
#define OMAP_ABE_S_DL2_GAIN_OUT_ADDR                       0x3788
#define OMAP_ABE_S_DL2_GAIN_OUT_SIZE                       0x60
#define OMAP_ABE_S_DC_HS_ADDR                              0x37E8
#define OMAP_ABE_S_DC_HS_SIZE                              0x8
#define OMAP_ABE_S_DC_HF_ADDR                              0x37F0
#define OMAP_ABE_S_DC_HF_SIZE                              0x8
#define OMAP_ABE_S_MCASP1_ADDR                             0x37F8
#define OMAP_ABE_S_MCASP1_SIZE                             0x30
#define OMAP_ABE_S_RNOISE_MEM_ADDR                         0x3828
#define OMAP_ABE_S_RNOISE_MEM_SIZE                         0x8
#define OMAP_ABE_S_CTRL_ADDR                               0x3830
#define OMAP_ABE_S_CTRL_SIZE                               0x90
#define OMAP_ABE_S_AMIC_96_48_DATA_ADDR                    0x38C0
#define OMAP_ABE_S_AMIC_96_48_DATA_SIZE                    0x98
#define OMAP_ABE_S_DMIC0_96_48_DATA_ADDR                   0x3958
#define OMAP_ABE_S_DMIC0_96_48_DATA_SIZE                   0x98
#define OMAP_ABE_S_DMIC1_96_48_DATA_ADDR                   0x39F0
#define OMAP_ABE_S_DMIC1_96_48_DATA_SIZE                   0x98
#define OMAP_ABE_S_DMIC2_96_48_DATA_ADDR                   0x3A88
#define OMAP_ABE_S_DMIC2_96_48_DATA_SIZE                   0x98
#define OMAP_ABE_S_DBG_8K_PATTERN_ADDR                     0x3B20
#define OMAP_ABE_S_DBG_8K_PATTERN_SIZE                     0x10
#define OMAP_ABE_S_DBG_16K_PATTERN_ADDR                    0x3B30
#define OMAP_ABE_S_DBG_16K_PATTERN_SIZE                    0x20
#define OMAP_ABE_S_DBG_24K_PATTERN_ADDR                    0x3B50
#define OMAP_ABE_S_DBG_24K_PATTERN_SIZE                    0x30
#define OMAP_ABE_S_DBG_48K_PATTERN_ADDR                    0x3B80
#define OMAP_ABE_S_DBG_48K_PATTERN_SIZE                    0x60
#define OMAP_ABE_S_DBG_96K_PATTERN_ADDR                    0x3BE0
#define OMAP_ABE_S_DBG_96K_PATTERN_SIZE                    0xC0
#define OMAP_ABE_S_MM_EXT_IN_ADDR                          0x3CA0
#define OMAP_ABE_S_MM_EXT_IN_SIZE                          0x60
#define OMAP_ABE_S_MM_EXT_IN_L_ADDR                        0x3D00
#define OMAP_ABE_S_MM_EXT_IN_L_SIZE                        0x60
#define OMAP_ABE_S_MM_EXT_IN_R_ADDR                        0x3D60
#define OMAP_ABE_S_MM_EXT_IN_R_SIZE                        0x60
#define OMAP_ABE_S_MIC4_ADDR                               0x3DC0
#define OMAP_ABE_S_MIC4_SIZE                               0x60
#define OMAP_ABE_S_MIC4_L_ADDR                             0x3E20
#define OMAP_ABE_S_MIC4_L_SIZE                             0x60
#define OMAP_ABE_S_SATURATION_7FFF_ADDR                    0x3E80
#define OMAP_ABE_S_SATURATION_7FFF_SIZE                    0x8
#define OMAP_ABE_S_SATURATION_ADDR                         0x3E88
#define OMAP_ABE_S_SATURATION_SIZE                         0x8
#define OMAP_ABE_S_XINASRC_BT_UL_ADDR                      0x3E90
#define OMAP_ABE_S_XINASRC_BT_UL_SIZE                      0x140
#define OMAP_ABE_S_XINASRC_BT_DL_ADDR                      0x3FD0
#define OMAP_ABE_S_XINASRC_BT_DL_SIZE                      0x140
#define OMAP_ABE_S_BT_DL_8K_TEMP_ADDR                      0x4110
#define OMAP_ABE_S_BT_DL_8K_TEMP_SIZE                      0x10
#define OMAP_ABE_S_BT_DL_16K_TEMP_ADDR                     0x4120
#define OMAP_ABE_S_BT_DL_16K_TEMP_SIZE                     0x20
#define OMAP_ABE_S_VX_DL_8_48_OSR_LP_DATA_ADDR             0x4140
#define OMAP_ABE_S_VX_DL_8_48_OSR_LP_DATA_SIZE             0xE0
#define OMAP_ABE_S_BT_UL_8_48_OSR_LP_DATA_ADDR             0x4220
#define OMAP_ABE_S_BT_UL_8_48_OSR_LP_DATA_SIZE             0xE0
#define OMAP_ABE_S_MM_DL_44P1_ADDR                         0x4300
#define OMAP_ABE_S_MM_DL_44P1_SIZE                         0x300
#define OMAP_ABE_S_TONES_44P1_ADDR                         0x4600
#define OMAP_ABE_S_TONES_44P1_SIZE                         0x300
#define OMAP_ABE_S_MM_DL_44P1_XK_ADDR                      0x4900
#define OMAP_ABE_S_MM_DL_44P1_XK_SIZE                      0x10
#define OMAP_ABE_S_TONES_44P1_XK_ADDR                      0x4910
#define OMAP_ABE_S_TONES_44P1_XK_SIZE                      0x10
#define OMAP_ABE_S_SRC_44P1_MULFAC1_ADDR                   0x4920
#define OMAP_ABE_S_SRC_44P1_MULFAC1_SIZE                   0x8
#define OMAP_ABE_S_SATURATION_EQ_ADDR                      0x4928
#define OMAP_ABE_S_SATURATION_EQ_SIZE                      0x8
#define OMAP_ABE_S_BT_DL_48_8_LP_NEW_DATA_ADDR             0x4930
#define OMAP_ABE_S_BT_DL_48_8_LP_NEW_DATA_SIZE             0x88
#define OMAP_ABE_S_BT_DL_8_48_OSR_LP_DATA_ADDR             0x49B8
#define OMAP_ABE_S_BT_DL_8_48_OSR_LP_DATA_SIZE             0x3C8
#define OMAP_ABE_S_VX_UL_48_8_LP_NEW_DATA_ADDR             0x4D80
#define OMAP_ABE_S_VX_UL_48_8_LP_NEW_DATA_SIZE             0x88
#define OMAP_ABE_S_VX_UL_8_48_OSR_LP_DATA_ADDR             0x4E08
#define OMAP_ABE_S_VX_UL_8_48_OSR_LP_DATA_SIZE             0x3C8
#define OMAP_ABE_S_EARP_48_96_LP_NEW_DATA_ADDR             0x51D0
#define OMAP_ABE_S_EARP_48_96_LP_NEW_DATA_SIZE             0x98
#define OMAP_ABE_S_IHF_48_96_LP_NEW_DATA_ADDR              0x5268
#define OMAP_ABE_S_IHF_48_96_LP_NEW_DATA_SIZE              0x98
#define OMAP_ABE_S_DMIC1_48_EQ_DATA_ADDR                   0x5300
#define OMAP_ABE_S_DMIC1_48_EQ_DATA_SIZE                   0x98
#define OMAP_ABE_S_DL1_M_EQ_BQ_1_DATA_ADDR                 0x5398
#define OMAP_ABE_S_DL1_M_EQ_BQ_1_DATA_SIZE                 0x28
#define OMAP_ABE_S_DL1_M_EQ_BQ_2_DATA_ADDR                 0x53C0
#define OMAP_ABE_S_DL1_M_EQ_BQ_2_DATA_SIZE                 0x28
#define OMAP_ABE_S_DL1_M_EQ_BQ_3_DATA_ADDR                 0x53E8
#define OMAP_ABE_S_DL1_M_EQ_BQ_3_DATA_SIZE                 0x28
#define OMAP_ABE_S_DL1_M_EQ_BQ_4_DATA_ADDR                 0x5410
#define OMAP_ABE_S_DL1_M_EQ_BQ_4_DATA_SIZE                 0x28
#define OMAP_ABE_S_DL1_M_EQ_BQ_5_DATA_ADDR                 0x5438
#define OMAP_ABE_S_DL1_M_EQ_BQ_5_DATA_SIZE                 0x28
