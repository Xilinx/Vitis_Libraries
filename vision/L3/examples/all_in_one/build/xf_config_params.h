/*
 * Copyright 2023 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define XF_NPPC XF_NPPC1 // XF_NPPC1 --1PIXEL , XF_NPPC2--2PIXEL ,XF_NPPC4--4 and XF_NPPC8--8PIXEL

#define XF_WIDTH 1920  // 3840  // MAX_COLS
#define XF_HEIGHT 1080 // 2160 // MAX_ROWS

#define XF_BAYER_PATTERN XF_BAYER_RG // bayer pattern Used in gaincontrol, demosaicing, rgbir2bayer

#define T_8U 0
#define T_10U 0
#define T_12U 0
#define T_16U 1

#define USE_HDR_FUSION 0
#define USE_RGBIR 0
#define USE_DEGAMMA 1
#define USE_AEC 1
#define USE_AWB 1
#define USE_CCM 1
#define USE_LTM 1
#define USE_GTM 0
#define USE_QND 0
#define USE_3DLUT 1
#define USE_CSC 0

#define DEGAMMA_KP 8

#define XF_CCM_TYPE XF_CCM_bt2020_bt709 /* Used in ccm */

#if T_8U
#define XF_SRC_T XF_8UC1 // XF_8UC1
#define XF_GTM_T XF_8UC3 // XF_8UC3
#define XF_DST_T XF_8UC3 // XF_8UC3
#elif T_16U
#define XF_SRC_T XF_16UC1 // XF_8UC1
#define XF_GTM_T XF_8UC3  // XF_8UC3
#define XF_DST_T XF_16UC3 // XF_8UC3
#elif T_10U
#define XF_SRC_T XF_10UC1 // XF_8UC1
#define XF_GTM_T XF_8UC3  // XF_8UC3
#define XF_DST_T XF_10UC3 // XF_8UC3
#elif T_12U
#define XF_SRC_T XF_12UC1 // XF_8UC1
#define XF_GTM_T XF_8UC3  // XF_8UC3
#define XF_DST_T XF_12UC3 // XF_8UC3
#endif

#define SIN_CHANNEL_TYPE XF_8UC1 /* Used in gtm */
#define AEC_SIN_CHANNEL_TYPE XF_16UC1

#define CVTYPE unsigned char
#define CV_INTYPE CV_8UC1
#define CV_OUTTYPE CV_8UC3

#define WB_TYPE XF_WB_SIMPLE /* Used in function_awb */

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64
#define LUT_PTR_WIDTH 128

#define NUM_V_BLANK_LINES 8 /* Used in HDR */
#define NUM_H_BLANK 8       /* Used in HDR */

#define MAX_HEIGHT 2160
#define MAX_WIDTH 1928

#define XF_USE_URAM 0 // uram enable Used in HDR, rgbir2bayer, lut3d
#define XF_CV_DEPTH_imgInput 3
#define XF_CV_DEPTH_imgInput1 3
#define XF_CV_DEPTH_hdr_out 3
#define XF_CV_DEPTH_LEF 3
#define XF_CV_DEPTH_SEF 3
#define XF_CV_DEPTH_rggb_out 3
#define XF_CV_DEPTH_rggb_out_stats 3
#define XF_CV_DEPTH_rggb_out_aec 3
#define XF_CV_DEPTH_aec_out 3
#define XF_CV_DEPTH_fullir_out 3
#define XF_CV_DEPTH_bpc_out 3
#define XF_CV_DEPTH_blc_out 3
#define XF_CV_DEPTH_dgamma_out 3
#define XF_CV_DEPTH_lsc_out 3
#define XF_CV_DEPTH_gain_out 3
#define XF_CV_DEPTH_demosaic_out 3
#define XF_CV_DEPTH_awb_out 3
#define XF_CV_DEPTH_aecin 3
#define XF_CV_DEPTH_dst 3
#define XF_CV_DEPTH_ccm 3
#define XF_CV_DEPTH_3dlut 3
#define XF_CV_DEPTH_lut_out 3
#define XF_CV_DEPTH_3XWIDTH 3 * XF_WIDTH
