/*
 * Copyright 2022 Xilinx, Inc.
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

#define CVTYPE unsigned char
#define CV_INTYPE CV_8UC1
#define CV_OUTTYPE CV_8UC3

#define WB_TYPE XF_WB_SIMPLE /* Used in function_awb */

#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128

#define NUM_V_BLANK_LINES 8 /* Used in HDR */
#define NUM_H_BLANK 8       /* Used in HDR */

#define XF_USE_URAM 0 // uram enable Used in HDR, rgbir2bayer, lut3d
#define XF_CV_DEPTH_imgInput 2
#define XF_CV_DEPTH_hdr_out 2
#define XF_CV_DEPTH_LEF 2
#define XF_CV_DEPTH_SEF 2
#define XF_CV_DEPTH_rggb_out 2
#define XF_CV_DEPTH_fullir_out 2
#define XF_CV_DEPTH_bpc_out 2
#define XF_CV_DEPTH_blc_out 2
#define XF_CV_DEPTH_lsc_out 2
#define XF_CV_DEPTH_gain_out 2
#define XF_CV_DEPTH_demosaic_out 2
#define XF_CV_DEPTH_ltm_in 2
#define XF_CV_DEPTH_aecin 2
#define XF_CV_DEPTH_dst 2
#define XF_CV_DEPTH_ccm 2
#define XF_CV_DEPTH_3dlut 2
#define XF_CV_DEPTH_awb_out 2
#define XF_CV_DEPTH_lut_out 2
#define XF_CV_DEPTH_3XWIDTH 3 * XF_WIDTH
